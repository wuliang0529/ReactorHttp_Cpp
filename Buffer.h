#pragma once
#include <string>
using namespace std;
//需要在TcpConnection.c   HttpRequest.c 和 HttpResponse.c多个文件中使用这个宏---所以选择定义在这
//控制服务器向客户端发送数据的方式：1.全部写入sendBuf再发送给客户端   2. 写一部分发送一部分
#define MSG_SEND_AUTO     //如果MSG_SEND_AUTO有效则为第一种发送方式    


//定义Buffer结构体
class Buffer
{
public:
    Buffer(int size);
    ~Buffer();
    //扩容buffer
    void extendRoom(int size);    //size表示需要写进内存的数据大小
    //得到剩余的可写的内存容量（未写---不包含已读过的内存哦）
    int writeableSize();
    //得到剩余的可读的内存容量（未读）
    int readableSize();
    //把数据写入内存：1.直接写  2.接收套接字数据（也需要写入buffer）
    int appendString(const char* data, int size);   //适用于所有的data数据（不一定是string）
    int appendString(const char* data);   //适用于data为string类型的数据
    int appendString(const string data);
    int socketRead(int fd);    //接收套接字数据并写入buffer
    //根据\r\n取出一行，找到其在数据块中的位置，返回该位置
    char* findCRLF();
    //发送数据
    int sendData(int socket);
    //得到读数据的起始位置
    inline char* getData() {
        return m_data + m_readPos;
    }
    //readPos的移动
    inline int readPosIncrease(int count) {
        return m_readPos += count; 
    }
private:
    char* m_data;       //辅助堆内存，用于读写时的缓存
    int m_capacity;     //存储buffer容量
    int m_readPos = 0;      //可读起始位置
    int m_writePos = 0;     //可写起始位置
}; 

