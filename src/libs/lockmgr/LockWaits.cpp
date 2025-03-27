#include <map>
#include <string.h>
#include <errno.h>
#include <pthread.h>

#include "LockWaits.h"
#include "base/config-all.h"
#include "Mdb_Exception.h"
#include "TableDescript.h"
#include "LockMgr.h"
#include "TransResource.h"
#include "debuglog.h"

LOCKWAITS_MAP LockWaits::m_lockWaitsMap;
pthread_mutex_t LockWaits::m_internalMutex = PTHREAD_MUTEX_INITIALIZER;

//Ԥ����һ���ź��� ��ֹ��ȴ��Ľ�������һ��post
void LockWaits::preWaits(const TransResource* pTransRes)
{
    struct sembuf tmpSembuf;
    if (pTransRes->m_semNo >= m_semNum)
    {
        log_error("pTransRes->m_semNo(" << pTransRes->m_semNo << ") >= m_semNum(" << m_semNum << ")");
        throw Mdb_Exception(__FILE__, __LINE__, "�źŵ� %d ���������ܴ�С %d!", pTransRes->m_semNo, m_semNum);
    }
    tmpSembuf.sem_num = pTransRes->m_semNo;
    tmpSembuf.sem_op  = -1;
    tmpSembuf.sem_flg = IPC_NOWAIT;
    //1.�ȳ��Լ���
    if (semop(m_semID, &tmpSembuf, 1) == -1)
    {
        //2.�����Լ������ɹ� ��˵�����źŵƱ�����sessionʹ�ù� ������sessionδ�������쳣�˳���
        this->post(pTransRes->m_semNo);
        //3.������ �ټ���
        tmpSembuf.sem_num = pTransRes->m_semNo;
        tmpSembuf.sem_op  = -1;
        tmpSembuf.sem_flg = 0;
        if (semop(m_semID, &tmpSembuf, 1) == -1)
        {
            log_error_callp("semop", m_semID << ", &tmpSembuf(" << pTransRes->m_semNo << ", -1, 0), 1");
            throw Mdb_Exception(__FILE__, __LINE__, "�źŵ�waitʧ�� %s!", strerror(errno));
        }
    }
    return;
}

//##ModelId=4C1F18510293
int LockWaits::waits(TableDesc*  pUndoTabledesc
                     , const TransResource* pTransRes
                     , TableOnUndo* pTransTable
                     , LockMgr* pLockMgr
                     , const MdbAddress* pMdbAddr
                     , const int& iOperType)
{
    struct sembuf tmpSembuf;
    if (pTransRes->m_semNo >= m_semNum)
    {
        log_error("pTransRes->m_semNo(" << pTransRes->m_semNo << ") >= m_semNum(" << m_semNum << ")");
        throw Mdb_Exception(__FILE__, __LINE__, "�źŵ� %d ���������ܴ�С %d!", pTransRes->m_semNo, m_semNum);
    }
    tmpSembuf.sem_num = pTransRes->m_semNo;
    tmpSembuf.sem_op  = -1;
    tmpSembuf.sem_flg = 0;
    //1. �ȴ�������
    if (semop(m_semID, &tmpSembuf, 1) == -1) // �Լ��ȴ��Լ����źŵ�
    {
        log_error_callp("semop", m_semID << ", &tmpSembuf(" << pTransRes->m_semNo << ", -1, 0), 1");
        throw Mdb_Exception(__FILE__, __LINE__, "�źŵ�waitʧ�� %s!", strerror(errno));
    }
    //2.�����Ѻ��Լ����źŵƸ�λ
    this->post(pTransRes->m_semNo);
    //3.���Ѻ���ûص�����
    // add by chenm 2012-09-29 ���insertʱ,����IndexHash.cpp��insert������goto���������������Դ���µ���������
    if (iOperType == DML_IDX_INSERT)
    {
        // �����insert hashͰʱ������,������,���������ж�
        return 0;
    }
    else
    {
        // over 2012-09-29 9:57:26
        return pLockMgr->lock(pUndoTabledesc, pTransTable, *pMdbAddr, iOperType);
    }
}

//##ModelId=4C1F18560042
void LockWaits::post(const int iSemaphoreId)
{
    struct sembuf tmpSembuf;
    if (iSemaphoreId >= m_semNum)
    {
        log_error("pTransRes->m_semNo(" << iSemaphoreId << ") >= m_semNum(" << m_semNum << ")");
        throw Mdb_Exception(__FILE__, __LINE__, "�źŵ� %d ���������ܴ�С %d!", iSemaphoreId, m_semNum);
    }
    tmpSembuf.sem_num = iSemaphoreId;
    tmpSembuf.sem_op  = 1;
    tmpSembuf.sem_flg = 0;
    if (semop(m_semID, &tmpSembuf, 1) == -1)
    {
        log_error_callp("semop", m_semID << ", &tmpSembuf(" << iSemaphoreId << ", 1, 0), 1");
        throw Mdb_Exception(__FILE__, __LINE__, "�źŵ�postʧ�� %s!", strerror(errno));
    }
    return;
}

