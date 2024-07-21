#include "Dispatcher.h"
#include <sys/epoll.h>
#include <unistd.h>   //close头文件
#include <stdlib.h>   //free、exit头文件
#include <stdio.h>    //perror头文件
#include "EpollDispatcher.h"

EpollDispatcher::EpollDispatcher(EventLoop *evloop) : Dispatcher(evloop)   //子类的evloop传递给父类evloop（调用父类构造函数）
{
    //创建epoll对象，用于监听文件描述符m_epfd（如果创建成功的话会返回一个大于0的数）
    m_epfd = epoll_create(10);   //参数为大于0的数即可，无实际意义
    if(m_epfd == -1) {
        perror("epoll_create");
        exit(0);
    }
    m_events = new struct epoll_event[m_maxNode];     //使用new为m_events分配内存，new一个epoll_event类型的数组地址传递给m_events
    m_name = "Epoll";
}

int EpollDispatcher::add()
{
    int ret = epollCtl(EPOLL_CTL_ADD);
    if(ret == -1) {
        perror("epoll_ctl add");
        exit(0);
    }
    return ret;
}

int EpollDispatcher::remove()
{
    int ret = epollCtl(EPOLL_CTL_DEL);
    if(ret == -1) {
        perror("epoll_ctl delete");
        exit(0);
    }
    
    //删除了文件描述符之后即可释放channel所在的TcpConnection模块的内存
    //思路---在channel结构体中定义的函数指针指向TcpConnection对应的销毁函数的地址即可（销毁类型的回调函数）
    m_channel->destoryCallBack(const_cast<void*>((m_channel->getArg())));    //const_cast是去掉了getArg返回地址的只读属性
    return ret;
}

int EpollDispatcher::modify()
{
    int ret = epollCtl(EPOLL_CTL_MOD);
    if(ret == -1) {
        perror("epoll_ctl modify");
        exit(0);
    }
    return ret;
}

int EpollDispatcher::dispatch(int timeout)
{
    int count = epoll_wait(m_epfd, m_events, m_maxNode, timeout * 1000);   //timeout自定义为s，但epoll_wait参数为ms
    for(int i=0; i<count; ++i) {
        int events = m_events[i].events;   // (struct epoll_event*) events 是结构体类型
        int fd = m_events[i].data.fd;     //data也是个结构体
        if(events & EPOLLERR || events & EPOLLHUP) {   //客户端断开连接  或  客户端已经断开连接，但服务器仍在发送数据
            //删除fd
            // epollRemove(Channel, evLoop);
            continue;
        }
        //eventActivate属于evLoop类，evLoop还未修改，暂时不管
        if(events & EPOLLIN) {    //读事件
            m_evLoop->eventActivate(fd, (int)FDEvent::ReadEvent);
        }
        if(events & EPOLLOUT) {   //写事件
            m_evLoop->eventActivate(fd, (int)FDEvent::WriteEvent);
        }    
    }
    return 0;
}

EpollDispatcher::~EpollDispatcher()
{
    close(m_epfd);
    //因为在构造函数中new了内存，所以需要delete释放
    delete []m_events;      //释放数组需要加中括号哦！！！！！！
}

int EpollDispatcher::epollCtl(int op)
{
    struct epoll_event ev;   //epoll_event结构体并初始化，用于epoll_ctll的最后一个参数传递
    ev.data.fd = m_channel->getSocket();
    int events = 0;   //表示内核的读、写
    if(m_channel->getEvent() & (int)FDEvent::ReadEvent) {    //强类型枚举需要加作用域
        events |= EPOLLIN;
    }
    if(m_channel->getEvent() & (int)FDEvent::WriteEvent) {   //用if是因为有可能读写同时发生哦！！！
        events |= EPOLLOUT;
    }
    ev.events = events;   //不能直接赋值为m_channel->events，因为channel的events是自己定义的，epoll_event的events是linux内核定义
    int ret = epoll_ctl(m_epfd, op, m_channel->getSocket(), &ev);
    return ret;
}
