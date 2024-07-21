#include "Channel.h"
#include <stdlib.h>   //malloc头文件

Channel::Channel(int fd, FDEvent events, handleFunc readFunc, handleFunc writeFunc, handleFunc destoryFunc, void *arg)
{
    m_fd = fd;
    m_events = (int)events;
    m_arg = arg;
    readCallBack = readFunc;
    writeCallBack = writeFunc;
    destoryCallBack = destoryFunc;
}

void Channel::writeEventEnable(bool flag)
{
    if(flag) {   //检测写事件
        // m_events |= (int)FDEvent::WriteEvent;  //c语言风格强制类型转换
        m_events |= static_cast<int>(FDEvent::WriteEvent);  //c++语言风格强制类型转换
    }
    else {       //不检测写事件
         //把第三位清零，原始为000100，取反后变成111011，第三位0与任何数相与都为0
        m_events = m_events & ~static_cast<int>(FDEvent::WriteEvent);  
    }
}

bool Channel::isWriteEventEnable()
{
    //writeEventEnable中已经设置了是否检测写事件
    return m_events & static_cast<int>(FDEvent::WriteEvent);    //返回值大于0即存在写事件，返回值为0则不存在写事件
}
