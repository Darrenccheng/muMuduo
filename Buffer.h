#pragma once

#include <cstddef>
#include <vector>
#include <string>
#include <algorithm>

// 网络库底层的缓冲器类型定义
class Buffer
{
public:
    static const size_t kCheapPrepend = 8; // 最前面预留空闲区的大小
    static const size_t kInitialSize = 1024; // 缓冲区的大小

    explicit Buffer(size_t initialSize = kInitialSize)
        : buffer_(kCheapPrepend + initialSize) // 可以在初始化列表里调用构造函数
        , readerIndex_(kCheapPrepend)
        , writerIndex_(kCheapPrepend)
    { }
    
    size_t readableBytes() const
    {
        return writerIndex_ - readerIndex_;
    }

    size_t writerBytes() const
    {
        return buffer_.size() - writerIndex_;
    }

    size_t prependableBytes() const
    {
        return readerIndex_;
    }

    // 返回缓冲区中可读数据的起始地址
    const char* peek() const
    {
        return begin() + readerIndex_;
    }

    // onMessage 把buffer中的char序列化为string
    void retrieve(size_t len)
    {
        if(len < readableBytes())
        {
            readerIndex_ += len;
        }
        else
        {
            retrieveAll();
        }
    }

    void retrieveAll()
    {
        readerIndex_ = writerIndex_ = kCheapPrepend;
    }
    
    std::string retrieveAllAsString()
    {
        return retriveAsString(readableBytes());
    }

    std::string retriveAsString(size_t len)
    {
        std::string result(peek(), len);
        retrieve(len);
        return result;
    }

    void ensureWriteableBytes(size_t len)
    {
        if(writerBytes() < len)
        {
            makeSpace(len); // 容器扩容
        }
    }

    // 把[data, data+len]内存上数据，添加到writerable缓冲区中
    void append(const char* data, size_t len)
    {
        ensureWriteableBytes(len);
        std::copy(data, data + len, beginWrite());
        writerIndex_ += len;
    }

    char* beginWrite()
    {
        return begin() + writerIndex_;
    }

    // 从fd上读取数据
    ssize_t readFd(int fd, int* saveErrno);
    // 写入到fd当中
    ssize_t writeFd(int fd, int* saveErrno);

private:
    char* begin()
    {
        return &*buffer_.begin();
    }

    const char* begin() const
    {
        return &*buffer_.begin();
    }

    void makeSpace(size_t len)
    {
        if(writerBytes() + prependableBytes() < len)
        {
            buffer_.resize(writerIndex_ + len);
        }
        else
        {
            size_t readable = readableBytes();
            std::copy(begin() + readerIndex_,
                    begin() + writerIndex_,
                    begin() + kCheapPrepend);
            readerIndex_ = kCheapPrepend;
            writerIndex_ = readerIndex_ + readable;
        }
    }

    std::vector<char> buffer_;  
    size_t readerIndex_; // 还未读取的数据的最开始
    size_t writerIndex_; // 可写区域的开始
};