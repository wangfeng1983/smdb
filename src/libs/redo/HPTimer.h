/** 高精度计时器
 * laism@lianchuang.com, 2010-06
 */

#ifndef _LINKAGE_MDB_REDO_HPTIMER_
#define _LINKAGE_MDB_REDO_HPTIMER_

#include <sys/time.h>

class HPTimer
{
    public:
        // 返回当前时间, 返回值整数部分为秒, 小数部分是微妙/1000000
        static double now(void)
        {
            struct timeval tv;
            struct timezone tz;
            ::gettimeofday(&tv, &tz);
            return tv.tv_sec + tv.tv_usec / 1000000.0;
        }
};

#endif
