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
            LATCHSH_FTOK_ID     = 0x7E,    //������ȡFTOKID
            MAX_LATCH_NUM       = 50,      //һ���߳̿�ͬʱ��mlatch����
            MAX_LATCHPROC_NUM   = 10000,   //�����߳���
            LATCHSHM_STATE_SIZE = 8,       //LATCH״̬λ��С
            MAX_MLATCH_NUM      = 10000000,//֧��MLATCH��������
            MAX_SLATCH_NUM      = 10000000,//֧��SLATCH��������
            MLATCH_OUTTIME      = 10000000,//MLATCHʱ�޷�ֵ10s
            SLATCH_OUTTIME      = 100000   //SLATCHʱ�޷�ֵ100ms
        };
};

class LatchNode
{
    public:
        volatile int m_latchlock;
        volatile int m_proclab;  //������Ϣ�±꣬���ڹ�������
};
class ProcessLatchInfo
{
    public:
        volatile int  m_state;      //0 δռ�ã�1 �Ѿ�ռ��
        int         m_pos;        //��PROC��Ϣ��lab
        pid_t       m_pid;        //����ID
        pthread_t   m_thid;       //�߳�ID
        time_t      m_time;       //�߳�����
        volatile  int m_slatchinfo; //����latch��Ϣ��0 δռ��,����Ϊռ��(sid)
        int         m_mlatchnum;  //���latch����
        int         m_mlatchinfo[LATCH_CONST::MAX_LATCH_NUM]; //��¼���LATCH��Ϣ
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
        T_NAMEDEF         m_dbName;    //���ݿ�����
        int               m_statesize; //״̬λ�ֽ���
        int               m_mLatchNum; //�ɶ��ռ�õ�LATCH����
        int               m_sLatchNum; //���ɶ��ռ�õ�LATCH����
        int               m_procNum;   //���������߳���
        size_t            m_shmsize;   //�����ڴ��С
        char*             m_shmaddr;   //�����ڴ���ʼ��ַ
        LatchNode*        m_mlatchaddr;//�ɶ���LATCH��ʼ��ַ
        //��0��ΪLATCHSHM�ڲ�LATCH,
        LatchNode*        m_slatchaddr;//���ɶ���LATCH��ʼ��ַ
        ProcessLatchInfo* m_procAddr;  //�߳���Ϣ��ʼ��ַ
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
