#pragma once
#include "EventLoop.h"
#include "WorkerThread.h"
#include <stdbool.h>
#include <vector>
using namespace std;

//定义线程池
class ThreadPool
{
public:
    ThreadPool(EventLoop* mainLoop, int count);
    ~ThreadPool();
    //启动线程池
    void run();
    //从线程池中取出某个线程的反应堆实例（存储对任务的一系列处理操作）---即选择处理任务的子线程，把任务队列中的任务放置到所选子线程的反应堆模型中
    EventLoop* takeWorkerEventLoop();
private:
    //主线程的反应堆模型--线程池中子线程数量为0时调用这个模型
    EventLoop* m_mainLoop;
    bool m_isStart;
    int m_threadNum;    //保存子线程的个数，即count的值
    vector<WorkerThread*> m_workerThreads;   //子线程数组指针
    int m_index;   //vector容器的下表索引
};

