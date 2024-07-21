#pragma once     //无论这个头文件被其他源文件引用多少次, 编译器只会对这个文件编译一次。
#include "EventLoop.h"
#include "Channel.h"
#include "Buffer.h"
#include "HttpRequest.h"
#include "HttpResponse.h"

class TcpConnection
{
public:
    TcpConnection(int fd, EventLoop* evLoop);
    ~TcpConnection();
    static int processRead(void* arg);   //静态成员不属于对象，属于类，所以需要传递TcpConnection对象参数
    static int processWrite(void* arg);
    static int destory(void* arg);
private:
    string m_name;
    EventLoop* m_evLoop;
    Channel* m_channel;
    Buffer* m_readBuf;
    Buffer* m_writeBuf;
    
    //http协议
    HttpRequest* m_request;
    HttpResponse* m_response;
};
