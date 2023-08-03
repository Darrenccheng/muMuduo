#pragma onec

#include<functional>
#include<vector>
#include<atomic>
#include<memory>
#include<mutex>

#include "noncopyable.h"
#include "Timestamp.h"
#include "CurrentThread.h"

class Channel;
class Poller;

//事件循环类，包含了两个大模块：channel poller（epoll的抽象）
class EventLoop : noncopyable
{
public:
    //回调函数的类型
    using Functor = std::function<void()>;

    EventLoop();
    ~EventLoop();

    //开启事件的循环
    void loop();
    //退出事件的循环
    void quit();

    Timestamp pollReturnTime() const { return pollReturnTime_; }

    //在当前loop中执行回调
    void runInLoop(Functor cb);
    //把cb放入队列中，唤醒loop所在的线程，执行cb
    void queueInLoop(Functor cb);

    //唤醒loop所在的线程
    void wakeup();

    //channel中使用eventloop的方法 --》 使用poller的方法
    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);
    bool hasChannel(Channel* channel);

    //一个线程一个loop-》判断eventloop是否在自己的线程中
    bool isInLoopThread() const { threadId_ == CurrentThread::tid(); }
private:
    void headleRead();  //  wake up
    void dopendingFunctors();   //  执行回调
    
    using ChannelList = std::vector<Channel*>;

    std::atomic_bool looping_;  //  控制事件循环是否正常进行
    std::atomic_bool quit_; //  标识退出循环

    const pid_t threadId_;  //  记录当前loop锁在的线程id

    Timestamp pollReturnTime_;  //  poller返回事件channels的时间
    std::unique_ptr<Poller> poller_;

    int wakeupFd_;  //  主要作用：当mainloop获取一个新连接用户的channel，通过轮询算法选择一个subloop，通过这个成员来唤醒subloop来处理channel
    std::unique_ptr<Channel> wakeupChannel_;

    ChannelList activeChannels_;    //  保存活跃的channel

    std::atomic_bool callingPendFunctors_;  //  标识当前loop是否有需要执行的回调操作
    std::vector<Functor> pendingFunctors_;  //  存储loop需要执行的回调操作
    std::mutex mutex_;  //  互斥锁，保护vector容器的线程安全
};