#include "WorkerThread.h"
#include <stdio.h>      //sprintf对应的头文件
#include <pthread.h>    //pthread_create头文件
#include "Log.h"

//子线程的任务函数
void WorkerThread::running() {     //注意：这是子线程执行的函数
    m_mutex.lock();
    m_evLoop = new EventLoop(m_name);
    m_mutex.unlock();
    //唤醒阻塞在条件变量上的某"一个"线程
    m_cond.notify_one();    //子线程创建完毕，唤醒主线程---让主线程获得锁，并继续执行
    m_evLoop->run();    //启动子线程
}


WorkerThread::WorkerThread(int index)
{
    m_threadID = this_thread::get_id();   //得到一个无效的id
    m_name = "SubThread-" + to_string(index);
    m_evLoop = nullptr;
    m_thread = nullptr;
}

WorkerThread::~WorkerThread()
{
    if(m_thread != nullptr) {
        delete m_thread;
    }
}

void WorkerThread::run()
{
    //创建子线程--需要给线程传递一个任务函数的地址，否则线程无法启动
    m_thread = new thread(&WorkerThread::running, this);    //第二个参数this是running的所有者    
    //检查子线程是否创建完毕
    //locker是局部对象，当被构造之后会自动对管理的所m_mutex加锁，等到wait方法的时候就把m_mutex锁解开
    unique_lock<mutex> locker(m_mutex);
    while (m_evLoop == nullptr)   //循环检查子线程的反应堆模型是否创建完毕
    {
        m_cond.wait(locker);    //阻塞主线程并释放m_mutex锁，以便让子线程获取并执行（子线程执行完毕调用notify_one唤醒主线程，此时locker对互斥锁加锁）
    }
}  //locker生命周期结束，并自动解开所管理的互斥锁

