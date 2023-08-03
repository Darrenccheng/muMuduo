#pragma once

#include <vector>
#include <sys/epoll.h>

#include "Poller.h"

class EventLoop;

class EPollPoller : public Poller
{
public:
    //构造，相当于epoll_create,创造出文件描述符，即私有成员变量epollfd_
    EPollPoller(EventLoop* loop);
    //析构，相当于去掉epollfd   del
    ~EPollPoller() override;

    // 重载基类中的抽象方法
    //相当于epoll_wait
    Timestamp poll(int timeoutMS, ChannelList* activeChannels) override;
    //相当于epoll_ctl，使用第二个参数相当于mod/del
    void updateChannel(Channel* channel) override;
    void removeChannel(Channel* channel) override;

private:
    //初始epoll_event的初始长度
    static const int KInitEventListSize = 16;

    //填写活跃的连接
    void fillActiveChannls(int numEvents, ChannelList* activeChannel);
    //更新channel通道
    void update(int operation, Channel* channel);

    //epoll_wait第二个参数即为epoll_event的数组，这里换成了vector，便于扩容
    using EventList = std::vector<epoll_event>;

    int epollfd_;
    EventList events_;
};