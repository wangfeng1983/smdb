#include "TableSpaceMgr.h"

TableSpaceMgr::TableSpaceMgr(SpaceInfo& r_spaceInfo)
{
    SpaceMgrBase::initialize(r_spaceInfo);
}
TableSpaceMgr::~TableSpaceMgr()
{
}

/**
 *createTbSpace ������ռ�.
 *@return  true �����ɹ�,false ʧ��
 */
bool TableSpaceMgr::createTbSpace(const int& r_flag)
{
    if (createSpace(m_spaceHeader) == false)
    {
#ifdef _DEBUG_
        cout << "createSpace false!" << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    return true;
}
bool TableSpaceMgr::deleteTbSpace()
{
    if (deleteSpace(m_spaceHeader) == false)
    {
#ifdef _DEBUG_
        cout << "deleteSpace false!" << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    return true;
}


//��ʼ����ռ�
bool TableSpaceMgr::initTbSpace(const int& r_flag, const char* r_path)
{
    if (r_flag == 0)
    {
        return f_initTbSpace();
    }
    else
    {
        if (loadDataFromFile(r_path) == false)
        {
#ifdef _DEBUG_
            cout << "loadDataFromFile() false!" << __FILE__ << __LINE__ << endl;
#endif
            return false;
        }
        size_t  t_offSet = 0;
        //1. ������ͷ��Ϣm_spaceHeader
        t_offSet += sizeof(SpaceInfo);
        //2.��ʼ��ҳ�б���Ϣ
        return m_pagesmgr.initialize(this, m_spaceHeader.m_shmAddr, t_offSet,
                                     m_spaceHeader.m_size - t_offSet,
                                     m_spaceHeader.m_pageSize, 0);
    }
}
//��һ�γ�ʼ����ռ�
bool TableSpaceMgr::f_initTbSpace()
{
    size_t  t_offSet = 0;
    //1. ��ʼ����ͷ��Ϣm_spaceHeader
    memcpy(m_pSpHeader, &m_spaceHeader, sizeof(SpaceInfo));
    t_offSet += sizeof(SpaceInfo);
    return m_pagesmgr.initialize(this, m_spaceHeader.m_shmAddr, t_offSet,
                                 m_spaceHeader.m_size - t_offSet,
                                 m_spaceHeader.m_pageSize, 1);
}

//attach��ʽ��ʼ����ռ�
bool TableSpaceMgr::attach_init()
{
    if (SpaceMgrBase::attach() == false)
    {
        return false;
    }
    size_t  t_offSet = 0;
    //1. ������ͷ��Ϣm_spaceHeader
    t_offSet += sizeof(SpaceInfo);
    //2.��ʼ��ҳ�б���Ϣ
    return m_pagesmgr.initialize(this, m_spaceHeader.m_shmAddr, t_offSet,
                                 m_spaceHeader.m_size - t_offSet,
                                 m_spaceHeader.m_pageSize, 0);
}

//����ҳ��
bool TableSpaceMgr::allocatePage(MemManager* r_memMgr,
                                 PageInfo* &r_pPage, ShmPosition& r_pagePos,
                                 const bool& r_undoflag)
{
    bool t_bret = true;
    size_t  t_pagepos = 0;
    t_bret = m_pagesmgr.allocatePage(r_memMgr, r_pPage, t_pagepos, r_undoflag);
    r_pagePos.setValue(m_spaceHeader.m_spaceCode, t_pagepos);
    return t_bret;
}

//���ձ�Space�ڵ�page
bool TableSpaceMgr::freePage(MemManager* r_memMgr, const ShmPosition& r_pagePos, const bool& r_undoflag)
{
    return m_pagesmgr.freePage(r_memMgr, r_pagePos.getOffSet(), r_undoflag);
}
//ͨ��Slotλ����ȡ��Ӧҳ��λ��
bool TableSpaceMgr::getPagePosBySlotPos(const size_t& r_slotPos, size_t& r_pagePos)
{
    return m_pagesmgr.getPagePosBySlotPos(r_slotPos, r_pagePos);
}

//���Խӿ�
bool TableSpaceMgr::dumpSpaceInfo(ostream& r_os)
{
    r_os << "---------���ݱ�ռ�:" << m_pSpHeader->m_spaceName << "���� ��ʼ!---------" << endl;
    SpaceMgrBase::dumpSpaceInfo(r_os);
    m_pagesmgr.dumpPagesInfo(r_os);
    r_os << "---------���ݱ�ռ�:" << m_pSpHeader->m_spaceName << "���� ��ֹ!---------" << endl;
    return true;
}

float TableSpaceMgr::getTableSpaceUsedPercent()
{
    size_t t_totalsize, t_freesize;
    m_pagesmgr.getPagesUseInfo(t_totalsize, t_freesize);
    return 1.0 - (float)t_freesize / t_totalsize;
}
void TableSpaceMgr::afterStartMdb()
{
    m_pagesmgr.startmdb_init();
}
