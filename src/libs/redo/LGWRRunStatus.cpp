#include "LGWRRunStatus.h"

#include <cstring>
#include <memory.h>
#include "Lock.h"
#include "RedoLogBuffer.h"
#include "HPTimer.h"

LGWRRunStatus::LGWRRunStatus(void* addr, Lock* lock)
    : m_lock(lock)
{
    m_status = (RunStatus*)addr;
}


LGWRRunStatus::~LGWRRunStatus(void)
{
}

bool LGWRRunStatus::setLGWRPID(pid_t pid)
{
    if (m_lock->lock())
    {
        m_status->lgwr_pid = pid;
        m_lock->unlock();
        return true;
    }
    else
    {
        return false;
    }
}

// 获取运行状态信息
bool LGWRRunStatus::get(RunStatus& status)
{
    if (m_lock->lock())
    {
        ::memcpy(&status, m_status, sizeof(RunStatus));
        m_lock->unlock();
        return true;
    }
    else
    {
        return false;
    }
}

// 获取运行状态信息，如果加锁失败就返回 false
bool LGWRRunStatus::tryGet(RunStatus& status)
{
    if (m_lock->trylock())
    {
        ::memcpy(&status, m_status, sizeof(RunStatus));
        m_lock->unlock();
        return true;
    }
    else
    {
        return false;
    }
}

// 更新运行状态信息
bool LGWRRunStatus::update(int version, bool flag, int group)
{
    if (m_lock->lock())
    {
        if (RedoLogBuffer::compareVersion(version, m_status->version) > 0)
            m_status->version = version;
        m_status->flag = flag;
        m_status->logfile_group = group;
        m_status->write_time = HPTimer::now();
        m_lock->unlock();
        return true;
    }
    else
    {
        return false;
    }
}
