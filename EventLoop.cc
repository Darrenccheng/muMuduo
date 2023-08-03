#include "EventLoop.h"
#include "Logger.h"
#include "Poller.h"
#include "EPollPoller.h"
#include "Channel.h"

#include<sys/eventfd.h>
#include<functional>


//  定义这个变量用于防止一个线程创建多个eventloop
__thread EventLoop* t_loopInThisThread = nullptr;

//  定义默认的Poller IO复用接口的超时时间
const int kPollTimeMs = 10000;

//  创建wakepfd, 用来通知唤醒subReactor处理新来的channel(每个subreactor都监听一个创建wakepfd的channel，当wakepfd事件来了后，就可以唤醒subchannel了)
int createEventfd()
{
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if(evtfd < 0)
    {
        LOG_FATAL("eventfd error:%d \n", errno);
    }
    return evtfd;
}

EventLoop::EventLoop()
    : looping_(false)
    , quit_(false)
    , callingPendFunctors_(false)
    , threadId_(CurrentThread::tid())
    , poller_(Poller::newDeafultPoller(this))
    , wakeupFd_(createEventfd())
    , wakeupChannel_(new Channel(this, wakeupFd_))
{
    LOG_DEBUG("EventLoop created %p in thread %d \n", this, threadId_);
    if(t_loopInThisThread)
    {
        //这个线程中已经创建了一个loop了
        LOG_FATAL("Another EventLoop %p exists in this thread %d \n", t_loopInThisThread, threadId_);
    }
    else
    {
        t_loopInThisThread = this;
    }

    //设置wakeupfd的事件类型和发生时间后的回调操作(其实回调操作在这里做啥不重要，关键是唤醒这个subreactor)
    wakeupChannel_->setReadCallBack(std::bind(&EventLoop::headleRead, this));
    //每一个eventloop都将监听wakeupchannel的epollin读事件了，来了wakeupchannel的epollin读事件，就会唤醒这个reactor
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop()
{
    wakeupChannel_->disableAll();   // 对所有事件都不感兴趣
    wakeupChannel_->remove();   //  移除这个channel（用于唤醒reactor的channel）
    ::close(wakeupFd_);
    t_loopInThisThread = nullptr;
}

void EventLoop::headleRead()
{
    uint64_t one = 1;
    ssize_t n = read(wakeupFd_, &one, sizeof one);
    if(n != sizeof one)
    {
        LOG_ERROR("EventLoop::handleRead() reads %lu bytes instead of 8 \n", n);
    }
}

//开启事件的循环
void EventLoop::loop()
{
    looping_ = true;
    quit_ = false;

    LOG_INFO("EventLoop %p start looping \n", this);

    //主loop与子loop之间，不是通过队列的形式派送已发生的fd，而是轮询
    while(!quit_)
    {
        activeChannels_.clear();
        //监听两类的fd client的fd、wakeupfd，监听到的fd放在activeChannel,事件没有来时，会阻塞在这里
        pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
        for(Channel* channel : activeChannels_)
        {
            //poller监听到哪些channel发生了事件，然后上报给eventloop，eventlool通知channel处理相应事件
            channel->handleEvent(pollReturnTime_);
        }

        //执行当前EventLoop事件循环需要处理的回调操作
        /**                                                              
         * IO线程 在mainloop中，产生一个新连接的fd，
         * mainloop会事先注册一个回调cb（需要subloop执行，此时通过上面的for，唤醒了对应的subloop，现在要执行之前mainloop注册的cb操作
        */
       dopendingFunctors();
    }

    LOG_INFO("EventLoop %p stop looping. \n", this);
    looping_ = false;
}

//退出事件的循环 1.在自己的线程中调用quit（直接退出） 2. 在其他线程中调用loop的quit
//主loop与子loop ，通过wakeupfd来唤醒对方
void EventLoop::quit()
{
    quit_ = true;

    //如果在其他的线程中调用quit， 如，在一个subloop中调用了mainloop的quit，则先唤醒
    if(!isInLoopThread())
    {
        wakeup();
    }
}

//在当前loop中执行回调
void EventLoop::runInLoop(Functor cb)
{
    if(isInLoopThread()) // 在当前Loop线程中，执行cb
    {
        cb();
    }
    else   // 在非当前loop中执行cb，则需要唤醒loop所在的线程，执行cb
    {
        queueInLoop(cb);
    }
}

//把cb放入队列中，唤醒loop所在的线程，执行cb
void EventLoop::queueInLoop(Functor cb)
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        pendingFunctors_.emplace_back(cb);
    }

    //唤醒相应的，需要执行上面的操作的loop线程
    //callingPendFunctors_ : 表示当前loop正在执行回调，但是loop又有了新的回调
    if(!isInLoopThread() || callingPendFunctors_)
    {
        wakeup(); // 唤醒loop所在的线程
    }
}

//唤醒loop所在的线程 向一个wakeupfd写数据，wakeupchannel就会发生读事件 则该loop就会被唤醒
void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t n = write(wakeupFd_, &one, sizeof one);
    if(n != sizeof one)
    {
        LOG_ERROR("EventLoop::wakeup() writes %lu bytes instead of 8 \n", n);
    }
}

//channel中使用eventloop的方法 --》 使用poller的方法
void EventLoop::updateChannel(Channel* channel)
{
    poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel* channel)
{
    poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel* channel)
{
    return poller_->hasChannel(channel);
}

void EventLoop::dopendingFunctors()  // 执行回调
{
    std::vector<Functor> functors;
    callingPendFunctors_ = true;

    {
        std::unique_lock<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_);
    }

    for(const Functor &functor : functors)
    {
        functor(); // 执行当前loop需要执行的回调操作
    }

    callingPendFunctors_ = false;
}