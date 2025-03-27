#include "TableDescript.h"
#include "PageInfo.h"
#include "MemManager.h"
#include "UndoMemMgr.h"

void  DescPageInfo::initPos()
{
    m_firstPage = NULL_SHMPOS;
    m_lastPage  = NULL_SHMPOS;
    m_fIdlePage = NULL_SHMPOS;
    m_lIdlePage = NULL_SHMPOS;
    m_lockinfo  = 0;
    m_idleSlots = 0;
}
void  DescPageInfo::initPos(MemManager* r_memMgr, const bool& r_undoflag)
{
    if (r_undoflag == true)
    {
        writeUndoInfo(r_memMgr);
        r_memMgr->getscn(m_scn);
    }
    initPos();
}

ostream& operator<<(ostream& r_os, const DescPageInfo& r_obj)
{
    r_os << "  �ڵ�Ĵ�С    (m_pageInfo.m_slotSize  ) = " << r_obj.m_slotSize << endl;
    r_os << "  ��һҳ        (m_pageInfo.m_firstPage ) = " << r_obj.m_firstPage << endl;
    r_os << "  ���һҳ      (m_pageInfo.m_lastPage  ) = " << r_obj.m_lastPage << endl;
    r_os << "  ��һ��δ��ҳ  (m_pageInfo.m_fIdlePage ) = " << r_obj.m_fIdlePage << endl;
    r_os << "  ���һ��δ��ҳ(m_pageInfo.m_lIdlePage ) = " << r_obj.m_lIdlePage << endl;
    r_os << "  ʱ���        (m_scn                  ) = " << r_obj.m_scn      << endl;
    r_os << "  Ӱ���ڴ�      (m_undopos              ) = " << r_obj.m_undopos  << endl;
    r_os << "  ����Ϣ        (m_lockinfo             ) = " << r_obj.m_lockinfo << endl;
    r_os << "  ���м�¼��    (m_idleSlots            ) = " << r_obj.m_idleSlots << endl;
    return r_os;
}

//����һ���µ�PAGE
void  DescPageInfo::addNewPage(MemManager*        r_memMgr,
                               const int&         r_descparlpos,
                               const ShmPosition& r_pagepos,
                               PageInfo*    r_page,
                               const bool&        r_undoflag)
{
    T_SCN t_scn;
    //дӰ�Ӽ�¼
    if (r_undoflag == true)
    {
        r_memMgr->getscn(t_scn);
        writeUndoInfo(r_memMgr);
        m_scn = t_scn;
        r_page->lock(); //add by gaojf 2012/7/5 9:33:11
        r_page->writeUndoInfo(r_memMgr);
        //r_page->lock(); //delete by gaojf 2012/7/5 9:33:17
        r_page->m_scn_page = t_scn;
        r_page->m_scn_slot = t_scn;
    }
    for (int t_l = 0; t_l < PAGE_ITLS_NUM; ++t_l)
    {
        //��PAGE����Ϣ
        memset(&(r_page->m_itls[t_l]), 0, sizeof(Pageitl));
    }
    r_page->m_descpos  = m_pos;
    r_page->m_descparlpos = r_descparlpos;
    r_page->m_desctype = m_desctype;
    r_page->m_useflag  = 0x01;
    r_page->initSlotList(m_slotSize, t_scn);
    r_page->m_next     = NULL_SHMPOS;
    r_page->m_nIdlePage = m_fIdlePage;
    if (r_undoflag == true) r_page->unlock();
    //1. �Ƚ���PAGE�����������(����ͷ��)
    if (m_lIdlePage == NULL_SHMPOS)
    {
        m_lIdlePage = r_pagepos;
    }
    m_fIdlePage = r_pagepos;
    //2. ����PAGE��������ҳ�����(����β������ֹselect����ʹ��)
    if (m_firstPage == NULL_SHMPOS)
    {
        m_firstPage = r_pagepos;
    }
    else
    {
        PageInfo* t_plastpage = NULL;
        if (r_memMgr->getPageInfo(m_lastPage, t_plastpage) == false)
        {
            throw Mdb_Exception(__FILE__, __LINE__, "����ҳ��λ��ȡҳ��ʧ��!");
        }
        if (r_undoflag == true)
        {
            t_plastpage->lock();  //��ֹ��CHKPT��д��ͻ
            t_plastpage->writeUndoInfo(r_memMgr);
            t_plastpage->m_next = r_pagepos;
            t_plastpage->m_scn_page = t_scn;
            t_plastpage->unlock();
        }
        else t_plastpage->m_next = r_pagepos;
    }
    m_lastPage = r_pagepos;
    m_idleSlots += r_page->m_idleNum;
}
//��PAGE����δ�������� (PAGE���Ƕ�����Ԫ��)
void DescPageInfo::addIdlePage(MemManager* r_memMgr, const ShmPosition& r_pagepos,
                               PageInfo* r_ppage, const bool& r_undoflag)
{
    T_SCN     t_scn;
    if (r_undoflag == true)
    {
        writeUndoInfo(r_memMgr);
        r_memMgr->getscn(t_scn);
        m_scn = t_scn;
    }
    //1. ����PAGE����ǿ��ж���
    m_fIdlePage = r_pagepos;
    if (m_lIdlePage == NULL_SHMPOS)
    {
        m_lIdlePage = r_pagepos;
    }
}

