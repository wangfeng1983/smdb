#ifndef _LINKAGE_MDB_REDO_LOCALBUFFER_H_
#define _LINKAGE_MDB_REDO_LOCALBUFFER_H_

#include <sys/types.h>

#include "RedoConst.h"

/// �ύ�� RedoLogBuffer ֮ǰʹ�õı�����־����
class LocalRedoBuffer
{
    public:
        LocalRedoBuffer(off_t capacity = redo_const::DEFAULT_LOCALBUFFER_SIZE);
        ~LocalRedoBuffer(void);

        bool initialize();

    public:
        /// ������־
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

        /// ���������
        void clear()
        {
            m_data_size = 0;
        }

        /// ���仺����
        bool extend(off_t new_size);

    private:
        char* m_data;       // ���ݿ��ַ
        off_t m_capacity;   // ����
        off_t m_data_size;  // ���ݴ�С
};

#endif

