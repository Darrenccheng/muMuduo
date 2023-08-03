#include "TcpConnection.h"
#include "Socket.h"
#include "Channel.h"
#include "Logger.h"
#include "EventLoop.h"

#include <functional>

TcpConnection::TcpConnection(EventLoop* loop,
                const std::string nameArg,
                int socketfd,
                const InetAddress& localAddr,
                const InetAddress& peerAddr)
    : loop_(loop)
    , name_(nameArg)
    , state_(kConnecting)
    , reading_(true)
    , socket_(new Socket(socketfd))
    , channel_(new Channel(loop_, socketfd))
    , localAddr_(localAddr)
    , peerAddr_(peerAddr)
    , highWaterMark_(64*1024*1024) // 64M
{
    // 给channel设置相应的回调函数，poller给channel通知感兴趣的时间发生了，channel就调用这个回调函数
    channel_->setReadCallBack(std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));
    channel_->setCloseCallBack(std::bind(&TcpConnection::handleClose, this));
    channel_->setWriteCallBack(std::bind(&TcpConnection::handleWrite, this));
    channel_->setErrorCallBack(std::bind(&TcpConnection::handleError, this));

    LOG_INFO("TcpConnection::ctor[%s] at fd = %d \n", name_.c_str(), socketfd);
    socket_->setKeepAlive(true);
}


TcpConnection::~TcpConnection()
{
    LOG_INFO("TcpConnection::dtor[%s] at fd = %d state=%d\n", name_.c_str(), channel_->fd(), (int)state_);
}

void TcpConnection::send(const std::string& buf)
{
    if(state_ == kConnected)
    {
        if(loop_->isInLoopThread())
        {
            sendInLoop(buf.c_str(), buf.size());
        }
        else
        {
            loop_->runInLoop(std::bind(&TcpConnection::sendInLoop, this, buf.c_str(), buf.size()));
        }
    }
}

// 发送数据，应用写得快，而内核把数据发送出去的慢，需要把待发送的数据写入缓冲区（如果内核发出去的速度没有应用块的话），而且设置了水位高度
void TcpConnection::sendInLoop(const void* data, size_t len)
{
    ssize_t nwrote = 0; // 具体发送的个数
    size_t remaining = len; // 剩余没有发送的
    bool faultError = false;

    // 之前调用过该connection的shutdown，已经关闭了连接，不能再进行发送了
    if(state_ == kDisconnected)
    {
        LOG_ERROR("disconnected, give up writing");
        return;
    }

    // 一开始只会对channel设置读感兴趣时间，则第一次写数据时，对写事件不感兴趣
    // 表示channel_第一次开始写数据，且缓冲区没有待发送的数据
    if(!channel_->isWriting() || outputBuffer_.readableBytes() == 0)
    {
        // 直接write
        nwrote = ::write(channel_->fd(), data, len);
        if(nwrote >= 0)
        {
            // 发送成功
            remaining = len - nwrote; // 剩余没有发送的
            if(remaining == 0 && writeComplateCallback_)
            {
                //全部发送完成，不需要再让channel对epollout事件感兴趣了，即不用再执行handlewerite回调了（不会再触发写回调向fd写数据了）
                loop_->queueInLoop(std::bind(writeComplateCallback_, shared_from_this()));

            }
        }
        else // nwrote < 0
        {
            nwrote = 0;
            if(errno != EWOULDBLOCK)
            {
                LOG_ERROR("TcpConnection::sendInLoop");
                if(errno == EPIPE || errno == ECONNRESET)
                {
                    faultError = true;
                }
            }
        }
    }

    // 说明当前这一次write没有把数据全部都发送出去，剩余的数据需要写入到缓冲区中，然后给channel，触发channel的epollout
    // 注册epollout事件，poller发现tcp缓冲区中有数据，就会通知channel，调用writeCallback回调
    // 就是调用tcpconnection：：handlewrite，把发送缓冲区中的数据全部发送完成
    if(!faultError && remaining > 0)
    {
        size_t oldLen = outputBuffer_.readableBytes(); // 目前缓冲区中待发送的长度
        if(oldLen + remaining >= highWaterMark_
            && oldLen < highWaterMark_
            && highWaterMarkCallback_)
        {
            loop_->queueInLoop(std::bind(highWaterMarkCallback_, shared_from_this(), oldLen + remaining));
        }
        outputBuffer_.append((char*)data + nwrote, remaining); // 把剩下没写完的放入缓冲区中
        if(!channel_->isWriting()) // 要让channel对写事件感兴趣，这样才能触发回调，把缓冲区中的数据写出来
        {
            channel_->enableWriting();
        }
    }
}

 // 关闭连接