void DescPageInfo::writeUndoInfo(MemManager* r_memMgr)throw(Mdb_Exception)
{
    UndoMemMgr* t_undomgr = r_memMgr->getUndoMemMgr();
    char*       t_value = new char[_UNDO_TABLE_VLEN[T_UNDO_TYPE_DESC] + 1];
    try
    {
        getundovalue(t_value);
        ShmPosition t_shmpos(0, m_pos);
        m_undopos = t_undomgr->insert(T_UNDO_TYPE_DESC, MDB_NULL_NAME, t_shmpos, m_scn, t_value, m_undopos);
    }
    catch (Mdb_Exception& e)
    {
        delete [] t_value;
        throw e;
    }
    delete [] t_value;
}
void DescPageInfo::clearUndoInfo(MemManager* r_memMgr, const T_SCN& r_scn) throw(Mdb_Exception)
{
    if (m_undopos == NULL_UNDOPOS) return;
    UndoMemMgr* t_undomgr = r_memMgr->getUndoMemMgr();
    t_undomgr->free(T_UNDO_TYPE_DESC, MDB_NULL_NAME, r_scn, 0, m_undopos);
}
void DescPageInfo::getundovalue(char* r_undovalue)
{
    int i = 0;
    memcpy(r_undovalue + i * sizeof(ShmPosition), &m_firstPage, sizeof(ShmPosition));
    ++i;
    memcpy(r_undovalue + i * sizeof(ShmPosition), &m_lastPage , sizeof(ShmPosition));
    ++i;
    memcpy(r_undovalue + i * sizeof(ShmPosition), &m_fIdlePage, sizeof(ShmPosition));
    ++i;
    memcpy(r_undovalue + i * sizeof(ShmPosition), &m_lIdlePage, sizeof(ShmPosition));
    ++i;
}
void DescPageInfo::setundovalue(const char* r_undovalue)
{
    int i = 0;
    memcpy(&m_firstPage, r_undovalue + i * sizeof(ShmPosition), sizeof(ShmPosition));
    ++i;
    memcpy(&m_lastPage , r_undovalue + i * sizeof(ShmPosition), sizeof(ShmPosition));
    ++i;
    memcpy(&m_fIdlePage, r_undovalue + i * sizeof(ShmPosition), sizeof(ShmPosition));
    ++i;
    memcpy(&m_lIdlePage, r_undovalue + i * sizeof(ShmPosition), sizeof(ShmPosition));
    ++i;
}

