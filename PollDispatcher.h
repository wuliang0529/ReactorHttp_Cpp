#pragma once
#include "Channel.h"
#include "EventLoop.h"
#include "Dispatcher.h"
#include <string>
#include <poll.h>
using namespace std;


class PollDispatcher : public Dispatcher {  
public:
    // 初始化---初始化poll、epoll或者select需要的数据块DispatherData 
    PollDispatcher(EventLoop* evloop);
    // 添加---待检测的文件描述符添加到poll、epoll或者是select中
    int add() override;
    // 修改
    int remove() override;
    // 删除
    int modify() override;
    // 事件监测，监测三种模型中是否有文件描述符被激活
    int dispatch(int timeout = 2) override;   //超时阈值timeout单位：s，默认值为2s
    // 清除数据（关闭fd或者释放内存）
    ~PollDispatcher();
private:
    int m_maxfd;    // 记录fds的最大字符位置
    struct pollfd* m_fds;    //每个fds[i]都是一个结构体，包括fd, events, revents
    int m_maxNode = 1024;
};
