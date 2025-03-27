#ifndef TRANSACTIONMGR_H_HEADER_INCLUDED_B3E0D7F7
#define TRANSACTIONMGR_H_HEADER_INCLUDED_B3E0D7F7

#include <string>
#include <map>
#include <vector>

#include "base/config-all.h"
#include "MdbConstDef.h"

USING_NAMESPACE(std)


//##ModelId=4C04845103A7
class MDBLatchMgr;
class LockWaits;
class TableOnUndo;
class TransactionMgr;
class TableDesc;
class LockMgr;
class TransResource;
class MdbAddress;
class MemManager;

/*
##  ����ֻ�ṩ���� �����������ݶ��󡪡�undo����ǰ�����,������ĳ�Ա,ͨ����ķ����Ĳ�������
## һ����һ��TransactionMgrʵ��
## �����������ṹ
   int SID_A
   int SID_B
   int SEM_NO_A
   ��������: _TX_"��ʽ����"
## ��������,�ֱ����ֶ�SID_A��SID_B��
   ��������_IDX_A_TX_"��ʽ����",_IDX_B_TX_"��ʽ����"
*/
typedef map<string, TransactionMgr*> TRANSACTIONMGRS_MAP;
typedef TRANSACTIONMGRS_MAP::iterator TRANSACTIONMGRS_MAP_ITR;
static const char* INDEX_A_PREFIX = "_IDX_A_TX_";
static const char* INDEX_B_PREFIX = "_IDX_B_TX_";
static const char* COLUMN_NAME_SID_A = "SID_A";
static const char* COLUMN_NAME_SID_B = "SID_B";
static const char* COLUMN_NAME_SEM_NO_A = "SEM_NO_A";
static const int    NULL_SID = -1;

class TransactionMgr
{
    public:
        //##ModelId=4C104BCF037A
        ~TransactionMgr();

        //##ModelId=4C12EC4C006D
        bool initialize(const T_NAMEDEF& dbName);

        //##ModelId=4C104BA502BF
        int registerSid(TableOnUndo* pTransTable       // undo�ռ��ϵ������������
                        , TransResource* pTransRes
                        , const int r_sId_b = -1
                                              , LockMgr* pLockMgr = NULL
                                                      , TableDesc*  pUndoTabledesc = NULL
                                                              , const MdbAddress* pMdbAddr = NULL
                                                                      , const int& iOperType = 0);

        //##ModelId=4C0484AD0392
        bool unregisterSid(const TransResource* pTransRes
                           , TableOnUndo* pTransTable);

        //##ModelId=4C1055700000
        bool getSessionList(map<string, vector<int> > & sidMap, MemManager* pMemMgr);

        static TransactionMgr* getInstance(const T_NAMEDEF& dbName);

    private:
        //##ModelId=4C1061EF029F
        void wakeupWaiting(int iSemNoA);
        TransactionMgr();
        bool isDeadLock(TableOnUndo* pTransTable
                        , const int r_sId_a
                        , const int r_sId_b);

    private:
        //##ModelId=4C104D8802DE
        MDBLatchMgr* m_pLatchMgr;
        LockWaits*   m_pLockWaits;

        static TRANSACTIONMGRS_MAP m_TrMgrsMap;
        static pthread_mutex_t m_internalMutex;
};



#endif /* TRANSACTIONMGR_H_HEADER_INCLUDED_B3E0D7F7 */
