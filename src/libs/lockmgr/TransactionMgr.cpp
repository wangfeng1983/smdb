#include <pthread.h>
#include "TransactionMgr.h"
#include "MDBLatchMgr.h"
#include "Mdb_Exception.h"
#include "LockWaits.h"
#include "TableOnUndo.h"
#include "TransResource.h"
#include "MemManager.h"
#include "HPTimer.h"
#include "LockMgr.h"


class SysTime1
{
    public:
        const char* toString()
        {
            ::gettimeofday(&tv, &tz);
            struct tm* stm = ::localtime(&tv.tv_sec);
            ::sprintf(m_time_string,
                      "%04d-%02d-%02d %02d:%02d:%02d.%06d",
                      stm->tm_year + 1900,
                      stm->tm_mon + 1,
                      stm->tm_mday,
                      stm->tm_hour,
                      stm->tm_min,
                      stm->tm_sec,
                      tv.tv_usec);
            return m_time_string;
        };

    private:
        struct timeval tv;
        struct timezone tz;
        char m_time_string[32]; //2008-12-18 12:00:00.xxxxxx
};

TRANSACTIONMGRS_MAP TransactionMgr::m_TrMgrsMap;
pthread_mutex_t TransactionMgr::m_internalMutex = PTHREAD_MUTEX_INITIALIZER;

//##ModelId=4C1061EF029F
TransactionMgr::TransactionMgr()
{
    m_pLatchMgr = NULL;
}

//##ModelId=4C104BCF037A
TransactionMgr::~TransactionMgr()
{
}

//##ModelId=4C12EC4C006D
bool TransactionMgr::initialize(const T_NAMEDEF& dbName)
{
    m_pLatchMgr = MDBLatchMgr::getInstance(dbName);
    m_pLockWaits = LockWaits::getInstance(dbName);
    return true;
}

//##ModelId=4C104BA502BF
int TransactionMgr::registerSid(TableOnUndo* pTransTable
                                , TransResource* pTransRes
                                , const int r_sId_b
                                , LockMgr* pLockMgr
                                , TableDesc*  pUndoTabledesc
                                , const MdbAddress* pMdbAddr
                                , const int& iOperType)
{
    RecordConvert* pRecConvert = pTransTable->getRecordConvert();
    int iRet = 0;
    // 1. 注册sid至事务管理器
    if (pTransRes->m_hasRegSid[pTransTable->getTableName()] == false)
    {
        void** pColNumValues;
        void* pRecord;
        // 构造插入值
        pColNumValues = new void*[pTransTable->m_tableDesc->m_tableDef.m_columnNum];
        pColNumValues[0] = (void*) & (pTransRes->m_sid);
        pColNumValues[1] = (void*)&NULL_SID;
        pColNumValues[2] = (void*) & (pTransRes->m_semNo);
        // 插入表
        m_pLatchMgr->mlatch_lock(pTransTable->getSrcLatchInfo());
        try
        {
            pRecord = pRecConvert->assembleRec((const void**)pColNumValues);
            pTransTable->insert(pRecord);
        }
        catch (Mdb_Exception& e)
        {
            m_pLatchMgr->mlatch_unlock(pTransTable->getSrcLatchInfo());
            // add by chenm 2012-12-09 解SlotLockMgr里的slot上的slatch锁
            if (r_sId_b != NULL_SID)
                pLockMgr->unSlatch(pMdbAddr->m_pos);
            // over 2012-12-09 
            throw e;
        }
        m_pLatchMgr->mlatch_unlock(pTransTable->getSrcLatchInfo());
        // 释放临时存储
        free(pRecord);
        delete [] pColNumValues;
        pTransRes->m_hasRegSid[pTransTable->getTableName()] = true;
    }
    if (r_sId_b != NULL_SID)
    {
        m_pLatchMgr->mlatch_lock(pTransTable->getSrcLatchInfo());
        // 2.判断是否死锁
        try
        {
            if (this->isDeadLock(pTransTable, pTransRes->m_sid, r_sId_b) == true)
            {
                throw Mdb_Exception(__FILE__, __LINE__, "%d 被 %d 死锁,%d回滚!", pTransRes->m_sid, r_sId_b, pTransRes->m_sid);
            }
            T_NAMEDEF indexName;
            sprintf(indexName, "%s%s", INDEX_A_PREFIX, pTransTable->getTableName());
            pTransTable->update(indexName, &(pTransRes->m_sid), COLUMN_NAME_SID_B, &(r_sId_b)); //update set SID_B=r_sId_b where SID_A= pTransRes->m_sid
            m_pLockWaits->preWaits(pTransRes);
        }
        catch (Mdb_Exception& e)
        {
            m_pLatchMgr->mlatch_unlock(pTransTable->getSrcLatchInfo());
            // add by chenm 2012-12-09 解SlotLockMgr里的slot上的slatch锁
            pLockMgr->unSlatch(pMdbAddr->m_pos);//m_pLatchMgr->slatch_unlock(pMdbAddr->m_pos);
            // over 2012-12-09 
            throw e;
        }
        m_pLatchMgr->mlatch_unlock(pTransTable->getSrcLatchInfo());
        // add by chenm 2012-12-09 等待被唤醒之前,解SlotLockMgr里的slot上的slatch锁
        pLockMgr->unSlatch(pMdbAddr->m_pos);//m_pLatchMgr->slatch_unlock(pMdbAddr->m_pos);
        // over 2012-12-09 

        // 3.等待被唤醒
        iRet = m_pLockWaits->waits(pUndoTabledesc, pTransRes
                                   , pTransTable, pLockMgr
                                   , pMdbAddr, iOperType);
    } 
    // r_sId_b == NULL_SID 的情况,在LockMgr中解锁 by chenm 2012-12-09 
    return iRet;
}

