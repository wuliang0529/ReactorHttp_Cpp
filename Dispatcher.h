#pragma once
#include "Channel.h"
#include "EventLoop.h"
#include <string>
using namespace std;

class EventLoop;   //声明--在EventLoop.h中也使用到了class Dispatcher


//如果类内的函数定义为纯虚函数，则此类为抽象类，不能实例化对象（本项目中没有实例化Dispatcher的需求）
class Dispatcher {  
public:
    // 初始化---初始化poll、epoll或者select需要的数据块DispatherData 
    Dispatcher(EventLoop* evLoop);
    // 添加---待检测的文件描述符添加到poll、epoll或者是select中
    virtual int add();
    // 修改
    virtual int remove();
    // 删除
    virtual int modify();
    // 事件监测，监测三种模型中是否有文件描述符被激活
    virtual int dispatch(int timeout = 2);   //超时阈值timeout单位：s，默认值为2s
    //动态获取channel
    inline void setChannel(Channel* channel) {
        m_channel = channel;
    }
    // 清除数据（关闭fd或者释放内存）
    virtual ~Dispatcher();
protected:
    string m_name = string();     //方便在实现多态时获取子类对象的名字
    Channel* m_channel;           //channel是封装fd的类，所以是动态改变的，需要动态获取
    EventLoop* m_evLoop;          //因为Dispatcher属于evLoop，所以evLoop固定不变
};
