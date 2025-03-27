#include "RedoLogBuffer.h"

#include <cstring>
#include <cstdlib>
#include <math.h>
#include <new>
#include <sys/types.h>
#include <signal.h>

#include "RedoConst.h"
#include "Lock.h"
#include "HPTimer.h"

#include "debuglog.h"

/** 循环队列.
 * head 总是指向数据的第一个字节, rear 总是指向下一个插入位置
 * 从尾部增加数据，从头部删除数据
 *
 * <pre>
 * +-------------------+         +-------------------+
 * |_ _ _ X X X X _ _ _|         |X X X _ _ _ _ X X X|
 * +------|-------|----+         +------|-------|----+
 *       head    rear                  rear    head
 * </pre>
 */
class LoopArray
{
    public:
        LoopArray(void* addr,
                  off_t& capacity,
                  off_t& head,
                  off_t& rear,
                  off_t& size)
            : m_addr((char*)addr), m_capacity(capacity),
              m_head(head), m_rear(rear), m_size(size)
        {
        }

        ~LoopArray()
        {
        }

        bool insert(const char* data, off_t length)
        {
            // 被调用前应该保证有足够空间，
            // 这里不再重复校验 m_capacity - m_size >= length
            // 插入数据的位置
            off_t pos = m_rear;
            if (pos + length <= m_capacity)
            {
                ::memcpy(m_addr + pos, data, length);
            }
            else // 数据分两段插入
            {
                off_t size = m_capacity - pos;
                ::memcpy(m_addr + pos, data, size);
                ::memcpy(m_addr, data + size, length - size);
            }
            m_rear += length;
            if (m_rear >= m_capacity)
                m_rear -= m_capacity;
            m_size += length;
            return true;
        }

        /// 从头部删除数据
        bool erase(off_t size)
        {
            if (size > m_size)
            {
                log_error("LoopArray::erase(), size>m_size, size=" << size
                          << ", m_size=" << m_size);
                return false;
            }
            m_head += size;
            if (m_head >= m_capacity)
                m_head -= m_capacity;
            m_size -= size;
            return true;
        }

        /** 获取队列中的数据.
         * @return >0表示成功获取; =0 表示队列中没有数据; <0 表示出错
         *                           (-1 - 非法参数; -2 - buffer太小)
         */
        int64_t get(char* buffer, off_t buffer_size)
        {
            if (buffer == 0 || buffer_size <= 0)
            {
                log_error("LoopArray::get(), buffer=" << (void*)buffer
                          << ", buffer_size=" << buffer_size);
                return -1;
            }
            if (m_size <= 0)
            {
                return 0;
            }
            // 为了保证redo日志的完整性，不能截断一条记录, 队列内容必须一次取完
            if (m_size > buffer_size)
            {
                log_error("LoopArray::get() error, m_size("
                          << m_size << ") > buffer_size(" << buffer_size << ")");
                return -2;
            }
            if (m_rear > m_head)
            {
                ::memcpy(buffer, m_addr + m_head, m_size);
            }
            else // 数据分两段, 长度为 capacity - head 和 rear
            {
                ::memcpy(buffer, m_addr + m_head, m_capacity - m_head);
                ::memcpy(buffer + (m_capacity - m_head), m_addr, m_rear);
            }
            return m_size;
        }

        off_t getFreeSize() const
        {
            return m_capacity - m_size;
        }

    private:
        char* m_addr;       /// 开始地址
        off_t& m_capacity;  /// 容量
        off_t& m_head;      /// 数据开始偏移量
        off_t& m_rear;      /// 下一个插入位置
        off_t& m_size;      /// 数据大小
};

RedoLogBuffer::RedoLogBuffer(void* addr, Lock* lock)
    : m_addr(addr), m_lock(lock)
{
    m_header_size = sizeof(Header);
    m_header = (Header*)addr;
    m_loopArray = 0;
}

RedoLogBuffer::~RedoLogBuffer(void)
{
    if (m_loopArray != 0)
    {
        delete m_loopArray;
        m_loopArray = 0;
    }
}

bool RedoLogBuffer::initialize()
{
    if (m_loopArray == 0)
    {
        try
        {
            m_loopArray = new LoopArray((char*)m_addr + sizeof(Header),
                                        m_header->capacity,
                                        m_header->head,
                                        m_header->rear,
                                        m_header->size);
        }
        catch (std::bad_alloc& e)
        {
            m_loopArray = 0;
            return false;
        }
    }
    return true;
}

bool RedoLogBuffer::setLGWRPID(pid_t pid)
{
    if (m_lock->lock())
    {
        m_header->lgwr_pid = pid;
        m_lock->unlock();
        return true;
    }
    else
    {
        log_error("RedoLogBuffer::setLGWRPID(), m_lock->lock() failed.");
        return false;
    }
}

