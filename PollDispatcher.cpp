//不需要有"PollDispatch.h"文件，因为只要保存函数的地址即可调用，不用把函数声明为全局函数
#include "Dispatcher.h"
#include <poll.h>
#include <stdlib.h>   //malloc头文件
#include <stdio.h>    //perror头文件
#include "PollDispatcher.h"

PollDispatcher::PollDispatcher(EventLoop *evloop) : Dispatcher(evloop)    //把子类构造函数的evloop传递给父类构造函数用于创建对象
{
    //为data成员赋值
    m_maxfd = 0;
    m_fds = new struct pollfd[m_maxfd];
    for(int i=0; i<m_maxNode; ++i) {
        m_fds[i].fd = -1;   //有效的文件描述符从0开始
        m_fds[i].events = 0;   //无事件(可以包括读写)
        m_fds[i].revents = 0;   //无触发时间（如果只触发读，则这里只是读）
    }
    m_name = "Poll";
}


PollDispatcher::~PollDispatcher()
{
    //因为在构造函数中new了堆内存，所以需要delete释放内存
    delete[]m_fds;
}

int PollDispatcher::add()
{
    int events = 0;   //表示内核的读、写
    if(m_channel->getEvent() & (int)FDEvent::ReadEvent) {   //强类型枚举不能进行隐式类型转换
        events |= POLLIN;
    }
    if(m_channel->getEvent() & (int)FDEvent::WriteEvent) {   //用if是因为有可能读写同时发生哦！！！
        events |= POLLOUT;
    }
    //找空闲位置把 events 及对应的 fd 存储到PollData对象中
    int i=0;
    for(; i<m_maxNode; ++i) {
        if(m_fds[i].fd == -1) {
            m_fds[i].fd = m_channel->getSocket();
            m_fds[i].events = events;
            m_maxfd = i > m_maxfd ? i : m_maxfd;
            break;
        }
    } 
    if(i >= m_maxNode) {   //证明超出了内存管理范围
        return -1;
    }
    return 0;
}

int PollDispatcher::remove()
{
    //找到 events 及对应的 fd ，在PollData对象中删除
    int i=0;
    for(; i<m_maxNode; ++i) {
        if(m_fds[i].fd == m_channel->getSocket()) {
            m_fds[i].fd = -1;        
            m_fds[i].events = 0;     //events表示请求的事件
            m_fds[i].revents = 0;    //revents表示返回的事件
            break;
        }
    } 
    //删除了文件描述符之后即可释放channel所在的TcpConnection模块的内存
    //思路---在channel结构体中定义的函数指针指向TcpConnection对应的销毁函数的地址即可（销毁类型的回调函数）
    m_channel->destoryCallBack(const_cast<void*>(m_channel->getArg()));   //const_cast去掉所得到地址的只读属性（定义的时候加了const修饰）

    if(i >= m_maxNode) {   //证明超出了内存管理范围
        return -1;
    }
    return 0;
}

int PollDispatcher::modify()
{
    int events = 0;   //表示内核的读、写
    if(m_channel->getEvent() & (int)FDEvent::ReadEvent) {   //判断用户是否需要读事件
        events |= POLLIN;
    }
    if(m_channel->getEvent() & (int)FDEvent::WriteEvent) {   //判断用户是否需要写事件，用if是因为有可能读写同时发生哦！！！
        events |= POLLOUT;
    }
    //找存储在PollData对象中的events 及对应的 fd 
    int i=0;
    for(; i<m_maxNode; ++i) {
        if(m_fds[i].fd == m_channel->getSocket()) {
            m_fds[i].events = events;   //只需要修改读写时间即可
            break;
        }
    } 
    if(i >= m_maxNode) {   //证明超出了内存管理范围
        return -1;
    }
    return 0;
}

int PollDispatcher::dispatch(int timeout)
{
    //poll函数用于监听多个文件描述符变化（可读、可写）的系统调用，返回值为满足条件的文件描述符数量
    int count = poll(m_fds, m_maxfd+1, timeout * 1000);   //timeout自定义为s，但epoll_wait参数为ms
    if(count == -1) {
        perror("poll");
        exit(0);
    }
    for(int i=0; i<=m_maxfd; ++i) {

        if(m_fds[i].fd == -1) {    //未被激活的文件描述符
            continue;
        }
        //----------------eventActivate属于evLoop类，evLoop还未修改，暂时不管
        if(m_fds[i].revents & POLLIN) {    //读事件
            //调用被激活的文件描述符对应的“读回调函数”---在EventLoop中的channel中存储
            m_evLoop->eventActivate(m_fds[i].fd, (int)FDEvent::ReadEvent);
        }
        if(m_fds[i].revents & POLLOUT) {   //写事件
            //调用被激活的文件描述符对应的“写回调函数”
            m_evLoop->eventActivate(m_fds[i].fd, (int)FDEvent::WriteEvent);
        }   
    }
    return 0;
}

