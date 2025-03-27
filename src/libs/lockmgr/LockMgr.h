// // ############################################
// // Source file :
// // Version     : 2.0
// // Language	   : ANSI C++
// // OS Platform : UNIX
// // Authors     : gao xile
// // E_mail      : gaoxl@lianchuang.com
// // Create      : 2010-06-12
// // Update      : 2010-06-12
// // Copyright(C): gaoxile, Linkage.
// // ############################################

#ifndef LOCKMGR_H_HEADER_INCLUDED_B3EB9484
#define LOCKMGR_H_HEADER_INCLUDED_B3EB9484

#include "base/config-all.h"
#include "MdbConstDef.h"
#include "MdbConstDef2.h"
#include "MDBLatchMgr.h"
#include "TransactionMgr.h"
#include "TransResource.h"

// 抽象类
// 定义接口
// 实现记录lock(包括slot和hash bucket)
//##ModelId=4BFF3A9B025F
class MemManager;
class UsedObjs;

class LockMgr
{
    public:
        LockMgr(MemManager* memMgr
                , TransResource* pTransRes
                , T_NAMEDEF& dbName)
        {
            m_pMemMgr    = memMgr;
            m_pTransRes  = pTransRes;
            m_pLatchMgr  = MDBLatchMgr::getInstance(dbName);
            m_sid        = pTransRes->m_sid;
            m_pTransMgr  = TransactionMgr::getInstance(dbName);
        }
        virtual ~LockMgr() {}

        //##ModelId=4BFF3BB4008D
        virtual int lock(TableDesc*  pUndoTabledesc
                         , TableOnUndo* pTransTable
                         , const MdbAddress& rMdbAddr
                         , const int& iOperType) = 0;

        //##ModelId=4BFF3BB80165
        virtual bool unlock(const T_SCN& r_scn) = 0;

        //##ModelId=4C1435ED01E8
        bool initial();
        void sLatch(const ShmPosition& rPos)
        {
            m_pLatchMgr->slatch_lock(rPos);
        }

        //##ModelId=4BFF3BB80165
        void unSlatch(const ShmPosition& rPos)
        {
            m_pLatchMgr->slatch_unlock(rPos);
        }

    protected:
        //##ModelId=4C143F93006F
        TransactionMgr* m_pTransMgr;
        MemManager*     m_pMemMgr;
        TransResource*  m_pTransRes;
        MDBLatchMgr*    m_pLatchMgr;
        int             m_sid;
};



#endif /* LOCKMGR_H_HEADER_INCLUDED_B3EB9484 */
