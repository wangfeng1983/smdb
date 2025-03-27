#include "TbMemInfo.h"
#include "PageInfo.h"

void TbMemInfo::clear()
{
    m_tbPagesInfo.clear();
    m_notFullPages.clear();
    m_slotSize = 0;
    m_pageNum = 0;
    m_totalSlotNum = 0;
    m_idleSlotNum = 0;
    m_usedSlotNum = 0;
}
void TbMemInfo::setPageInfo(const PageInfo* r_pPage)
{
    TbPageInfo t_pageInfo;
    t_pageInfo.m_pagePos.setSpaceCode(r_pPage->m_spaceCode);
    t_pageInfo.m_pagePos.setOffSet(r_pPage->m_pos);
    t_pageInfo.m_pageSize = r_pPage->m_pageSize;
    t_pageInfo.m_slotNum = r_pPage->m_slotNum;
    t_pageInfo.m_idleNum = r_pPage->m_idleNum;
    m_tbPagesInfo.push_back(t_pageInfo);
    m_pageNum++;
    m_totalSlotNum += r_pPage->m_slotNum;
    m_idleSlotNum += r_pPage->m_idleNum;
    m_usedSlotNum += r_pPage->m_slotNum - r_pPage->m_idleNum;
}

bool TbMemInfo::dumpInfo(ostream& r_os)
{
    r_os << "-------------表或索引内存信息--------------" << endl;
    r_os << "  表名        :" << m_tbName << endl;
    r_os << "  表类型      :" << m_tbType << endl;
    r_os << "  slotSize    :" << m_slotSize << endl;
    r_os << "  总页数      :" << m_pageNum << endl;
    r_os << "  总节点数    :" << m_totalSlotNum << endl;
    r_os << "  空闲节点数  :" << m_idleSlotNum << endl;
    r_os << "  已使用节点数:" << m_usedSlotNum << endl;
    r_os << "  -----详细页面信息--------------------" << endl;
    for (vector<TbPageInfo>::iterator r_itr = m_tbPagesInfo.begin();
            r_itr != m_tbPagesInfo.end(); r_itr++)
    {
        r_os << "    页面位置:" << r_itr->m_pagePos << endl;
        r_os << "    页面大小:" << r_itr->m_pageSize << endl;
        r_os << "    总节点数:" << r_itr->m_slotNum << endl;
        r_os << "    空闲节点数:" << r_itr->m_idleNum << endl;
        r_os << "    已使用节点数:" << r_itr->m_slotNum - r_itr->m_idleNum << endl;
        r_os << "  -------------------------------------" << endl;
    }
    r_os << "  -----未满页面信息--------------------" << endl;
    for (vector<ShmPosition>::iterator r_nFItr = m_notFullPages.begin();
            r_nFItr != m_notFullPages.end(); r_nFItr++)
    {
        r_os << *r_nFItr << "  ";
        if (r_nFItr == (m_notFullPages.end() - 1)) r_os << endl;
    }
    r_os << "-------------------------------------------" << endl;
    return true;
}

