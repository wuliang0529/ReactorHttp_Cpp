#pragma once
#include "Channel.h"
#include "Dispatcher.h"
#include <thread>
#include <queue>
#include <map>
#include <mutex>
#include <string>
using namespace std;

//处理节点中channel的方式
enum class ElemType : char{ADD, DELETE, MODIFY};

//定义任务队列的节点---使用c++的STL存储任务节点
struct ChannelElement
{
    ElemType type;       //如何处理该节点中的channel----使用枚举
    Channel* channel;     
};
class Dispatcher;   //声明--在Dispatcher.h中也使用到了class EventLoop
class EventLoop {
public:
    EventLoop();       //初始化主线程反应堆模型---主线程名字固定
    EventLoop(const string threadName);    //初始化子线程反应堆模型，子线程名字需要区分
    ~EventLoop(); 
    //启动反应堆模型
    int run();     //启动反应堆模型
    //处理被激活的文件描述符fd
    int eventActivate(int fd, int event);    //被激活的文件描述符，fd对应的读/写事件
    //添加任务到任务队列
    int addTask(Channel* channel, ElemType type);
    //处理任务队列中的任务
    int processTaskQ();
    //处理dispatcher中的节点--->为processTask服务，根据type类型调用相应的add，remove，modify函数
    int add(Channel* channel);      //任务队列中的任务节点添加到dispatcher监测集合里边
    int remove(Channel* channel);   //把任务节点从dispatcher监测集合中移除
    int modify(Channel* channel);   //修改任务节点的处理事件
    //释放channel（删除channel的后序处理）
    int freeChannel(Channel* channel);

    //回调函数----为什么非得加到类中---->项目是面向对象的
    static int readLocalMessage(void* arg);   //使用函数指针指向回调函数时：必须定义为static类成员函数，否则不可以赋值函数指针
    int readMessage();    //使用可调用对象包装器包装函数指针后指向回调函数（可以定义为类的非静态成员函数）

    //返回类的私有成员变量----当前eventLoop所属的线程id
    inline thread::id getThreadID() {  
        return m_threadID;
    }
private:
    void taskWakeup();
private:
    bool m_isQuit;   //标记当前的EventLoop是否退出了，如果退出了就是true，如果没退出就是false
    //该指针指向子类的示例  poll、epoll、select
    Dispatcher* m_dispatcher;
    //任务队列
    queue<ChannelElement*> m_taskQ;
    //map---文件描述符和channel的映射关系
    map<int, Channel*> m_channelMap;    //key为fd，value为封装fd的channel实例
    //线程id，name、mutex
    thread::id m_threadID;   //thread是类，id是类中的一个类型
    string m_threadName;
    mutex m_mutex;
    int m_socketPair[2];    //存储本地通信的fd，通过socketpair初始化
};

