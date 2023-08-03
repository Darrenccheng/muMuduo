#include "Acceptor.h"
#include "Logger.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

static int createNobolcking()
{
    int sockfd = ::socket(PF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if(sockfd < 0)
    {
        LOG_FATAL("%s:%s:%d listen socket create err:%d \n", __FILE__, __FUNCTION__, __LINE__, errno);
    }
    return sockfd;
}


Acceptor::Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reuseport)
    : loop_(loop)
    , acceptSocket_(createNobolcking()) // 创建socket，用于监听新用户的连接
    , acceptChannel_(loop, acceptSocket_.fd()) // 把sockfd打包成channel
    , listenning_(false)
{
    acceptSocket_.setResuseAddr(true);
    acceptSocket_.setResusePort(true);
    acceptSocket_.bindAddress(listenAddr); // 将socket与一个socket地址bind绑定
    // TcpServer::start() // 将Acceptor.Listen  有新用户连接，就执行一个回调connfd->channel，并放入到一个subloop中
    // baseLoop 把新连接打包成channel，进行回调
    acceptChannel_.setReadCallBack(std::bind(&Acceptor::handleRead, this));
}

Acceptor:: ~Acceptor()
{
    acceptChannel_.disableAll();
    acceptChannel_.remove();
}


void Acceptor::listen()
{
    listenning_ = true;
    acceptSocket_.listen();
    acceptChannel_.enableReading(); 
}

// listenfd有事件发生了，有新用户的连接了
void Acceptor::handleRead()
{
    InetAddress peerAddr; // 客户端的地址
    int connfd = acceptSocket_.accept(&peerAddr); // 得到监听读写事件的socket
    if(connfd >= 0)
    {
        if(newConnecionCallback_)
        {
            newConnecionCallback_(connfd, peerAddr); // 轮询找到一个subloop，唤醒，分发当前新客户端的channel
        }
        else
        {
            ::close(connfd);
        }
    }
    else
    {
        LOG_ERROR("%s:%s:%d accept create err:%d \n", __FILE__, __FUNCTION__, __LINE__, errno);
        if(errno == EMFILE)
        {
            LOG_ERROR("%s:%s:%d socket reached limit! \n", __FILE__, __FUNCTION__, __LINE__);
        }
    }
}