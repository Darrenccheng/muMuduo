#include "Thread.h"
#include "CurrentThread.h"

#include <semaphore.h>

std::atomic_int32_t Thread::numCreated_(0);

Thread::Thread(ThreadFunc func, const std::string name)
    : started_(false)
    , joined_(false)
    , tid_(0)
    , func_(std::move(func))
    , name_(name)
{
    setDefaultName();
}

Thread::~Thread()
{
    // 只有线程开始了，且没有同时进行join()  join()会阻塞主线程
    if(started_ && !joined_)
    {
        thread_->detach(); // thread类提供了分离线程的方法 成为一个守护线程，主线程无法再取得子线程的控制权，主线程结束，守护线程就自动结束
    }
}

// 一个Thread对象，记录的为一个新线程的详细详细
void Thread::start()
{
    started_ = true;
    sem_t sem; // 信号量
    sem_init(&sem, false, 0);

    //开启线程
    thread_ = std::shared_ptr<std::thread> (new std::thread([&](){
        //获取tid的值
        tid_ = CurrentThread::tid();

        sem_post(&sem);
        //开启一个新的线程
        func_();
    }));

    // 必须等待获取上面新创建的线程id值
    sem_wait(&sem);
}

void Thread::join()
{
    joined_ = true;
    thread_->join(); // 相当于pthread_join()
}

void Thread::setDefaultName()
{
    int num = ++numCreated_;
    if(name_.empty())
    {
        char buf[32] = {0};
        snprintf(buf, sizeof buf, "Thread%d", num);
        name_ = buf;
    }
}