//��δ������������Slot
// ��������� r_num, ���뵽������r_retnum, ���뵽�ĵ�ַr_slotAddrList
// r_undoflag Desc���� �Ƿ���ҪдUndo��Ϣ
void  DescPageInfo::new_mem(MemManager* r_memMgr,
                            const int& r_num,
                            int& r_retnum,
                            vector<MdbAddress> &r_slotAddrList,
                            const bool& r_undoflag) throw(Mdb_Exception)
{
    r_retnum = 0;
    if (m_fIdlePage == NULL_SHMPOS) return; //�޿���ҳ��
    PageInfo*   t_page = NULL;
    char*       t_addr;
    ShmPosition t_pagepos;
    T_SCN       t_scn;
    t_pagepos = m_fIdlePage;
    int  t_neednum = r_num, t_newnum = 0;
    r_memMgr->getscn(t_scn);
    do
    {
        if (r_memMgr->getPhAddr(t_pagepos, t_addr) == false)
        {
            throw Mdb_Exception(__FILE__, __LINE__, "����ҳ��λ��ȡҳ��ʧ��!");
        }
        t_page = (PageInfo*)t_addr;
        t_page->allocateNode(t_neednum, r_slotAddrList, t_newnum);
        t_page->m_scn_slot = t_scn;
        r_retnum += t_newnum;
        t_neednum -= t_newnum;
        if (t_neednum <= 0) break;
        t_pagepos = t_page->m_nIdlePage;
    }
    while (!(t_pagepos == NULL_SHMPOS));
    if (t_page->m_useflag == 0x01)
    {
        if (!(m_fIdlePage == t_pagepos))
        {
            //��ҳ�䶯��
            if (r_undoflag == true)
            {
                r_memMgr->getscn(t_scn);
                writeUndoInfo(r_memMgr);
                m_scn = t_scn;
            }
            m_fIdlePage = t_pagepos;
        }
    }
    else
    {
        //����ָ����һ���ǿ���Page��NULL_SHMPOS
        if (r_undoflag == true)
        {
            r_memMgr->getscn(t_scn);
            writeUndoInfo(r_memMgr);
            m_scn = t_scn;
        }
        if (t_page->m_nIdlePage == NULL_SHMPOS)
        {
            //���һҳ����������
            m_fIdlePage = NULL_SHMPOS;
            m_lIdlePage = NULL_SHMPOS;
        }
        else
        {
            m_fIdlePage = t_page->m_nIdlePage;
        }
    }
    m_idleSlots -= r_retnum;
}
void  DescPageInfo::free_mem(MemManager* r_memMgr, const ShmPosition& r_slotaddr,
                             const ShmPosition& r_pagepos, PageInfo* r_pPage,
                             const bool& r_undoflag,
                             const T_SCN& r_scn) throw(Mdb_Exception)
{
    bool t_flag = (r_pPage->m_useflag == 0x02);
    if (r_undoflag == true)
    {
        r_pPage->lock();
        //add by gaojf 2012/7/5 10:10:37
        if (t_flag == true) //������ҪдӰ��
        {
            r_pPage->writeUndoInfo(r_memMgr);
        }
        r_pPage->freeNode(r_slotaddr.getOffSet(), r_scn);
        if (t_flag == true) //����
        {
            //r_pPage->writeUndoInfo(r_memMgr); //modified by gaojf 2012/7/5 10:10:31
            r_pPage->m_nIdlePage = m_fIdlePage;
            addIdlePage(r_memMgr, r_pagepos, r_pPage, r_undoflag);
        }
        r_pPage->unlock();
    }
    else
    {
        r_pPage->freeNode(r_slotaddr.getOffSet(), r_scn);
        if (t_flag == true) //����
        {
            r_pPage->m_nIdlePage = m_fIdlePage;
            addIdlePage(r_memMgr, r_pagepos, r_pPage, r_undoflag);
        }
    }
    ++m_idleSlots;
}

