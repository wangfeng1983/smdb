#include "TableSpaceMgr.h"

TableSpaceMgr::TableSpaceMgr(SpaceInfo& r_spaceInfo)
{
    SpaceMgrBase::initialize(r_spaceInfo);
}
TableSpaceMgr::~TableSpaceMgr()
{
}

/**
 *createTbSpace 创建表空间.
 *@return  true 创建成功,false 失败
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


//初始化表空间
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
        //1. 跳过块头信息m_spaceHeader
        t_offSet += sizeof(SpaceInfo);
        //2.初始化页列表信息
        return m_pagesmgr.initialize(this, m_spaceHeader.m_shmAddr, t_offSet,
                                     m_spaceHeader.m_size - t_offSet,
                                     m_spaceHeader.m_pageSize, 0);
    }
}
//第一次初始化表空间
bool TableSpaceMgr::f_initTbSpace()
{
    size_t  t_offSet = 0;
    //1. 初始化块头信息m_spaceHeader
    memcpy(m_pSpHeader, &m_spaceHeader, sizeof(SpaceInfo));
    t_offSet += sizeof(SpaceInfo);
    return m_pagesmgr.initialize(this, m_spaceHeader.m_shmAddr, t_offSet,
                                 m_spaceHeader.m_size - t_offSet,
                                 m_spaceHeader.m_pageSize, 1);
}

//attach方式初始化表空间
bool TableSpaceMgr::attach_init()
{
    if (SpaceMgrBase::attach() == false)
    {
        return false;
    }
    size_t  t_offSet = 0;
    //1. 跳过块头信息m_spaceHeader
    t_offSet += sizeof(SpaceInfo);
    //2.初始化页列表信息
    return m_pagesmgr.initialize(this, m_spaceHeader.m_shmAddr, t_offSet,
                                 m_spaceHeader.m_size - t_offSet,
                                 m_spaceHeader.m_pageSize, 0);
}

//申请页面
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

//回收本Space内的page
bool TableSpaceMgr::freePage(MemManager* r_memMgr, const ShmPosition& r_pagePos, const bool& r_undoflag)
{
    return m_pagesmgr.freePage(r_memMgr, r_pagePos.getOffSet(), r_undoflag);
}
//通过Slot位置求取对应页面位置
bool TableSpaceMgr::getPagePosBySlotPos(const size_t& r_slotPos, size_t& r_pagePos)
{
    return m_pagesmgr.getPagePosBySlotPos(r_slotPos, r_pagePos);
}

//调试接口
bool TableSpaceMgr::dumpSpaceInfo(ostream& r_os)
{
    r_os << "---------数据表空间:" << m_pSpHeader->m_spaceName << "内容 起始!---------" << endl;
    SpaceMgrBase::dumpSpaceInfo(r_os);
    m_pagesmgr.dumpPagesInfo(r_os);
    r_os << "---------数据表空间:" << m_pSpHeader->m_spaceName << "内容 终止!---------" << endl;
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
