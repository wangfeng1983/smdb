#ifndef _LINKAGE_MDB_REDO_LGWRRUNSTATUS_H_
#define _LINKAGE_MDB_REDO_LGWRRUNSTATUS_H_

#include <ctime>
#include <unistd.h>
#include <sys/types.h>

class Lock;

/// LGWR 的运行状态信息，放在共享内容中
class LGWRRunStatus
{
    public:
        /// LGWR 运行状态
        typedef struct _RUNSTATUS
        {
            pid_t   lgwr_pid;
            double  write_time;         // 最后一次写入磁盘的时间
            int     version;            // 写出时的RedoLogBuffer版本
            bool    flag;               // 是否成功标记
            int     logfile_group;      // 日志文件序号
        } RunStatus;

    public:
        LGWRRunStatus(void* addr, Lock* lock);
        ~LGWRRunStatus(void);

        /// 设置 LogWriter pid, 由 LGWR 在启动时设置
        bool setLGWRPID(pid_t pid);

        /// 获取运行状态信息
        bool get(RunStatus& status);

        /// 获取运行状态信息，如果加锁失败就返回 false
        bool tryGet(RunStatus& status);

        /// 更新运行状态信息
        bool update(int version, bool flag, int group);

    private:
        RunStatus*  m_status;
        Lock*       m_lock;
};

#endif
