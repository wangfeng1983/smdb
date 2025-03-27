#include "DataPagesMgr.h"
#include "SpaceMgrBase.h"
#include "MemManager.h"

DataPagesMgr::DataPagesMgr()
{
}
DataPagesMgr::~DataPagesMgr()
{
}

//r_spacemgr  ��Ӧ��ռ�������
//r_spaceaddr ��ռ���ʼ��ַ
//r_offset    pages����ʼƫ����
//r_segsize   �ε��ܴ�С  (�������б�)
//r_pagesize  ÿ��PAGE��С
//r_flag  1 ��һ�γ�ʼ������Ҫ��ʼ�����б���Ϣ
bool DataPagesMgr::initialize(SpaceMgrBase* r_spacemgr,
                              char*         r_spaceaddr,
                              const size_t& r_offset,
                              const size_t& r_segsize,
                              const size_t& r_pagesize,
                              const int     r_flag)

{
    m_spacemgr = r_spacemgr;
    m_spaceaddr = r_spaceaddr;
    m_pagesize = r_pagesize;
    m_spaceCode = r_spacemgr->getSpaceCode();
    size_t  t_offSet = r_offset;
    size_t i = 0;
    for (i = 0; i < MDB_PARL_NUM; ++i)
    {
        m_pagelist[i] = (PageListInfo*)(m_spaceaddr + t_offSet);
        if (r_flag == 1) m_pagelist[i]->init(t_offSet, m_pagesize);
        t_offSet += sizeof(PageListInfo);
    }
    m_firstpagepos = t_offSet;
    m_segsize      = r_segsize - (t_offSet - r_offset);
    if (r_flag == 1)
    {
        size_t t_pageNum = m_segsize / m_pagesize;
        PageInfo* t_page = NULL;
        for (i = 0; i < t_pageNum; ++i)
        {
            t_page = (PageInfo*)(m_spaceaddr + t_offSet);
            t_page->init(m_spaceCode, t_offSet, i % MDB_PARL_NUM, m_pagesize);
            m_pagelist[i % MDB_PARL_NUM]->addpage(m_spaceaddr, m_spaceCode, t_offSet);
            t_offSet += m_pagesize;
        }
    }
    return true;
}

//����ҳ��
bool DataPagesMgr::allocatePage(MemManager* r_memMgr, PageInfo* &r_pPage,
                                size_t& r_pagePos, const bool& r_undoflag)
{
    bool t_bret = true, t_flag = false, t_flag2 = false;
    size_t t_pagepos = 0;
    int  i = 0;
    try
    {
        do
        {
            t_flag2 = false;
            //1. ѡ���б�
            for (i = 0; i < MDB_PARL_NUM; ++i)
            {
                if (m_pagelist[i]->m_idleNum <= 0) continue;
                if (m_spacemgr->trylock(i) == true)
                {
                    if (m_pagelist[i]->m_idleNum <= 0) //���б�������
                    {
                        m_spacemgr->unlock(i);
                        continue;
                    }
                    else
                    {
                        t_flag = true; //ѡ�����б�
                        break;
                    }
                }
                else
                {
                    t_flag2 = true;//�н���������ס
                    continue;
                }
            }
            if (t_flag == true) break; //ѡ�����б�
        }
        while (t_flag2 == true);
        if (i == MDB_PARL_NUM) throw Mdb_Exception(__FILE__, __LINE__, "��ռ�����!");
    }
    catch (Mdb_Exception& e)
    {
#ifdef _DEBUG_
        //cout<<e.GetString()<<" "<<__FILE__<<__LINE__<<endl;
#endif
        return false;
    }
    try
    {
        t_bret = m_pagelist[i]->new_page(r_memMgr, m_spaceaddr, m_spaceCode, r_pagePos, r_pPage, r_undoflag);
    }
    catch (Mdb_Exception& e)
    {
#ifdef _DEBUG_
        cout << e.GetString() << endl;
#endif
        m_spacemgr->unlock(i);
        throw e;
    }
    m_spacemgr->unlock(i);
    return t_bret;
}

