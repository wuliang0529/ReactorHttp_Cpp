#pragma once
#include "Buffer.h"
#include <map>
#include <string>
#include <functional>
using namespace std;

//定义状态码枚举
enum class StatusCode {
    Unknown,                     //默认为未知
     OK = 200,                   //请求成功
    MovedPermanently = 301,      //重定向
    MovedTemporarily = 302,      //临时重定向
    BadRequest = 400,            //请求失败
    NotFound = 404               //请求资源未找到
};


//定义回复响应结构体
class HttpResponse{
public:
    HttpResponse();
    ~HttpResponse();
    //可调用对象包装器作为回调函数，用来组织要回复给客户端的数据块
    function<void(const string, Buffer*, int socket)> sendDataFunc;   //返回值类型是void
    //添加响应头
    void addHeader(const string key, const string value);
    //组织http响应数据
    void prepareMsg(Buffer* sendBuf, int socket);
    //设置状态码
    inline void setStatusCode(StatusCode code) {
        m_statusCode = code;
    }
    //设置相应的文件的名字---也可能是目录哦！！！！
    inline void setFileName(string name) {
        m_fileName = name;
    }
private:
    //(1)状态行：状态码，状态描述
    StatusCode m_statusCode;     //根据状态码即可从map中查找到对应的状态描述
    string m_fileName;
    //(2)响应头 -- 键值对
    map<string, string> m_headers;    //哈希表存放键值对
    const map<int, string> m_info = {           //定义状态码和描述的对应关系
        {200, "OK"},
        {301, "MovedPermanently"},
        {302, "MovedTemporarily"},
        {400, "BadRequest"},
        {404, "NotFound"},
    };
    //(3)空行不做处理
    //(4)body数据块---使用回调函数--放在public中的可调用包装器
};
