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

//预先置一次信号量 防止其等待的进程先做一次post
void LockWaits::preWaits(const TransResource* pTransRes)
{
    struct sembuf tmpSembuf;
    if (pTransRes->m_semNo >= m_semNum)
    {
        log_error("pTransRes->m_semNo(" << pTransRes->m_semNo << ") >= m_semNum(" << m_semNum << ")");
        throw Mdb_Exception(__FILE__, __LINE__, "信号灯 %d 大于设置总大小 %d!", pTransRes->m_semNo, m_semNum);
    }
    tmpSembuf.sem_num = pTransRes->m_semNo;
    tmpSembuf.sem_op  = -1;
    tmpSembuf.sem_flg = IPC_NOWAIT;
    //1.先尝试加锁
    if (semop(m_semID, &tmpSembuf, 1) == -1)
    {
        //2.若尝试加锁不成功 则说明该信号灯被其它session使用过 且其它session未解锁就异常退出了
        this->post(pTransRes->m_semNo);
        //3.解锁后 再加锁
        tmpSembuf.sem_num = pTransRes->m_semNo;
        tmpSembuf.sem_op  = -1;
        tmpSembuf.sem_flg = 0;
        if (semop(m_semID, &tmpSembuf, 1) == -1)
        {
            log_error_callp("semop", m_semID << ", &tmpSembuf(" << pTransRes->m_semNo << ", -1, 0), 1");
            throw Mdb_Exception(__FILE__, __LINE__, "信号灯wait失败 %s!", strerror(errno));
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
        throw Mdb_Exception(__FILE__, __LINE__, "信号灯 %d 大于设置总大小 %d!", pTransRes->m_semNo, m_semNum);
    }
    tmpSembuf.sem_num = pTransRes->m_semNo;
    tmpSembuf.sem_op  = -1;
    tmpSembuf.sem_flg = 0;
    //1. 等待被唤醒
    if (semop(m_semID, &tmpSembuf, 1) == -1) // 自己等待自己的信号灯
    {
        log_error_callp("semop", m_semID << ", &tmpSembuf(" << pTransRes->m_semNo << ", -1, 0), 1");
        throw Mdb_Exception(__FILE__, __LINE__, "信号灯wait失败 %s!", strerror(errno));
    }
    //2.被唤醒后将自己的信号灯复位
    this->post(pTransRes->m_semNo);
    //3.唤醒后调用回调函数
    // add by chenm 2012-09-29 解决insert时,由于IndexHash.cpp里insert方法的goto语句引起的锁多个资源导致的死锁问题
    if (iOperType == DML_IDX_INSERT)
    {
        // 如果是insert hash桶时被唤醒,则不锁定,返回重现判断
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
        throw Mdb_Exception(__FILE__, __LINE__, "信号灯 %d 大于设置总大小 %d!", iSemaphoreId, m_semNum);
    }
    tmpSembuf.sem_num = iSemaphoreId;
    tmpSembuf.sem_op  = 1;
    tmpSembuf.sem_flg = 0;
    if (semop(m_semID, &tmpSembuf, 1) == -1)
    {
        log_error_callp("semop", m_semID << ", &tmpSembuf(" << iSemaphoreId << ", 1, 0), 1");
        throw Mdb_Exception(__FILE__, __LINE__, "信号灯post失败 %s!", strerror(errno));
    }
    return;
}

int LockWaits::getSemKey(const T_NAMEDEF& dbName)
{
    int semKey = 0;
    // 根据库名算key
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
        throw Mdb_Exception(__FILE__, __LINE__, "创建系统信号量 %d 失败!", m_semID);
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
                throw Mdb_Exception(__FILE__, __LINE__, "semctl失败!");
            }
        }
    }
    return;
}

// 销毁semaphore
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
    // 需要在进程的不同内线程间进行互斥
    pthread_mutex_lock(&(LockWaits::m_internalMutex));
    itr = m_lockWaitsMap.find(dbName);
    if (itr == m_lockWaitsMap.end())
    {
        //log_info("返回 new LockWaits() 对象, dbName=" << dbName);
        LockWaits* pLockWaits = new LockWaits();
        pLockWaits->init(dbName, iSemNum);
        m_lockWaitsMap.insert(LOCKWAITS_MAP::value_type(dbName, pLockWaits));
        pthread_mutex_unlock(&(LockWaits::m_internalMutex));
        return pLockWaits;
    }
    else
    {
        //log_info("返回现有的 LockWait 对象, dbName=" << dbName);
        pthread_mutex_unlock(&(LockWaits::m_internalMutex));
        return itr->second;
    }
}

// 供PMon使用 对于异常退出的进程 将其信号灯复位
bool LockWaits::resetOneSem(const int iSemNum)
{
    union semun
    {
        int val;
        struct m_semID_ds* buf;
        ushort* array;
    } arg;
    arg.val = 1;
    log_msg("信号灯复位，semctl(" << m_semID << ", " << iSemNum << ", SETVAL, arg(val=1))");
    int ret = semctl(m_semID, iSemNum, SETVAL, arg);
    if (ret == -1)
    {
        log_error_callp("semctl", m_semID << ", " << iSemNum << ", SETVAL, arg(val=1)");
        throw Mdb_Exception(__FILE__, __LINE__, "信号灯%d semctl失败,%s!", iSemNum, strerror(errno));
    }
    return true;
}