//##ModelId=4C0484AD0392
bool TransactionMgr::unregisterSid(const TransResource* pTransRes
                                   , TableOnUndo* pTransTable)
{
    T_NAMEDEF indexName;
    vector<MdbAddress> recordVector;
    recordVector.clear();
    RecordConvert* pRecConvert = pTransTable->getRecordConvert();
    m_pLatchMgr->mlatch_lock(pTransTable->getSrcLatchInfo());
    try
    {
        if (pTransRes->m_sessionQuit == true) // 会话退出时,才删除事务管理器中的注册信息
        {
            sprintf(indexName, "%s%s", INDEX_A_PREFIX, pTransTable->getTableName());
            pTransTable->deleteRec(indexName, &(pTransRes->m_sid));       // delete where SID_A= r_sId
        }
        sprintf(indexName, "%s%s", INDEX_B_PREFIX, pTransTable->getTableName());
        pTransTable->select(indexName, (const char*) & (pTransRes->m_sid), recordVector); // select where SID_B= pTransRes->m_sid
        if (recordVector.size() > 0)
        {
            int iSidA, iSemNoA;
            pRecConvert->setRecordValue(recordVector[0].m_addr + sizeof(Undo_Slot));
            pRecConvert->getColumnValue(COLUMN_NAME_SID_A, &iSidA);
            pRecConvert->getColumnValue(COLUMN_NAME_SEM_NO_A, &iSemNoA);
            // 将其他等待pTransRes->m_sid的会话，等待对象更新成 iSidA
            if (recordVector.size() > 1)
            {
                pTransTable->update(indexName, &(pTransRes->m_sid), COLUMN_NAME_SID_B, &(iSidA)); // update set SID_B=iSidA where SID_B=pTransRes->m_sid
            }
            this->wakeupWaiting(iSemNoA);
            // 更新被唤醒会话的TX信息 将其等待的会话id 变成 -1 即无等待
            sprintf(indexName, "%s%s", INDEX_A_PREFIX, pTransTable->getTableName());
            pTransTable->update(indexName, &iSidA, COLUMN_NAME_SID_B, &(NULL_SID));
        }
    }
    catch (Mdb_Exception& e)
    {
        m_pLatchMgr->mlatch_unlock(pTransTable->getSrcLatchInfo());
        throw e;
    }
    m_pLatchMgr->mlatch_unlock(pTransTable->getSrcLatchInfo());
    return true;
}

//##ModelId=4C1055700000
bool TransactionMgr::getSessionList(map<string, vector<int> > & sidMap, MemManager* pMemMgr)

