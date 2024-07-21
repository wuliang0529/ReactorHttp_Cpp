//不需要有"SelectDispatch.h"文件，因为只要保存函数的地址即可调用，不用把函数声明为全局函数
#include "Dispatcher.h"
#include <sys/select.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>    //malloc头文件
#include "Log.h"
#include "SelectDispatcher.h"

SelectDispatcher::SelectDispatcher(EventLoop *evloop) : Dispatcher(evloop)  //把子类构造函数的evloop传递给父类构造函数用于创建对象
{
    //为data成员赋值---读写集合均赋值为0(使用宏函数)
    FD_ZERO(&m_readSet);
    FD_ZERO(&m_writeSet);
    m_name = "Select";
}


void SelectDispatcher::setFdSet()
{
    if(m_channel->getEvent() & (int)FDEvent::ReadEvent) {
        FD_SET(m_channel->getSocket(), &m_readSet);     //调用FD_SET函数时，会在集合相应位置设置为1
    }
    if(m_channel->getEvent() & (int)FDEvent::WriteEvent) {   //用if是因为有可能读写同时发生哦！！！
        FD_SET(m_channel->getSocket(), &m_writeSet);
    } 
}

void SelectDispatcher::clearFdSet()
{
    if(m_channel->getEvent() & (int)FDEvent::ReadEvent) {
        FD_CLR(m_channel->getSocket(), &m_readSet);     //调用FD_CLR函数时，会在集合相应位置设置为1
    }
    if(m_channel->getEvent() & (int)FDEvent::WriteEvent) {   //用if是因为有可能读写同时发生哦！！！
        FD_CLR(m_channel->getSocket(), &m_writeSet);
    }
}


SelectDispatcher::~SelectDispatcher()
{
    //并没有在构造函数中new申请内存，所以不需要在析构函数中释放 
}

int SelectDispatcher::add()
{
    if(m_channel->getSocket() >= m_maxSize) {   //如果fd大于最大的可操作位置，返回-1
        return -1;
    }
    setFdSet();
    return 0;
}

int SelectDispatcher::remove()
{
    clearFdSet();
    
    //删除了文件描述符之后即可释放channel所在的TcpConnection模块的内存
    //思路---在channel结构体中定义的函数指针指向TcpConnection对应的销毁函数的地址即可（销毁类型的回调函数）
    m_channel->destoryCallBack(const_cast<void*>(m_channel->getArg()));
    return 0;
}

int SelectDispatcher::modify()      //在本项目中读写事件是互斥的
{
    // setFdSet();
    // clearFdSet();
    if(m_channel->getEvent() & (int)FDEvent::ReadEvent) {
        FD_SET(m_channel->getSocket(), &m_readSet);     //调用FD_SET函数时，会在集合相应位置设置为1
        FD_CLR(m_channel->getSocket(), &m_writeSet);
    }
    if(m_channel->getEvent() & (int)FDEvent::WriteEvent) {   //用if是因为有可能读写同时发生哦！！！
        FD_SET(m_channel->getSocket(), &m_writeSet);
        FD_CLR(m_channel->getSocket(), &m_readSet);  

    } 
    return 0;
}

int SelectDispatcher::dispatch(int timeout)
{
    struct timeval val;
    val.tv_sec = timeout;   //ms
    val.tv_usec = 0;     //s，必须初始化为0，因为使用的时候是让结构体里的两个时间单位相加，不初始化会产生一个随机数相加
    fd_set rdtmp = m_readSet;
    fd_set wrtmp = m_writeSet;   //因为select函数的第二、三、四参数为传入传出参数，会修改原始数据，所以创建临时集合
    int count = select(m_maxSize, &rdtmp, &wrtmp, NULL, &val);   //设置NULL的位置为异常接收处理，这里不需要 
    if(count == -1) {
        perror("select");
        exit(0);
    }
    for(int i=0; i<m_maxSize; ++i) {   //i代表读写集合的第i个标志位
        // Debug("i: %d", i);
        if(FD_ISSET(i, &rdtmp)) {    //读集合的第i位有效
            m_evLoop->eventActivate(i, (int)FDEvent::ReadEvent);
        }
        if(FD_ISSET(i, &wrtmp)) {    //写集合的第i位有效
            m_evLoop->eventActivate(i, (int)FDEvent::WriteEvent);
        }
    }
    return 0;
}
