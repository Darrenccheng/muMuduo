#pragma once

#include "noncopyable.h"
#include "InetAddress.h"
#include "Socket.h"
#include "Channel.h"

#include <functional>

class EventLoop;

class Acceptor : noncopyable
{
public:
    using newConnecionCallback = std::function<void(int sockfd, const InetAddress&)>;

    Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reuseport);
    ~Acceptor();

    void setNewConnectionCallBcak(const newConnecionCallback& cb)
    {
        newConnecionCallback_ = cb;
    }

    bool listenning() const { return listenning_; }

    void listen();
private:
    void handleRead();

    EventLoop* loop_; // Acceptor 用的就是用户定义的那个baseLoop，也称为mainLoop，用于监听新用户的连接
    Socket acceptSocket_; // 用于监听新用户的socket
    Channel acceptChannel_;
    newConnecionCallback newConnecionCallback_; // 监听到新用户连接后的回调函数，把connfd打包称为channel，在用getnectloop唤醒一个subloop
    bool listenning_;
};