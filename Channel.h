#pragma once
#include <functional> 

//定义函数指针---即事件对应的回调函数
// typedef int (*handleFunc) (void* arg);
// using handleFunc = int(*)(void*);

//定义文件描述符的读写事件标志---强类型枚举（不能当做整型来用，是FDEvent类型）
enum class FDEvent{
    TimeOut = 0x01,         //读写超时
    ReadEvent = 0x02,       //读事件
    WriteEvent = 0x04       //写事件
};

//通道类
class Channel
{
public:
    using handleFunc = std::function<int(void*)>;   //可调用包装器封装函数指针，使用using定义别名
    //初始化Channel---使用构造函数
    Channel(int fd, FDEvent events, handleFunc readFunc, handleFunc writeFunc, handleFunc destoryFunc, void* arg);    //arg是回调函数的参数
    //修改fd的写事件（检测 or 不检测）
    void writeEventEnable( bool flag);    //flag用于判断是否设置channel的写事件
    //判断是否需要检测文件描述符的写事件（读事件一定需要，任何操作都需要读事件）
    bool isWriteEventEnable(); 
    //回调函数
    handleFunc readCallBack;   //读回调
    handleFunc writeCallBack;  //写回调
    handleFunc destoryCallBack;  //销毁回调
    //取出私有成员的值
    inline int getEvent() {
        return m_events;
    }
    inline int getSocket() {
        return m_fd;
    }
    inline const void* getArg() {   //传递只读的地址
        return m_arg;
    }
private:
    //文件描述符----监听(只用一个)、连接
    int m_fd;
    //文件描述符对应的事件---读、写、读写
    int m_events;
    //回调函数的参数
    void* m_arg;
};

