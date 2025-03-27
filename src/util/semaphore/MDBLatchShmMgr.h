#ifndef MDBLatchShmMgr_H_INCLUDE_20100519_
#define MDBLatchShmMgr_H_INCLUDE_20100519_

#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include "MdbConstDef.h"
#include "MDBLatchInfo.h"

class MDBLatchShmMgr
{
    public:
        MDBLatchShmMgr();
        virtual ~MDBLatchShmMgr();
    public:
        static bool getLatchShmAddr(LatchShmInfo& r_shminfo, ProcessLatchInfo* &r_procInfo);
        static bool createLatchShm(LatchShmInfo& r_shminfo);
        static bool deleteLatchShm(LatchShmInfo& r_shminfo);
        static bool delProcInfo(const char* r_dbname, const pthread_t r_threadid);
        //�Ե�����LATCH�ļ����ͽ��� -1 �쳣  0 δ��ȡ��  1 OK
        //r_platch latchnodeָ�� r_srcid latch��Ӧ��ID  r_procInfo������Ϣ r_proclab������Ϣ�±�
        static int  slatch_lock(LatchNode* r_platch, const int& r_srcid, ProcessLatchInfo* r_procInfo);
        static void slatch_unlock(LatchNode* r_platch, const int& r_srcid, ProcessLatchInfo* r_procInfo);
        static int  mlatch_lock(LatchNode* r_platch, const int& r_srcid, ProcessLatchInfo* r_procInfo,
                                const size_t t_outtime = LATCH_CONST::MLATCH_OUTTIME);
        static void mlatch_unlock(LatchNode* r_platch, const int& r_srcid, ProcessLatchInfo* r_procInfo);
        //��r_shminfo��Ӧ����Ϣ��ӡ����,������ʹ��
        static void dumpShmInfo(const LatchShmInfo& r_shminfo);
    protected:
        //��0�����У���ʹ��
        static bool setProcInfo(LatchShmInfo& r_shminfo, ProcessLatchInfo* &r_procInfo);
        static bool getProcInfo(LatchShmInfo& r_shminfo, ProcessLatchInfo* &r_procInfo);
        static bool getProcInfo(LatchShmInfo& r_shminfo, const pthread_t r_threadid, ProcessLatchInfo* &r_procInfo);
    public:
        //timeout Ϊѭ����΢����,lock�ɹ�����true ���򷵻�false
        static bool latch_lock(volatile int& r_lvalue, const size_t timeout);
        static void latch_unlock(volatile int& r_lvalue);
    protected:
        static vector<LatchShmInfo> m_latchShmList;
        static pthread_mutex_t      m_innerMutex;
};

#endif //MDBLatchShmMgr_H_INCLUDE_20100519_
