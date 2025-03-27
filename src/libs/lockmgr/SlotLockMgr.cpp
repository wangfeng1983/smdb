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

/* ����ֵ:
	0---��Ч����
	1---�������ɾ��ָ��ִ�к�,������¼��Ӧ�ٷ���
	2---�����㸽������,δ����
	3---�������Ự��ռ���� �ͷź� �ٱ����Ự����
*/
int SlotLockMgr::lock(TableDesc*  pUndoTabledesc
                      , TableOnUndo* pTransTable
                      , const MdbAddress& rMdbAddr
                      , const int& iOperType)
{
    bool isFirstSetITL = false;

    //1.ע��ITL��Ϣ
    if (!this->hasSettedITL(rMdbAddr))
    {
        isFirstSetITL = true;
        this->setITL();
    }
    //2.����slot
    int iRet = this->lockSlot(pUndoTabledesc, pTransTable, rMdbAddr, iOperType);

    return iRet;
}

/*
	��������ʱ���
	������ scn ���� commitʱ���뵽��ʱ���
*/
bool SlotLockMgr::unlock(const T_SCN& r_scn)
{
    TRANSRES_POOL_ITR itr;
    // �ͷ�slot ��Ҫ��slot��latch
    for (itr = m_pTransRes->m_lockedSlot.begin(); itr != m_pTransRes->m_lockedSlot.end(); ++itr)
    {
        m_pLatchMgr->slatch_lock(itr->first.m_pos);
        ((UsedSlot*)itr->first.m_addr)->m_scn = r_scn;
        ((UsedSlot*)itr->first.m_addr)->unlock();
        m_pLatchMgr->slatch_unlock(itr->first.m_pos);
    }
    // �ͷ�itl��Ϣ �����latch
    for (itr = m_pTransRes->m_lockedPage.begin(); itr != m_pTransRes->m_lockedPage.end(); ++itr)
    {
        (((PageInfo*)itr->first.m_addr)->m_itls)[itr->second - 1].m_state = ITL_NOT_USE;
    }
    // ��page��ʱ���
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
    m_pMemMgr->getPageBySlot(rMdbAddr.m_pos, m_pageMdbAddr.m_pos, t_pPageInfo); // ����slot�õ���Ӧ��page��ַ
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
                m_iItlNo = i + 1;                    // ���òۺ�
                m_pTransRes->m_lockedPage.insert(TRANSRES_POOL::value_type(m_pageMdbAddr, m_iItlNo)); // ��¼������page��Ϣ
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
/* ����ֵ:
	0---��Ч����
	1---�������ɾ��ָ��ִ�к�,������¼��Ӧ�ٷ���
	2---�и�������(����������)����� ��������������
*/
int SlotLockMgr::lockSlot(TableDesc*  pUndoTabledesc
                          , TableOnUndo* pTransTable
                          , const MdbAddress& rMdbAddr
                          , const int& iOperType)
{
    UsedSlot* pUsedSlot;
    int iRet = 0;
    pUsedSlot = (UsedSlot*)rMdbAddr.m_addr;
    //1 ��ȡslot latch
    m_pLatchMgr->slatch_lock(rMdbAddr.m_pos);
    //2 �ж�slot lock����Ƿ���Ч
    if (pUsedSlot->islock())  // ��lock
    {
        //2.1.1 �Ƿ����������˸ü�¼
        int iItlNo = (pUsedSlot->m_lockinfo & 0x70) >> 4;
        if (((PageInfo*)m_pageMdbAddr.m_addr)->m_itls[iItlNo - 1].m_sid == m_sid)
        {
            m_pLatchMgr->slatch_unlock(rMdbAddr.m_pos);
            //2.1.1.1 ��ɾ������
            if ((pUsedSlot->m_lockinfo | 0xF7) == 0xFF)
            {
                iRet = 1;
            }
            else
            {
                // �жϸ�������
                if (m_pTransRes->m_pExpression != NULL)
                {
                    if (m_pTransRes->m_pExpression->evaluate(rMdbAddr.m_addr + sizeof(UsedSlot), m_pTransRes->m_pRecConvert, (const void**)m_pTransRes->m_pCondValues) == false)
                    {
                        iRet =  2; // �����㸽������,����
                    }
                }
            }
        }
        //2.1.2 �����Ự�����ü�¼
        else
        {
            //2.1.2.1 �ͷ�slot latch
            //m_pLatchMgr->slatch_unlock(rMdbAddr.m_pos); disabled by chenm 2012-12-09 �ŵ�m_pTransMgr->registerSid��ȥ����

            // add by chenm 2012-10-12 11:08:35 ���������session��lock,���Ȱѱ�session��ITL��Ϣע��,�Է�ֹITL��λ�����õ��������
            TRANSRES_POOL_ITR itr;
            itr = m_pTransRes->m_lockedPage.find(m_pageMdbAddr);
            if (itr != m_pTransRes->m_lockedPage.end())
            {
                ((PageInfo*)m_pageMdbAddr.m_addr)->m_itls[itr->second - 1].m_state = ITL_NOT_USE;
                m_pTransRes->m_lockedPage.erase(itr);
            }
            // over 2012-10-12 11:09:31

            //2.1.2.2 ע��sid
            iRet = m_pTransMgr->registerSid(pTransTable
                                            , m_pTransRes
                                            , ((PageInfo*)m_pageMdbAddr.m_addr)->m_itls[iItlNo - 1].m_sid
                                            , this
                                            , pUndoTabledesc
                                            , &rMdbAddr
                                            , iOperType);
            // add by chenm 2010-12-10 9:42:01
            if (iRet == 0) // ��Ч����
            {
                iRet = 3;	// �����Ѻ����Ч����
                // add by chenm 2012-09-29 ���insertʱ,����IndexHash.cpp��insert������goto���������������Դ���µ���������
                if (iOperType == DML_IDX_INSERT)
                {
                    // �����insert hashͰʱ������,������,���������ж�
                    m_pTransMgr->unregisterSid(m_pTransRes, pTransTable);
                    return iRet;
                }
                // over 2012-10-08 9:53:44
            }
            // over 2010-12-10 9:42:07
        }
    }
    else // ��lock
    {
        if (((PageInfo*)m_pageMdbAddr.m_addr)->m_desctype == '1')   // ��ű����ݵ�slot
        {
            // �жϸ�������
            if (m_pTransRes->m_pExpression != NULL)
            {
                if (m_pTransRes->m_pExpression->evaluate(rMdbAddr.m_addr + sizeof(UsedSlot), m_pTransRes->m_pRecConvert, (const void**)m_pTransRes->m_pCondValues) == false)
                {
                    m_pLatchMgr->slatch_unlock(rMdbAddr.m_pos);
                    return 2; // �����㸽������,�ͷ�slot latch ����
                }
            }
            if (iOperType == DML_UPDATE)  // �Ƿ���²���
            {
                try
                {
                    //2.2.1 ����ǰ������undo�ռ� 2.2.2 ��ǰ����ָ��
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
                //2.2.3 �ò������� update
                pUsedSlot->m_lockinfo &= 0xF7;
            }
            // 2.2.3 �ò�������delete
            else if (iOperType == DML_DELETE)
            {
                // ��¼ delete ʱҲ����ǰ���� -- 2012-08-30
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
        else // ����������ݵ�slot
        {
            // ����Ч��Ǽӵ� value ����, 0-��Ч, 1-��Ч --2012-08-30
            char record_value[pUndoTabledesc->m_valueSize]; // m_valueSize �Ѿ������˶����һ���ֽ�
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
        //2.2.4 ����slot
        pUsedSlot->lock(m_iItlNo);
        //2.2.5 slot���++
        pUsedSlot->addno();
        //2.2.6 ʱ�����0
        //pUsedSlot->m_scn = T_SCN();//2012/9/5 17:55:57 gaojf
        //2.2.7 �ͷ�slot latch
        m_pLatchMgr->slatch_unlock(rMdbAddr.m_pos);
        //3 ע��sid
        m_pTransMgr->registerSid(pTransTable, m_pTransRes);
        //4 ע����ס��slot
        m_pTransRes->m_lockedSlot.insert(TRANSRES_POOL::value_type(rMdbAddr, iOperType));
    }
    return iRet;
}

