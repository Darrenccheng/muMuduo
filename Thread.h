#pragma once

#include <memory>
#include <thread>
#include <functional>
#include <string>
#include <atomic>

#include "noncopyable.h"

class Thread : noncopyable
{
public:
    using ThreadFunc = std::function<void()>;

    explicit Thread(ThreadFunc, const std::string name = std::string());
    ~Thread();

    void start();
    void join();

    bool started() const { return started_; }
    pid_t tid() const { return tid_; }
    const std::string& name() const { return name_; }

    static int numCreated() { return numCreated_; }
private:
    void setDefaultName();

    bool started_; // 线程开始的标志
    bool joined_;  // 线程回收其他线程的标志
    std::shared_ptr<std::thread> thread_; // 使用C++提供的线程类，用只能指针控制线程对象产生的时机
    pid_t tid_;
    ThreadFunc func_; // 存储线程函数
    std::string name_; // 线程名字
    static std::atomic_int32_t numCreated_; //静态变量，记录产生的线程对象的个数
};