int LockWaits::getSemKey(const T_NAMEDEF& dbName)
{
    int semKey = 0;
    // ���ݿ�����key
    for (int i = 0; i < strlen(dbName); ++i)
    {
        semKey += (dbName[i] - 65) * (10 ^ i);
    }
    //log_info("semKey = " << semKey << ", dbName=" << dbName);
    return semKey;
}

//##ModelId=4C1F18740122
void LockWaits::init(const T_NAMEDEF& dbName
                     , const int iSemNum)
{
    bool isFirst = false;
    int ret;
    int semKey = this->getSemKey(dbName);
    m_semNum = iSemNum;
    m_semID = semget(semKey, 0, 0);
    if (m_semID == -1)
    {
        m_semID = semget(semKey, m_semNum, IPC_CREAT | IPC_EXCL | 0666);
        isFirst = true;
    }
    if (m_semID < 0)
    {
        log_error_callp("semget", semKey << ", " << m_semNum << ", IPC_CREAT | IPC_EXCL | 0666");
        throw Mdb_Exception(__FILE__, __LINE__, "����ϵͳ�ź��� %d ʧ��!", m_semID);
    }
    if (isFirst)
    {
        union semun
        {
            int val;
            struct m_semID_ds* buf;
            ushort* array;
        } arg;
        for (int i = 0; i < m_semNum; i++)
        {
            arg.val = 1;
            ret = semctl(m_semID, i, SETVAL, arg);
            if (ret == -1)
            {
                log_error_callp("semctl", m_semID << ", " << i << ", SETVAL, arg(val=1)");
                semctl(m_semID, 0, IPC_RMID);
                throw Mdb_Exception(__FILE__, __LINE__, "semctlʧ��!");
            }
        }
    }
    return;
}

// ����semaphore
bool LockWaits::destroy(const T_NAMEDEF& dbName)
{
    int ret;
    int semKey = this->getSemKey(dbName);
    if (m_semID > 0)
    {
        ret = semctl(m_semID, 0, IPC_RMID);
        if (ret == -1)
        {
            log_error_callp("semctl", m_semID << ", 0, IPC_RMID");
            throw Mdb_Exception(__FILE__, __LINE__, "In SysSemaphore:sem_destroy():sem_destroy->semctl error");
        }
    }
    return true;
}

//##ModelId=4C1F1B3800BC
LockWaits* LockWaits::getInstance(const T_NAMEDEF& dbName
                                  , const int iSemNum)
{
    LOCKWAITS_MAP_ITR itr;
    // ��Ҫ�ڽ��̵Ĳ�ͬ���̼߳���л���
    pthread_mutex_lock(&(LockWaits::m_internalMutex));
    itr = m_lockWaitsMap.find(dbName);
    if (itr == m_lockWaitsMap.end())
    {
        //log_info("���� new LockWaits() ����, dbName=" << dbName);
        LockWaits* pLockWaits = new LockWaits();
        pLockWaits->init(dbName, iSemNum);
        m_lockWaitsMap.insert(LOCKWAITS_MAP::value_type(dbName, pLockWaits));
        pthread_mutex_unlock(&(LockWaits::m_internalMutex));
        return pLockWaits;
    }
    else
    {
        //log_info("�������е� LockWait ����, dbName=" << dbName);
        pthread_mutex_unlock(&(LockWaits::m_internalMutex));
        return itr->second;
    }
}

// ��PMonʹ�� �����쳣�˳��Ľ��� �����źŵƸ�λ
bool LockWaits::resetOneSem(const int iSemNum)
{
    union semun
    {
        int val;
        struct m_semID_ds* buf;
        ushort* array;
    } arg;
    arg.val = 1;
    log_msg("�źŵƸ�λ��semctl(" << m_semID << ", " << iSemNum << ", SETVAL, arg(val=1))");
    int ret = semctl(m_semID, iSemNum, SETVAL, arg);
    if (ret == -1)
    {
        log_error_callp("semctl", m_semID << ", " << iSemNum << ", SETVAL, arg(val=1)");
        throw Mdb_Exception(__FILE__, __LINE__, "�źŵ�%d semctlʧ��,%s!", iSemNum, strerror(errno));
    }
    return true;
}


