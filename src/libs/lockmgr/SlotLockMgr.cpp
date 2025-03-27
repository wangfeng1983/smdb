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
#include <map>

#include "base/config-all.h"
#include "SlotLockMgr.h"
#include "PageInfo.h"
#include "MdbConstDef2.h"
#include "TableOnUndo.h"
#include "TransResource.h"
#include "Expression.h"

/* 返回值:
	0---有效锁定
	1---本事务的删除指令执行后,这条记录不应再返回
	2---不满足附加条件,未锁定
	3---有其他会话先占用了 释放后 再被本会话锁定
*/
int SlotLockMgr::lock(TableDesc*  pUndoTabledesc
                      , TableOnUndo* pTransTable
                      , const MdbAddress& rMdbAddr
                      , const int& iOperType)
{
    bool isFirstSetITL = false;

    //1.注册ITL信息
    if (!this->hasSettedITL(rMdbAddr))
    {
        isFirstSetITL = true;
        this->setITL();
    }
    //2.锁定slot
    int iRet = this->lockSlot(pUndoTabledesc, pTransTable, rMdbAddr, iOperType);

    return iRet;
}

/*
	解锁并置时间戳
	参数： scn —— commit时申请到的时间戳
*/
bool SlotLockMgr::unlock(const T_SCN& r_scn)
{
    TRANSRES_POOL_ITR itr;
    // 释放slot 需要加slot的latch
    for (itr = m_pTransRes->m_lockedSlot.begin(); itr != m_pTransRes->m_lockedSlot.end(); ++itr)
    {
        m_pLatchMgr->slatch_lock(itr->first.m_pos);
        ((UsedSlot*)itr->first.m_addr)->m_scn = r_scn;
        ((UsedSlot*)itr->first.m_addr)->unlock();
        m_pLatchMgr->slatch_unlock(itr->first.m_pos);
    }
    // 释放itl信息 无需加latch
    for (itr = m_pTransRes->m_lockedPage.begin(); itr != m_pTransRes->m_lockedPage.end(); ++itr)
    {
        (((PageInfo*)itr->first.m_addr)->m_itls)[itr->second - 1].m_state = ITL_NOT_USE;
    }
    // 置page的时间戳
    for (itr = m_pTransRes->m_lockedPage.begin(); itr != m_pTransRes->m_lockedPage.end(); ++itr)
    {
        m_pLatchMgr->slatch_lock(itr->first.m_pos);
        ((PageInfo*)itr->first.m_addr)->m_scn_slot = r_scn;
        m_pLatchMgr->slatch_unlock(itr->first.m_pos);
    }
    return true;
}

//##ModelId=4C0DE4FA002B
bool SlotLockMgr::hasSettedITL(const MdbAddress& rMdbAddr)
{
    ShmPosition tmpShmPosi;
    TRANSRES_POOL_ITR itr;
    PageInfo* t_pPageInfo;
    m_pMemMgr->getPageBySlot(rMdbAddr.m_pos, m_pageMdbAddr.m_pos, t_pPageInfo); // 根据slot得到对应的page地址
    m_pageMdbAddr.m_addr = (char*)t_pPageInfo;
    itr = m_pTransRes->m_lockedPage.find(m_pageMdbAddr);
    if (itr == m_pTransRes->m_lockedPage.end())
    {
        return false;
    }
    else
    {
        m_iItlNo = itr->second;
        return true;
    }
}

//##ModelId=4C0DE4FF004E
bool SlotLockMgr::setITL()
{
    m_iItlNo = 0;
    while (m_iItlNo == 0)
    {
        m_pLatchMgr->slatch_lock(m_pageMdbAddr.m_pos);
        for (int i = 0; i < PAGE_ITLS_NUM; ++i)
        {
            if (((PageInfo*)m_pageMdbAddr.m_addr)->m_itls[i].m_state == ITL_NOT_USE)
            {
                m_iItlNo = i + 1;                    // 可用槽号
                m_pTransRes->m_lockedPage.insert(TRANSRES_POOL::value_type(m_pageMdbAddr, m_iItlNo)); // 记录锁定的page信息
                ((PageInfo*)m_pageMdbAddr.m_addr)->m_itls[i].m_state = ITL_IN_USE;
                ((PageInfo*)m_pageMdbAddr.m_addr)->m_itls[i].m_sid   = m_sid;
                break;
            }
        }
        m_pLatchMgr->slatch_unlock(m_pageMdbAddr.m_pos);
    }
    return true;
}

