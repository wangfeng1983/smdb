#include "PageInfo.h"
#include "SpaceInfo.h"
#include "MemManager.h"

//申请r_num个元素，r_slotAddrList初始化在外部，该处只push_back
//在外部已经判断有足够的元素提供分配。
void PageInfo::allocateNode(const int& r_num,
                            vector<MdbAddress>& r_slotAddrList,
                            int& r_newNum)
{
    char* t_pageAddr = (char*)this;
    static size_t  t_nullPos = 0;
    MdbAddress  t_addr;
    IdleSlot*   t_pSlot;
    r_newNum = 0;
    if (m_fIdleSlot == 0 || m_fIdleSlot > m_pageSize + m_pos)
    {
        m_fIdleSlot = 0;
        m_idleNum = 0;
        m_useflag = 0x02;
        return;
    }
    t_addr.m_pos.setSpaceCode(m_spaceCode);
    while (m_fIdleSlot != 0 && r_num > r_newNum)
    {
        t_addr.m_pos.setOffSet(m_fIdleSlot);
        t_addr.m_addr = t_pageAddr + (m_fIdleSlot - m_pos);
        t_pSlot = (IdleSlot*)(t_addr.m_addr);
        memcpy(&m_fIdleSlot, &(t_pSlot->m_next), sizeof(size_t));
        ++r_newNum;
        --m_idleNum;
        t_pSlot->init(); //表示该记录已经分配
        ((UsedSlot*)t_pSlot)->m_undopos = NULL_UNDOPOS; //add by gaojf 2010-8-6 14:40
        r_slotAddrList.push_back(t_addr);
    };
    if (m_idleNum == 0 || m_fIdleSlot == 0)
    {
        memcpy(&m_fIdleSlot, &t_nullPos, sizeof(size_t));
        memcpy(&m_lIdleSlot, &t_nullPos, sizeof(size_t));
        m_useflag = 0x02;
    }
    return;
}

