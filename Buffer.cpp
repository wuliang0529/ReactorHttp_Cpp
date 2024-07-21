// //buffer用于实现套接字通信时的数据传入和传出（存储通讯时的数据）
// //整体结构：Buffer[data, readPos, writePos, capacity]
// #define _GNU_SOURCE    //memmem需要的宏定义（查看man文档可看）
#include "Buffer.h"
#include <stdio.h>     //NULL对应的头文件
#include <stdlib.h>    //free对应的头文件
#include <sys/uio.h>   //readv对应的头文件
#include <string.h>    
#include <strings.h>   ////memmem头文件
#include <sys/socket.h>   //send头文件
#include <unistd.h>   //usleep头文件
#include "Log.h"

Buffer::Buffer(int size) : m_capacity(size)
{
    m_data = (char*)malloc(size);   //其实是sizeof(size * char);  申请一块儿堆内存（buffer里的一块儿）
    bzero(m_data, size);
}

Buffer::~Buffer()
{
    if(m_data != nullptr) {
        free(m_data);
    }
}

//向buffer内存中写数据前先调用此函数
void Buffer::extendRoom(int size)
{
    //1.内存够用且不存在内存需要合并的问题---不需要扩容
    if(writeableSize() >= size) {
        return;
    }
    //2.内存够用但需要内存合并后才可以---不需要扩容
    else if(m_readPos + writeableSize() >= size) {
        //得到未读的内存大小
        int readable = readableSize();   
        //移动内存
        memcpy(m_data, m_data + m_readPos, readable);    //buffer->data指向buffer的起始地址
        //更新位置
        m_readPos = 0;
        m_writePos = readable;
    }
    //3.内存不够用---需要扩容
    else {
        void* temp = realloc(m_data, m_capacity + size);   //temp保存扩容后的buffer地址
        if(temp == nullptr) {
            return;  //扩容失败
        }
        memset((char*)temp + m_capacity, 0, size);    //从temp到buffer->capacity的内存无需初始化
        //更新数据
        m_data = static_cast<char*>(temp);
        m_capacity += size;
    }
}

int Buffer::writeableSize()
{
    return m_capacity - m_writePos;
}

int Buffer::readableSize()
{
    return m_writePos - m_readPos;
}

//适用于向buffer中写入任何类型的data数据
int Buffer::appendString(const char *data, int size)
{
    if(data == nullptr || size <= 0) {    
        return -1;
    }
    //扩容---不一定需要扩容
    extendRoom(size);
    //数据拷贝-----把数据保存到buffer
    memcpy(m_data + m_writePos, data, size);
    m_writePos += size;
    return 0;
}

//适用于向buffer写入string字符串的情况
int Buffer::appendString(const char *data)
{
    int size = strlen(data);
    // Debug("数据成功添加到sendbuf,可发送数量: %d", size);  //有数据
    int ret = appendString(data, size);
    return ret;
}

int Buffer::appendString(const string data)
{
    int ret = appendString(data.data());
    return ret;
}

//接收套接字数据并写入buffer
int Buffer::socketRead(int fd)
{
    //接收套接字数据read/recv/readv（readv更高级）
    struct iovec vec[2];

    int writeable = writeableSize();
    vec[0].iov_base = m_data + m_writePos;
    vec[0].iov_len = writeable;

    char* temp = (char*)malloc(40960);   //40k字节
    vec[1].iov_base = temp;
    vec[1].iov_len = 40960;

    int result = readv(fd, vec, 2);
    if(result == -1) {
        return -1;    //函数调用失败
    }
    else if(result <= writeable) {   //未使用新申请的temp内存空间
        m_writePos += result;
    }
    else{
        m_writePos = m_capacity;    //capacity已使用完毕
        appendString(temp, result-writeable);   //把temp中的数据写入到buffer（buffAppendData中有扩容逻辑）
    }
    free(temp);
    return result;
}

char *Buffer::findCRLF()
{
    //strstr --> 大字符串中匹配子字符串（遇到\0结束）  char *strstr(const char *haystack, const char *needle);
    //memmem --> 大数据块中匹配子数据块（需要制定数据库大小） void *memmem(const void *haystack, size_t haystacklen, const void *needle, size_t needlelen); 
    char* ptr = static_cast<char*>(memmem(m_data + m_readPos, readableSize(), "\r\n", 2));
    return ptr;
}

int Buffer::sendData(int socket)
{
    //1.判断有无数据
    int readable = readableSize();
    // Debug("可读数据量是多少呢：%d", readable);
    if(readable > 0) {
        int count = send(socket, m_data + m_readPos, readable, MSG_NOSIGNAL);
        if(count > 0) {
            m_readPos += count;
            usleep(1);
        }
        return count;
    }
    return 0;
}