void DescPageInfos::lock()
{
    //1. ����������
    while (_check_lock((atomic_p)&m_lockinfo, 0, 1) == true)
    {
        //δ����
        //1.1 �ж��Ƿ�ʱ
        time_t t_nowtime;
        time(&t_nowtime);
        if (difftime(t_nowtime, m_locktime) > 5) //��ʱ�ж�
        {
            //ǿ�ƽ���
            time(&m_locktime);
            _clear_lock((atomic_p)&m_lockinfo, 0);
        }
    }
    time(&m_locktime);
}
void DescPageInfos::unlock()
{
    _clear_lock((atomic_p)&m_lockinfo, 0);
}
void DescPageInfos::setdescinfo(const char& r_type, const size_t& r_offset)
{
    for (int i = 0; i < MDB_PARL_NUM; ++i)
    {
        m_pageInfo[i].m_desctype = r_type;
        m_pageInfo[i].m_pos      = r_offset;
    }
}
void DescPageInfos::clearUndoInfo(MemManager* r_memMgr, const T_SCN& r_scn) throw(Mdb_Exception)
{
    for (int i = 0; i < MDB_PARL_NUM; ++i)
    {
        m_pageInfo[i].clearUndoInfo(r_memMgr, r_scn);
    }
}
//����ʱ���շ����㷨��ȡĿ���б�ָ�� t_descpginfo
//-1ȫæ��Χ��ȴ�   0�޿���SLOT������PAGE  1OK
int DescPageInfos::getNewDescPageInfo(DescPageInfo* &t_descpginfo, int& r_pos)
{
    t_descpginfo = NULL; //���ó�ʼֵ
    lock();
    int  t_iRet = -1;
    bool t_busyflag = true;
    for (int i = 0; i < MDB_PARL_NUM; ++i)
    {
        /* modified by gaojf 2010/11/19 11:41:16
        if(m_pageInfo[i].m_lockinfo!=0)
        { //�ж��Ƿ�ʱ
          time_t t_nowtime;
          time(&t_nowtime);
          if(difftime(t_nowtime,m_pageInfo[i].m_locktime)>5) //��ʱ�ж�
          { //ǿ���ͷ�
            time(&(m_pageInfo[i].m_locktime));
            m_pageInfo[i].m_lockinfo=0;
          }
        }
        if(m_pageInfo[i].m_lockinfo!=0) continue;
        */
        if (m_pageInfo[i].lock() == false) continue;
        else m_pageInfo[i].unlock();
        if (m_pageInfo[i].m_idleSlots == 0)
        {
            //���б��޿�����
            t_iRet = 0;
            if (t_descpginfo == NULL)
            {
                t_descpginfo = &(m_pageInfo[i]);
                r_pos = i;
            }
            continue;
        }
        else
        {
            //if(m_pageInfo[i].m_lockinfo==0)
            //{
            t_iRet = 1;
            t_descpginfo = &(m_pageInfo[i]);
            r_pos = i;
            break;  //�ҵ���
            //}
        }
    }
    if (t_iRet >= 0)
    {
        //m_pageInfo[r_pos].m_lockinfo=1; //��ָ���б���
        //time(&(m_pageInfo[r_pos].m_locktime));
        m_pageInfo[r_pos].lock(); //modified by gaojf 2010/11/19 11:43:29
    }
    unlock();
    return t_iRet;
}
DescPageInfo* DescPageInfos::getFreeDescPageInfo(const int& r_pos)
{
    lock();
    /*
    while (m_pageInfo[r_pos].m_lockinfo!=0)
    { //�ж��Ƿ�ʱ
      unlock();
      usleep(100); //����100us
      time_t t_nowtime;
      time(&t_nowtime);
      if(difftime(t_nowtime,m_pageInfo[r_pos].m_locktime)>5) //��ʱ�ж�
      { //ǿ���ͷ�
        m_pageInfo[r_pos].m_lockinfo=0;
        time(&(m_pageInfo[r_pos].m_locktime));
      }
      lock();
    }
    m_pageInfo[r_pos].m_lockinfo=1;
    time(&(m_pageInfo[r_pos].m_locktime));
    */
    while (m_pageInfo[r_pos].islock())
    {
        unlock();
        usleep(100); //����100us
        time_t t_nowtime;
        time(&t_nowtime);
        if (difftime(t_nowtime, m_pageInfo[r_pos].m_locktime) > 5) //��ʱ�ж�
        {
            //ǿ���ͷ�
            m_pageInfo[r_pos].unlock();
            time(&(m_pageInfo[r_pos].m_locktime));
        }
        lock();
    }
    m_pageInfo[r_pos].lock();
    unlock();
    return  &(m_pageInfo[r_pos]);
}
//falseȫæ��Χ��ȴ�   true OK
bool DescPageInfos::getFreeDescPageInfo(PageInfo* r_ppage, DescPageInfo* &t_descpginfo)
{
    //disabled by chenm 2010-11-28 ��ĳЩ����� �ᵼ��pageInfo��m_nIdlePageָ������,
    //������DescPageInfo::new_mem������ѭ��
    //if(r_ppage->m_useflag != 0x02) //�ͷ�ǰ��δ��,��Ҫ�ҵ�֮ǰ���б�
    {
        t_descpginfo = getFreeDescPageInfo(r_ppage->m_descparlpos);
        return true;
    }
    t_descpginfo = NULL; //���ó�ʼֵ
    lock();
    int    t_minpos = -1;
    size_t t_idleslot = 0;
    for (int i = 0; i < MDB_PARL_NUM; ++i)
    {
        //if(m_pageInfo[i].m_lockinfo!=0)
        if (m_pageInfo[i].islock())
        {
            //�ж��Ƿ�ʱ
            time_t t_nowtime;
            time(&t_nowtime);
            if (difftime(t_nowtime, m_pageInfo[i].m_locktime) > 5) //��ʱ�ж�
            {
                //ǿ���ͷ�
                //m_pageInfo[i].m_lockinfo=0;
                m_pageInfo[i].unlock();
                time(&(m_pageInfo[i].m_locktime));
            }
            else continue;
        }
        if ((t_minpos < 0) || (m_pageInfo[i].m_idleSlots < t_idleslot))
        {
            t_minpos  = i;
            t_idleslot = m_pageInfo[i].m_idleSlots;
        }
    }
    if (t_minpos >= 0)
    {
        //m_pageInfo[t_minpos].m_lockinfo=1;
        //time(&(m_pageInfo[t_minpos].m_locktime));
        m_pageInfo[t_minpos].lock();
        t_descpginfo = &(m_pageInfo[t_minpos]);
        r_ppage->m_descparlpos = t_minpos;
    }
    unlock();
    return (t_minpos >= 0);
}
//ǰ�᣺��ռ��Ѿ�����
//���ܣ�����ռ�ͱ���������
bool TableDesc::addSpace(const char* r_spaceName, const T_SCN& r_scn)
{
    bool t_bret = m_tableDef.addSpace(r_spaceName);
    m_scn = r_scn;
    return t_bret;
}


