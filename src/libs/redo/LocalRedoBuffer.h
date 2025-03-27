#ifndef _LINKAGE_MDB_REDO_LOCALBUFFER_H_
#define _LINKAGE_MDB_REDO_LOCALBUFFER_H_

#include <sys/types.h>

#include "RedoConst.h"

/// 提交到 RedoLogBuffer 之前使用的本地日志缓存
class LocalRedoBuffer
{
    public:
        LocalRedoBuffer(off_t capacity = redo_const::DEFAULT_LOCALBUFFER_SIZE);
        ~LocalRedoBuffer(void);

        bool initialize();

    public:
        /// 增加日志
        bool add(const char* log);

        const void* getData() const
        {
            return m_data;
        }

        off_t getCapacity() const
        {
            return m_capacity;
        }

        off_t getDataSize() const
        {
            return m_data_size;
        }

        off_t getFreeSize() const
        {
            off_t free_size = m_capacity - m_data_size - 1;
            return free_size > 0 ? free_size : 0;
        }

        /// 清除缓冲区
        void clear()
        {
            m_data_size = 0;
        }

        /// 扩充缓冲区
        bool extend(off_t new_size);

    private:
        char* m_data;       // 数据块地址
        off_t m_capacity;   // 容量
        off_t m_data_size;  // 数据大小
};

#endif

