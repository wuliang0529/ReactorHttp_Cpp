#include "TcpServer.h"
#include <arpa/inet.h>    //socket头文件
#include "TcpConnection.h"
#include <stdio.h>        //perror头文件
#include <stdlib.h>       //malloc头文件
#include "Log.h"     //日志输出


//定义连接函数
int TcpServer::acceptConnection(void* arg) {
    TcpServer* server = static_cast<TcpServer*>(arg);    //静态函数不能访问类的对象----需要去掉static属性
    //和客户端建立连接
    int cfd = accept(server->m_lfd, NULL, NULL);         //accept是标准c函数
    //从线程池中取出一个子线程的反应堆实例，去处理cfd
    EventLoop* evLoop = server->m_threadPool->takeWorkerEventLoop();   //需要把这个封装到tcpConntion
    //将cfd放到TcpConnection中处理
    new TcpConnection(cfd, evLoop);     //函数里面实现了对cfd的封装
    return 0;
}


TcpServer::TcpServer(unsigned short port, int threadNum)
{
    m_port = port;
    setListen();       //初始化监听的文件描述符m_lfd
    m_threadNum = threadNum;
    m_mainLoop = new EventLoop;
    m_threadPool = new ThreadPool(m_mainLoop, threadNum);
}

TcpServer::~TcpServer()
{
}

void TcpServer::setListen()
{
    //1. 创建监听的fd
    m_lfd = socket(AF_INET, SOCK_STREAM, 0);
    if(m_lfd == -1) {
        perror("socket");
        return;
    }
    //2. 设置端口复用
    int opt = 1;
    int ret = setsockopt(m_lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt);
    if(ret == -1) {
        perror("setsockopt");
        return;
    }
    //3. 绑定
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(m_port);
    addr.sin_addr.s_addr = INADDR_ANY;    //绑定本地ip地址：设置为零地址的宏（INADDR_ANY），表示可以绑定本机的任意IP地址
    ret = bind(m_lfd, (struct sockaddr*)&addr, sizeof addr);
    if(ret == -1){
        perror("bind");
        return;
    }
    //4. 设置监听
    ret = listen(m_lfd, 128);
    if(ret == -1) {
        perror("listern");
        return;
    }    
}

void TcpServer::run()
{
    //启动线程池---启动服务器之前先启动线程池
    m_threadPool->run();
    //处理监听文件描述符fd（只有这一个文件描述符需要执行）,添加检测的任务
    //初始化一个channel
    Channel* channel = new Channel(m_lfd, FDEvent::ReadEvent, acceptConnection, nullptr, nullptr, this);    //注意*************这个this
    m_mainLoop->addTask(channel, ElemType::ADD);
    //启动反应堆模型
    m_mainLoop->run();
}
