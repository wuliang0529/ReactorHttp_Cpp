#pragma once

#include "EventLoop.h"
#include <thread>
#include <string>
#include <mutex>
#include <condition_variable>
using namespace std;

class WorkerThread
{
public:
    WorkerThread(int index);
    ~WorkerThread();
    //启动线程
    void run();
    //在类外可以获取子线程的反应堆实例m_evLoop
    inline EventLoop* getEventLoop() {
        return m_evLoop;
    }
private:
    void running();   //子线程的任务函数
private:
    thread* m_thread;        //保存线程的指针
    thread::id m_threadID;
    string m_name;
    mutex m_mutex;    //互斥锁，实现线程同步
    condition_variable m_cond;   //条件变量，阻塞线程
    EventLoop* m_evLoop;   //反应堆模型--每个线程都有一个反应堆模型
};