//���ձ�Space�ڵ�page
bool DataPagesMgr::freePage(MemManager* r_memMgr, const size_t& r_pagePos, const bool& r_undoflag)
{
    bool t_bret = true;
    PageInfo* t_pPageInfo = (PageInfo*)(m_spaceaddr + r_pagePos);
    try
    {
        m_spacemgr->lock(t_pPageInfo->m_parlpos);
    }
    catch (Mdb_Exception& e)
    {
#ifdef _DEBUG_
        cout << e.GetString() << endl;
#endif
        return false;
    }
    if (r_undoflag == true)
    {
        t_pPageInfo->lock(); //add by gaojf 2012/7/5 10:22:44
        T_SCN t_scn;
        try
        {
            t_pPageInfo->writeUndoInfo(r_memMgr);
        }catch(Mdb_Exception& e)
        { //����쳣���ʱ�Ľ��� 2012/10/17 14:07:03
            t_pPageInfo->unlock();
            m_spacemgr->unlock(t_pPageInfo->m_parlpos);
            throw e;
        }
        r_memMgr->getscn(t_scn);
        //t_pPageInfo->lock(); //modified by gaojf 2012/7/5 10:22:27
        t_pPageInfo->m_useflag = 0x00;
        if (m_pagelist[t_pPageInfo->m_parlpos]->m_fIdlePage == 0)
        {
            t_pPageInfo->m_next.setValue(0, m_pagelist[t_pPageInfo->m_parlpos]->m_fIdlePage);
        }
        else
        {
            t_pPageInfo->m_next.setValue(m_spaceCode, m_pagelist[t_pPageInfo->m_parlpos]->m_fIdlePage);
        }
        t_pPageInfo->m_scn_page = t_scn;
        t_pPageInfo->unlock();
        t_bret = m_pagelist[t_pPageInfo->m_parlpos]->free_page(r_memMgr, m_spaceaddr,
                 m_spaceCode, r_pagePos, r_undoflag);
    }
    else
    {
        t_pPageInfo->m_useflag = 0x00;
        if (m_pagelist[t_pPageInfo->m_parlpos]->m_fIdlePage == 0)
        {
            t_pPageInfo->m_next.setValue(0, m_pagelist[t_pPageInfo->m_parlpos]->m_fIdlePage);
        }
        else
        {
            t_pPageInfo->m_next.setValue(m_spaceCode, m_pagelist[t_pPageInfo->m_parlpos]->m_fIdlePage);
        }
        t_bret = m_pagelist[t_pPageInfo->m_parlpos]->free_page(r_memMgr, m_spaceaddr,
                 m_spaceCode, r_pagePos, r_undoflag);
    }
    m_spacemgr->unlock(t_pPageInfo->m_parlpos);
    return t_bret;
}
//ͨ��Slotλ����ȡ��Ӧҳ��λ��
bool DataPagesMgr::getPagePosBySlotPos(const size_t& r_slotPos, size_t& r_pagePos)
{
    if (r_slotPos > (m_firstpagepos + m_segsize) || r_slotPos < m_firstpagepos)
    {
        return false;
    }
    //�����Slot���ڵڼ���page(0.1.2....)
    size_t t_num = (r_slotPos - m_firstpagepos) / m_pagesize;
    r_pagePos = m_firstpagepos + t_num * m_pagesize;
    return true;
}

