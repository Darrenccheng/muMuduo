#pragma once

#include "noncopyable.h"
#include "Timestamp.h"

#include <functional>
#include <memory>

class EventLoop;

/*
EventLoop包括多个Channel +1个 epoll.   对应于reactor模型的事件分发器
epoll本身就是一个io复用的，用于监听事件
Channel封装了epoll中的sockfd、感兴趣的事件、绑定了poller具体返回的事件
Channel可以理解为通道。
*/
class Channel : noncopyable
{
public:
    //事件发生的函数回调对象类型
    using EventCallback = std::function<void()>;
    using ReadEventCallBack = std::function<void(Timestamp)>;

    Channel(EventLoop * loop, int fd);
    ~Channel();

    //fd得到poller通知后，处理事件的
    void handleEvent(Timestamp receiveTime);

    //设置回调函数
    void setReadCallBack(ReadEventCallBack cb) { readCallback_ = std::move(cb); }
    void setWriteCallBack(EventCallback cb) { writeCallback_ = std::move(cb); }
    void setCloseCallBack(EventCallback cb) { closeCallback_ = std::move(cb); }
    void setErrorCallBack(EventCallback cb) { errorCallback_ = std::move(cb); }

    //防止channel被remove了，channel还在执行回调操作，使用弱指针来监测channel是否还在
    void tie(const std::shared_ptr<void>& obj);

    int fd() const {return fd_; }
    int events() const {return events_; }
    void set_revents(int revt) {revents_ = revt; }

    //设置fd相应的事件状态,有poller来设置，所以用update
    void enableReading() { events_ |= kReadEvent; update(); }
    void disableReading() { events_ &= ~kReadEvent; update(); }
    void enableWriting() { events_ |= kWriteEvent; update(); }
    void disableWriting() { events_ &= ~kWriteEvent; update(); }
    void disableAll() { events_ = kNoneEvent; update(); }

    //返回fd当前事件状态
    bool isNoeEvent() const { return events_ == kNoneEvent; }
    bool isReading() const { return events_ & kReadEvent; }
    bool isWriting() const { return events_ & kWriteEvent; }

    int index() { return index_; }
    void set_index(int idx) { index_ = idx; }

    //一个线程对应一个时间分发器->one loop per thread
    //返回当前channel所在的loop
    EventLoop* ownerLoop() { return loop_; }
    void remove();
private:

    void update();
    void handleEventWithGuard(Timestamp receiveTime);

    //对感兴趣的事件类型的状态描述
    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop* loop_; // 事件循环
    const int fd_; // fd，poller监听的对象
    int events_; //注册fd感兴趣的事件
    int revents_; //poller返回的具体的事件
    int index_;

    std::weak_ptr<void> tie_;//用于监视channel对象，是否被析构
    bool tied_;


    //channel通道获取fd最终发生的具体事件events，所以他负责调用具体事件的回调函数
    ReadEventCallBack readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;
};