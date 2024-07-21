#include "ThreadPool.h"
#include <assert.h>    //断言函数assert的头文件
#include <stdlib.h>    //malloc的头文件
#include <pthread.h>   //thread_self头文件
#include <thread>
#include "Log.h"

ThreadPool::ThreadPool(EventLoop *mainLoop, int count)
{
    m_index = 0;
    m_isStart = false;
    m_mainLoop = mainLoop;
    m_threadNum = count;
    m_workerThreads.clear();
}

ThreadPool::~ThreadPool()
{
    //基于范围的循环----迭代器
    for(auto it : m_workerThreads) {
        delete it;
    }
}

//该函数必须被主线程执行
void ThreadPool::run()
{
    assert(!m_isStart);   //断言判断是否存在线程池或者线程池是否启动（不存在或者已经启动则直接返回）
    if(m_mainLoop->getThreadID() != this_thread::get_id()) {   //指向threadPoolRun函数的线程不是主线程
        exit(0);
    }
    m_isStart = true;
    if(m_threadNum > 0) {   //判断子线程数量是否大于0
        for(int i=0; i<m_threadNum; ++i) {
            //创建子线程
            WorkerThread* subThread = new WorkerThread(i);
            //启动子线程
            subThread->run();
            //添加子线程到vector
            m_workerThreads.push_back(subThread);
        }
    }
}

//该函数必须被主线程执行
EventLoop *ThreadPool::takeWorkerEventLoop()
{
    assert(m_isStart);   //断言判断线程池是否启动
    if(m_mainLoop->getThreadID() != this_thread::get_id()) {      //如果不是主线程
        exit(0);
    }
    //从线程池中找一个子线程，然后取出里边的反应堆实例
    EventLoop* evLoop = m_mainLoop;    //初始化evLoop为主线程的反应堆实例是因为，如果子线程个数小于0，则任务有主线程执行，这种情况为单反应堆模型
    if(m_threadNum > 0) {
        evLoop = m_workerThreads[m_index]->getEventLoop();
        m_index = ++m_index % m_threadNum;     //更新pool->index，保证任务分配雨露均沾
    }
    return evLoop;
}