//���Խӿ�
bool DataPagesMgr::dumpPagesInfo(ostream& r_os)
{
    PageInfo*  t_pPage;
    r_os << "m_firstpagepos=" << m_firstpagepos << endl;
    r_os << "m_segsize     =" << m_segsize << endl;
    for (int i = 0; i < MDB_PARL_NUM; ++i)
    {
        r_os << "-------ҳ�б���Ϣ��ʼ---------------" << endl;
        r_os << "m_pos      =" << m_pagelist[i]->m_pos << endl;
        r_os << "m_pageSize =" << m_pagelist[i]->m_pageSize << endl;
        r_os << "m_pageNum  =" << m_pagelist[i]->m_pageNum << endl;
        r_os << "m_idleNum  =" << m_pagelist[i]->m_idleNum << endl;
        r_os << "m_fIdlePage=" << m_pagelist[i]->m_fIdlePage << endl;
        r_os << "m_scn      =" << m_pagelist[i]->m_scn << endl;
        r_os << "m_undopos  =" << m_pagelist[i]->m_undopos << endl;
        r_os << "-------ҳ�б���Ϣ��ֹ---------------" << endl;
        r_os << "-------����ҳ����Ϣ-----------------" << endl;
        size_t    t_idlePage = 0;
        if (m_pagelist[i]->m_fIdlePage != 0)
        {
            t_pPage = (PageInfo*)(m_spaceaddr + m_pagelist[i]->m_fIdlePage);
            while (1)
            {
                t_idlePage++;
                r_os << "m_spaceCode      =" << t_pPage->m_spaceCode << endl;
                r_os << "m_pos            =" << t_pPage->m_pos << endl;
                r_os << "m_next           =" << t_pPage->m_next << endl;
                r_os << "m_nIdlePage      =" << t_pPage->m_nIdlePage << endl;
                r_os << "m_pageSize       =" << t_pPage->m_pageSize << endl;
                if (t_pPage->m_next == NULL_SHMPOS) break;
                t_pPage = (PageInfo*)(m_spaceaddr + t_pPage->m_next.getOffSet());
            };
            if (m_pagelist[i]->m_idleNum != t_idlePage)
            {
                r_os << "����ҳ�����Ϳ���ҳ���б���!" << endl;
                r_os << "m_pagelist[i]->m_idleNum = " << m_pagelist[i]->m_idleNum << endl;
                r_os << "t_idlePage = " << t_idlePage << endl;
            }
        }
    }
    r_os << "-------����ҳ����Ϣ��ʼ-------------" << endl;
    char* t_pPage0 = m_spaceaddr + m_firstpagepos;
    size_t t_offset = m_firstpagepos;
    for (size_t t_pagepos = 0; ; ++t_pagepos)
    {
        t_offset = m_firstpagepos + t_pagepos * m_pagesize;
        if (t_offset > (m_segsize + m_firstpagepos)) break;
        t_pPage = (PageInfo*)(t_pPage0 + t_pagepos * m_pagesize);
        r_os << "m_useflag        =" << char('0' + t_pPage->m_useflag) << endl;
        r_os << "m_spaceCode      =" << t_pPage->m_spaceCode << endl;
        r_os << "m_parlpos        =" << t_pPage->m_parlpos << endl;
        r_os << "m_pos            =" << t_pPage->m_pos << endl;
        r_os << "m_next           =" << t_pPage->m_next << endl;
        r_os << "m_nIdlePage      =" << t_pPage->m_nIdlePage << endl;
        r_os << "m_pageSize       =" << t_pPage->m_pageSize << endl;
        r_os << "m_scn_page       =" << t_pPage->m_scn_page << endl;
        r_os << "m_scn_slot       =" << t_pPage->m_scn_slot << endl;
        r_os << "m_slotSize       =" << t_pPage->m_slotSize << endl;
        r_os << "m_slotNum        =" << t_pPage->m_slotNum << endl;
        r_os << "m_idleNum        =" << t_pPage->m_idleNum << endl;
        r_os << "m_undopos        =" << t_pPage->m_undopos << endl;
    }
    r_os << "-------����ҳ�������ʼ-------------" << endl;
    return true;
}

