#include <mymuduo/TcpServer.h>
#include <mymuduo/Logger.h>

#include <string>
#include <functional>

class EchoServer
{
public:
    EchoServer(EventLoop* loop,
            const InetAddress& addr,
            const std::string& name)
        : server_(loop, addr, name)
        , loop_(loop)
    {
        // 注册回调函数  
        server_.setConnectionCallback(
            std::bind(&EchoServer::onConnection, this, std::placeholders::_1)
        );

        server_.setMessgeCallback(
            std::bind(&EchoServer::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
        );

        server_.setThreadNum(3);
    }

    void start()
    {
        server_.start();
    }

private:
    // 建立连接或断开的回调函数
    void onConnection(const TcpConnectionPtr& conn)
    {
        if(conn->connected())
        {
            LOG_INFO("Connection UP : %s \n", conn->peerAddress().toIpPort().c_str());
        }
        else
        {
            LOG_INFO("Connection DOWN: %S \n", conn->peerAddress().toIpPort().c_str());
        }
    }

    // 可读写事件回调
    void onMessage(const TcpConnectionPtr& conn,
                Buffer* buf,
                Timestamp time)
    {
        std::string msg = buf->retrieveAllAsString(); // 从缓冲区读取数据，发送结果出去
        conn->send(msg);
        conn->shutdown(); // 写端关闭
    }

    EventLoop* loop_;
    TcpServer server_;
};

int main()
{
    EventLoop loop;
    InetAddress addr(8000);
    EchoServer server(&loop, addr, "EchoServer-01"); // Acceptor non-blocking listenfd create bind
    server.start(); // listen loopthread listenfd -> acceptChannel ->mainLoop
    loop.loop(); // 启动mainLoop的底层poller

    return 0;
}