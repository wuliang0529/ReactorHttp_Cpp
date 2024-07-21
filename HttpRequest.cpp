// #define _GNU_SOURCE     //memmem需要
#include "HttpRequest.h"
#include <stdio.h>      //null的头文件
#include <strings.h>    //strncasecmp头文件
#include <string.h>     //strlen头文件
#include <stdlib.h>     //malloc头文件
#include <sys/stat.h>   //stat函数头文件
#include <dirent.h>     //scandir函数 与 dirent结构体 的头文件
#include <fcntl.h>
#include <unistd.h>     //lseek函数头文件
#include <assert.h>     //assert头文件
#include <pthread.h>    //pthread_create头文件
#include <ctype.h>      //isxdigit头文件
#include "Buffer.h"
#include "Log.h"
  
#define HeaderSize 12

HttpRequest::HttpRequest()
{
    reset();
}

HttpRequest::~HttpRequest()
{

}

void HttpRequest::reset()
{
    m_curState = ProcessState::ParseReqLine;
    m_method = string();        //重置指针
    m_url = string(); 
    m_version = string();
    m_reqHeaders.clear();
}

ProcessState HttpRequest::getState()
{
    return m_curState;
}

void HttpRequest::addHeader(const string key, const string value)
{
    if(key.empty() || value.empty()) {
        return; 
    }
    m_reqHeaders.insert(make_pair(key, value));
}

////只有客户端发送的是基于post的http请求，此函数才会用到（需要得到content-type等类型）
string HttpRequest::getHeader(const string key)
{
    auto item = m_reqHeaders.find(key);
    if(item == m_reqHeaders.end()) {
        return string();
    }
    return item->second;
}


//分割请求行辅助函数
char* HttpRequest::splitRequestLine(const char* start, const char* end, const char* sub, function<void(string)> callback)   //sub是指要匹配的字符串
{
    char* space = const_cast<char*>(end);   //初始化为end是因为如果sub等于null，length可以正确计算
    if(sub != nullptr) { 
        space = static_cast<char*>(memmem(start, end-start, sub, strlen(sub)));  //返回的结果是匹配子串的起始位置
        assert(space != NULL);  
    }
    int length = space - start;
    //调用回调函数，将分割内容保存至对应的m_method、m_url及m_version字符串中
    callback(string(start, length));
    return space + 1; //方便下一次调用此函数（加1是为了跳过空格）
}

bool HttpRequest::parseHttpRequestLine(Buffer *readBuf)
{
    // 读出请求行,保存字符串结束位置
    char* end = readBuf->findCRLF();
    //保存字符串起始位置
    char* start = readBuf->getData();
    //请求行总长度
    int lineSize = end - start;

    if(lineSize > 0) {
        //请求行：get xxx/xxx.txt http/1.1
        //得到请求方式 get   
        auto methodFunc = bind(&HttpRequest::setMethod, this, placeholders::_1);   //可调用对象包装器不能包装类的非静态成员函数，需要使用可调用对象绑定的绑定为可调用对象
        start = splitRequestLine(start, end, " ", methodFunc);
        auto urlFunc = bind(&HttpRequest::setUrl, this, placeholders::_1);
        start = splitRequestLine(start, end, " ", urlFunc);
        auto versionFunc = bind(&HttpRequest::setVersion, this, placeholders::_1);
        splitRequestLine(start, end, NULL, versionFunc);
        //移动Buffer中的readPos指针---为解析请求头做准备  
        
        readBuf->readPosIncrease(lineSize);
        readBuf->readPosIncrease(2);   //跳过"\r\n",2个字符;
        //解析状态修改
        setState(ProcessState::ParseReqHeaders);
        return true;
    }
    return false;

}

