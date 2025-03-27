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
    // 1. ע��sid�����������
    if (pTransRes->m_hasRegSid[pTransTable->getTableName()] == false)
    {
        void** pColNumValues;
        void* pRecord;
        // �������ֵ
        pColNumValues = new void*[pTransTable->m_tableDesc->m_tableDef.m_columnNum];
        pColNumValues[0] = (void*) & (pTransRes->m_sid);
        pColNumValues[1] = (void*)&NULL_SID;
        pColNumValues[2] = (void*) & (pTransRes->m_semNo);
        // �����
        m_pLatchMgr->mlatch_lock(pTransTable->getSrcLatchInfo());
        try
        {
            pRecord = pRecConvert->assembleRec((const void**)pColNumValues);
            pTransTable->insert(pRecord);
        }
        catch (Mdb_Exception& e)
        {
            m_pLatchMgr->mlatch_unlock(pTransTable->getSrcLatchInfo());
            // add by chenm 2012-12-09 ��SlotLockMgr���slot�ϵ�slatch��
            if (r_sId_b != NULL_SID)
                pLockMgr->unSlatch(pMdbAddr->m_pos);
            // over 2012-12-09 
            throw e;
        }
        m_pLatchMgr->mlatch_unlock(pTransTable->getSrcLatchInfo());
        // �ͷ���ʱ�洢
        free(pRecord);
        delete [] pColNumValues;
        pTransRes->m_hasRegSid[pTransTable->getTableName()] = true;
    }
    if (r_sId_b != NULL_SID)
    {
        m_pLatchMgr->mlatch_lock(pTransTable->getSrcLatchInfo());
        // 2.�ж��Ƿ�����
        try
        {
            if (this->isDeadLock(pTransTable, pTransRes->m_sid, r_sId_b) == true)
            {
                throw Mdb_Exception(__FILE__, __LINE__, "%d �� %d ����,%d�ع�!", pTransRes->m_sid, r_sId_b, pTransRes->m_sid);
            }
            T_NAMEDEF indexName;
            sprintf(indexName, "%s%s", INDEX_A_PREFIX, pTransTable->getTableName());
            pTransTable->update(indexName, &(pTransRes->m_sid), COLUMN_NAME_SID_B, &(r_sId_b)); //update set SID_B=r_sId_b where SID_A= pTransRes->m_sid
            m_pLockWaits->preWaits(pTransRes);
        }
        catch (Mdb_Exception& e)
        {
            m_pLatchMgr->mlatch_unlock(pTransTable->getSrcLatchInfo());
            // add by chenm 2012-12-09 ��SlotLockMgr���slot�ϵ�slatch��
            pLockMgr->unSlatch(pMdbAddr->m_pos);//m_pLatchMgr->slatch_unlock(pMdbAddr->m_pos);
            // over 2012-12-09 
            throw e;
        }
        m_pLatchMgr->mlatch_unlock(pTransTable->getSrcLatchInfo());
        // add by chenm 2012-12-09 �ȴ�������֮ǰ,��SlotLockMgr���slot�ϵ�slatch��
        pLockMgr->unSlatch(pMdbAddr->m_pos);//m_pLatchMgr->slatch_unlock(pMdbAddr->m_pos);
        // over 2012-12-09 

        // 3.�ȴ�������
        iRet = m_pLockWaits->waits(pUndoTabledesc, pTransRes
                                   , pTransTable, pLockMgr
                                   , pMdbAddr, iOperType);
    } 
    // r_sId_b == NULL_SID �����,��LockMgr�н��� by chenm 2012-12-09 
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
        if (pTransRes->m_sessionQuit == true) // �Ự�˳�ʱ,��ɾ������������е�ע����Ϣ
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
            // �������ȴ�pTransRes->m_sid�ĻỰ���ȴ�������³� iSidA
            if (recordVector.size() > 1)
            {
                pTransTable->update(indexName, &(pTransRes->m_sid), COLUMN_NAME_SID_B, &(iSidA)); // update set SID_B=iSidA where SID_B=pTransRes->m_sid
            }
            this->wakeupWaiting(iSemNoA);
            // ���±����ѻỰ��TX��Ϣ ����ȴ��ĻỰid ��� -1 ���޵ȴ�
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
        throw Mdb_Exception(__FILE__, __LINE__, "��ȡtable�б�ʧ��!");
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
    //�����в�ѯռ�ûỰ����Դ��Ϣ
    for (int i = 0; i < recordVector.size(); ++i)
    {
        pRecConvert->setRecordValue(recordVector[i].m_addr + sizeof(Undo_Slot));
        pRecConvert->getColumnValue(COLUMN_NAME_SID_B, &iSidB);
        if (iSidB != 0)
        {
            if (iSidB == r_sId_a)
            {
                // ������
                //              abort();
                return true;
            }
            else
            {
                // �ݹ��ж�
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
    // ��Ҫ�ڽ��̵Ĳ�ͬ���̼߳���л���
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