void TcpConnection::shutdown()
{
    if(state_ == kConnected)
    {
        setState(kDisconnecting); // 缓冲区中还有数据人可以发送没发送完成后，在handlewrite中，kDisconnecting时会调用shutdowninloop
        loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
    }
}

void TcpConnection::shutdownInLoop()
{
    if(!channel_->isWriting()) // outbuffer中的数据已经全部发送完成了
    {
        socket_->shutdownWrite(); // 关闭写端
    }
}

// 建立连接
void TcpConnection::connectEstablished()
{
    setState(kConnected);
    channel_->tie(shared_from_this()); // channel与当前的tcpconnection对象绑定
    channel_->enableReading(); // 像poller注册channel的epollin事件

    connectionCallback_(shared_from_this()); // 连接建立和连接断开都会调用
}

// 销毁连接
void TcpConnection::connectDestroyed()
{   
    if(state_ == kConnected)
    {
        setState(kDisconnected);
        channel_->disableAll(); // 从poller中del
        connectionCallback_(shared_from_this());
    }
    channel_->remove(); // 把channel从poller中删除
}


void TcpConnection::handleRead(Timestamp receiveTime)
{
    int saveErrno = 0;
    ssize_t n = inputBuffer_.readFd(channel_->fd(), &saveErrno);
    if(n > 0) // 触发了读取事件，且读取到了数据
    {
        // 已建立连接的用户，与可读事件发生了，调用用户传入的会掉操作
        messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
    }
    else if(n == 0)
    {
        handleClose();
    }
    else{
        errno = saveErrno;
        LOG_ERROR("TcpConnection::handleRead");
        handleError();
    }
}

void TcpConnection::handleWrite()
{
    if(channel_->isWriting())
    {
        int saveErrno = 0;
        ssize_t n = outputBuffer_.writeFd(channel_->fd(), &saveErrno);
        if(n > 0)
        {
            outputBuffer_.retrieve(n);
            if(outputBuffer_.readableBytes() == 0)
            {
                // 缓冲区中的数据写完了，全部放到了fd中
                channel_->disableWriting();
                if(writeComplateCallback_)
                {
                    loop_->queueInLoop(std::bind(writeComplateCallback_, shared_from_this()));
                }
                if(state_ == kDisconnecting)
                {
                    shutdownInLoop();
                }
            }
        }
        else
        {
            LOG_ERROR("TcpConnection::handleWrite");
        }
    }
    else
    {
        LOG_ERROR("TcpConnection fd=%d is down, on more writing \n", channel_->fd());
    }
}

// poller -> channel.closeCallback -> tcpconnection::handClose
void TcpConnection::handleClose()
{
    LOG_INFO("TcpConnection::handleClose fd=%d state=%d", channel_->fd(), (int)state_);
    setState(kDisconnected);
    channel_->disableAll();

    TcpConnectionPtr connPtr(shared_from_this());
    connectionCallback_(connPtr); // 执行连接关闭的回调(用户创建的业务处理函数)
    closeCallback_(connPtr); // 关闭连接的回调 执行的为TcpServer::removeConnection回调方法
}

void TcpConnection::handleError()
{
    int optval;
    socklen_t optlen = sizeof optval;
    int err = 0;
    if(::getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
    {
        err = errno;
    }
    else 
    {
        err = optval;
    }
    LOG_ERROR("TcpConnection::handleError name:%s - SO_ERROR:%d \n", name_.c_str(), err);
}