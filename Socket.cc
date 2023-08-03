#include "Socket.h"
#include "InetAddress.h"
#include "Logger.h"

#include <unistd.h>
#include <strings.h>
#include<sys/socket.h>
#include <netinet/tcp.h>

Socket::~Socket()
{
    close(sockfd_);
}

// 将socket 与 一个socket地址绑定
void Socket::bindAddress(const InetAddress &localaddr)
{
    if(0 != ::bind(sockfd_, (sockaddr*)localaddr.getSockAddr(), sizeof(sockaddr_in)))
    {
        LOG_FATAL("bind sockfd: %d fail \n", sockfd_);
    }
}

void Socket::listen()
{
    if(0 != ::listen(sockfd_, 1024))
    {
        LOG_FATAL("listen sockfd: %d fail \n", sockfd_);
    }
}

int Socket::accept(InetAddress* peeraddr)
{
    sockaddr_in addr; // 被接受连接的远端socket地址
    socklen_t len = sizeof addr;
    bzero(&addr, len);
    int connfd = ::accept4(sockfd_, (sockaddr*)&addr, &len, SOCK_CLOEXEC | SOCK_NONBLOCK); // 服务端提供connfd来与远端的被连接的socket通信
    if(connfd > 0)
    {
        peeraddr->setSockAddr(addr);
    }
    return connfd;
}

void Socket::shutdownWrite()
{
    if(shutdown(sockfd_, SHUT_WR) < 0)
    {
        LOG_ERROR("shutdownwrite error");
    }
}

void Socket::setTcpNoDelay(bool on)
{
    int optval  = on ? 1 : 0;
    ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof optval);
}
void Socket::setResuseAddr(bool on)
{
    int optval  = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);
}
void Socket::setResusePort(bool on)
{
    int optval  = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof optval);
}

void Socket::setKeepAlive(bool on)
{
    int optval  = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof optval);
}