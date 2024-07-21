#include "HttpResponse.h"
#include <strings.h>    //bzero对挺的头文件
#include <string.h>
#include <stdlib.h>    //malloc对应的头文件
#include <stdio.h>     //sprintf对应的头文件
#include "Log.h"

#define ResHeaderSize 16   //定义一个response响应请求中headers存储容量的宏（用于开辟内存）

HttpResponse::HttpResponse()
{
    m_fileName = string();
    m_headers.clear();
    m_statusCode = StatusCode::Unknown;
    sendDataFunc = nullptr;    //sendDataFunc是可调用对象包装器的对象
}

HttpResponse::~HttpResponse()
{
}

void HttpResponse::addHeader(const string key, const string value)
{
    if(key.empty() || value.empty()) {
        return;
    }
    //添加key和value到map中
    m_headers.insert(make_pair(key, value));
}

void HttpResponse::prepareMsg(Buffer *sendBuf, int socket)
{
    //状态行  
    char tmp[1024] = { 0 };
    sprintf(tmp, "HTTP/1.1 %d, %s\r\n", m_statusCode, (m_info.at((int)m_statusCode)).data());   //at取值为只读（const）属性，m_statusCode是枚举类型哦！！
    sendBuf->appendString(tmp);  
    //响应头
    for (auto it = m_headers.begin(); it != m_headers.end(); ++it)
    {
        sprintf(tmp, "%s: %s\r\n", it->first.data(), it->second.data());
        sendBuf->appendString(tmp);
    }
    //空行
    sendBuf->appendString("\r\n");
#ifndef MSG_SEND_AUTO
    sendBuf->sendData(socket);    //第二种数据发送方式-----写一部分发送一部分
#endif
    //回复的数据
    sendDataFunc(m_fileName, sendBuf, socket);
}
