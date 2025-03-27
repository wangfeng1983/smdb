// ############################################
// Source file :
// Version     : 2.0
// Language	   : ANSI C++
// OS Platform : UNIX
// Authors     : gao xile
// E_mail      : gaoxl@lianchuang.com
// Create      : 2010-06-12
// Update      : 2010-06-12
// Copyright(C): gaoxile, Linkage.
// ############################################

#ifndef SLOTLOCKMGR_H_HEADER_INCLUDED_B3EBBA3F
#define SLOTLOCKMGR_H_HEADER_INCLUDED_B3EBBA3F

#include "LockMgr.h"

// slot lock
/*
	## ��slot���м�lock���� �������ݼ�¼��slot�������ڵ��slot
	## һ��sessionһ��SlotLockMgrʵ��
*/
//##ModelId=4C0477EA021F

class PageInfo;

const char ITL_IN_USE = 0x1;
const char ITL_NOT_USE = 0x0;

class SlotLockMgr : public LockMgr
{
    public:
        SlotLockMgr(MemManager* memMgr
                    , TransResource* pTransRes
                    , T_NAMEDEF& dbName): LockMgr(memMgr, pTransRes, dbName)
        {
        }
        virtual ~SlotLockMgr() {}

        //##ModelId=4BFF3BB4008D
        virtual int lock(TableDesc*  pUndoTabledesc
                         , TableOnUndo* pTransTable
                         , const MdbAddress& rMdbAddr
                         , const int& iOperType);

        //##ModelId=4BFF3BB80165
        virtual bool unlock(const T_SCN& r_scn);

    protected:
        //##ModelId=4C0DE4FA002B
        bool hasSettedITL(const MdbAddress& rMdbAddr);

        // ��Ҫ�ж�page��ITL���Ƿ��п���
        // ���û�п��� ��Ҫ�ȴ�
        //##ModelId=4C0DE4FF004E
        bool setITL();

        //##ModelId=4C0DE51E012F
        int lockSlot(TableDesc*  pUndoTabledesc
                     , TableOnUndo* pTransTable
                     , const MdbAddress& rMdbAddr
                     , const int& iOperType);

    private:
        int         m_iItlNo;
        MdbAddress  m_pageMdbAddr;

};



#endif /* SLOTLOCKMGR_H_HEADER_INCLUDED_B3EBBA3F */
