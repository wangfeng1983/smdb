#ifndef _LINKAGE_MDB_REDO_LGWRRUNSTATUS_H_
#define _LINKAGE_MDB_REDO_LGWRRUNSTATUS_H_

#include <ctime>
#include <unistd.h>
#include <sys/types.h>

class Lock;

/// LGWR ������״̬��Ϣ�����ڹ���������
class LGWRRunStatus
{
    public:
        /// LGWR ����״̬
        typedef struct _RUNSTATUS
        {
            pid_t   lgwr_pid;
            double  write_time;         // ���һ��д����̵�ʱ��
            int     version;            // д��ʱ��RedoLogBuffer�汾
            bool    flag;               // �Ƿ�ɹ����
            int     logfile_group;      // ��־�ļ����
        } RunStatus;

    public:
        LGWRRunStatus(void* addr, Lock* lock);
        ~LGWRRunStatus(void);

        /// ���� LogWriter pid, �� LGWR ������ʱ����
        bool setLGWRPID(pid_t pid);

        /// ��ȡ����״̬��Ϣ
        bool get(RunStatus& status);

        /// ��ȡ����״̬��Ϣ���������ʧ�ܾͷ��� false
        bool tryGet(RunStatus& status);

        /// ��������״̬��Ϣ
        bool update(int version, bool flag, int group);

    private:
        RunStatus*  m_status;
        Lock*       m_lock;
};

#endif
