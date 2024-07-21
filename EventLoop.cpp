#include "EventLoop.h"
#include <stdlib.h>   //free、malloc、exit头文件
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>   //close、write头文件
#include <stdio.h>    //perror头文件
#include <assert.h>   //assert头文件
#include "Log.h"
#include "PollDispatcher.h"
#include "EpollDispatcher.h"
#include "SelectDispatcher.h"
 
/**关于exit
 * （1）exit和return的区别
 * 如果main()在一个递归程序中，exit()仍然会终止程序；但return将控制权移交给递归的前一级，直到最初的那一级，此时return才会终止程序。
 * return和exit()的另一个区别在于，即使在除main()之外的函数中调用exit()，它也将终止程序。 
 * （2）exit() 里面的参数，是传递给其父进程的。
 * （3）在 main() 函数里面 return 会导致程序永远不退出。exit(3) 是C标准库函数，也是最常用的进程退出函数。
 */

//用于构造主线程实例
EventLoop::EventLoop() : EventLoop(string())    //委托构造函数-------在以当前构造函数调用另外一个构造函数    
{
}
//用于构造子线程实例
EventLoop::EventLoop(const string threadName)
{
    m_isQuit = true;   //默认不启动eventLoop反应堆模型
    m_threadID = this_thread::get_id();   //获取线程id
    m_threadName = threadName == string() ? "MainThread" : threadName;    //给线程初始化名字
    
    //初始化dispatcher和dispatcherData----三个dispatcher取其一  
    m_dispatcher = new EpollDispatcher(this);   //this是当前的EventLoop实例对象    
    // m_dispatcher = new PollDispatcher(this);   //this是当前的EventLoop实例对象   
    // m_dispatcher = new SelectDispatcher(this);   //this是当前的EventLoop实例对象   

    //使用的stl容器queue存储，无需初始化，默认为空
    //初始化map--也无需初始化
    m_channelMap.clear();

    //用于创建一对无中间介质的连接的套接字。这种类型的套接字通常被用于在父子进程之间，或者同一个进程内部的线程之间的通信。
    int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, m_socketPair);
    //执行成功返回0，执行失败返回-1
    if(ret == -1) {
        perror("socketpair");
        exit(0);    //退出整个进程
    }
    // 指定规则: evLoop->socketPair[0]发送数据， evLoop->socketPair[1]接收数据
    //创建evLoop->socketPair[1]对应的channel节点并添加到任务队列
#if 0
    Channel* channel = new Channel(m_socketPair[1], FDEvent::ReadEvent, 
                        readLocalMessage, nullptr, nullptr, this);   //this（当前的类实例对象）是传入回调函数的参数
#else
    auto obj = bind(&EventLoop::readMessage, this);    //this是绑定的对象
    Channel* channel = new Channel(m_socketPair[1], FDEvent::ReadEvent, 
                        obj, nullptr, nullptr, this);   //this（当前的类实例对象）是传入回调函数的参数
#endif
    addTask(channel, ElemType::ADD);    //将创建的channel添加到eventloop的任务队列

}

EventLoop::~EventLoop()
{
}

int EventLoop::run()
{
    m_isQuit = false;     //false代表反应堆模型为退出（true表示未启动）
    //比较线程ID是否正常
    if(m_threadID != this_thread::get_id()) {
        perror("thread_id");
        return -1;
    }
    // Debug("反应堆模型启动............");
    //循环处理事件
    while(!m_isQuit) {
        //监测的是evLoop->dispatcher的实例化对象(上边初始化的是EpollDispatcher)
        m_dispatcher->dispatch();   //超时时长默认值 2s
        processTaskQ();    //子线程解除阻塞后执行任务队列的处理函数（只有子线程可以处理任务队列中的任务）
    }
    return 0;
}

int EventLoop::eventActivate(int fd, int event)
{
    if(fd < 0) {   //判断无效数据
        return -1;
    }
    Channel* channel = m_channelMap[fd];
    assert(channel->getSocket() == fd);   //断言判断传入的fd与取出的channel中的fd是否相同，不相同则异常退出
    if((event & (int)FDEvent::ReadEvent) && channel->readCallBack) {
        channel->readCallBack(const_cast<void*>(channel->getArg()));
    }
    if((event & (int)FDEvent::WriteEvent) && channel->writeCallBack) {
        channel->writeCallBack(const_cast<void*>(channel->getArg()));
    }
    return 0;
}

