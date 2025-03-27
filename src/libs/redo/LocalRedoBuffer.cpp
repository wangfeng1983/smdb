#include "LocalRedoBuffer.h"

#include <cstring>
#include <new>

#include "debuglog.h"

LocalRedoBuffer::LocalRedoBuffer(off_t capacity)
    : m_capacity(capacity)
{
    m_data = 0;
    m_data_size = 0;
}

LocalRedoBuffer::~LocalRedoBuffer(void)
{
    if (m_data != 0)
    {
        delete []m_data;
        m_data = 0;
    }
    m_data_size = 0;
}

bool LocalRedoBuffer::initialize()
{
    if (m_capacity <= 0)
    {
        log_error("invalid m_capacity value : " << m_capacity);
        return false;
    }
    if (0 == m_data)
    {
        try
        {
            m_data = new char[m_capacity];
        }
        catch (std::bad_alloc)
        {
            m_data = 0;
            log_error("new char[" << m_capacity << "] failed");
            return false;
        }
        ::memset(m_data, 0, m_capacity);
        m_data_size = 0;
        return true;
    }
    else
    {
        log_warn("class initialied");
        return true;
    }
}

bool LocalRedoBuffer::add(const char* log)
{
    if (0 == log)
        return false;
    off_t log_size = ::strlen(log);
    if (log_size <= 0)
        return true;
    if (log_size > getFreeSize())
    {
        log_error("本地缓冲区放不下日志内容, log_size[" << log_size
                  << "] > getFreeSize()[" << getFreeSize() << "]");
        return false;
    }
    // 附加日志内容
    ::memcpy(m_data + m_data_size, log, log_size);
    m_data_size += log_size;
    m_data[m_data_size] = '\n';
    m_data_size++;
    return true;
}

bool LocalRedoBuffer::extend(off_t new_size)
{
    if (new_size < 0)
    {
        log_error("invalid parameter new_size " << new_size);
        return false;
    }
    else if (new_size < m_capacity)
    {
        log_error("new_size[" << new_size <<
                  "] < m_capacity[" << m_capacity << "]");
        return false;
    }
    else if (new_size == m_capacity)
    {
        return true;
    }
    char* new_data;
    try
    {
        new_data = new char[new_size];
    }
    catch (std::bad_alloc& e)
    {
        log_error("new char[" << new_size << "] failed");
        return false;
    }
    if (m_data != 0)
    {
        if (m_data_size > 0)
            ::memcpy(new_data, m_data, m_data_size);
        delete []m_data;
    }
    m_data = new_data;
    m_capacity = new_size;
    return true;
}