{
    vector<TableDef>    tableDefList;
    TableOnUndo*        pTransTable;
    T_NAMEDEF cTransTableName;
    vector<int> sids;
    T_NAMEDEF indexName;
    vector<MdbAddress> recordVector;
    if (!pMemMgr->getTableDefList(tableDefList))
    {
        throw Mdb_Exception(__FILE__, __LINE__, "获取table列表失败!");
    }
    for (int i = 0; i < tableDefList.size(); ++i)
    {
        sids.clear();
        if (tableDefList[i].m_tableType == DATA_TABLE)
        {
            sprintf(cTransTableName, "%s%s", _TX_TBPREFIX, tableDefList[i].m_tableName);
            pTransTable = new TableOnUndo(cTransTableName, pMemMgr);
            pTransTable->initialize();
            m_pLatchMgr->mlatch_lock(pTransTable->getSrcLatchInfo());
            try
            {
                sprintf(indexName, "%s%s", INDEX_A_PREFIX, pTransTable->getTableName());
                pTransTable->select(indexName, NULL, recordVector); // select where SID_B= pTransRes->m_sid
                for (int j = 0; j < recordVector.size(); ++j)
                {
                    int iSidA, iSemNoA;
                    pTransTable->getRecordConvert()->setRecordValue(recordVector[j].m_addr + sizeof(Undo_Slot));
                    pTransTable->getRecordConvert()->getColumnValue(COLUMN_NAME_SID_A, &iSidA);
                    sids.push_back(iSidA);
                }
            }
            catch (Mdb_Exception& e)
            {
                m_pLatchMgr->mlatch_unlock(pTransTable->getSrcLatchInfo());
                delete pTransTable;
                throw e;
            }
            m_pLatchMgr->mlatch_unlock(pTransTable->getSrcLatchInfo());
            delete pTransTable;
            if (sids.size() > 0)
            {
                sidMap.insert(map<string, vector<int> >::value_type(tableDefList[i].m_tableName, sids));
            }
        }
    }
    return true;
}


//##ModelId=4C0484BD025B
bool TransactionMgr::isDeadLock(TableOnUndo* pTransTable
                                , const int r_sId_a
                                , const int r_sId_b)
{
    int iSidB;
    T_NAMEDEF indexName;
    vector<MdbAddress> recordVector;
    recordVector.clear();
    RecordConvert* pRecConvert = pTransTable->getRecordConvert();
    sprintf(indexName, "%s%s", INDEX_A_PREFIX, pTransTable->getTableName());
    pTransTable->select(indexName, (const char*) & (r_sId_b), recordVector); //where SID_A= r_sId_b
    //从其中查询占用会话的资源信息
    for (int i = 0; i < recordVector.size(); ++i)
    {
        pRecConvert->setRecordValue(recordVector[i].m_addr + sizeof(Undo_Slot));
        pRecConvert->getColumnValue(COLUMN_NAME_SID_B, &iSidB);
        if (iSidB != 0)
        {
            if (iSidB == r_sId_a)
            {
                // 是死锁
                //              abort();
                return true;
            }
            else
            {
                // 递归判断
                return this->isDeadLock(pTransTable, r_sId_a, iSidB);
            }
        }
    }
    return false;
}

//##ModelId=4C0484E40150
void TransactionMgr::wakeupWaiting(int iSemNoA)
{
    m_pLockWaits->post(iSemNoA);
}

TransactionMgr* TransactionMgr::getInstance(const T_NAMEDEF& dbName)
{
    TRANSACTIONMGRS_MAP_ITR itr;
    // 需要在进程的不同内线程间进行互斥
    pthread_mutex_lock(&(TransactionMgr::m_internalMutex));
    itr = m_TrMgrsMap.find(dbName);
    if (itr == m_TrMgrsMap.end())
    {
        TransactionMgr* pTrMgr = new TransactionMgr();
        pTrMgr->initialize(dbName);
        m_TrMgrsMap.insert(TRANSACTIONMGRS_MAP::value_type(dbName, pTrMgr));
        pthread_mutex_unlock(&(TransactionMgr::m_internalMutex));
        return pTrMgr;
    }
    else
    {
        pthread_mutex_unlock(&(TransactionMgr::m_internalMutex));
        return itr->second;
    }
}