bool HttpRequest::parseHttpRequestHeader(Buffer *readBuf)
{
    char* end = readBuf->findCRLF();   //找到\r, 注意：每行的结尾是\r\n
    if(end != nullptr){
        char* start = readBuf->getData();
        int lineSize = end - start;
        //基于键值对的“: ”取搜索key和value 
        char* middle = static_cast<char*>(memmem(start, lineSize, ": ", 2));
        if(middle != nullptr) {

            int keyLen = middle - start;
            int valueLen = end - middle - 2;
            if(keyLen > 0 && valueLen > 0) {
                string key(start, keyLen);    //key值
                string value(middle + 2, valueLen);   //value值
                addHeader(key, value);
            } 
            //移动读数据的位置
            readBuf->readPosIncrease(lineSize);
            readBuf->readPosIncrease(2);
        }   
        else {  //如果middle为空，则证明解析到了get请求的第三部分--->空行，忽略post请求
            //跳过空行
            readBuf->readPosIncrease(2);
            //修改解析状态
            setState(ProcessState::ParseReqDone);   //忽略了post请求，如果是post的请求就得解析第四部分的数据块
        }
        return true;
    }
    return false;
}

bool HttpRequest::parseHttpRequest(Buffer *readBuf, HttpResponse *response, Buffer *sendBuf, int socket)
{
    bool flag = true;
    while(m_curState != ProcessState::ParseReqDone) {
        switch (m_curState)
        {
        case ProcessState::ParseReqLine:
            flag = parseHttpRequestLine(readBuf);
            break;
        case ProcessState::ParseReqHeaders:
            flag = parseHttpRequestHeader(readBuf);
            break;
        case ProcessState::ParseReqBody:
            break;
        default:
            break;  
        }
        if(!flag) {
            return flag;
        }
        //判断是否解析完毕了，如果解析完毕了，需要准备回复的数据
        if(m_curState == ProcessState::ParseReqDone) {
            //1. 根据解析出的原始数据，对客户端的请求做出处理
            // Debug("到这一步了没呢。。。。。。。。。。。。");    //到这步没问题
            processHttpRequest(response);
            //2. 组织响应数据并发送给客户端
            response->prepareMsg(sendBuf, socket);
        }
    }
    setState(ProcessState::ParseReqLine);   //状态还原，保证还能继续处理第二条及之后的客户端请求
    return flag;
}
 
//处理基于get的http请求
bool HttpRequest::processHttpRequest(HttpResponse *response)
{
    //str：字符串，case：不区分大小写，cmp: 比较
    if(strcasecmp(m_method.data(), "get") != 0) {    //data是将字符串转化为char*类型（c_str()也有这个功能）
        return -1;
    }
    m_url = decodeMsg(m_url);
    //处理客户端请求的静态资源（目录或者文件）
    //（1）获取文件的属性---是根目录，还是子目录或文件
    //注意****************如果m_url.data()返回值类型是const char*， 则这需要修改，添加const属性，c++17之后返回值类型就无const了
    const char* file = nullptr;              
    if(strcmp(m_url.data(), "/") == 0) {
        file = "./";          //如果请求的资源为服务器的根目录，则把根目录转化为相对路径
    }
    else{
        file = m_url.data() + 1;
    }
    //（2）判断请求的资源在服务器中是否存在
    struct stat st;    //用于存储获取到的文件属性信息
    int ret = stat(file, &st);    //获取文件的属性并存储至st
    // Debug("是否成功获取到了请求资源的属性..........");      //可以到达这里

    if(ret == -1) {              //如果文件不存在--即属性获取失败
        //文件不存在---回复404
        response->setFileName("404.html");
        response->setStatusCode(StatusCode::NotFound);
        //响应头
        response->addHeader("Content-type", getFileType(".html"));
        response->sendDataFunc = sendFile;
        return 0;
    }
    response->setFileName(file);
    response->setStatusCode(StatusCode::OK);
    //（3）判断资源类型(目录 or 非目录)
    if(S_ISDIR(st.st_mode)) {
        //把这个目录中的内容发送给客户端
        //响应头
        response->addHeader("Content-type", getFileType(".html"));     //发送目录网页
        response->sendDataFunc = sendDir;
    }
    else {
        //把文件的内容发送给客户端
        // strcpy(response->fileName, file);
        // response->statusCode = OK;
        // strcpy(response->statusMsg, "Ok");
        //响应头
        response->addHeader("Content-type", getFileType(file));    //发送文件的内容
        response->addHeader("Content-length", to_string(st.st_size));   //发送文件大小
        response->sendDataFunc = sendFile;
    }    
    return false;
}

