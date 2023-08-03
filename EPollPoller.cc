#include "EPollPoller.h"
#include "Logger.h"
#include "Channel.h"

#include<string.h>
#include<errno.h>
#include <unistd.h>

//channel未添加到poller中
const int kNew = -1;    //  channel的成员index_=-1（新创建好的channel，为knew状态）
//channel以及添加到poller中
const int kAdded = 1;
//channel从poller中删除了
const int kDeleted = 2;

EPollPoller::EPollPoller(EventLoop* loop)
    : Poller(loop)
    , epollfd_(::epoll_create1(EPOLL_CLOEXEC))
    , events_(KInitEventListSize)
{
    if(epollfd_ < 0) 
    {
        //create错误，则返回-1，且errno被设置为指示错误
        LOG_FATAL("epoll_create error:%d \n", errno);
    }
}

EPollPoller::~EPollPoller()
{
    ::close(epollfd_);
}

//
void EPollPoller::updateChannel(Channel* channel)
{
    const int index = channel->index();
    LOG_INFO("func=%s => fd=%d events=%d index = %d \n",__FUNCTION__, channel->fd(), channel->events(), index);

    if(index == kNew || index == kDeleted)
    {
        //这个channel从未添加到poller中
        if(index == kNew)
        {
            int fd = channel->fd();
            channels_[fd] = channel;
        }

        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD, channel);
    }
    else //这个channel已在poller中了
    {
        int fd = channel->fd();
        if(channel->isNoeEvent())
        {
            //channel对任何事情都不感兴趣了，则删除这个CHANNEL，但是还会在channelmap中
            channel->set_index(kDeleted);
            update(EPOLL_CTL_DEL, channel);
        }
        else
        {
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

//从poller中删除channel
void EPollPoller::removeChannel(Channel* channel) 
{
    int fd = channel->fd();
    channels_.erase(fd);

    LOG_INFO("fun=%s => fd=%d \n", __FUNCTION__, fd);

    int index = channel->index();
    //是添加过的channel,还在poller中
    if(index == kAdded)
    {
        update(EPOLL_CTL_DEL, channel);
    }
    channel->set_index(kNew);
}

void EPollPoller::update(int operation, Channel* channel)
{
    epoll_event event;
    bzero(&event, sizeof event);

    int fd = channel->fd();

    event.events = channel->events();
    event.data.fd = fd;
    event.data.ptr = channel;

    if(::epoll_ctl(epollfd_, operation, fd, &event) < 0)
    {
        //出错了
        if(operation == EPOLL_CTL_DEL)
        {
            //时删除有错误，则影响不大
            LOG_ERROR("epoll_ctl del error:%d \n",errno);
        }
        else
        {
            //添加或修改有误，则无法执行了，后面用到channel时会找不到
            LOG_FATAL("epoll_ctl add/mod error:%d\n", errno);
        }
    }
}

Timestamp EPollPoller::poll(int timeoutMS, ChannelList* activeChannels)
{
    LOG_DEBUG("fun=%s =>fd total count%lu \n", __FUNCTION__, channels_.size());

    //最多监听的个数：最多把events_放满
    int numEvents = ::epoll_wait(epollfd_, &(*events_.begin()), static_cast<int>(events_.size()), timeoutMS);
    int saveErrno = errno;  //  存储一下错误信息，防止在执行其他函数时，errno被修改
    Timestamp now(Timestamp::now());

    if(numEvents > 0)
    {
        //监听到了事件
        LOG_DEBUG("%d events happpend \n", numEvents);
        fillActiveChannls(numEvents, activeChannels);

        //如果返回的活跃事件填充满了，可能还不够，则需要扩容
        if(numEvents == events_.size())
        {
            events_.resize(2 * events_.size());
        }
    }
    else if(numEvents == 0)
    {
        //没有监听到事件，超时了
        LOG_DEBUG("%s timeout! \n", __FUNCTION__);
    }
    else
    {
        //错误
        if(saveErrno != EINTR)
        {
            //不是发生中断引起的
            errno = saveErrno;
            LOG_DEBUG("EPollPoller::Poll() err!");
        } 
    }

    return now;
}

void EPollPoller::fillActiveChannls(int numEvents, ChannelList* activeChannel)
{
    for(int i = 0; i < numEvents; i++)
    {
        Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
        channel->set_revents(events_[i].events);
        activeChannel->push_back(channel);
    }
}