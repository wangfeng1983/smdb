#ifndef LOCKWAITS_H_HEADER_INCLUDED_B3E0EC62
#define LOCKWAITS_H_HEADER_INCLUDED_B3E0EC62

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <map>
#include <string>

#include "base/config-all.h"
#include "MdbConstDef2.h"
USING_NAMESPACE(std)


//##ModelId=4C1F175B0266
class LockWaits;
class TableDesc;
class TableOnUndo;
class LockMgr;
class MdbAddress;
class TransResource;

typedef map<string, LockWaits*> LOCKWAITS_MAP;
typedef LOCKWAITS_MAP::iterator LOCKWAITS_MAP_ITR;

class LockWaits
{
    public:
        void preWaits(const TransResource* pTransRes);

        // 获取自身信号灯 等待被唤醒
        //##ModelId=4C1F18510293
        int waits(TableDesc*  pUndoTabledesc
                  , const TransResource* pTransRes
                  , TableOnUndo* pTransTable
                  , LockMgr* pLockMgr
                  , const MdbAddress* pMdbAddr
                  , const int& iOperType);

        // 释放等待会话的信号灯(非自身) 唤醒等待会话
        //##ModelId=4C1F18560042
        void post(const int iSemaphoreId);

        //##ModelId=4C1F1B3800BC
        static LockWaits* getInstance(const T_NAMEDEF& dbName, const int iSemNum = LOCKWAITS_SEM_MAX);

        void init(const T_NAMEDEF& dbName
                  , const int iSemNum);

        bool destroy(const T_NAMEDEF& dbName);

        bool resetOneSem(const int iSemNum);

    private:
        //##ModelId=4C1F185A0311
        LockWaits()
        {
            m_semID = 0;
        }
        int getSemKey(const T_NAMEDEF& dbName);

    private:
        int m_semID;
        int m_semNum;
        // disabled by chenm 2012-10-15 15:48:01
        //struct sembuf m_sembuf;
        // over2012-10-15 15:48:15
        static LOCKWAITS_MAP m_lockWaitsMap;
        //##ModelId=4C1F1E0C0124
        static pthread_mutex_t m_internalMutex;
};



#endif /* LOCKWAITS_H_HEADER_INCLUDED_B3E0EC62 */