int EventLoop::addTask(Channel *channel, ElemType type)
{  
    m_mutex.lock();     
    //创建节点并初始化---不管type是什么操作，都得先添加到任务队列
    ChannelElement* node = new ChannelElement;
    node->channel = channel;
    node->type = type;
    //添加节点到任务队列
    m_taskQ.push(node);
    m_mutex.unlock();       //解锁
    //处理节点
    /**
     * 处理的细节分析
     * 1. 对于节点的添加：可能是当前的线程(即反应堆模型所属者)也可能是其他线程（主线程）
     *     1). 修改fd的事件，当前子线程发起，当前子线程处理
     *     2). 添加新的fd，添加任务节点的操作是由主线程发起的
     * 2. 不能让主线程处理任务队列，需要由当前的子线程去处理
     */
    if(m_threadID == this_thread::get_id()) {    //执行当前函数的线程id是否等于反应堆模型所属者线程id
        //直接处理任务队列中的任务
        processTaskQ();
    }  
    else{ 
        //主线程---需要告诉子线程去处理任务队列中的任务
        //子线程的两种状态：1.在工作  2. 在阻塞（此时需要自定义一个fd（在EventLoop中），通过该fd发送数据给当前节点的文件描述符（激活）去唤醒子线程）
        taskWakeup();   //如果线程被阻塞，主线程调用此函数即可唤醒子线程，子线程是在dispacth函数（eventLoopRun函数那里）那结束阻塞
    }
    return 0;
}

int EventLoop::processTaskQ()
{
    while(!m_taskQ.empty()) {    //遍历任务队列
        m_mutex.lock();      //加锁-----操作任务队列时加锁就行
        //取出头节点
        ChannelElement* node = m_taskQ.front();
        //删除节点
        m_taskQ.pop();   
        m_mutex.unlock();    //解锁
        Channel* channel = node->channel;
        if(node->type == ElemType::ADD) {            //添加---将文件描述符添加到dispatcher（poll/epoll/select）的监测集合中
            add(channel);
        }
        else if(node->type == ElemType::DELETE) {    //删除---将文件描述符从dispatcher的监测集合中删除
            remove(channel);
        }
        else if(node->type == ElemType::MODIFY) {    //修改事件---读/写
            modify(channel);
        }
        delete node;
    }
    return 0;
}

int EventLoop::add(Channel *channel)
{
    int fd = channel->getSocket();
    //找到fd对应的数组元素位置，并存储
    if(m_channelMap.find(fd) == m_channelMap.end()) {
        //添加到channelMap
        m_channelMap.insert(make_pair(fd, channel));
        //动态设置channel
        m_dispatcher->setChannel(channel);         //父类指针调用子类对象的函数----多态
        int ret = m_dispatcher->add();     //添加到dispatcher监测集合
        return ret;   //添加成功
    }
    return 0;   //添加失败
}

int EventLoop::remove(Channel *channel)
{
    int fd = channel->getSocket();
     //要删除的文件描述符及对应的channel并不在channelmap中存储（证明一定不会再dispatcher监测集合）
    if(m_channelMap.find(fd) == m_channelMap.end()) {
        return -1;     //相当于什么都没做
    }
    m_dispatcher->setChannel(channel);
    int ret = m_dispatcher->remove();           //父类指针调用子类对象的函数----多态
    return ret;    //删除成功
}

int EventLoop::modify(Channel *channel)
{
    int fd = channel->getSocket();
    if(m_channelMap.find(fd) == m_channelMap.end()) {
        return -1;
    }
    m_dispatcher->setChannel(channel);
    int ret = m_dispatcher->modify();        //父类指针调用子类对象的函数----多态
    return ret;
}

int EventLoop::freeChannel(Channel *channel)
{
    auto it = m_channelMap.find(channel->getSocket());    //返回值是迭代器类型
    if(it != m_channelMap.end()) {
        // 删除键值对
        m_channelMap.erase(it);
        //关闭文件描述符
        close(channel->getSocket());
        // 释放内存空间
        delete channel;
    }
    return 0;
}

int EventLoop::readLocalMessage(void *arg)
{
    //静态成员函数不属于对象，所以需要传递实例化对象
    EventLoop* evLoop = static_cast<EventLoop*>(arg);   //类型转换 void* --->  EventLoop*    
    char buf[256];    //接收数据
    read(evLoop->m_socketPair[1], buf, sizeof(buf));
    return 0;
}

int EventLoop::readMessage()
{
    char buf[256];    //接收数据
    read(m_socketPair[1], buf, sizeof(buf));
    return 0;
}

//写数据(向evLoop->socketPair[0]中写，发送到evLoop->socketPair[1]，从而激活evLoop->socketPair[1])
void EventLoop::taskWakeup()
{
    const char* msg = "我的目的就是唤醒evLoop->socketPair[1]";
    write(m_socketPair[0], msg, strlen(msg));
}
