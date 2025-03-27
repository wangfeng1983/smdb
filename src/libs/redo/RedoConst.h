// REDO ��صĳ�������
#ifndef _LINKAGE_MDB_REDO_REDOCONST_H_
#define _LINKAGE_MDB_REDO_REDOCONST_H_

#include <signal.h>
#include <inttypes.h>
#include <limits.h>
#include <sys/types.h>

namespace redo_const
{
    const int MDB_VERSION = 2;                      // MDB �汾
    const int MIN_LOGGROUP_COUNT = 2;               // redo ��־�ļ����ٸ���
    const int MIN_LOGFILE_SIZE = 1024 * 1024;       // redo ��־�ļ���С����
    const int MIN_LOGBUFFER_COUNT = 1;              // redo log �������ٸ���
    const int MIN_LOGBUFFER_SIZE = 1024 * 64;       // redo log ������С����
    const int DEFAULT_LOCALBUFFER_SIZE = 1024 * 32; // Ĭ�ϵı��� redolog ��������С

    // ������������
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

    // �ź�
    namespace signal
    {
        const int NOTIFY = SIGUSR1; // ֪ͨ logwriter д��־���ź�
        const int ALARM  = SIGALRM; // ��ʱ�����ź�
    }

    namespace lock
    {
        const int LOCK_TRYS   = 5;  // ���Լ�����������
        const int UNLOCK_TRYS = 10; // ���Խ�����������

        // Ҫ������Դ����
        typedef enum _LOCK_TYPE
        {
            BUFFER,
            STATUS
        } LockType;
    }

    namespace buffer
    {
        const int MAX_NOTIFY_SIZE     = 1024 * 1024;    // RedoLogBuffer ��Ҫ֪ͨд��ʱ��������
        const double MAX_NOTIFY_RATIO = 0.5;            // RedoLogBuffer ��Ҫ֪ͨд����ʹ�ñ���
        const double MAX_INSERT_TIME  = 30.0;   // ����־���뻺����ʱ���ȴ�ʱ�䣬��λΪ�룬������С������
        const int INSERT_INTERVAL     = 100;    // ÿ�γ��Խ���־���뻺������ʱ��������λΪ΢��

        const int VERSION_MAX = INT_MAX; // ���汾��
        const int VERSION_MIN = 0;       // ��С�汾��
    }

    namespace writer
    {
        const int WRITE_INTERVAL     = 3;       // logwriter д��־��ʱ��������λΪ��
        const int USLEEPTIME_ON_IDLE = 50000;   // logwriter ����ʱ��˯��ʱ�䣬��λΪ΢��

        // redo ��־�ļ�ͷ�ı��
        static const char* LOGFILE_DESCRIPTION = "Linakge MDB REDO LOG";
        const int LOGFILE_DESCRIPTION_LEN = 21;

        // ���ݿ���
        const int32_t SIG_BLOCK_START = 0x8FDF780F; // ��ͷ���
        const int32_t SIG_BLOCK_END   = 0x8F866F7B; // ��β���
        const int32_t SIG_EOF         = 0x8F3B190B; // �ļ��������

        // ÿ�ζ���д����ֽ���
        const int EXTRA_SIZE_WRITE = sizeof(int32_t) * 3 + sizeof(off_t) * 2; // ��ͷ|��С|...|��С|��β|SIG_EOF
        const int EXTRA_SIZE_COUNT = sizeof(int32_t) * 2 + sizeof(off_t) * 2; // ����SIG_EOF

        // ��ʱ���ӵ��ļ���������С���
        const int MIN_SUB_GROUP = 1;
        const int MAX_SUB_GROUP = INT_MAX;

        // д����־�ķ�ʽ
        typedef enum _REDOLOG_WRITE_MODE
        {
            BUFFERED = 0,       // ϵͳ�����I/O
            DIO,                // ֱ��I/O
            MMAP,               // mmap
            INVALID_WRITE_MODE  // ��Ч��д��ģʽ�������б����
        } WriteMode;
    }

    // redo ��־�ļ��е� valid_flag
    typedef enum _VALID_FLAG
    {
        INVALID = 0,
        VALID = 1
    } ValidFlag;

    // redo ��־�ļ��е� active_flag
    typedef enum _ACTIVE_STATE
    {
        INACTIVE = 0,
        ACTIVE = 1
    } ActiveFlag;
}

#endif
