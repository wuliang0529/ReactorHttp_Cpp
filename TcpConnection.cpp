#include "TcpConnection.h"
#include "HttpRequest.h"
// #include <strings.h>
#include <stdlib.h>    //free、malloc头文件
#include <stdio.h>    //sprintf头文件
#include "Log.h"     //日志输出

int TcpConnection::processRead(void* arg) {
    TcpConnection* conn = static_cast<TcpConnection*>(arg);
    //接收数据
    int socket = conn->m_channel->getSocket();
    int count = conn->m_readBuf->socketRead(socket);   //count为接收到的字节数
    //日志打印
    Debug("接收到的http请求数据：%s", conn->m_readBuf->getData());

    if(count > 0) {
        //接收到了http请求      
        //解析http请求
#ifdef MSG_SEND_AUTO   
        //修改文件描述符的读事件为读写事件---需要等线程执行完毕读回调后才能处理执行回调（弊端：需要等到所有的数据写入到writebuff后才能处理写回调，那如果文件太大写不下岂不是凉凉）
        conn->m_channel->writeEventEnable(true);
        conn->m_evLoop->addTask(conn->m_channel, ElemType::MODIFY);
#endif  
        bool flag = conn->m_request->parseHttpRequest(conn->m_readBuf, conn->m_response, conn->m_writeBuf, socket);
        if(!flag) {
            //解析失败，回复一个简单的html页面
            string errMsg = "Http/1.1 400 Bad Request\r\n\r\n";
            conn->m_writeBuf->appendString(errMsg);    //只需要写入sendBuf（这里是writeBuf）即可，不需要发送
        }
    }
    else {
        //接收字节数count == 0时;
#ifdef MSG_SEND_AUTO       
        conn->m_evLoop->addTask(conn->m_channel, ElemType::DELETE);  //接收到的数据是空数据，则断开连接
#endif
    }
    //断开连接的三种情况：1.接收字节数count<0;  2.解析失败发送html页面后   3. flag=true时，存储完成并发送Buf通讯结束
    //断开连接--涉及内存的释放  
#ifndef MSG_SEND_AUTO       
    conn->m_evLoop->addTask(conn->m_channel, ElemType::DELETE);  //如果是第一种发送方式，则不能调用该语句（因为将数据写入writeBuf后会直接断开连接，并未发送）    
#endif
    return 0;
}

int TcpConnection::processWrite(void* arg) {
    //日志打印
    Debug("开始发送数据了（基于写事件发送）....");
    TcpConnection* conn = static_cast<TcpConnection*>(arg);
    //发送数据
    int count = conn->m_writeBuf->sendData(conn->m_channel->getSocket());
    if(count > 0) {
        //判断数据是否被全部发送出去
        if(conn->m_writeBuf->readableSize() == 0) {
            //1. 不再检测写事件---修改channel保存的事件
            conn->m_channel->writeEventEnable(false);
            //2. 修改dispatcher检测集合---添加任务节点
            conn->m_evLoop->addTask(conn->m_channel, ElemType::MODIFY);   //从读写，修改为只读
            //3. 删除这个节点----前两步可写可不写，直接删除节点就行
            conn->m_evLoop->addTask(conn->m_channel, ElemType::DELETE);
        }
    }
    return 0;
}

int TcpConnection::destory(void* arg)
{
    TcpConnection* conn = static_cast<TcpConnection*>(arg);
    if(conn != nullptr) {
        delete conn;   
    }
    return 0;
}

TcpConnection::TcpConnection(int fd, EventLoop *evLoop)
{
    m_evLoop = evLoop;       //所以执行此函数的线程是evLoop所属线程
    m_readBuf = new Buffer(10240);
    m_writeBuf = new Buffer(10240);
    //http协议初始化
    m_request = new HttpRequest;
    m_response = new HttpResponse;
    m_name = "Connection-" + to_string(fd); 
    //对fd封装成channel，processRead是fd对应的读回调函数
    m_channel = new Channel(fd, FDEvent::ReadEvent, processRead, processWrite, destory, this);   //processRead是接收客户端的http请求
    //把channel添加到子线程的反应堆模型evLoop，子线层反应堆实例evLoop检测到任务队列中有任务(m_hannel)后，就把任务m_channel添加的待监测集合
    evLoop->addTask(m_channel, ElemType::ADD);
}

TcpConnection::~TcpConnection()
{
    if(m_readBuf && m_readBuf->readableSize() == 0 &&
        m_writeBuf && m_writeBuf->readableSize() == 0)
    {
        delete m_readBuf;
        delete m_writeBuf;
        delete m_request;
        delete m_response;
        m_evLoop->freeChannel(m_channel);
    } 
    Debug("连接断开, 释放资源, gameover, connName: %s", m_name);

}
