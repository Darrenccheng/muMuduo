#include "EventLoopThread.h"
#include "EventLoopThreadPool.h"

#include<memory>
/**
 * 多线程Reactor模型的典型使用：
 * 1个主线程baseLoop专门监听新客户的连接(listen)
 * accept获取一个客户端通信的clientfd(connfd)，然后把这个clientfd给一个工作线程（loop）（subloop)
 * 这个clientfd的读写操作，都在这个工作线程loop中完成
 * 这样的好处是：
 * 1. 主线程可以最大吞吐量的获取新连接，可以处理更大的并发量（因为只处理新的连接，已建立的
 * 连接的fd都在工作线程中处理
 * 2. 可以充分利用多喝cpu的能力， 一般 线程数 = cpu核心数
 * 
*/

EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseLoop, const std::string &nameArg)
    : baseLoop_(baseLoop)
    , name_(nameArg)
    , started_(false)
    , numThreads_(0)
    , next_(0)
{

}
EventLoopThreadPool::~EventLoopThreadPool()
{

}

void EventLoopThreadPool::start(const ThreadInitCallback& cb)
{
    started_ = true;

    for(int i = 0; i < numThreads_; i++)
    {
        char buf[name_.size() + 32];
        snprintf(buf, sizeof buf, "%s%d", name_.c_str(), i);
        // 创建一个EventLoopThread
        EventLoopThread* t = new EventLoopThread(cb, buf);
        threads_.push_back(std::unique_ptr<EventLoopThread>(t)); // 把线程放入线程池
        loops_.push_back(t->startLoop()); // 开启的线程对应的loop,并返回loop的地址
    }

    //若用户没有设置线程数量，则为单线程，一个loop监听用户连接与读写事件
    if(numThreads_ == 0 && cb)
    {
        cb(baseLoop_);
    }
}


// 如果是工作在多线程中（多个reactor),baseloop默认以轮询的方式分配channel给subloop
EventLoop* EventLoopThreadPool::getNextLoop()
{
    EventLoop* loop = baseLoop_;

    if(!loops_.empty()) // 通过一个轮询获取下一个处理事件的loop
    {
        loop = loops_[next_];
        next_++;
        if(next_ >= loops_.size())
            next_ = 0;
    }

    return loop;
}

// 主要是为了返回工作线程，而不是主线程。如果没有设置工作线程的数量，则baseloop也是主线程，这时才会返回主线程
std::vector<EventLoop*> EventLoopThreadPool::getAllLoops()
{
    if(loops_.empty())
    {
        return std::vector<EventLoop*>(1, baseLoop_);
    }
    else 
        return loops_;
}