bool DataPagesMgr::getPagesUseInfo(size_t& r_totalsize, size_t& r_freesize)
{
    r_totalsize = m_segsize;
    r_freesize = 0;
    for (int i = 0; i < MDB_PARL_NUM; ++i)
    {
        r_freesize  += m_pagelist[i]->m_idleNum * m_pagesize;
    }
    return true;
}
void DataPagesMgr::startmdb_init()
{
    PageInfo*  t_pPage;
    for (int i = 0; i < MDB_PARL_NUM; ++i)
    {
        m_pagelist[i]->m_scn.setOffSet(0);
        m_pagelist[i]->m_undopos.setOffSet(0);
    }
    char* t_pPage0 = m_spaceaddr + m_firstpagepos;
    size_t t_offset = m_firstpagepos;
    size_t t_slotPos;
    UsedSlot* t_pUsedSlot = NULL;
    IdleSlot* t_pIdleSlot_pre = NULL; //add by gaojf 2012/7/5 13:09:56
    IdleSlot* t_pIdleSlot = NULL; //add by gaojf 2012/7/5 13:09:56
    for (size_t t_pagepos = 0; ; ++t_pagepos)
    {
        t_offset = m_firstpagepos + t_pagepos * m_pagesize;
        if (t_offset > (m_segsize + m_firstpagepos)) break;
        t_pPage = (PageInfo*)(t_pPage0 + t_pagepos * m_pagesize);
        t_pPage->m_scn_page.setOffSet(0);
        t_pPage->m_scn_slot.setOffSet(0);
        t_pPage->m_undopos.setOffSet(0);
        if (t_pPage->m_useflag != 0x00) //��ʹ��
        {
            for (int t_l = 0; t_l < PAGE_ITLS_NUM; ++t_l)
            {
                //��PAGE����Ϣ
                memset(&(t_pPage->m_itls[t_l]), 0, sizeof(Pageitl));
            }
            //��Ĭ��û���� modified by gaojf 2012/7/5 13:22:46 begin
            t_pPage->m_idleNum   = 0;
            t_pPage->m_useflag   = 0x02;
            t_pPage->m_fIdleSlot = 0;
            t_pPage->m_lIdleSlot = 0;
            t_pIdleSlot_pre = t_pIdleSlot = NULL;
            //��Ĭ��û���� modified by gaojf 2012/7/5 13:22:46 end
            //������ҳ����SLOT
            t_pUsedSlot = (UsedSlot*)(t_pPage->getFirstSlot(t_slotPos));
            for (size_t t_slotNum = 0; t_slotNum < t_pPage->m_slotNum; ++t_slotNum)
            {
                if (!(t_pUsedSlot->isUnUsed()))
                {
                    //���SLOT��SCN,UNDOָ��,��λ��Ϣ
                    t_pUsedSlot->clearITL();
                    t_pUsedSlot->clearSCN();
                    t_pUsedSlot->clearUndoPos();
                }
                else //IdleSlot���´�һ�� add by gaojf 2012/7/5 13:02:06 begin
                {
                    t_pIdleSlot = (IdleSlot*)t_pUsedSlot;
                    ++(t_pPage->m_idleNum);
                    t_pIdleSlot->free();
                    t_pIdleSlot->m_scn = T_SCN();
                    t_pIdleSlot->m_next = 0;
                    if (t_pPage->m_lIdleSlot == 0)
                    {
                        t_pPage->m_fIdleSlot = t_slotPos;
                        t_pPage->m_lIdleSlot = t_slotPos;
                    }
                    else  //���ýڵ�������һ��idleslot��
                    {
                        t_pIdleSlot_pre->m_next = t_slotPos;
                        t_pPage->m_lIdleSlot = t_slotPos;
                    }
                    t_pIdleSlot_pre = t_pIdleSlot;
                    t_pPage->m_useflag = 0x01; //δ��
                }//IdleSlot���´�һ�� add by gaojf 2012/7/5 13:02:06 end
                t_pUsedSlot = (UsedSlot*)(((char*)t_pUsedSlot) + t_pPage->m_slotSize);
                t_slotPos += t_pPage->m_slotSize;//add by gaojf 2012/7/5 13:09:22
            }
        }
    }
    //���¼���PageListInfo.m_idleNum 2012/8/23 10:19:31 begin
    //��ֹ��ʧ�����page
    size_t t_idlepageNum=0;
    size_t t_idlepagepos=0;
    for (int j = 0; j < MDB_PARL_NUM; ++j)
    {
    	t_idlepageNum=0;
    	t_idlepagepos=m_pagelist[j]->m_fIdlePage;
    	while(t_idlepagepos!=0)
    	{
    		++t_idlepageNum;
    		t_pPage = (PageInfo*)(m_spaceaddr + t_idlepagepos);
    		t_idlepagepos=t_pPage->m_next.getOffSet();
    	};
    	m_pagelist[j]->m_idleNum=t_idlepageNum;
    }
    ///////////////////////////////////////////////////////
}
