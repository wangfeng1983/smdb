#ifndef MDBLatchInfo_H_INCLUDE_20100519_
#define MDBLatchInfo_H_INCLUDE_20100519_
#include <stdio.h>
#include <stdlib.h>
#include "MdbConstDef.h"
#include <pthread.h>


class LATCH_CONST
{
    public:
        enum
        {
            LATCHSH_FTOK_ID     = 0x7E,    //用来获取FTOKID
            MAX_LATCH_NUM       = 50,      //一个线程可同时锁mlatch个数
            MAX_LATCHPROC_NUM   = 10000,   //最大的线程数
            LATCHSHM_STATE_SIZE = 8,       //LATCH状态位大小
            MAX_MLATCH_NUM      = 10000000,//支持MLATCH锁最大个数
            MAX_SLATCH_NUM      = 10000000,//支持SLATCH锁最大个数
            MLATCH_OUTTIME      = 10000000,//MLATCH时限罚值10s
            SLATCH_OUTTIME      = 100000   //SLATCH时限罚值100ms
        };
};

class LatchNode
{
    public:
        volatile int m_latchlock;
        volatile int m_proclab;  //进程信息下标，便于管理：解锁
};
class ProcessLatchInfo
{
    public:
        volatile int  m_state;      //0 未占用，1 已经占用
        int         m_pos;        //该PROC信息的lab
        pid_t       m_pid;        //进程ID
        pthread_t   m_thid;       //线程ID
        time_t      m_time;       //线程心跳
        volatile  int m_slatchinfo; //单个latch信息：0 未占用,否则为占用(sid)
        int         m_mlatchnum;  //多个latch个数
        int         m_mlatchinfo[LATCH_CONST::MAX_LATCH_NUM]; //记录多个LATCH信息
    public:
        ProcessLatchInfo() {}
        ProcessLatchInfo(const ProcessLatchInfo& right)
        {
            memcpy(this, &right, sizeof(ProcessLatchInfo));
        }
        friend int operator==(const ProcessLatchInfo& left, const ProcessLatchInfo& right)
        {
            return (left.m_pid == right.m_pid &&
                    left.m_thid == right.m_thid);
        }
        void init()
        {
            m_state = 1;
            m_pid   = getpid();
            m_thid  = pthread_self();
            m_mlatchnum = 0;
            m_slatchinfo = 0;
            time(&(m_time));
        }
};

class LatchShmInfo
{
    public:
        T_NAMEDEF         m_dbName;    //数据库名称
        int               m_statesize; //状态位字节数
        int               m_mLatchNum; //可多个占用的LATCH个数
        int               m_sLatchNum; //不可多个占用的LATCH个数
        int               m_procNum;   //允许的最大线程数
        size_t            m_shmsize;   //共享内存大小
        char*             m_shmaddr;   //共享内存起始地址
        LatchNode*        m_mlatchaddr;//可多用LATCH起始地址
        //第0个为LATCHSHM内部LATCH,
        LatchNode*        m_slatchaddr;//不可多用LATCH起始地址
        ProcessLatchInfo* m_procAddr;  //线程信息起始地址
    public:
        LatchShmInfo()
        {
            m_statesize = LATCH_CONST::LATCHSHM_STATE_SIZE;
            m_procNum   = LATCH_CONST::MAX_LATCHPROC_NUM;
            m_mLatchNum = LATCH_CONST::MAX_MLATCH_NUM;
            m_sLatchNum = LATCH_CONST::MAX_SLATCH_NUM;
        }
        friend int operator<(const LatchShmInfo& left, const LatchShmInfo& right)
        {
            return (strcasecmp(left.m_dbName, right.m_dbName) < 0);
        }
        friend int operator==(const LatchShmInfo& left, const LatchShmInfo& right)
        {
            return (strcasecmp(left.m_dbName, right.m_dbName) == 0);
        }
        void computeSize()
        {
            m_shmsize =  m_statesize +
                         (m_mLatchNum + m_sLatchNum) * sizeof(LatchNode) +
                         m_procNum * sizeof(ProcessLatchInfo);
        }
        void setaddress(char* r_shmaddr)
        {
            size_t   t_offset = 0;
            m_shmaddr = r_shmaddr;
            t_offset += m_statesize;
            m_mlatchaddr = (LatchNode*)(m_shmaddr + t_offset);
            t_offset += m_mLatchNum * sizeof(LatchNode);
            m_slatchaddr = (LatchNode*)(m_shmaddr + t_offset);
            t_offset += m_sLatchNum * sizeof(LatchNode);
            m_procAddr   = (ProcessLatchInfo*)(m_shmaddr + t_offset);
        }
};


#endif //MDBLatchInfo_H_INCLUDE_20100519_