//##ModelId=4C0DE51E012F
/* 返回值:
	0---有效锁定
	1---本事务的删除指令执行后,这条记录不应再返回
	2---有附加条件(非索引条件)的情况 附加条件不满足
*/
int SlotLockMgr::lockSlot(TableDesc*  pUndoTabledesc
                          , TableOnUndo* pTransTable
                          , const MdbAddress& rMdbAddr
                          , const int& iOperType)
{
    UsedSlot* pUsedSlot;
    int iRet = 0;
    pUsedSlot = (UsedSlot*)rMdbAddr.m_addr;
    //1 获取slot latch
    m_pLatchMgr->slatch_lock(rMdbAddr.m_pos);
    //2 判断slot lock标记是否有效
    if (pUsedSlot->islock())  // 有lock
    {
        //2.1.1 是否自身锁定了该记录
        int iItlNo = (pUsedSlot->m_lockinfo & 0x70) >> 4;
        if (((PageInfo*)m_pageMdbAddr.m_addr)->m_itls[iItlNo - 1].m_sid == m_sid)
        {
            m_pLatchMgr->slatch_unlock(rMdbAddr.m_pos);
            //2.1.1.1 是删除操作
            if ((pUsedSlot->m_lockinfo | 0xF7) == 0xFF)
            {
                iRet = 1;
            }
            else
            {
                // 判断附加条件
                if (m_pTransRes->m_pExpression != NULL)
                {
                    if (m_pTransRes->m_pExpression->evaluate(rMdbAddr.m_addr + sizeof(UsedSlot), m_pTransRes->m_pRecConvert, (const void**)m_pTransRes->m_pCondValues) == false)
                    {
                        iRet =  2; // 不满足附加条件,返回
                    }
                }
            }
        }
        //2.1.2 其它会话锁定该记录
        else
        {
            //2.1.2.1 释放slot latch
            //m_pLatchMgr->slatch_unlock(rMdbAddr.m_pos); disabled by chenm 2012-12-09 放到m_pTransMgr->registerSid里去解锁

            // add by chenm 2012-10-12 11:08:35 如果有其它session的lock,则先把本session的ITL信息注销,以防止ITL槽位不够用的情况发生
            TRANSRES_POOL_ITR itr;
            itr = m_pTransRes->m_lockedPage.find(m_pageMdbAddr);
            if (itr != m_pTransRes->m_lockedPage.end())
            {
                ((PageInfo*)m_pageMdbAddr.m_addr)->m_itls[itr->second - 1].m_state = ITL_NOT_USE;
                m_pTransRes->m_lockedPage.erase(itr);
            }
            // over 2012-10-12 11:09:31

            //2.1.2.2 注册sid
            iRet = m_pTransMgr->registerSid(pTransTable
                                            , m_pTransRes
                                            , ((PageInfo*)m_pageMdbAddr.m_addr)->m_itls[iItlNo - 1].m_sid
                                            , this
                                            , pUndoTabledesc
                                            , &rMdbAddr
                                            , iOperType);
            // add by chenm 2010-12-10 9:42:01
            if (iRet == 0) // 有效锁定
            {
                iRet = 3;	// 被唤醒后的有效锁定
                // add by chenm 2012-09-29 解决insert时,由于IndexHash.cpp里insert方法的goto语句引起的锁多个资源导致的死锁问题
                if (iOperType == DML_IDX_INSERT)
                {
                    // 如果是insert hash桶时被唤醒,则不锁定,返回重现判断
                    m_pTransMgr->unregisterSid(m_pTransRes, pTransTable);
                    return iRet;
                }
                // over 2012-10-08 9:53:44
            }
            // over 2010-12-10 9:42:07
        }
    }
    else // 无lock
    {
        if (((PageInfo*)m_pageMdbAddr.m_addr)->m_desctype == '1')   // 存放表数据的slot
        {
            // 判断附加条件
            if (m_pTransRes->m_pExpression != NULL)
            {
                if (m_pTransRes->m_pExpression->evaluate(rMdbAddr.m_addr + sizeof(UsedSlot), m_pTransRes->m_pRecConvert, (const void**)m_pTransRes->m_pCondValues) == false)
                {
                    m_pLatchMgr->slatch_unlock(rMdbAddr.m_pos);
                    return 2; // 不满足附加条件,释放slot latch 返回
                }
            }
            if (iOperType == DML_UPDATE)  // 是否更新操作
            {
                try
                {
                    //2.2.1 复制前镜像至undo空间 2.2.2 置前镜像指针
                    pUsedSlot->m_undopos = m_pMemMgr->getUndoMemMgr()->insert(pUndoTabledesc
                                           , rMdbAddr.m_pos
                                           , pUsedSlot->m_scn
                                           , rMdbAddr.m_addr + sizeof(UsedSlot)
                                           , pUsedSlot->m_undopos);
                }
                catch (Mdb_Exception& e)
                {
                    m_pLatchMgr->slatch_unlock(rMdbAddr.m_pos);
                    throw e;
                }
                //2.2.3 置操作类型 update
                pUsedSlot->m_lockinfo &= 0xF7;
            }
            // 2.2.3 置操作类型delete
            else if (iOperType == DML_DELETE)
            {
                // 记录 delete 时也插入前镜像 -- 2012-08-30
                try
                {
                    pUsedSlot->m_undopos =
                        m_pMemMgr->getUndoMemMgr()->insert(pUndoTabledesc,
                                                           rMdbAddr.m_pos,
                                                           pUsedSlot->m_scn,
                                                           rMdbAddr.m_addr + sizeof(UsedSlot),
                                                           pUsedSlot->m_undopos);
                }
                catch (Mdb_Exception& e)
                {
                    m_pLatchMgr->slatch_unlock(rMdbAddr.m_pos);
                    throw e;
                }
                pUsedSlot->m_lockinfo |= 0x08;
            }
        }
        else // 存放索引数据的slot
        {
            // 将有效标记加到 value 后面, 0-有效, 1-无效 --2012-08-30
            char record_value[pUndoTabledesc->m_valueSize]; // m_valueSize 已经包含了额外的一个字节
            memcpy(record_value, rMdbAddr.m_addr + sizeof(UsedSlot), pUndoTabledesc->m_valueSize - 1);
            record_value[pUndoTabledesc->m_valueSize - 1] = ((pUsedSlot->m_lockinfo & 0x08) == 0x08) ? 0x01 : 0x00;

            try
            {
                pUsedSlot->m_undopos = m_pMemMgr->getUndoMemMgr()->insert(pUndoTabledesc
                                       , rMdbAddr.m_pos
                                       , pUsedSlot->m_scn
                                       , record_value
                                       , pUsedSlot->m_undopos);
            }
            catch (Mdb_Exception& e)
            {
                m_pLatchMgr->slatch_unlock(rMdbAddr.m_pos);
                throw e;
            }
        }
        //2.2.4 锁定slot
        pUsedSlot->lock(m_iItlNo);
        //2.2.5 slot序号++
        pUsedSlot->addno();
        //2.2.6 时间戳置0
        //pUsedSlot->m_scn = T_SCN();//2012/9/5 17:55:57 gaojf
        //2.2.7 释放slot latch
        m_pLatchMgr->slatch_unlock(rMdbAddr.m_pos);
        //3 注册sid
        m_pTransMgr->registerSid(pTransTable, m_pTransRes);
        //4 注册锁住的slot
        m_pTransRes->m_lockedSlot.insert(TRANSRES_POOL::value_type(rMdbAddr, iOperType));
    }
    return iRet;
}

