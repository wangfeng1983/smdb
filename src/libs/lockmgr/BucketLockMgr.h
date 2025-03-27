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

#ifndef BUCKETLOCKMGR_H_HEADER_INCLUDED_B3EBE9C2
#define BUCKETLOCKMGR_H_HEADER_INCLUDED_B3EBE9C2

#include "LockMgr.h"

// ÊµÏÖË÷ÒýhashÍ°lock
//##ModelId=4C0477F00391
class BucketLockMgr : public LockMgr
{
    public:
        BucketLockMgr(MemManager* memMgr
                      , TransResource* pTransRes
                      , T_NAMEDEF& dbName): LockMgr(memMgr, pTransRes, dbName)
        {
        }
        virtual ~BucketLockMgr() {}

        virtual int lock(TableDesc*  pUndoTabledesc
                         , TableOnUndo* pTransTable
                         , const MdbAddress& rMdbAddr
                         , const int& iOperType);

        virtual bool unlock(const T_SCN& r_scn);

};



#endif /* BUCKETLOCKMGR_H_HEADER_INCLUDED_B3EBE9C2 */
