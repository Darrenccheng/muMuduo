#pragma once

#include "noncopyable.h"

#include <functional>
#include <vector>
#include <memory>

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool : noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    EventLoopThreadPool(EventLoop* baseLoop, const std::string &nameArg);
    ~EventLoopThreadPool();

    void setThreadNum(int numThreads) { numThreads_ = numThreads; }

    void start(const ThreadInitCallback& cb = ThreadInitCallback());

    // 如果是工作在多线程中（多个reactor),baseloop默认以轮询的方式分配channel给subloop
    EventLoop* getNextLoop();

    std::vector<EventLoop*> getAllLoops();

    bool started() const { return started_; }
    const std::string name() const { return name_; }
private:
    EventLoop* baseLoop_; // 用户创建的loop，至少存在这一个loop。一般只用于接受新连接，
                          //并把连接的fd发放给工作线程，主线程不用放在线程池中
    std::string name_;
    bool started_;
    int numThreads_;
    int next_;
    std::vector<std::unique_ptr<EventLoopThread>> threads_; //存储线程的线程池，只存储工作线程
    std::vector<EventLoop*> loops_; // 存储线程对应的loop
};