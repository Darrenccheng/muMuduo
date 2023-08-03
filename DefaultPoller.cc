#include "Poller.h"
#include "EPollPoller.h"

#include <stdlib.h>

Poller* Poller::newDeafultPoller(EventLoop *loop)
{
    if(::getenv("MUDUO_US_POLL"))
    {
        //返回poll
        return nullptr;
    }
    else
    {
        //返回epoll
        return new EPollPoller(loop); // 生成poller实例
    }
}