//初始化
void PageInfo::initSlotList(const int& r_slotSize, const T_SCN& r_scn)
{
    char* t_pageAddr = (char*)this;
    m_slotSize = r_slotSize;
    m_slotNum  = (m_pageSize - sizeof(PageInfo)) / m_slotSize;
    m_idleNum  = m_slotNum;
    m_useflag  = 0x01;
    m_fIdleSlot = m_pos + sizeof(PageInfo);
    m_lIdleSlot = m_fIdleSlot + (m_slotNum - 1) * r_slotSize;
    IdleSlot* t_pSlot = (IdleSlot*)(t_pageAddr + sizeof(PageInfo));
    size_t    t_offSet = m_fIdleSlot;
    size_t    t_offSet2 = 0;
    for (size_t i = 0; i < m_slotNum; ++i)
    {
        t_pSlot->m_lockinfo = char(0); //0字符
        t_pSlot->m_scn      = r_scn;
        if (i == m_slotNum - 1)
        {
            t_offSet2 = 0;
            memcpy(&(t_pSlot->m_next), &t_offSet2, sizeof(size_t));
            break;
        }
        else
        {
            t_offSet2 = t_offSet + r_slotSize;
            memcpy(&(t_pSlot->m_next), &t_offSet2, sizeof(size_t));
            t_offSet = t_offSet2;
            t_pSlot = (IdleSlot*)(((char*)t_pSlot) + r_slotSize);
        }
    }
}
//释放Slot
void PageInfo::freeNode(const size_t& r_slotPos, const T_SCN& r_scn)
{
    char* t_pageAddr = (char*)this;
    IdleSlot* t_pSlot, *t_prevSlot;
    t_pSlot = (IdleSlot*)(t_pageAddr + r_slotPos - m_pos);
    t_pSlot->free(); //0字符,表示未使用
    static size_t  t_nullPos = 0;
    memcpy(&(t_pSlot->m_next), &t_nullPos, sizeof(size_t));
    if (m_lIdleSlot == 0)
    {
        //第一节点
        m_fIdleSlot = m_lIdleSlot = r_slotPos;
    }
    else
    {
        t_prevSlot = (IdleSlot*)(t_pageAddr + m_lIdleSlot - m_pos);
        t_prevSlot->m_next = r_slotPos;
        m_lIdleSlot = r_slotPos;
    }
    ++m_idleNum;
    t_pSlot->m_scn = r_scn;
    m_scn_slot     = r_scn;
    m_useflag = 0x01;
}
void PageInfo::init(const unsigned int r_spacecode, const size_t r_pagepos,
                    const int r_parlpos, const size_t r_pagesize)
{
    memset(this, 0, sizeof(PageInfo));
    m_spaceCode = r_spacecode;
    m_pos       = r_pagepos;
    m_parlpos   = r_parlpos;
    m_pageSize  = r_pagesize;
}
//写影子信息,包括更新时间戳和影子位置信息
void PageInfo::writeUndoInfo(MemManager* r_memMgr) throw(Mdb_Exception)
{
    ShmPosition t_doffset(m_spaceCode, m_pos);
    UndoMemMgr* t_undomgr = r_memMgr->getUndoMemMgr();
    char*       t_value = new char[_UNDO_TABLE_VLEN[T_UNDO_TYPE_PAGEINFO] + 1];
    try
    {
        getundovalue(t_value);
        m_undopos = t_undomgr->insert(T_UNDO_TYPE_PAGEINFO, MDB_NULL_NAME, t_doffset, m_scn_page, t_value, m_undopos);
    }
    catch (Mdb_Exception& e)
    {
        delete [] t_value;
        throw e;
    }
    delete [] t_value;
}
void PageInfo::getundovalue(char* r_undovalue)
{
    int t_offset = 0;
    r_undovalue[t_offset] = m_useflag;
    ++t_offset;
    r_undovalue[t_offset] = m_desctype;
    ++t_offset;
    memcpy(r_undovalue + t_offset, &m_descpos, sizeof(size_t));
    t_offset += sizeof(size_t);
    memcpy(r_undovalue + t_offset, &m_next, sizeof(ShmPosition));
    t_offset += sizeof(ShmPosition);
    memcpy(r_undovalue + t_offset, &m_nIdlePage, sizeof(ShmPosition));
    t_offset += sizeof(ShmPosition);
}
void PageInfo::setundovalue(const char* r_undovalue)
{
    int t_offset = 0;
    m_useflag = r_undovalue[t_offset];
    ++t_offset;
    m_desctype = r_undovalue[t_offset];
    ++t_offset;
    memcpy(&m_descpos, r_undovalue + t_offset, sizeof(size_t));
    t_offset += sizeof(size_t);
    memcpy(&m_next, r_undovalue + t_offset, sizeof(ShmPosition));
    t_offset += sizeof(ShmPosition);
    memcpy(&m_nIdlePage, r_undovalue + t_offset, sizeof(ShmPosition));
    t_offset += sizeof(ShmPosition);
}


