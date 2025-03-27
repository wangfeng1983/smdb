#include "RedoLog.h"

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "LocalRedoBuffer.h"
#include "RedoLock.h"
#include "LGWRRunStatus.h"
#include "RedoLogBuffer.h"
#include "RedoLogFile.h"

#include "debuglog.h"

RedoLog::RedoLog()
{
    m_redoLogBuffer = 0;
    m_localRedoBuffer = 0;
    m_lgwrRunStatus = 0;
    m_config = 0;
    m_scn = -1;
}

RedoLog::~RedoLog()
{
    if (m_localRedoBuffer != 0)
    {
        /* 如果有还有未提交的数据表示程序出现异常了
         * 不提交剩余日志对系统已经没什么影响

            // 本地缓冲区还有未提交的数据
            if (m_localRedoBuffer->getDataSize() > 0)
            {
                submit();
            }
        */
        delete m_localRedoBuffer;
        m_localRedoBuffer = 0;
    }
    if (m_redoLogBuffer != 0)
    {
        delete m_redoLogBuffer;
        m_redoLogBuffer = 0;
    }
    if (m_lgwrRunStatus != 0)
    {
        delete m_lgwrRunStatus;
        m_lgwrRunStatus = 0;
    }
    // 释放锁资源, 如果是在初始化的时候创建的锁
    for (std::vector<Lock*>::iterator iter = m_locksNeedRelease.begin();
            iter != m_locksNeedRelease.end();
            ++iter)
    {
        delete *iter;
    }
    m_locksNeedRelease.clear();
}

bool RedoLog::initialize(const RedoConfig* config)
{
    if (config == 0)
    {
        log_error("config == NULL");
        return false;
    }
    m_config = config;
    if (config->redoLogAddress == 0) // 只能初始化日志、缓冲区，不能提交日志
    {
        log_warn("config->redoLogAddress == NULL, 将不能提交日志");
        return true;
    }
    // 如果使用多个RedoLogBuffer，则随机挑一个
    int index = 0;
    if (config->logBufferCount > 1)
    {
        srand((unsigned int)time(NULL));
        index = rand() % (config->logBufferCount);
    }
    //log_info("use RedoLogBuffer " << index);
    Lock* lock = 0;
    // RedoLogBuffer 的锁
    try
    {
        lock = new RedoLock(redo_const::lock::BUFFER,
                            index,
                            config->dbname.c_str());
    }
    catch (std::bad_alloc& e)
    {
        lock = 0;
        log_info("new RedoLock() failed");
        return false;
    }
    m_locksNeedRelease.push_back(lock);
    if (!lock->initialize())
    {
        log_error("lock->initialize() failed");
        return false;
    }
    int record_size = sizeof(RedoLogBuffer::Header)
                      + config->logBufferSize * 1024
                      + sizeof(LGWRRunStatus::RunStatus);
    char* address = (char*)config->redoLogAddress + record_size * index;
    try
    {
        m_redoLogBuffer = new RedoLogBuffer(address, lock);
    }
    catch (std::bad_alloc& e)
    {
        m_redoLogBuffer = 0;
        log_error("new RedoLogBuffer(...) failed");
        return false;
    }
    if (!m_redoLogBuffer->initialize())
    {
        log_error("m_redoLogBuffer->initialize() failed");
        return false;
    }
    // LGWRRunStatus 的锁
    try
    {
        lock = new RedoLock(redo_const::lock::STATUS,
                            index,
                            config->dbname.c_str());
        m_locksNeedRelease.push_back(lock);
    }
    catch (std::bad_alloc& e)
    {
        lock = 0;
        log_info("new RedoLock() failed");
        return false;
    }
    if (!lock->initialize())
    {
        log_error("lock->initialize() failed");
        return false;
    }
    address += sizeof(RedoLogBuffer::Header) + config->logBufferSize * 1024;
    try
    {
        m_lgwrRunStatus = new LGWRRunStatus(address, lock);
    }
    catch (std::bad_alloc& e)
    {
        m_lgwrRunStatus = 0;
        log_error("new LGWRRunStatus(...) failed");
        return false;
    }
    // 创建 LocalRedoBuffer
    try
    {
        m_localRedoBuffer = new LocalRedoBuffer();
    }
    catch (std::bad_alloc& e)
    {
        m_localRedoBuffer = 0;
        log_error("new LocalRedoBuffer(...) failed");
        return false;
    }
    if (!m_localRedoBuffer->initialize())
    {
        log_error("m_localRedoBuffer->initialize() failed");
        return false;
    }
    return true;
}

