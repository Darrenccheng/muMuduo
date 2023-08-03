#include "Channel.h"
#include "EventLoop.h"
#include "Logger.h"

#include <sys/epoll.h>



//对感兴趣的事件类型的状态描述
const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;

Channel::Channel(EventLoop* loop, int fd)
    :loop_(loop), fd_(fd), events_(0), revents_(0), index_(-1), tied_(false)
{ }

Channel::~Channel()
{

}

//tie目前还没有调用过
void Channel::tie(const std::shared_ptr<void>& obj)
{
    tie_ = obj;
    tied_ = true;
}

//在channel所属的eventloop中，把当前的channel删除
void Channel::remove()
{
    loop_-> removeChannel(this);
}

//更新通道，相当于调用epoll_ctl，但是channel与poller属于两个组件，无法直接注册
//通过EvebtLoop来注册更新
void Channel::update()
{
    //通过channel所属的eventloop，来注册fd的events事件
    loop_->updateChannel(this);
}

//fd得到poller通知后，处理事件的
void Channel::handleEvent(Timestamp receiveTime)
{
    if(tied_)
    {
        std::shared_ptr<void> guard = tie_.lock();
        if(guard)
            handleEventWithGuard(receiveTime);
    }
    else
        handleEventWithGuard(receiveTime);
}

//根据poller发生的具体事件，有channel来负责具体的回调操作
void Channel::handleEventWithGuard(Timestamp receiveTime)
{
    //打印日志
    LOG_INFO("channel handleEvent revents: %d\n", revents_);

    if((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN))
    {
        if(closeCallback_)
            closeCallback_();
    }

    if(revents_ & EPOLLERR)
    {
        if(errorCallback_)
            errorCallback_();
    }

    if(revents_ & (EPOLLIN | EPOLLPRI))
    {
        if(readCallback_)
            readCallback_(receiveTime);
    }

    if(revents_ & EPOLLOUT)
    {
        if(writeCallback_)
            writeCallback_();
    }
}