#pragma once

#include<unistd.h>
#include<sys/syscall.h>

namespace CurrentThread
{
    extern __thread int t_cachedTid;

    //把tid放到缓存中
    void cachedTid();

    inline int tid()
    {
        if(__builtin_expect(t_cachedTid == 0, 0))
        {
            //说明没有缓存当前线程的tid
            cachedTid();
        }
        return t_cachedTid;
    }
}