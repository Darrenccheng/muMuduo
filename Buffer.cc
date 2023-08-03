#include "Buffer.h"

#include <sys/uio.h>
#include <unistd.h>

/**
 * 从fd上读取数据 Poller工作在LT模式->缓冲区北邮读完，就会一直上报
 * Buffer缓冲区有大小的， 但是从fd上读取数据的时候，却不知道tcp最终的大小
*/
ssize_t Buffer::readFd(int fd, int* saveErrno)
{
    char extrabuf[65536] = {0}; // 栈的的内存空间  64k

    struct iovec vec[2]; // 不连续的内存对应的结构体

    // 从fd读数据，就是把数据写到buffer中
    const size_t writable = writerBytes(); // 这个是buffer底层缓冲区剩余可以写的空间的大小
    vec[0].iov_base = begin() + writerIndex_; // fd的内容可以写入的第一块缓冲区
    vec[0].iov_len = writable;

    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof extrabuf;

    const int iovcnt = (writable < sizeof extrabuf) ? 2 : 1; // 指定需要的缓冲区的个数（至少要写满extrabuf）
    const ssize_t n = ::readv(fd, vec, iovcnt);
    if(n < 0)
    {
        *saveErrno = errno;
    }
    else if(n <= writable) // 读的总字节数小于buffer底层的缓冲区大小-> buffer的可写缓冲区够存储读出来的数据了
    {
        writerIndex_ += n;
    }
    else // ectrabuf里面也写入数据了
    {
        writerIndex_ = buffer_.size();
        append(extrabuf, n - writable);
    }

    return n;
}

ssize_t Buffer::writeFd(int fd, int* saveErrno)
{
    ssize_t n = ::write(fd, peek(),readableBytes());
    if(n < 0)
    {
        *saveErrno = errno;
    }
    return n;
}