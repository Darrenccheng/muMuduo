#pragma once

#include <memory>
#include <functional>

class Buffer;
class TcpConnection;
class Timestamp;

using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
using ConnectionCallback = std::function<void(const TcpConnectionPtr&)>; // 新连接来了，用户处理的回调函数
using CloseCallback = std::function<void(const TcpConnectionPtr&)>;
using WriteComplateCallback = std::function<void(const TcpConnectionPtr&)>;
using MessageCallback = std::function<void(const TcpConnectionPtr&,
                                        Buffer*,
                                        Timestamp)>; // 已连接用户有读写事件来了，要处理的回调函数
using HighWaterMarkCallback = std::function<void(const TcpConnectionPtr&, size_t)>;