bool RedoLog::initializeLogFiles(const std::vector<int>& channel_list)
{
    int fd;
    struct stat filestat;
    char filename[PATH_MAX];
    RedoLogFile::Header header;
    char c_zero = 0;
    off_t filesize = m_config->logFileSize * 1024 * 1024;
    ::memset(&header, 0, sizeof(RedoLogFile::Header));
    ::memcpy(header.description,
             redo_const::writer::LOGFILE_DESCRIPTION,
             redo_const::writer::LOGFILE_DESCRIPTION_LEN);
    header.mdb_version = redo_const::MDB_VERSION;
    header.valid_flag = redo_const::VALID;
    header.active_flag = redo_const::INACTIVE;
    header.scn_min = header.scn_max = header.scn_ckp = -1;
    header.write_size = 0;
    header.block_count = 0;
    const char* mdb_home = ::getenv("MDB_HOME");
    if (mdb_home == 0 || *mdb_home == '\0')
    {
        log_error("MDB_HOME not set");
        return false;
    }
    for (int group = 0; group < m_config->logGroupCount; group++)
    {
        int channel;
        for (std::vector<int>::const_iterator iter = channel_list.begin();
                iter != channel_list.end();
                ++iter)
        {
            channel = *iter;
            ::sprintf(filename, "%s/%s/redo_%s_%d_%03d.log",
                      mdb_home,
                      m_config->path.c_str(),
                      m_config->dbname.c_str(),
                      channel,
                      group);
            fd = ::open(filename, O_CREAT | O_RDWR, 0644);
            if (fd < 0)
            {
                log_error("open(" << filename << ") failed: " << ::strerror(errno));
                return false;
            }
            if (fstat(fd, &filestat) < 0)
            {
                ::close(fd);
                ::unlink(filename);
                log_error("fstat() failed: " << ::strerror(errno));
                return false;
            }
            header.group = group;
            header.subgroup = 0;
            ::write(fd, &header, sizeof(RedoLogFile::Header));
            ::write(fd, &redo_const::writer::SIG_EOF, sizeof(int32_t));
            if (filestat.st_size < filesize)
            {
                ::lseek(fd, filesize - 1, SEEK_SET);
                ::write(fd, &c_zero, sizeof(char));
            }
            else if (filestat.st_size > filesize)
            {
                ::ftruncate(fd, filesize);
            }
            ::close(fd);
            // 删除临时增加的文件
            for (int subgroup = redo_const::writer::MIN_SUB_GROUP;
                    subgroup <= redo_const::writer::MAX_SUB_GROUP;
                    subgroup++)
            {
                ::memset(filename, 0, PATH_MAX);
                ::sprintf(filename, "%s/%s/redo_%s_%d_%03d_%03d.log",
                          mdb_home,
                          m_config->path.c_str(),
                          m_config->dbname.c_str(),
                          channel,
                          group,
                          subgroup);
                if (::access(filename, R_OK) == 0)
                    ::unlink(filename);
                else
                    break;
            }
        }
    }
    return true;
}

off_t RedoLog::getRedoLogAreaSize()
{
    return (sizeof(RedoLogBuffer::Header) +
            m_config->logBufferSize * 1024 +
            sizeof(LGWRRunStatus::RunStatus)) * m_config->logBufferCount;
}

bool RedoLog::initializeRedoLogArea()
{
    int recordSize = sizeof(RedoLogBuffer::Header)
                     + m_config->logBufferSize * 1024 + sizeof(LGWRRunStatus::RunStatus);
    for (int i = 0; i < m_config->logBufferCount; i++)
    {
        RedoLogBuffer::Header* header =
            (RedoLogBuffer::Header*)((char*)m_config->redoLogAddress +
                                     recordSize * i);
        ::memset(header, 0, sizeof(RedoLogBuffer::Header));
        header->lgwr_pid = 0;
        header->scn_min = -1;
        header->scn_max = -1;
        header->version = redo_const::buffer::VERSION_MIN;
        header->capacity = m_config->logBufferSize * 1024;
        LGWRRunStatus::RunStatus* status =
            (LGWRRunStatus::RunStatus*)((char*)header +
                                        sizeof(RedoLogBuffer::Header) +
                                        m_config->logBufferSize * 1024);
        ::memset(status, 0, sizeof(LGWRRunStatus::RunStatus));
        status->version = redo_const::buffer::VERSION_MIN;
    }
    return true;
}

bool RedoLog::add(const char* log, LogType type, T_SCN scn)
{
    off_t log_size = ::strlen(log);
    if (log_size <= 0)
        return false;
    if (m_localRedoBuffer->getFreeSize() < log_size)
    {
        log_info("本地缓冲区将满，先提交到RedoLogBuffer");
        if (!submit())
            return false;
        if (m_localRedoBuffer->getFreeSize() < log_size)
        {
            log_info("本地缓冲区设置的太小，放不下一条日志, 需要扩充");
            if (!m_localRedoBuffer->extend(log_size + 1))
                return false;
        }
    }
    if (m_localRedoBuffer->add(log))
    {
        if (type == TRANS_COMMIT || type == DDL)
        {
            //log_info("TRANS_COMMIT 或 DDL 日志，需要提交");
            m_scn = scn.getOffSet();
            if (!submit())
                return false;
            m_redoLogBuffer->notifyWriter();
        }
        return true;
    }
    else
        return false;
}

bool RedoLog::hasFinished()
{
    // 本地缓冲区还有未提交的数据
    if (m_localRedoBuffer->getDataSize() > 0)
    {
        if (!submit())
            return false;
        m_redoLogBuffer->notifyWriter();
    }
    if (!m_lgwrRunStatus->get(m_lgwrStatus))
        return false;
    if (m_lgwrStatus.flag && RedoLogBuffer::compareVersion(m_lgwrStatus.version, m_bufferHeader.version) >= 0)
        return true;
    else
        return false;
}

bool RedoLog::submit()
{
    if (m_localRedoBuffer->getDataSize() > 0)
    {
        if (m_redoLogBuffer->insert(m_localRedoBuffer->getData(),
                                    m_localRedoBuffer->getDataSize(),
                                    m_scn,
                                    m_scn,
                                    &m_bufferHeader))
        {
            m_localRedoBuffer->clear();
            m_scn = -1;
            return true;
        }
        else
            return false;
    }
    else // 没有数据可提交
        return true;
}
