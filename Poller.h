#pragma once

#include<unordered_map>
#include<vector>

#include "noncopyable.h"
#include "Timestamp.h"

class Channel;
class EventLoop;

class Poller : noncopyable
{
public:
    using ChannelList = std::vector<Channel*>; //存储poller监听到的发生的channel

    Poller(EventLoop* loop);
    virtual ~Poller() = default;

    //给所有IO复用保留统一的接口 相当于poll 或者epoll 具体的，由传输对象而定 传入超时时间、监听的channel 相当于epoll_wait
    virtual Timestamp poll(int timeoutMS, ChannelList* activeChannels) = 0;

    //相当于epoll_ctl中设置事件
    //修改注册在fd上的事件
    virtual void updateChannel(Channel* channel) = 0;
    //删除fd上的注册事件
    virtual void removeChannel(Channel* channel) = 0;

    //判断channel是否在poller中
    bool hasChannel(Channel*) const;

    //eventloop可以通过该接口获得 默认的 IO复用的具体实现（是poll还是poller）
    static Poller* newDeafultPoller(EventLoop* loop);
protected:
    //poller监听的是eventloop里的channel   map存的为poller感兴趣的事件
    //key:sockfd value:对应的channel
    using ChannelMap = std::unordered_map<int, Channel*>;
    ChannelMap channels_;
private:
    EventLoop* ownerLoop_; //poller所属的eventloop
};