bool RedoLogBuffer::insert(const void* data,
                           off_t data_size,
                           int64_t scn_min,
                           int64_t scn_max,
                           Header* header)
{
    if (0 == data || data_size <= 0)
        return false;
    if (data_size > m_header->capacity)  // 数据太大，整个缓冲区都放不下
        return false;
    if (m_lock->lock())
    {
        if (m_loopArray->getFreeSize() < data_size)
        {
            m_lock->unlock();
            // 通知lgwr(将日志写入磁盘)
            log_info("RedoLogBuffer剩余空间不足, 需通知 LogWriter");
            notifyWriter();
            // TODO: 改进等待机制
            double now =  HPTimer::now();
            log_info("RedoLogBuffer::insert() 开始等待");
            do
            {
                ::usleep(redo_const::buffer::INSERT_INTERVAL);
                if (m_lock->lock())
                {
                    off_t free_size = m_loopArray->getFreeSize();
                    m_lock->unlock();
                    if (free_size >= data_size)
                    {
                        return insert(data,
                                      data_size,
                                      scn_min,
                                      scn_max,
                                      header);
                    }
                }
            }
            while (HPTimer::now() - now <= redo_const::buffer::MAX_INSERT_TIME);
            // 超时
            log_warn("RedoLogBuffer::insert() 等待超时");
            return false;
        }
        bool needNotify = false;
        // 将数据插入缓冲区
        bool ret = m_loopArray->insert((char*)data, data_size);
        if (ret)
        {
            m_header->insert_time = HPTimer::now();
            // 更新 scn_min, scn_max
            if (m_header->scn_min > scn_min || m_header->scn_min <= 0)
                m_header->scn_min = scn_min;
            if (m_header->scn_max < scn_max)
                m_header->scn_max = scn_max;
            // 增加版本号
            if (m_header->version < redo_const::buffer::VERSION_MAX)
                m_header->version++;
            else
                m_header->version = redo_const::buffer::VERSION_MIN;
            getHeader(header);
            if (header != 0)
            {
                // 数据量或者使用比例达到设定值
                if (header->size >= redo_const::buffer::MAX_NOTIFY_SIZE ||
                        header->size >= header->capacity * redo_const::buffer::MAX_NOTIFY_RATIO)
                {
                    log_info("数据量达到 " << header->size << ", 需通知 LogWriter 写日志");
                    needNotify = true;
                }
            }
        }
        m_lock->unlock();
        if (needNotify)
            notifyWriter();
        return ret;
    }
    return false;
}

int64_t RedoLogBuffer::get(void* buffer, off_t buffer_size, Header* header)
{
    if (m_lock->lock())
    {
        int64_t data_size = m_loopArray->get((char*)buffer, buffer_size);
        if (data_size > 0)
        {
            getHeader(header);
            m_header->scn_min = m_header->scn_max = -1;
        }
        else if (data_size < 0)
        {
            log_error("RedoLogBuffer::get(), data_size = " << data_size);
        }
        m_lock->unlock();
        return data_size;
    }
    else
    {
        log_error("RedoLogBuffer::get(), m_lock->lock() failed");
        return -1;
    }
}

bool RedoLogBuffer::erase(off_t length, Header* header)
{
    if (m_lock->lock())
    {
        bool ret = m_loopArray->erase(length);
        if (ret)
        {
            m_header->write_time = HPTimer::now();
            getHeader(header);
        }
        m_lock->unlock();
        return ret;
    }
    else
    {
        log_debug("RedoLogBuffer::erase(), m_lock->lock() failed");
        return false;
    }
}

void RedoLogBuffer::notifyWriter()
{
    if (m_header->lgwr_pid > 0)
    {
        //log_info("通知 LogWriter 写日志");
        ::kill(m_header->lgwr_pid, redo_const::signal::NOTIFY);
    }
    else
    {
        log_error("RedoLogBuffer::Header.lgwr_pid <= 0, 无法通知 LogWriter 进程");
    }
}

// 将缓存头部的信息复制到 header
void RedoLogBuffer::getHeader(Header* header)
{
    if (header != 0)
    {
        ::memcpy(header, m_addr, m_header_size);
    }
}

int RedoLogBuffer::compareVersion(int v1, int v2)
{
    static int first_quarter = redo_const::buffer::VERSION_MAX / 4.0;
    static int last_quarter = first_quarter * 3.0;
    if (v1 > v2 || (v1 < first_quarter && v2 > last_quarter)) // 考虑到版本号归零的情况
        return 1;
    else if (v1 == v2)
        return 0;
    else
        return -1;
}

