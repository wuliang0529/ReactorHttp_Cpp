#pragma once
#include "Channel.h"
#include "EventLoop.h"
#include "Dispatcher.h"
#include <string>
#include <sys/select.h>
using namespace std;


class SelectDispatcher : public Dispatcher {  
public:
    // 初始化---初始化poll、epoll或者select需要的数据块DispatherData 
    SelectDispatcher(EventLoop* evloop);
    // 析构函数---清除数据（关闭fd或者释放内存）
    ~SelectDispatcher();
    // 添加---待检测的文件描述符添加到poll、epoll或者是select中
    int add() override;
    // 修改
    int remove() override;
    // 删除
    int modify() override;
    // 事件监测，监测三种模型中是否有文件描述符被激活
    int dispatch(int timeout = 2) override;   //超时阈值timeout单位：s，默认值为2s
private:
    //避免代码冗余，定义额外函数
    void setFdSet(); 
    void clearFdSet();
    //在文件描述符集合readSet和writeSet中最多存储1024个，在内核写死的
    fd_set m_readSet;      // 文件描述符读集合，对内核的操作小于等于传入的文件描述符（有些未被激活）
    fd_set m_writeSet;    //文件描述符写集合，写从内核传出的数据
    const int m_maxSize = 1024;
};
