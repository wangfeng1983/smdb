// REDO 相关的常量定义
#ifndef _LINKAGE_MDB_REDO_REDOCONST_H_
#define _LINKAGE_MDB_REDO_REDOCONST_H_

#include <signal.h>
#include <inttypes.h>
#include <limits.h>
#include <sys/types.h>

namespace redo_const
{
    const int MDB_VERSION = 2;                      // MDB 版本
    const int MIN_LOGGROUP_COUNT = 2;               // redo 日志文件最少个数
    const int MIN_LOGFILE_SIZE = 1024 * 1024;       // redo 日志文件最小容量
    const int MIN_LOGBUFFER_COUNT = 1;              // redo log 缓存最少个数
    const int MIN_LOGBUFFER_SIZE = 1024 * 64;       // redo log 缓存最小容量
    const int DEFAULT_LOCALBUFFER_SIZE = 1024 * 32; // 默认的本地 redolog 缓冲区大小

    // 控制区参数名
    namespace param
    {
        static const char* PATH         = "REDOLOGFILE_PATH";
        static const char* LOGFILE_SIZE = "REDOLOGFILE_SIZE";
        static const char* GROUP_COUNT  = "REDOLOGFILE_GROUP_COUNT";
        static const char* WRITE_MODE   = "REDOLOGFILE_WRITE_MODE";
        static const char* BUFFER_COUNT = "REDOLOGBUFFER_COUNT";
        static const char* BUFFER_SIZE  = "REDOLOGBUFFER_SIZE";
        static const char* SYNC_MODE    = "REDOLOG_SYNC_MODE";
    }

    // 信号
    namespace signal
    {
        const int NOTIFY = SIGUSR1; // 通知 logwriter 写日志的信号
        const int ALARM  = SIGALRM; // 定时器的信号
    }

    namespace lock
    {
        const int LOCK_TRYS   = 5;  // 尝试加锁的最大次数
        const int UNLOCK_TRYS = 10; // 尝试解锁的最大次数

        // 要锁的资源类型
        typedef enum _LOCK_TYPE
        {
            BUFFER,
            STATUS
        } LockType;
    }

    namespace buffer
    {
        const int MAX_NOTIFY_SIZE     = 1024 * 1024;    // RedoLogBuffer 需要通知写出时的数据量
        const double MAX_NOTIFY_RATIO = 0.5;            // RedoLogBuffer 需要通知写出的使用比率
        const double MAX_INSERT_TIME  = 30.0;   // 将日志插入缓冲区时最大等待时间，单位为秒，可以用小数部分
        const int INSERT_INTERVAL     = 100;    // 每次尝试将日志插入缓冲区的时间间隔，单位为微秒

        const int VERSION_MAX = INT_MAX; // 最大版本号
        const int VERSION_MIN = 0;       // 最小版本号
    }

    namespace writer
    {
        const int WRITE_INTERVAL     = 3;       // logwriter 写日志的时间间隔，单位为秒
        const int USLEEPTIME_ON_IDLE = 50000;   // logwriter 空闲时的睡眠时间，单位为微妙

        // redo 日志文件头的标记
        static const char* LOGFILE_DESCRIPTION = "Linakge MDB REDO LOG";
        const int LOGFILE_DESCRIPTION_LEN = 21;

        // 数据块标记
        const int32_t SIG_BLOCK_START = 0x8FDF780F; // 块头标记
        const int32_t SIG_BLOCK_END   = 0x8F866F7B; // 块尾标记
        const int32_t SIG_EOF         = 0x8F3B190B; // 文件结束标记

        // 每次额外写入的字节数
        const int EXTRA_SIZE_WRITE = sizeof(int32_t) * 3 + sizeof(off_t) * 2; // 块头|大小|...|大小|块尾|SIG_EOF
        const int EXTRA_SIZE_COUNT = sizeof(int32_t) * 2 + sizeof(off_t) * 2; // 不计SIG_EOF

        // 临时增加的文件的最大和最小编号
        const int MIN_SUB_GROUP = 1;
        const int MAX_SUB_GROUP = INT_MAX;

        // 写入日志的方式
        typedef enum _REDOLOG_WRITE_MODE
        {
            BUFFERED = 0,       // 系统缓冲的I/O
            DIO,                // 直接I/O
            MMAP,               // mmap
            INVALID_WRITE_MODE  // 无效的写入模式，放在列表最后
        } WriteMode;
    }

    // redo 日志文件中的 valid_flag
    typedef enum _VALID_FLAG
    {
        INVALID = 0,
        VALID = 1
    } ValidFlag;

    // redo 日志文件中的 active_flag
    typedef enum _ACTIVE_STATE
    {
        INACTIVE = 0,
        ACTIVE = 1
    } ActiveFlag;
}

#endif