bool TableDesc::setSlotSize()
{
    //modified by gaojf 2010-5-14 17:21 for mdb2.0
    if (m_tableDef.m_tableType == UNDO_TABLE)
    {
        m_fixSize = sizeof(Undo_Slot);
    }
    else
    {
        m_fixSize = sizeof(UsedSlot);
    }
    m_valueSize = m_tableDef.getSlotSize();
    if (m_valueSize < sizeof(ShmPosition))
    {
        m_valueSize = sizeof(ShmPosition);
    }
    m_slotSize = m_fixSize + m_valueSize;
    for (int i = 0; i < MDB_PARL_NUM; ++i)
    {
        m_pageInfo[i].m_slotSize  = m_slotSize;
    }
    return true;
}

bool TableDesc::addIndex(const size_t& r_indexPos, const char* r_indexName, const T_SCN& r_scn)
{
    m_indexPosList[m_indexNum] = r_indexPos;
    strcpy(m_indexNameList[m_indexNum], r_indexName);
    m_indexNum++;
    m_scn = r_scn;
    return true;
}
bool TableDesc::deleteIndex(const size_t& r_indexPos, const T_SCN& r_scn)
{
    int t_pos;
    for (t_pos = 0; t_pos < m_indexNum; t_pos++)
    {
        if (m_indexPosList[t_pos] == r_indexPos)
        {
            break;
        }
    }
    if (t_pos >= m_indexNum)
    {
#ifdef _DEBUG_
        cout << "û���ҵ�����������!" << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    for (; t_pos < m_indexNum - 1; t_pos++)
    {
        m_indexPosList[t_pos] = m_indexPosList[t_pos + 1];
        memcpy(m_indexNameList[t_pos], m_indexNameList[t_pos + 1], sizeof(T_NAMEDEF));
    }
    m_indexNum--;
    m_scn = r_scn;
    return true;
}

void TableDesc::initByTableDef(const TableDef& r_tableDef)
{
    m_tableDef = r_tableDef;
    m_indexNum = 0;
    for (int i = 0; i < MDB_PARL_NUM; ++i)
    {
        m_pageInfo[i].initPos();
    }
    setSlotSize();
    m_lockinfo = 0;
    return ;
}
bool IndexDesc::addSpace(const char* r_spaceName, const T_SCN& r_scn)
{
    bool t_bret = m_indexDef.addSpace(r_spaceName);
    m_scn = r_scn;
    return t_bret;
}

void IndexDesc::initByIndeDef(const IndexDef& r_indexDef)
{
    m_indexDef = r_indexDef;
    for (int i = 0; i < MDB_PARL_NUM; ++i)
    {
        m_pageInfo[i].initPos();
    }
    setSlotSize();
    m_tablePos = 0;
    m_lockinfo = 0;
    return ;
}
bool IndexDesc::setSlotSize()
{
    //modified by gaojf 2010-5-14 17:21 for mdb2.0
    m_fixSize = sizeof(UsedSlot);
    m_valueSize = m_indexDef.getSlotSize();
    if (m_valueSize < sizeof(ShmPosition))
    {
        m_valueSize = sizeof(ShmPosition);
    }
    m_slotSize = m_fixSize + m_valueSize;
    for (int i = 0; i < MDB_PARL_NUM; ++i)
    {
        m_pageInfo[i].m_slotSize  = m_slotSize;
    }
    return true;
}
ostream& operator<<(ostream& os, const TableDesc& r_obj)
{
    string out;
    char cLine[1024];
    os << "-------------table desc begin-----------------------" << endl;
    os << r_obj.m_tableDef;
    sprintf(cLine, "������(m_indexNum):%d\n", r_obj.m_indexNum);
    out += cLine;
    if (r_obj.m_indexNum > 0)
    {
        sprintf(cLine, "%-40s %26s\n", "��������(m_indexNameList[])", "����λ��(m_indexPosList[])");
        out += cLine;
        for (int i = 0; i < r_obj.m_indexNum; i++)
        {
            sprintf(cLine, "%-40s %26ld\n", r_obj.m_indexNameList[i], r_obj.m_indexPosList[i]);
            out += cLine;
            //os<<r_obj.m_indexNameList[i]<<","<<r_obj.m_indexPosList[i]<<endl;
        }
    }
    out += "\n";
    sprintf(cLine, "��¼��(m_recordNum):%ld\n", r_obj.m_recordNum);
    out += cLine;
    out += "Page��Ϣ(m_pageInfo):\n";
    os << out;
    for (int j = 0; j < MDB_PARL_NUM; ++j)
    {
        os << "-------------------------------------------------" << endl;
        os << r_obj.m_pageInfo[j];
        os << "-------------------------------------------------" << endl;
    }
    os << "-------------table desc end-----------------------" << endl;
    return os;
}
ostream& operator<<(ostream& os, const IndexDesc& r_obj)
{
    os << "-------------index desc begin-----------------------" << endl;
    os << r_obj.m_indexDef;
    os << "m_fixSize  =" << r_obj.m_fixSize << endl;
    os << "m_valueSize=" << r_obj.m_valueSize << endl;
    os << "m_slotSize =" << r_obj.m_slotSize << endl;
    os << "������λ��(m_tablePos)=" << r_obj.m_tablePos << endl;
    os << "����ͷλ��(m_header  )=" << r_obj.m_header << endl;
    for (int j = 0; j < MDB_PARL_NUM; ++j)
    {
        os << "-------------------------------------------------" << endl;
        os << r_obj.m_pageInfo[j];
        os << "-------------------------------------------------" << endl;
    }
    os << "-------------index desc end-----------------------" << endl;
    return os;
}

void TableDesc::dumpInfo(ostream& r_os)
{
    r_os << *this;
}

void IndexDesc::dumpInfo(ostream& r_os)
{
    r_os << *this;
}

//��һ���յ�ҳ��ӿ����б����ͷ�
void  DescPageInfos::freePageIdle(UndoMemMgr* r_undomgr, const size_t& r_pagepos,
                                  PageInfo* r_ppage) throw(Mdb_Exception)
{
    //1. �ҵ���ҳ���ڵķ���ҳ���б�
    int     t_idlepos = r_ppage->m_descparlpos;
    size_t  t_pgpos = 0, t_nextpos = 0;
    PageInfo*   t_ppageinfo;
    DescPageInfo& t_pglist = m_pageInfo[t_idlepos];
    t_pgpos = t_pglist.m_fIdlePage.getOffSet();
    if (t_pgpos == r_pagepos)
    {
        //��һҳ
        t_pglist.m_idleSlots -= r_ppage->m_idleNum;
        if (t_pglist.m_lIdlePage.getOffSet() == r_pagepos)
        {
            //Ҳ�����һҳ
            t_pglist.m_lIdlePage = NULL_SHMPOS;
            t_pglist.m_fIdlePage = NULL_SHMPOS;
        }
        else
        {
            t_pglist.m_fIdlePage = r_ppage->m_nIdlePage;
        }
    }
    else
    {
        while (t_pgpos != 0)
        {
            r_undomgr->getPageInfo(t_pgpos, t_ppageinfo);
            t_nextpos = t_ppageinfo->m_nIdlePage.getOffSet();
            if (t_nextpos == r_pagepos)
            {
                t_pglist.m_idleSlots -= r_ppage->m_idleNum;
                t_ppageinfo->m_nIdlePage = r_ppage->m_nIdlePage;
                if (t_pglist.m_lIdlePage.getOffSet() == r_pagepos)
                {
                    t_pglist.m_lIdlePage.setValue(r_undomgr->getSpaceCode(), t_pgpos);
                }
                //�ҵ��˸��б�
            }
            else t_pgpos = t_nextpos;
        }
    }
}

//��һ���յ�ҳ���ȫ�б����ͷ�
void  DescPageInfos::freePage(UndoMemMgr* r_undomgr, const size_t& r_pagepos,
                              PageInfo* r_ppage) throw(Mdb_Exception)
{
    int t_tpagepos = -1;
    size_t      t_pgpos = 0, t_nextpos = 0;
    PageInfo*   t_ppageinfo;
    int t_i;
    //2. �ҵ���Ҳ���ڵ�����ҳ���б�
    lock();
    for (t_i = 0; t_i < MDB_PARL_NUM; ++t_i)
    {
        try
        {
            DescPageInfo& t_pglist = m_pageInfo[t_i];
            t_pgpos = t_pglist.m_firstPage.getOffSet();
            if (t_pgpos == r_pagepos)
            {
                //��һҳ
                t_tpagepos = t_i;
                break;
            }
            else
            {
                while (t_pgpos != 0)
                {
                    r_undomgr->getPageInfo(t_pgpos, t_ppageinfo);
                    t_nextpos = t_ppageinfo->m_next.getOffSet();
                    if (t_nextpos == r_pagepos)
                    {
                        t_tpagepos = t_i;
                        break;
                        //�ҵ��˸��б�
                    }
                    else t_pgpos = t_nextpos;
                }
            }
        }
        catch (Mdb_Exception& e)
        {
            unlock();
            throw e;
        }
        if (t_tpagepos >= 0) break;
    }
    unlock();
    if (t_tpagepos < 0)
    {
        throw Mdb_Exception(__FILE__, __LINE__, "����ҳ���б���δ�ҵ���Ӧ��ҳ��!");
    }
    DescPageInfo& t_pglist2 = m_pageInfo[t_tpagepos];
    time_t t_nowtime;
    lock();
    //while (t_pglist2.m_lockinfo!=0)
    while (t_pglist2.islock())
    {
        //�ж��Ƿ�ʱ
        unlock();
        usleep(100); //����100us
        time(&t_nowtime);
        if (difftime(t_nowtime, t_pglist2.m_locktime) > 5) //��ʱ�ж�
        {
            //ǿ���ͷ�
            //t_pglist2.m_lockinfo=0;
            t_pglist2.unlock();
            time(&(t_pglist2.m_locktime));
        }
        lock();
    }
    //t_pglist2.m_lockinfo=1; //�����б�
    t_pglist2.lock(); //�����б�
    try
    {
        t_pgpos = t_pglist2.m_firstPage.getOffSet();
        if (t_pgpos == r_pagepos)
        {
            //��һҳ
            if (t_pglist2.m_lastPage.getOffSet() == r_pagepos)
            {
                //Ҳ�����һҳ
                t_pglist2.m_lastPage = NULL_SHMPOS;
                t_pglist2.m_firstPage = NULL_SHMPOS;
            }
            else
            {
                t_pglist2.m_firstPage = r_ppage->m_next;
            }
        }
        else
        {
            while (t_pgpos != 0)
            {
                r_undomgr->getPageInfo(t_pgpos, t_ppageinfo);
                t_nextpos = t_ppageinfo->m_next.getOffSet();
                if (t_nextpos == r_pagepos)
                {
                    t_ppageinfo->m_next = r_ppage->m_next;
                    if (t_pglist2.m_lastPage.getOffSet() == r_pagepos)
                    {
                        t_pglist2.m_lastPage.setValue(r_undomgr->getSpaceCode(), t_pgpos);
                    }
                    //�ҵ��˸��б�
                }
                else t_pgpos = t_nextpos;
            }
        }
    }
    catch (Mdb_Exception& e3)
    {
        //t_pglist2.m_lockinfo=0; //�ͷŸ��б���
        t_pglist2.unlock(); //�ͷŸ��б���
        unlock();
        throw e3;
    }
    //t_pglist2.m_lockinfo=0; //�ͷŸ��б���
    t_pglist2.unlock(); //�ͷŸ��б���
    unlock();
}

void DescPageInfos::startmdb_init()
{
    m_lockinfo = 0;
    for (int i = 0; i < MDB_PARL_NUM; ++i)
    {
        m_pageInfo[i].m_scn.setOffSet(0);
        m_pageInfo[i].m_undopos.setOffSet(0);
    }
    m_scn.setOffSet(0);
}

void DescPageInfos::initpageinfoscn(const T_SCN& r_scn)
{
    for (int i = 0; i < MDB_PARL_NUM; ++i)
    {
        m_pageInfo[i].m_scn = r_scn;
    }
    m_scn = r_scn;
}


bool DescPageInfo::lock()
{
    while (_check_lock((atomic_p)&m_lockinfo, 0, 1) == true)
    {
        //����δ������֮ǰ������
        //�ж��Ƿ�ʱ
        time_t t_nowtime;
        time(&t_nowtime);
        if (difftime(t_nowtime, m_locktime) > 5) //��ʱ�ж�
        {
            //ǿ���ͷ�
            time(&(m_locktime));
            unlock();
        }
        else
        {
            return false; //�Ѿ�������������δ��ʱ
        }
    }
    time(&(m_locktime));
    return true; //�ɹ�������
}
void DescPageInfo::unlock()
{
    _clear_lock((atomic_p)&m_lockinfo, 0);
}
//true ����  false����
bool DescPageInfo::islock()
{
    return (m_lockinfo != 0);
}