void* PageInfo::getFirstSlot(size_t& r_offset)
{
    r_offset = m_pos + sizeof(PageInfo);
    return (void*)(((char*)this) + sizeof(PageInfo));
}
void PageInfo::lock()
{
    //1. 锁定描述符
    while (_check_lock((atomic_p)&m_lacthinfo, 0, 1) == true)
    {
        //未锁定
        //1.1 判断是否超时
        time_t t_nowtime;
        time(&t_nowtime);
        if (difftime(t_nowtime, m_locktime) > 2) //超时判断
        {
            //强制解锁
            _clear_lock((atomic_p)&m_lacthinfo, 0);
        }
    }
    time(&m_locktime);
}
void PageInfo::unlock()
{
    _clear_lock((atomic_p)&m_lacthinfo, 0);
}
//add by gaojf 2010-5-17 8:34
//申请一个页面  true OK，false 未申请到
bool  PageListInfo::new_page(MemManager* r_memMgr,
                             char*      r_spaceaddr,
                             const unsigned int r_spacecode,
                             size_t&    r_pagepos,
                             PageInfo* &r_ppage,
                             const bool& r_undoflag)
{
    T_SCN t_scn;
    if (m_idleNum <= 0 || m_fIdlePage == 0)
    {
        m_idleNum = 0;
        m_lIdlePage = 0; //add by gaojf 2012/7/5 9:20:20
        return false;
    }
    //从头部取一个页面
    r_ppage   = (PageInfo*)(r_spaceaddr + m_fIdlePage);
    if (r_undoflag == true)
    {
        r_memMgr->getscn(t_scn);
        writeUndoInfo(r_spacecode, r_memMgr);
        m_scn = t_scn;
    }
    r_pagepos   = m_fIdlePage;
    m_fIdlePage = r_ppage->m_next.getOffSet();
    //if (m_fIdlePage == 0) m_lIdlePage = 0; //最后一段
    if (m_fIdlePage == 0) //最后一段  add by gaojf 2012/7/5 9:20:43
    {
        m_lIdlePage = 0;
        m_idleNum = 0;
    }
    else
    {
        --m_idleNum;
    }
    return true;
}
//释放一个页面  true OK，false 失败
//在外部将页面的m_next指向该列表的首页面
bool PageListInfo::free_page(MemManager*      r_memMgr,
                             char*      r_spaceaddr,
                             const unsigned int  r_spacecode,
                             const size_t& r_pagepos,
                             const bool& r_undoflag)
{
    if (r_undoflag == true)
    {
        T_SCN t_scn;
        r_memMgr->getscn(t_scn);
        writeUndoInfo(r_spacecode, r_memMgr);
        m_scn = t_scn;
    }
    //2.将页面插入空闲队列
    if (m_fIdlePage == 0)
    {
        //第一个空闲页面
        m_fIdlePage = m_lIdlePage = r_pagepos;
    }
    else
    {
        //追加在空闲段最前面
        m_fIdlePage = r_pagepos;
    }
    ++m_idleNum;
    return true;
}
void  PageListInfo::init(const size_t r_pos, const size_t r_pagesize)
{
    memset(this, 0, sizeof(PageListInfo));
    m_pos      = r_pos;
    m_pageSize = r_pagesize;
}
void  PageListInfo::addpage(char* r_spaceaddr, const unsigned int& r_spaceCode,
                            const size_t r_pos)
{
    //加在最后面
    PageInfo* t_ppage = (PageInfo*)(r_spaceaddr + r_pos);
    t_ppage->m_next = NULL_SHMPOS;
    if (m_fIdlePage == 0)
    {
        m_fIdlePage = m_lIdlePage = r_pos;
    }
    else
    {
        t_ppage = (PageInfo*)(r_spaceaddr + m_lIdlePage);
        t_ppage->m_next.setValue(r_spaceCode, r_pos);
        m_lIdlePage = r_pos;
    }
    ++m_pageNum;
    ++m_idleNum;
}
//写影子信息,包括更新时间戳和影子位置信息
void PageListInfo::writeUndoInfo(const unsigned int r_spacecode,
                                 MemManager* r_memMgr) throw(Mdb_Exception)
{
    UndoMemMgr* t_pundomgr = r_memMgr->getUndoMemMgr();
    ShmPosition t_doffset(r_spacecode, m_pos);
    char*        t_value = new char[_UNDO_TABLE_VLEN[T_UNDO_TYPE_SPACEINFO] + 1];
    try
    {
        getundovalue(t_value);
        m_undopos = t_pundomgr->insert(T_UNDO_TYPE_SPACEINFO, MDB_NULL_NAME,
                                       t_doffset, m_scn , t_value, m_undopos);
    }
    catch (Mdb_Exception& e)
    {
        delete [] t_value;
        throw e;
    }
    delete [] t_value;
}
void PageListInfo::getundovalue(char* r_undovalue)
{
    memcpy(r_undovalue, &m_fIdlePage, sizeof(size_t));
    memcpy(r_undovalue + sizeof(size_t), &m_lIdlePage, sizeof(size_t));
}
void PageListInfo::setundovalue(const char* r_undovalue)
{
    memcpy(&m_fIdlePage, r_undovalue, sizeof(size_t));
    memcpy(&m_lIdlePage, r_undovalue + sizeof(size_t), sizeof(size_t));
}



