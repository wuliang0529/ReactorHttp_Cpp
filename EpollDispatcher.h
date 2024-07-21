#pragma once
#include "Channel.h"
#include "EventLoop.h"
#include "Dispatcher.h"
#include <string>
#include <sys/epoll.h>
using namespace std;


class EpollDispatcher : public Dispatcher {  
public:
    // 初始化---初始化poll、epoll或者select需要的数据块DispatherData 
    EpollDispatcher(EventLoop* evloop);
    // 添加---待检测的文件描述符添加到poll、epoll或者是select中
    int add() override;
    // 修改
    int remove() override;
    // 删除
    int modify() override;
    // 事件监测，监测三种模型中是否有文件描述符被激活
    int dispatch(int timeout = 2) override;   //超时阈值timeout单位：s，默认值为2s
    // 清除数据（关闭fd或者释放内存）
    ~EpollDispatcher();
private:
    //由于epoll_ADD  epoll_Remove  epoll_Modify实现基本相同，故自定义epollCtl减少代码冗余
    int epollCtl(int op);
    int m_epfd;   //根节点
    struct epoll_event* m_events;
    const int m_maxNode = 520;     //最大节点数
};
