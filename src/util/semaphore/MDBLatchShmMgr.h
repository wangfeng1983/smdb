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
        //对单、多LATCH的加锁和解锁 -1 异常  0 未获取到  1 OK
        //r_platch latchnode指针 r_srcid latch对应的ID  r_procInfo进程信息 r_proclab进程信息下标
        static int  slatch_lock(LatchNode* r_platch, const int& r_srcid, ProcessLatchInfo* r_procInfo);
        static void slatch_unlock(LatchNode* r_platch, const int& r_srcid, ProcessLatchInfo* r_procInfo);
        static int  mlatch_lock(LatchNode* r_platch, const int& r_srcid, ProcessLatchInfo* r_procInfo,
                                const size_t t_outtime = LATCH_CONST::MLATCH_OUTTIME);
        static void mlatch_unlock(LatchNode* r_platch, const int& r_srcid, ProcessLatchInfo* r_procInfo);
        //将r_shminfo对应的信息打印出来,供测试使用
        static void dumpShmInfo(const LatchShmInfo& r_shminfo);
    protected:
        //第0个空闲，不使用
        static bool setProcInfo(LatchShmInfo& r_shminfo, ProcessLatchInfo* &r_procInfo);
        static bool getProcInfo(LatchShmInfo& r_shminfo, ProcessLatchInfo* &r_procInfo);
        static bool getProcInfo(LatchShmInfo& r_shminfo, const pthread_t r_threadid, ProcessLatchInfo* &r_procInfo);
    public:
        //timeout 为循环的微妙数,lock成功返回true 否则返回false
        static bool latch_lock(volatile int& r_lvalue, const size_t timeout);
        static void latch_unlock(volatile int& r_lvalue);
    protected:
        static vector<LatchShmInfo> m_latchShmList;
        static pthread_mutex_t      m_innerMutex;
};

#endif //MDBLatchShmMgr_H_INCLUDE_20100519_
