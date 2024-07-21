#pragma once
#include "EventLoop.h"
#include "ThreadPool.h"


class TcpServer
{
public:
    TcpServer(unsigned short port, int threadNum);
    ~TcpServer();
    //初始化监听fd
    void setListen();
    //启动服务器
    void run();
    static int acceptConnection(void* arg);
private:
    int m_threadNum;             //线程池中线程个数
    EventLoop* m_mainLoop;       //主线程的反应堆模型
    ThreadPool* m_threadPool;    //主线程的线程池     
    int m_lfd;                   //服务器用于监听的文件描述符
    unsigned short m_port;       //服务器用于监听的端口号
};
