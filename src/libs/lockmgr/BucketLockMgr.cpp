// // ############################################
// // Source file :
// // Version     : 2.0
// // Language	  : ANSI C++
// // OS Platform : UNIX
// // Authors     : gao xile
// // E_mail      : gaoxl@lianchuang.com
// // Create      : 2010-06-12
// // Update      : 2010-06-12
// // Copyright(C): gaoxile, Linkage.
// // ############################################

#include "BucketLockMgr.h"
#include "IndexHash.h"

// 0 无其他会话锁定 ;非0 有其他会话先占用了 释放后 再被本会话锁定 和SlotLockMgr有所区别
int BucketLockMgr::lock(TableDesc*  pUndoTabledesc
                        , TableOnUndo* pTransTable
                        , const MdbAddress& rMdbAddr
                        , const int& iOperType)
{
    int iRet = 0;
    //1 获取hash桶 latch
    m_pLatchMgr->slatch_lock(rMdbAddr.m_pos);
    //2 判断slot lock标记是否有效
    if (((HashIndexNode*)rMdbAddr.m_addr)->m_lockinfo == BUCKET_LOCKED)
    {
        //2.1.1 是否自身锁定了该记录
        if (((HashIndexNode*)rMdbAddr.m_addr)->m_sid == m_sid)
        {
            m_pLatchMgr->slatch_unlock(rMdbAddr.m_pos);
            iRet = 1;
        }
        //2.1.2 其它会话锁定该记录
        else
        {
            //2.1.2.1 释放slot latch
            //m_pLatchMgr->slatch_unlock(rMdbAddr.m_pos); disabled by chenm 2012-12-09 放到m_pTransMgr->registerSid里去解锁
            //2.1.2.2 注册sid 等待被唤醒
            m_pTransMgr->registerSid(pTransTable
                                     , m_pTransRes
                                     , ((HashIndexNode*)rMdbAddr.m_addr)->m_sid
                                     , this
                                     , pUndoTabledesc
                                     , &rMdbAddr
                                     , iOperType);
            iRet = 2;
        }
    }
    else
    {
        if (((HashIndexNode*)rMdbAddr.m_addr)->m_nodepos == NULL_SHMPOS)   // 这里再判断一次hash桶节点地址是否为空,是为了本会话之前被阻塞,被唤醒后,如果阻塞它的会话,已经修改了hash桶节点,那么本会话就无需再锁hash桶节点了
        {
            // 2.2.1 锁定
            ((HashIndexNode*)rMdbAddr.m_addr)->m_sid      = m_pTransRes->m_sid;
            ((HashIndexNode*)rMdbAddr.m_addr)->m_lockinfo = BUCKET_LOCKED;
            //2.2.2 释放hash桶 latch
            m_pLatchMgr->slatch_unlock(rMdbAddr.m_pos);
            //2.2.3 注册sid
            m_pTransMgr->registerSid(pTransTable, m_pTransRes);
            //2.2.4 注册锁住的hash桶节点
            m_pTransRes->m_lockedIdxNode.insert(TRANSRES_POOL::value_type(rMdbAddr, iOperType));
        }
        else
        {
            m_pLatchMgr->slatch_unlock(rMdbAddr.m_pos);
        }
    }
    return iRet;
}

bool BucketLockMgr::unlock(const T_SCN& r_scn)
{
    TRANSRES_POOL_ITR itr;
    // 释放slot 需要加slot的latch
    for (itr = m_pTransRes->m_lockedIdxNode.begin(); itr != m_pTransRes->m_lockedIdxNode.end(); ++itr)
    {
        m_pLatchMgr->slatch_lock(itr->first.m_pos);
        ((HashIndexNode*)itr->first.m_addr)->m_scn = r_scn;
        ((HashIndexNode*)itr->first.m_addr)->m_lockinfo = BUCKET_FREE;
        m_pLatchMgr->slatch_unlock(itr->first.m_pos);
    }
    return true;
}