// 解码辅助函数
int HttpRequest::hexToDec(char c)
{
    if(c >= '0' && c <= '9') {
        return c - '0';
    }
    if(c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    if(c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    return 0;
}

//from为被解码的数据
string HttpRequest::decodeMsg(string msg)
{
    string str = string();   //存储解码的结果
    const char* from = msg.data();
    //Linux%E5%86%85%E6%A0%B8.jpg
    for(; *from != '\0'; ++from) {
        //isxdigit判断是不是16进制格式，取值在0-f
        if(from[0] == '%' && isxdigit(from[1]) && isxdigit(from[2])){
            //16进制  --->  10进制int   ----> 字符型char （整型到字符型存在隐式的自动类型转换）
            str.append(1, hexToDec(from[1])*16 + hexToDec(from[2]));
            from += 2;
        }
        else{
            //拷贝字符，复制
            str.append(1, *from);
        }
    }
    str.append(1, '\0');    //字符串结尾
    return str;
}

const string HttpRequest::getFileType(const string name)
{
    //判断的类型如：a.jpg、a.mp4、a.html
    //从左向右找“.”，找到之后从左向右就可以得到完整的后缀名
    const char* dot = strrchr(name.data(), '.');    //strrchr是从右向左找
    if(dot == nullptr) {
        return "text/plain; charset=utf-8";   //纯文本   ----返回值是char*类型，而函数的类型是string,这里存在隐式的类型转换
    }
    if(strcmp(dot, ".html") == 0 || strcmp(dot, ".htm") == 0){
        return "text/html; charset=utf-8";
    }
    if(strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0){
        return "image/jpeg";
    }
    if(strcmp(dot, ".gif") == 0){
        return "image/gif";
    }
    if(strcmp(dot, ".png") == 0){
        return "image/png";
    }
    if(strcmp(dot, ".css") == 0){
        return "text/css";
    }
    if(strcmp(dot, ".au") == 0){
        return "audio/basic";
    }
    if(strcmp(dot, ".wav") == 0){
        return "audio/wav";
    }
    if(strcmp(dot, ".avi") == 0){
        return "video/x-msvideo";
    }
    if(strcmp(dot, ".mov") == 0 || strcmp(dot, ".qt") == 0){
        return "video/quicktime";
    }
    if(strcmp(dot, ".mpeg") == 0 || strcmp(dot, ".mpe") == 0){
        return "video/mpeg";
    }
    if(strcmp(dot, ".vrml") == 0 || strcmp(dot, ".wrl") == 0){
        return "model/vrml";
    }
    if(strcmp(dot, ".midi") == 0 || strcmp(dot, ".mid") == 0){
        return "audio/quicktime";
    }
    if(strcmp(dot, ".mp3") == 0){
        return "audio/mpeg";
    }
    if(strcmp(dot, ".mp4") == 0) {
        return "video/mp4";
    }
    if(strcmp(dot, ".ogg") == 0) {
        return "application/ogg";
    }
    if(strcmp(dot, ".pac") == 0) {
        return "application/x-ns-proxy-autoconfig";
    }
    return "text/plain; charset=utf-8";
}


/**html网页结构  ----如果客户端请求的资源为目录，则需要组织成html网页发送给客户端
 * <html>
 *      <head>
 *          <title>name</title>
 *          <meta http-equiv="content-type" content="text/html; charset=UTF-8">   //解决网页乱码问题
 *      </head>
 *      <body>
 *          <table>
 *              <tr>
 *                  <td>内容</td>
 *              </tr>
 *              <tr>
 *                  <td>内容</td>
 *              </tr>
 *          </table> 
 *      </body>
 * </html>
 */
//需要把他定义为与 组织回复给客户端数据块的函数指针 相同的类型，这样才能让函数指针指向该函数的地址，调用该函数
void HttpRequest::sendDir(const string dirName, Buffer *sendBuf, int cfd)
{
    char buf[4096] = { 0 };
    sprintf(buf, "<html><head><title>%s</title><meta charset=\"%s\"></head><body><table>", dirName.data(), "UTF-8"); 
    struct dirent** namelist;
    int num = scandir(dirName.data(), &namelist, NULL, alphasort);
    for(int i=0; i<num; ++i) {
        //取出文件名namelist，指向的是一个指针数组struct diren* tmp[]
        char* name = namelist[i]->d_name;    //取出来的是相对于dirName的相对路径，需要拼接一下
        struct stat st;
        char subPath[1024] = { 0 };
        sprintf(subPath, "%s/%s", dirName.data(), name);
        stat(subPath, &st);   //用于判断subPath是文件还是目录
        if(S_ISDIR(st.st_mode)) {
            sprintf(buf + strlen(buf), "<tr><td><a href=\"%s/\">%s</a></td>%ld<td></td></tr>",name, name, st.st_size);
        }
        else{
            sprintf(buf + strlen(buf), "<tr><td><a href=\"%s\">%s</a></td>%ld<td></td></tr>",name, name, st.st_size);
        }
        // send(cfd, buf, strlen(buf), 0);    
        sendBuf->appendString(buf);   //将要发送的数据写入sendBuf
#ifndef MSG_SEND_AUTO
        sendBuf->sendData(cfd);    //第二种数据发送方式-----写一部分发送一部分(第一种是全部写入sendBuf后再发送：存在可能写不下程序死掉的问题)
#endif
        memset(buf, 0, sizeof(buf));
        free(namelist[i]);     //释放一级指针
    }
    sprintf(buf, "</table></body></html>");
    // send(cfd, buf, strlen(buf), 0);
    sendBuf->appendString(buf);   //将要发送的数据写入sendBuf
#ifndef MSG_SEND_AUTO
    sendBuf->sendData(cfd);   //第二种数据发送方式-----写一部分发送一部分
#endif
    free(namelist);   //释放一下二级指针
    // return 0;

}

void HttpRequest::sendFile(const string filename, Buffer * sendBuf, int cfd)
{
    //1.打开文件
    int fd = open(filename.data(), O_RDONLY);   //只读的方式打开文件
    assert(fd > 0);   //断言判断文件是否打开成功
#if 1
    while(1){
        char buf[1024] = { 0 };
        int len = read(fd, buf, sizeof buf);
        if(len > 0) {
            // send(cfd, buf, len, 0);
            sendBuf->appendString(buf, len);   //不能使用bufferAppendString，因为这里的buf不一定是以\0结尾，会出错
#ifndef MSG_SEND_AUTO
            sendBuf->sendData(cfd);     //第二种数据发送方式-----写一部分发送一部分
#endif            
            // usleep(1);    //已经在bufferSendData中让程序休眠了1s
        }
        else if(len == 0) {
            break;
        }
        else{
            close(cfd);      //读文件失败，把文件描述符关掉
            perror("read");
            // return -1;
        }
    }
#else      //我们需要把文件存储到buf中，而不是直接发送给客户端
    //使用这种方式发送数据只适合发送文件，不适合发送目录（所以如果要使用这种高效率发送数据的方式，需要使用另外一个函数来发送目录）
    //使用sendfile效率更高---号称“零拷贝函数”，减少了在内核态的拷贝
    int size = lseek(fd, 0, SEEK_END);   //通过指针从头到尾的偏移量获得文件的大小---此时指针已经移动到文件尾部了
    lseek(fd, 0, SEEK_SET);
    off_t offset = 0;      //设置偏移量位置
    while(offset < size) {
        int ret = sendfile(cfd, fd, &offset, size-offset);    //从偏移处开始发数据，发完之后更新偏移量
        if(ret == -1) {
            perror("sendfile");
        }
    }
#endif
    close(fd);
    // return 0;

}
