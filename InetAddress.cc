#include "InetAddress.h"

#include<strings.h>
#include <string.h>
#include <arpa/inet.h>

InetAddress::InetAddress(uint16_t port, std::string ip )
{
    bzero(&addr_,sizeof addr_);
    addr_.sin_family= AF_INET;
    addr_.sin_port= htons(port);
    addr_.sin_addr.s_addr= inet_addr(ip.c_str());
}

std::string InetAddress::toIp() const
{
    char buff[64]= {0};
    ::inet_ntop(AF_INET,&addr_.sin_addr,buff, sizeof buff); //网络字节序转化为字符串
    return buff;
}

std::string InetAddress::toIpPort() const
{
    char buff[64]= {0};
    ::inet_ntop(AF_INET,&addr_.sin_addr,buff, sizeof buff); //网络字节序转化为字符串
    size_t buffend = strlen(buff);
    uint16_t port= ntohs(addr_.sin_port);
    sprintf(buff+buffend, ":%u", port);
    return buff;
}

uint16_t InetAddress::toPort() const
{
    return ntohs(addr_.sin_port);
}

// #include <iostream>
// int main()
// {
//     InetAddress addr(8080);
//     std::cout << addr.toIpPort() << std::endl;

//     return 0;
// }