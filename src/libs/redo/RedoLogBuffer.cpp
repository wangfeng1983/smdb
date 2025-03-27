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

/** ѭ������.
 * head ����ָ�����ݵĵ�һ���ֽ�, rear ����ָ����һ������λ��
 * ��β���������ݣ���ͷ��ɾ������
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
            // ������ǰӦ�ñ�֤���㹻�ռ䣬
            // ���ﲻ���ظ�У�� m_capacity - m_size >= length
            // �������ݵ�λ��
            off_t pos = m_rear;
            if (pos + length <= m_capacity)
            {
                ::memcpy(m_addr + pos, data, length);
            }
            else // ���ݷ����β���
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

        /// ��ͷ��ɾ������
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

        /** ��ȡ�����е�����.
         * @return >0��ʾ�ɹ���ȡ; =0 ��ʾ������û������; <0 ��ʾ����
         *                           (-1 - �Ƿ�����; -2 - buffer̫С)
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
            // Ϊ�˱�֤redo��־�������ԣ����ܽض�һ����¼, �������ݱ���һ��ȡ��
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
            else // ���ݷ�����, ����Ϊ capacity - head �� rear
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
        char* m_addr;       /// ��ʼ��ַ
        off_t& m_capacity;  /// ����
        off_t& m_head;      /// ���ݿ�ʼƫ����
        off_t& m_rear;      /// ��һ������λ��
        off_t& m_size;      /// ���ݴ�С
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
    if (data_size > m_header->capacity)  // ����̫���������������Ų���
        return false;
    if (m_lock->lock())
    {
        if (m_loopArray->getFreeSize() < data_size)
        {
            m_lock->unlock();
            // ֪ͨlgwr(����־д�����)
            log_info("RedoLogBufferʣ��ռ䲻��, ��֪ͨ LogWriter");
            notifyWriter();
            // TODO: �Ľ��ȴ�����
            double now =  HPTimer::now();
            log_info("RedoLogBuffer::insert() ��ʼ�ȴ�");
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
            // ��ʱ
            log_warn("RedoLogBuffer::insert() �ȴ���ʱ");
            return false;
        }
        bool needNotify = false;
        // �����ݲ��뻺����
        bool ret = m_loopArray->insert((char*)data, data_size);
        if (ret)
        {
            m_header->insert_time = HPTimer::now();
            // ���� scn_min, scn_max
            if (m_header->scn_min > scn_min || m_header->scn_min <= 0)
                m_header->scn_min = scn_min;
            if (m_header->scn_max < scn_max)
                m_header->scn_max = scn_max;
            // ���Ӱ汾��
            if (m_header->version < redo_const::buffer::VERSION_MAX)
                m_header->version++;
            else
                m_header->version = redo_const::buffer::VERSION_MIN;
            getHeader(header);
            if (header != 0)
            {
                // ����������ʹ�ñ����ﵽ�趨ֵ
                if (header->size >= redo_const::buffer::MAX_NOTIFY_SIZE ||
                        header->size >= header->capacity * redo_const::buffer::MAX_NOTIFY_RATIO)
                {
                    log_info("�������ﵽ " << header->size << ", ��֪ͨ LogWriter д��־");
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
        //log_info("֪ͨ LogWriter д��־");
        ::kill(m_header->lgwr_pid, redo_const::signal::NOTIFY);
    }
    else
    {
        log_error("RedoLogBuffer::Header.lgwr_pid <= 0, �޷�֪ͨ LogWriter ����");
    }
}

// ������ͷ������Ϣ���Ƶ� header
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
    if (v1 > v2 || (v1 < first_quarter && v2 > last_quarter)) // ���ǵ��汾�Ź�������
        return 1;
    else if (v1 == v2)
        return 0;
    else
        return -1;
}

