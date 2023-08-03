#include "CurrentThread.h"

namespace CurrentThread
{
    __thread int t_cachedTid = 0;
    void cachedTid()
    {
        if(t_cachedTid == 0)
        {
            //通过系统调用获取当前线程的值
            t_cachedTid = static_cast<int>(syscall(SYS_gettid));
        }
    }
}
