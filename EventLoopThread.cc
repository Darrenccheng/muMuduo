#include "EventLoopThread.h"
#include "EventLoop.h"

EventLoopThread::EventLoopThread(const ThreadInitCallback& cb, const std::string& name)
    : loop_(nullptr)
    , exiting_(false)
    , thread_(std::bind(&EventLoopThread::threadFunc, this), name)
    , mutex_()
    , cond_()
    , callback_(cb)
{

}

EventLoopThread:: ~EventLoopThread()
{
    exiting_ = true;
    if(loop_ != nullptr)
    {
        loop_ = nullptr;
        thread_.join();
    }
}

EventLoop* EventLoopThread::startLoop()
{
    thread_.start(); // 每此调用都会启动一个新线程，并在下面开启一个eventloop 即：one loop per thread
    EventLoop* loop = nullptr; // 每开启一个loop
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while( loop_ == nullptr)
            cond_.wait(lock); // 资源没有创建好，阻塞线程
        loop = loop_;
    }
    return loop;
}

//这个方法单独在新的线程中运行（线程函数）
void EventLoopThread::threadFunc()
{
    EventLoop loop; // 创建一个独立的eventloop，和上面的线程是意义对应的one loop per thread

    if(callback_)
    {
        callback_(&loop);
    }

    {
        std::unique_lock<std::mutex> lock(mutex_);
        loop_ = &loop; // 创建完loop_后，就不为空了，就可以退出上面一个函数的while循环了
        cond_.notify_one(); // 资源创建好了，唤醒阻塞的线程
    }

    loop.loop(); // 开启循环， evevtLoop => Poller
    std::unique_lock<std::mutex> lock(mutex_);
    loop_ = nullptr;
}

