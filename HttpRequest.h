#pragma once
#include "Buffer.h"
#include "HttpResponse.h"
#include <stdbool.h>
#include <string>
#include <map>
#include <functional>
using namespace std;

//当前解析状态----强类型枚举
enum class ProcessState : char{
    ParseReqLine,      //请求行  （GET  ./1.jpg  http/1.1）
    ParseReqHeaders,   //请求头（key ： value  键值对）
    ParseReqBody,      //请求的数据块(get请求这部分为空)
    ParseReqDone       //请求解析完毕
};

//定义http请求结构体
class HttpRequest{
public:
    HttpRequest();
    ~HttpRequest();
    //重置HttpRequest实例----只需要重置指针指向的内存中的额数据，而不需要销毁整个结构体的内存地址
    void reset();     //重置私有成员变量
    //获取处理状态
    ProcessState getState();   //ProcessState是上边定义的强类型枚举
    //添加请求头---把键值对添加到reqHeaders结构体数组中
    void addHeader(const string key, const string value);
    //从reqHeaders结构体数组中根据key值取出value值
    string getHeader(const string key);
    //解析请求行
    bool parseHttpRequestLine(Buffer* readBuf);
    //解析请求头
    bool parseHttpRequestHeader(Buffer* readBuf);
    //解析http请求协议
    bool parseHttpRequest(Buffer* readBuf, HttpResponse* response, Buffer* sendBuf, int socket);
    //处理http请求
    bool processHttpRequest(HttpResponse* response);
    //解码特殊字符--to存储解码之后的数据，传出参数，from被解码的数据，传入参数
    string decodeMsg(string from); 
    //解析文件类型
    const string getFileType(const string name);
    //发送目录
    static void sendDir(const string dirName, Buffer* sendBuf, int cfd);
    //发送文件
    static void sendFile(const string filename, Buffer* sendBuf, int cfd);
    //method、url及version赋值
    inline void setMethod(string method) {
        m_method = method;
    }
    inline void setUrl(string url) {
        m_url = url;
    }
    inline void setVersion(string version) {
        m_version = version;
    }
    //当前状态的修改
    inline void setState(ProcessState state) {
        m_curState = state;
    }
private:
    // 分割请求行辅助函数
    char* splitRequestLine(const char* start, const char* end, const char* sub, function<void(string)> callback);
    //解码辅助函数
    int hexToDec(char c);
private:
    string m_method;
    string m_url;
    string m_version;
    map<string, string> m_reqHeaders;   //存储请求头的键值对
    ProcessState m_curState;    //标记当前解析的状态---解析到哪一步了
};

