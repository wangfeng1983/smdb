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

// 0 �������Ự���� ;��0 �������Ự��ռ���� �ͷź� �ٱ����Ự���� ��SlotLockMgr��������
int BucketLockMgr::lock(TableDesc*  pUndoTabledesc
                        , TableOnUndo* pTransTable
                        , const MdbAddress& rMdbAddr
                        , const int& iOperType)
{
    int iRet = 0;
    //1 ��ȡhashͰ latch
    m_pLatchMgr->slatch_lock(rMdbAddr.m_pos);
    //2 �ж�slot lock����Ƿ���Ч
    if (((HashIndexNode*)rMdbAddr.m_addr)->m_lockinfo == BUCKET_LOCKED)
    {
        //2.1.1 �Ƿ����������˸ü�¼
        if (((HashIndexNode*)rMdbAddr.m_addr)->m_sid == m_sid)
        {
            m_pLatchMgr->slatch_unlock(rMdbAddr.m_pos);
            iRet = 1;
        }
        //2.1.2 �����Ự�����ü�¼
        else
        {
            //2.1.2.1 �ͷ�slot latch
            //m_pLatchMgr->slatch_unlock(rMdbAddr.m_pos); disabled by chenm 2012-12-09 �ŵ�m_pTransMgr->registerSid��ȥ����
            //2.1.2.2 ע��sid �ȴ�������
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
        if (((HashIndexNode*)rMdbAddr.m_addr)->m_nodepos == NULL_SHMPOS)   // �������ж�һ��hashͰ�ڵ��ַ�Ƿ�Ϊ��,��Ϊ�˱��Ự֮ǰ������,�����Ѻ�,����������ĻỰ,�Ѿ��޸���hashͰ�ڵ�,��ô���Ự����������hashͰ�ڵ���
        {
            // 2.2.1 ����
            ((HashIndexNode*)rMdbAddr.m_addr)->m_sid      = m_pTransRes->m_sid;
            ((HashIndexNode*)rMdbAddr.m_addr)->m_lockinfo = BUCKET_LOCKED;
            //2.2.2 �ͷ�hashͰ latch
            m_pLatchMgr->slatch_unlock(rMdbAddr.m_pos);
            //2.2.3 ע��sid
            m_pTransMgr->registerSid(pTransTable, m_pTransRes);
            //2.2.4 ע����ס��hashͰ�ڵ�
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
    // �ͷ�slot ��Ҫ��slot��latch
    for (itr = m_pTransRes->m_lockedIdxNode.begin(); itr != m_pTransRes->m_lockedIdxNode.end(); ++itr)
    {
        m_pLatchMgr->slatch_lock(itr->first.m_pos);
        ((HashIndexNode*)itr->first.m_addr)->m_scn = r_scn;
        ((HashIndexNode*)itr->first.m_addr)->m_lockinfo = BUCKET_FREE;
        m_pLatchMgr->slatch_unlock(itr->first.m_pos);
    }
    return true;
}
