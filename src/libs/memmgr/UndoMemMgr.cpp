#include "UndoMemMgr.h"
#include "lockMgr/TransactionMgr.h"

bool UndoMemMgr::initialize(SpaceInfo& r_spaceInfo)
{
    SpaceMgrBase::initialize(r_spaceInfo);
    return true;
}
/**
 * insert  ����Ӱ����Ϣ: ����UNDO��¼��ƫ����.
 * @param  r_type         : ��������
 * @param  r_tbname       : ԭ���ݱ���(ֻ���ǲ��������SLOT����)
 * @param  r_doffset      : ԭʼ���ݵ�ƫ����
 * @param  r_scn          : ʱ���
 * @param  r_value        : ֵ
 * @param  r_prevUndopos  : ǰ���汾��Ӱ��ƫ��(�ձ�ʾ��)
 * @return ��������UNDO��¼��ƫ�����������������׳��쳣
 */
UndoPosition UndoMemMgr::insert(const T_UNDO_DESCTYPE&  r_type   ,
                                const T_NAMEDEF&        r_tbname ,
                                const ShmPosition&      r_doffset,
                                const T_SCN&            r_scn    ,
                                const char*             r_value  ,
                                const UndoPosition&     r_prevUndopos) throw(Mdb_Exception)
{
    TableDesc*    t_pundodesc = NULL;
    //1. ��ȡ��������
    getUndoDesc(r_type, r_tbname, t_pundodesc);
    return insert(t_pundodesc, r_doffset, r_scn, r_value, r_prevUndopos);
}

UndoPosition UndoMemMgr::insert(TableDesc*              r_pundodesc,
                                const ShmPosition&      r_doffset,
                                const T_SCN&            r_scn    ,
                                const char*             r_value  ,
                                const UndoPosition&     r_prevUndopos) throw(Mdb_Exception)
{
    UndoPosition  t_slotpos;
    MdbAddress    t_addr;
    try
    {
        //1. �ж�Ӱ����Ϣ�Ƿ��ǰһ��һ�£����һ����������д
        size_t t_preoffset = r_prevUndopos.getOffSet();
        if (t_preoffset != 0)
        {
            Undo_Slot* t_preslot = (Undo_Slot*)(m_spaceHeader.m_shmAddr + t_preoffset);
            if (memcmp((char*)(t_preslot) + UNDO_VAL_OFFSET, r_value, r_pundodesc->m_valueSize) == 0)
            {
                return r_prevUndopos; //����Ҫ��дֱ�ӷ����ϴ�Ӱ����Ϣ
            }
        }
        allocateSlot(r_pundodesc, t_addr);
    }
    catch (Mdb_Exception& e)
    {
#ifdef _DEBUG_
        e.toString();
        cout << __FILE__ << __LINE__ << endl;
#endif
        throw e;
    }
    //2.����ֵ
    Undo_Slot* t_undoslot = (Undo_Slot*)(t_addr.m_addr);
    t_undoslot->m_lockinfo = 0x80;
    //t_undoslot->m_doffset = r_doffset;
    t_undoslot->m_scn     = r_scn;
    t_undoslot->m_next    = r_prevUndopos;
    memcpy((char*)(t_addr.m_addr) + UNDO_VAL_OFFSET, r_value, r_pundodesc->m_valueSize);
    ++(r_pundodesc->m_recordNum);
    t_slotpos.setOffSet(t_addr.m_pos.getOffSet());
    return t_slotpos;
}
/**
 * free  ����Ӱ����Ϣ����ָ���Ľڵ��Ӧ��Ӱ�ӽ��л���.
 * @param  r_type         : ��������
 * @param  r_tbname       : ԭ���ݱ���(ֻ���ǲ��������SLOT����)
 * @param  r_undopos      : Ӱ��ƫ����
 */
void UndoMemMgr::free(const T_UNDO_DESCTYPE&  r_type   ,
                      const T_NAMEDEF&        r_tbname ,
                      const T_SCN&            r_scn    ,
                      const int&              r_flag   ,
                      UndoPosition&     r_undopos) throw(Mdb_Exception)
{
    TableDesc*    t_pundodesc = NULL;
    //1. ��ȡ��������
    getUndoDesc(r_type, r_tbname, t_pundodesc);
    free(t_pundodesc, r_scn, r_flag, r_undopos);
}
void UndoMemMgr::free(TableDesc*              r_tabledesc,
                      const T_SCN&            r_scn    ,
                      const int&              r_flag   ,
                      UndoPosition&     r_undopos) throw(Mdb_Exception)
{
    try
    {
        Undo_Slot*    t_pundoslot = NULL;
        ShmPosition   t_slotPos_shm;
        UndoPosition* t_preslot_pos = &r_undopos;
        UndoPosition  t_nextslot;
        size_t        t_slotoffset;
        int           t_flag = r_flag;
        bool          t_fisrtflag = true;
        t_nextslot = r_undopos;
        while (!(t_nextslot == NULL_UNDOPOS))
        {
            t_slotoffset = t_nextslot.getOffSet();
            if ((t_slotoffset < m_spinfo->m_pageoffset) ||
                    (t_slotoffset >= (m_spinfo->m_pageoffset + m_spinfo->m_pagetsize)))
            {
                //Խ����
                break;
            }
            t_pundoslot = (Undo_Slot*)(m_spaceHeader.m_shmAddr + t_slotoffset);
            t_nextslot = t_pundoslot->m_next;
            if ((t_pundoslot->m_lockinfo & 0x80) == 0) break; //��SLOTδʹ��
            if (t_flag == 2)
            {
                //ǰ���Ѿ���ɾ������
                t_slotPos_shm.setValue(m_spaceHeader.m_spaceCode, t_slotoffset);
                deleteSlot(r_tabledesc, t_slotPos_shm);
                --(r_tabledesc->m_recordNum);
            }
            else if (t_flag == 1 && t_fisrtflag == true)
            {
                //��һ���ڵ� ��ɾ��
                t_preslot_pos = &(t_pundoslot->m_next);
                t_flag = 0; //���������Ҫɾ��
            }
            else if (t_pundoslot->m_scn > r_scn)
            {
                //����Ҫɾ��
                t_preslot_pos = &(t_pundoslot->m_next);
            }
            else
            {
                //�ýڵ���Ҫɾ��
                t_slotPos_shm.setValue(m_spaceHeader.m_spaceCode, t_slotoffset);
                deleteSlot(r_tabledesc, t_slotPos_shm);
                --(r_tabledesc->m_recordNum);
                t_flag = 2;
                *t_preslot_pos = NULL_UNDOPOS; //ֻ��ɾ���ĵ�һ����Ҫ���ϸ��ڵ��λ�����
            }
            t_fisrtflag = false;
        }
    }
    catch (Mdb_Exception& e)
    {
#ifdef _DEBUG_
        e.toString();
        cout << __FILE__ << __LINE__ << endl;
#endif
        throw e;
    }
}
char* UndoMemMgr::getUndoInfo(const UndoPosition&     r_undopos) throw(Mdb_Exception)
{
    Undo_Slot* t_pundoslot = (Undo_Slot*)(m_spaceHeader.m_shmAddr + r_undopos.getOffSet());
    return ((char*)t_pundoslot) + UNDO_VAL_OFFSET;
}
char* UndoMemMgr::getUndoInfo(const T_SCN&            r_scn    ,
                              const UndoPosition&     r_undopos) throw(Mdb_Exception)
{
    Undo_Slot* t_pundoslot = NULL;
    return getUndoInfo(r_scn, r_undopos, t_pundoslot);
}
/**
 * getUndoInfo    ��ȡӰ����Ϣ
 * @param  r_type         : ��������
 * @param  r_tbname       : ԭ���ݱ���(ֻ���ǲ��������SLOT����)
 * @param  r_scn          : ʱ���
 * @param  r_undopos      : Ӱ��ƫ����
 */
char* UndoMemMgr::getUndoInfo(const T_SCN&            r_scn    ,
                              const UndoPosition&     r_undopos,
                              Undo_Slot*       &r_pundo) throw(Mdb_Exception)
{
    try
    {
        Undo_Slot*    t_pundoslot = NULL;
        UndoPosition  t_nextslot;
        r_pundo = NULL;
        t_nextslot = r_undopos;
        while (!(t_nextslot == NULL_UNDOPOS))
        {
            // add by chenm 2010-11-16
            size_t t_slotoffset = t_nextslot.getOffSet();
            if ((t_slotoffset < m_spinfo->m_pageoffset) ||
                    (t_slotoffset >= (m_spinfo->m_pageoffset + m_spinfo->m_pagetsize)))
            {
                //Խ����
                break;
            }
            // over 2010-11-16
            t_pundoslot = (Undo_Slot*)(m_spaceHeader.m_shmAddr + t_nextslot.getOffSet());
            if (t_pundoslot->m_scn > r_scn)
            {
                t_nextslot = t_pundoslot->m_next;
            }
            else
            {
                r_pundo = t_pundoslot;
                return ((char*)t_pundoslot) + UNDO_VAL_OFFSET;
            }
        }
    }
    catch (Mdb_Exception& e)
    {
#ifdef _DEBUG_
        e.toString();
        cout << __FILE__ << __LINE__ << endl;
#endif
        throw e;
    }
    return NULL;
}
/**
 * getUndoInfo    ��ȡӰ����Ϣ
 * @param  r_scn1/r_scn2  : ʱ���(r_scn1,r_scn2)֮���
 * @param  r_undopos      : Ӱ��ƫ����
 */
char* UndoMemMgr::getUndoInfo(const T_SCN&            r_scn1    ,
                              const T_SCN&            r_scn2   ,
                              const UndoPosition&     r_undopos) throw(Mdb_Exception)
{
    Undo_Slot* t_pundoslot = NULL;
    char* t_value = getUndoInfo(r_scn2, r_undopos, t_pundoslot);
    if (t_pundoslot == NULL) return NULL;
    if (t_pundoslot->m_scn >= r_scn1)
    {
        return t_value ;
    }
    else
    {
        return NULL;
    }
}

//���ݱ�ռ䶨����Ϣ,������ռ�
bool UndoMemMgr::createUndoSpace(SpaceInfo& r_spaceInfo)
{
    if (createSpace(m_spaceHeader) == false) //����Ҫ������Ӧ���ļ�
    {
#ifdef _DEBUG_
        cout << "createSpace false!" << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    r_spaceInfo = *(getSpaceInfo());
    return true;
}

//��ʼ����ռ�
bool UndoMemMgr::initSpaceInfo(const size_t& r_descnum,
                               const size_t& r_redosize,
                               const size_t& r_indexsize)
{
    //��ʼ��
    size_t  t_offSet = 0;
    size_t  t_spinfo_offset = 0;
    //1. ��ʼ����������ռ�ͷm_spaceHeader��Ϣ
    memcpy(m_pSpHeader, &m_spaceHeader, sizeof(SpaceInfo));
    t_offSet += sizeof(SpaceInfo);
    //1-2 Ԥ��m_spinfo�ռ�
    m_spinfo = (Undo_spaceinfo*)(m_spaceHeader.m_shmAddr + t_offSet);
    m_spinfo->m_tablenums = r_descnum;
    m_spinfo->m_redosize  = r_redosize;
    m_spinfo->m_indexsize = r_indexsize;
    if (m_pSpHeader->m_size <= m_spinfo->m_pageoffset + m_spinfo->m_redosize + m_spinfo->m_indexsize)
    {
#ifdef _DEBUG_
        cout << "m_pSpHeader->m_size��С!" << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    t_offSet += sizeof(Undo_spaceinfo);
    //2.��ʼ��UNDO����������Ϣ
    if (m_tbDescMgr.initElements(m_spaceHeader.m_shmAddr, t_offSet, m_spinfo->m_tablenums) == false)
    {
#ifdef _DEBUG_
        cout << "m_tbDescMgr.initElements false!" << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    t_offSet += m_tbDescMgr.getSize();
    //��ʼ��Undo����������
    if (m_indexDescMgr.initElements(m_spaceHeader.m_shmAddr, t_offSet, m_spinfo->m_tablenums) == false)
    {
#ifdef _DEBUG_
        cout << "m_indexDescMgr.initElements false!" << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    t_offSet += m_indexDescMgr.getSize();
    //��ʼ��m_spinfo��Ϣ
    m_spinfo->m_pageoffset = t_offSet;
    m_spinfo->m_pagetsize = m_pSpHeader->m_size - m_spinfo->m_pageoffset
                            - m_spinfo->m_redosize - m_spinfo->m_indexsize;
    if (m_spinfo->m_pagetsize <= 0)
    {
#ifdef _DEBUG_
        cout << "page�ռ䲻������������ô�С" << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    m_spinfo->m_indexoffset = m_spinfo->m_pageoffset + m_spinfo->m_pagetsize;
    m_spinfo->m_redooffset = m_spinfo->m_indexoffset + m_spinfo->m_indexsize;
    m_redoaddr = m_spaceHeader.m_shmAddr + m_spinfo->m_redooffset;
    m_indexSegsMgr.initialize(m_spaceHeader.m_shmAddr, m_spinfo->m_indexoffset,
                              m_spinfo->m_indexsize, 1);
    m_dpagesMgr.initialize(this, m_spaceHeader.m_shmAddr, m_spinfo->m_pageoffset,
                           m_spinfo->m_pagetsize, m_spaceHeader.m_pageSize, 1);
    return true;
}

//��ȡREDO�ڴ����Ϣ��REDO���ڴ������REDO�ڲ�����
void UndoMemMgr::getRedobufInfo(char * &r_addr, size_t& r_size)
{
    r_addr = m_redoaddr;
    r_size = m_spinfo->m_redosize;
}

//attach��ʽinit
bool UndoMemMgr::attach_init(SpaceInfo& r_spaceInfo)
{
    initialize(r_spaceInfo);
    if (SpaceMgrBase::attach() == false)
    {
#ifdef _DEBUG_
        cout << "SpaceMgrBase::attach() false!" << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    r_spaceInfo = m_spaceHeader;
    size_t  t_offSet = 0;
    //1. ������ռ�ͷm_spaceHeader��Ϣ
    t_offSet += sizeof(SpaceInfo);
    //2. attach��ʼ��m_spinfo
    m_spinfo = (Undo_spaceinfo*)(m_spaceHeader.m_shmAddr + t_offSet);
    t_offSet += sizeof(Undo_spaceinfo);
    //3.��ʼ������������Ϣ
    if (m_tbDescMgr.attach_init(m_spaceHeader.m_shmAddr, t_offSet) == false)
    {
#ifdef _DEBUG_
        cout << "m_tbDescMgr.attach_init false!" << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    t_offSet += m_tbDescMgr.getSize();
    if (m_indexDescMgr.attach_init(m_spaceHeader.m_shmAddr, t_offSet) == false)
    {
#ifdef _DEBUG_
        cout << "m_indexDescMgr.attach_init false!" << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    t_offSet += m_indexDescMgr.getSize();
    m_redoaddr = m_spaceHeader.m_shmAddr + m_spinfo->m_redooffset;
    m_indexSegsMgr.initialize(m_spaceHeader.m_shmAddr, m_spinfo->m_indexoffset,
                              m_spinfo->m_indexsize, 0);
    m_dpagesMgr.initialize(this, m_spaceHeader.m_shmAddr, m_spinfo->m_pageoffset,
                           m_spinfo->m_pagetsize, m_spaceHeader.m_pageSize, 0);
    //��ʼ��
    T_NAMEDEF  t_undotbname;
    for (int i = (int)T_UNDO_TYPE_DESC; i < (int)T_UNDO_TYPE_UNDEF; ++i)
    {
        //if ((i == (int)T_UNDO_TYPE_TABLEDATA) || (i == (int)T_UNDO_TYPE_INDEXDATA))
        if (i == (int)T_UNDO_TYPE_TABLEDATA)
        {
            //���ݱ������slot��Ӧ��undo�����ڴ˴���
            continue;
        }
        else
        {
            sprintf(t_undotbname, "%s%s", _UNDO_TBPREFIX, _UNDO_TABLE[i]);
            getTbDescByname(t_undotbname, m_undodesc[i]);
        }
    }
    return true;
}

//���ݱ����ʼ����Ӧ��Undo��������
void UndoMemMgr::initUndoDesc(const TableDef& r_tabledef, TableDesc& r_undodesc)
{
    memset(&r_undodesc, 0, sizeof(TableDesc));
    memcpy(&(r_undodesc.m_tableDef), &r_tabledef, sizeof(TableDef));
    sprintf(r_undodesc.m_tableDef.m_tableName, "%s%s", _UNDO_TBPREFIX, r_tabledef.m_tableName);
    r_undodesc.m_tableDef.m_tableType = UNDO_TABLE;
    r_undodesc.m_tableDef.m_logType = 0;
    r_undodesc.m_tableDef.m_lockType = 0;
    r_undodesc.m_tableDef.m_keyClumnNum = 0;
    r_undodesc.m_tableDef.m_keyFlag = 0;
    r_undodesc.m_tableDef.m_spaceNum = 1;
    r_undodesc.m_indexNum = 0;
    strcpy(r_undodesc.m_tableDef.m_spaceList[0], m_spaceHeader.m_spaceName);
    for (int i = 0; i < MDB_PARL_NUM; ++i)
    {
        r_undodesc.m_pageInfo[i].initPos();
    }
    r_undodesc.setSlotSize();
}

//���ݱ����ʼ����Ӧ��TX��������
void UndoMemMgr::initTxDesc(const TableDef& r_tabledef, TableDesc& r_txdesc)
{
    memset(&r_txdesc, 0, sizeof(TableDesc));
    sprintf(r_txdesc.m_tableDef.m_tableName, "%s%s", _TX_TBPREFIX, r_tabledef.m_tableName);
    int offset = 0;
    TableDef& txTbDef = r_txdesc.m_tableDef;
    r_txdesc.m_tableDef.m_tableType = TX_TABLE;
    r_txdesc.m_tableDef.m_columnNum = 3;
    for (int i = 0; i < txTbDef.m_columnNum; ++i)
    {
        if (i == 0)
        {
            strcpy(txTbDef.m_columnList[i].m_name, COLUMN_NAME_SID_A);
            txTbDef.m_columnList[i].m_type = VAR_TYPE_INT;
            txTbDef.m_columnList[i].m_len = 4;
            txTbDef.m_columnList[i].m_offSet = offset;
            offset += txTbDef.m_columnList[i].m_len;
        }
        else if (i == 1)
        {
            strcpy(txTbDef.m_columnList[i].m_name, COLUMN_NAME_SID_B);
            txTbDef.m_columnList[i].m_type = VAR_TYPE_INT;
            txTbDef.m_columnList[i].m_len = 4;
            txTbDef.m_columnList[i].m_offSet = offset;
            offset += txTbDef.m_columnList[i].m_len;
        }
        else
        {
            strcpy(txTbDef.m_columnList[i].m_name, COLUMN_NAME_SEM_NO_A);
            txTbDef.m_columnList[i].m_type = VAR_TYPE_INT;
            txTbDef.m_columnList[i].m_len = 4;
            txTbDef.m_columnList[i].m_offSet = offset;
            offset += txTbDef.m_columnList[i].m_len;
        }
    }
    r_txdesc.m_tableDef.m_logType = 0;
    r_txdesc.m_tableDef.m_lockType = 0;
    r_txdesc.m_tableDef.m_keyClumnNum = 0;
    r_txdesc.m_tableDef.m_keyFlag = 0;
    r_txdesc.m_tableDef.m_spaceNum = 1;
    r_txdesc.m_indexNum = 0;
    strcpy(r_txdesc.m_tableDef.m_spaceList[0], m_spaceHeader.m_spaceName);
    for (int i = 0; i < MDB_PARL_NUM; ++i)
    {
        r_txdesc.m_pageInfo[i].initPos();
    }
    r_txdesc.setSlotSize();
}

//����ԭ���ݱ�������slot��Ӧ��Undo�� 2012/9/18 7:51:53 gaojf
bool UndoMemMgr::createUndoIdxDesc(const TableDesc& r_tabledesc)
{
    TableDesc  t_undodesc;
    TableDef   t_undotbdefs[T_UNDO_TYPE_UNDEF]; ///<�̶������������ṹ
    TableDesc* t_pundodesc = NULL;
    //1. ��ʼ�� �̶������������ṹ ht_undotbdefs
    initUndoFixDesc(t_undotbdefs);
    sprintf(t_undotbdefs[T_UNDO_TYPE_INDEXDATA].m_tableName, "%s%s", _UNDO_TABLE[T_UNDO_TYPE_INDEXDATA],
            r_tabledesc.m_tableDef.m_tableName);
    initUndoDesc(t_undotbdefs[T_UNDO_TYPE_INDEXDATA], t_undodesc);
    createUndoTbDesc(t_undodesc, t_pundodesc);
    return true;
}

//����UNDO��������Ϣ
bool UndoMemMgr::createUndoDescs(const vector<TableDesc> &r_tabledescs)
{
    TableDesc  t_undodesc;
    TableDef   t_undotbdefs[T_UNDO_TYPE_UNDEF];///<�̶������������ṹ
    TableDesc* t_pundodesc = NULL;
    //1. ��ʼ�� �̶������������ṹ t_undotbdefs
    initUndoFixDesc(t_undotbdefs);
    try
    {
        //�����̶�����UNDO��������
        for (int i = (int)T_UNDO_TYPE_DESC; i < (int)T_UNDO_TYPE_UNDEF; ++i)
        {
            if ((i == (int)T_UNDO_TYPE_TABLEDATA) || (i == (int)T_UNDO_TYPE_INDEXDATA))
            {
                //���ݱ� ���������ڴ˴���
                continue;
            }
            else
            {
                initUndoDesc(t_undotbdefs[i], t_undodesc);
            }
            //cout<<t_undodesc<<endl;
            createUndoTbDesc(t_undodesc, t_pundodesc);
            m_undodesc[i] = t_pundodesc;
        }
        //�������ݲ��ֱ�������
        for (vector<TableDesc>::const_iterator t_itr = r_tabledescs.begin();
                t_itr != r_tabledescs.end(); ++t_itr)
        {
            initUndoDesc(t_itr->m_tableDef, t_undodesc);
            createUndoTbDesc(t_undodesc, t_pundodesc);
            createTxDesc(*t_itr);
            createUndoIdxDesc(*t_itr); //��������slot��Undo�еı�ÿ��ʵ���һ��undo�� 2012/9/18 7:35:54
        }
    }
    catch (Mdb_Exception& e)
    {
#ifdef _DEBUG_
        e.toString();
        cout << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    return true;
}
//����һ����������
bool UndoMemMgr::createUndoDesc(const TableDesc& r_tabledesc)
{
    TableDesc  t_undodesc, *t_pundodesc;
    initUndoDesc(r_tabledesc.m_tableDef, t_undodesc);
    createUndoTbDesc(t_undodesc, t_pundodesc);
    return true;
}
//����һ�����Ӧ��TX�� ����������
bool UndoMemMgr::createTxDesc(const TableDesc& r_tabledesc)
{
    TableDesc t_txdesc, *t_ptxdesc;
    initTxDesc(r_tabledesc.m_tableDef, t_txdesc);
    createUndoTbDesc(t_txdesc, t_ptxdesc);
    //������һ������
    IndexDef saIndexDef;
    sprintf(saIndexDef.m_indexName, "%s%s", INDEX_A_PREFIX, t_txdesc.m_tableDef.m_tableName);
    saIndexDef.m_indexType = HASH_INDEX;
    saIndexDef.m_hashSize = 512;
    strcpy(saIndexDef.m_tableName, t_txdesc.m_tableDef.m_tableName);
    saIndexDef.m_columnNum = 1;
    strcpy(saIndexDef.m_columnList[0], COLUMN_NAME_SID_A);
    saIndexDef.m_spaceNum = t_txdesc.m_tableDef.m_spaceNum;
    for (int i = 0; i < t_txdesc.m_tableDef.m_spaceNum; i++)
    {
        strcpy(saIndexDef.m_spaceList[i], t_txdesc.m_tableDef.m_spaceList[i]);
    }
    saIndexDef.m_isUnique = false;
    IndexDesc* pIndexDesc;
    createIndex(saIndexDef, pIndexDesc);
    IndexDef sbIndexDef;
    sprintf(sbIndexDef.m_indexName, "%s%s", INDEX_B_PREFIX, t_txdesc.m_tableDef.m_tableName);
    sbIndexDef.m_indexType = HASH_INDEX;
    sbIndexDef.m_hashSize = 512;
    strcpy(sbIndexDef.m_tableName, t_txdesc.m_tableDef.m_tableName);
    sbIndexDef.m_columnNum = 1;
    strcpy(sbIndexDef.m_columnList[0], COLUMN_NAME_SID_B);
    sbIndexDef.m_spaceNum = t_txdesc.m_tableDef.m_spaceNum;
    for (int i = 0; i < t_txdesc.m_tableDef.m_spaceNum; i++)
    {
        strcpy(sbIndexDef.m_spaceList[i], t_txdesc.m_tableDef.m_spaceList[i]);
    }
    sbIndexDef.m_isUnique = false;
    createIndex(sbIndexDef, pIndexDesc);
    return true;
}

//ɾ��һ����������
bool UndoMemMgr::deleteUndoDesc(const TableDesc& r_tabledesc)
{
    try
    {
        TableDesc  t_undodesc;
        initUndoDesc(r_tabledesc.m_tableDef, t_undodesc);
        deleteUndoTbDesc(t_undodesc);
    }
    catch (Mdb_Exception& e)
    {
#ifdef _DEBUG_
        e.toString();
        cout << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    return true;
}
//ɾ��һ������������Ӧ��tx�� ��drop����
bool UndoMemMgr::deleteTxDesc(const TableDesc& r_tabledesc)
{
    return true;
}
//����һ����������
void UndoMemMgr::createUndoTbDesc(const TableDesc& r_undodesc, TableDesc* &r_pundodesc)
{
    SpaceMgrBase::lock();
    try
    {
        //cout<<"---����������Ϣ-------"<<endl;
        //m_tbDescMgr.dumpInfo(cout);
        if (m_tbDescMgr.getElement(r_undodesc, r_pundodesc) == true)
        {
#ifdef _DEBUG_
            cout << "UNDO�������Ѿ�����{" << r_undodesc << "}!" << __FILE__ << __LINE__ << endl;
#endif
            throw Mdb_Exception(__FILE__, __LINE__, "UNDO������{%s}�Ѿ�����!",
                                r_undodesc.m_tableDef.m_tableName);
        }
        int t_flag = 1; //֧�ֱ�����������
        if (m_tbDescMgr.addElement(r_undodesc, r_pundodesc, t_flag) == false)
        {
            throw Mdb_Exception(__FILE__, __LINE__, "m_tbDescMgr.addElementʧ��!");
        }
        r_pundodesc->setdescinfo('1', m_tbDescMgr.getOffset(r_pundodesc));
        //cout<<"---����������Ϣ-------"<<endl;
        //m_tbDescMgr.dumpInfo(cout);
    }
    catch (Mdb_Exception& e)
    {
        SpaceMgrBase::unlock();
        throw e;
    }
    m_spinfo->updatetime();
    SpaceMgrBase::unlock();
}
//ɾ��һ����������
void UndoMemMgr::deleteUndoTbDesc(const TableDesc& r_undodesc)
{
    TableDesc* t_pundodesc = NULL;
    SpaceMgrBase::lock();
    try
    {
        int t_flag = 1; //֧�ֱ�����������
        if (m_tbDescMgr.deleteElement(r_undodesc, t_flag) == false)
        {
            throw Mdb_Exception(__FILE__, __LINE__, "m_tbDescMgr.deleteElementʧ��!");
        }
    }
    catch (Mdb_Exception& e)
    {
        SpaceMgrBase::unlock();
        throw e;
    }
    m_spinfo->updatetime();
    SpaceMgrBase::unlock();
}

void UndoMemMgr::updateTbDescTime()
{
    try
    {
        SpaceMgrBase::lock();
    }
    catch (Mdb_Exception& e)
    {
        return;
    }
    m_spinfo->updatetime();
    SpaceMgrBase::unlock();
}

//��ȡһ����������
void UndoMemMgr::getUndoDesc(const T_UNDO_DESCTYPE&  r_type, const char* r_tablename, TableDesc* &r_pundodesc)
{
    TableDesc t_undodesc;
    if (r_type == T_UNDO_TYPE_TABLEDATA)
    {
        sprintf(t_undodesc.m_tableDef.m_tableName, "_Undo_%s", r_tablename);
    }
    //�������������ݱ���_Undo__i_+������ȡ 2012/9/18 7:39:56 gaojf
    else if (r_type == T_UNDO_TYPE_INDEXDATA)
    {
        sprintf(t_undodesc.m_tableDef.m_tableName, "%s%s%s", _UNDO_TBPREFIX, _UNDO_TABLE[T_UNDO_TYPE_INDEXDATA], r_tablename);
    }
    //2012/9/18 7:40:20 gaojf end
    else if (r_type < T_UNDO_TYPE_UNDEF)
    {
        r_pundodesc = m_undodesc[r_type];
        return;
    }
    else
    {
        throw Mdb_Exception(__FILE__, __LINE__, "UNDO����������{%d}������!",
                            (int)r_type);
    }
    SpaceMgrBase::lock();
    if (m_tbDescMgr.getElement(t_undodesc, r_pundodesc) == false)
    {
        SpaceMgrBase::unlock();
        throw Mdb_Exception(__FILE__, __LINE__, "UNDO����������{%s}������!",
                            t_undodesc.m_tableDef.m_tableName);
    }
    SpaceMgrBase::unlock();
}

//����һ���ڵ���ڴ�
void UndoMemMgr::allocateSlot(DescPageInfos*    r_descPageInfos,
                              MdbAddress&       r_addr) throw(Mdb_Exception)
{
    vector<MdbAddress> t_addrList;
    int                t_newnum = 0;
    t_addrList.clear();
    DescPageInfo* t_descpageinfo = NULL;
    int t_iRet = -1, t_pos = 0;
    do
    {
        t_iRet = r_descPageInfos->getNewDescPageInfo(t_descpageinfo, t_pos);
        if (t_iRet == -1)
        {
            usleep(100000); //����100ms
        }
    }
    while (t_iRet == -1);
    try
    {
        do
        {
            //1. �ӱ�������ҳ������SLot
            t_descpageinfo->new_mem(m_memmgr, 1, t_newnum, t_addrList, false);
            if (t_newnum != 1)
            {
                //2.���ռ�����page
                size_t        t_pagepos;
                PageInfo*     t_ppage = NULL;
                if (m_dpagesMgr.allocatePage(m_memmgr, t_ppage, t_pagepos, false) == false)
                {
                    throw Mdb_Exception(__FILE__, __LINE__, "Undo��ռ�����,�޿���PAGE!");
                }
                else
                {
                    ShmPosition t_pageShmpos(m_spaceHeader.m_spaceCode, t_pagepos);
                    //3.�����뵽��page����r_descPageInfo
                    t_descpageinfo->addNewPage(m_memmgr, t_pos, t_pageShmpos, t_ppage, false);
                    continue;
                }
            }
            else
            {
                r_addr = t_addrList[0];
                break;
            }
        }
        while (1);
    }
    catch (Mdb_Exception& e)
    {
#ifdef _DEBUG_
        e.toString();
        cout << __FILE__ << __LINE__ << endl;
#endif
        //modified by gaojf 2010/11/19 12:50:32
        //t_descpageinfo->m_lockinfo=0;
        t_descpageinfo->unlock();
        throw e;
    }
    //modified by gaojf 2010/11/19 12:50:32
    //t_descpageinfo->m_lockinfo=0;
    t_descpageinfo->unlock();
}

void UndoMemMgr::deleteSlot(DescPageInfos*        r_pdescpages,
                            const ShmPosition&    r_addr) throw(Mdb_Exception)
{
    bool          t_bRet = true;
    ShmPosition   t_pagepos;
    PageInfo*     t_ppage = NULL;
    deleteSlot(r_pdescpages, r_addr, t_pagepos, t_ppage);
    /*
    if(t_ppage->m_slotNum == t_ppage->m_idleNum)
    { //��ҳ��Ϊ��,��黹����ռ�
      //1. ����ҳ�Ӹñ����������ͷ�
      r_pdescpages->freePage(this,t_pagepos.getOffSet(),t_ppage);
      //2. ����ҳ�����ռ���ж���
      m_dpagesMgr.freePage(m_memmgr,t_pagepos.getOffSet(),false);
    }
    */
}

//�ͷ�һ���ڵ���ڴ�:���ҳ���Ѿ�ȫ������,�ͷŸ���ռ�
void UndoMemMgr::deleteSlot(DescPageInfos* r_pdescpages,
                            const ShmPosition&   r_addr,
                            ShmPosition&   r_pagepos,
                            PageInfo*     &r_ppage) throw(Mdb_Exception)
{
    DescPageInfo* t_descpageinfo = NULL;
    bool t_idleflag = false;
    if (getPageBySlot(r_addr, r_pagepos, r_ppage) == false)
    {
        throw Mdb_Exception(__FILE__, __LINE__, "����SLot��ȡҳ��λ��ʧ��!");
    }
    bool t_bRet = true;
    do
    {
        t_bRet = r_pdescpages->getFreeDescPageInfo(r_ppage, t_descpageinfo);
        if (t_bRet == false)
        {
            usleep(100000); //����100ms
        }
    }
    while (t_bRet == false);
    try
    {
        t_descpageinfo->free_mem(m_memmgr, r_addr, r_pagepos, r_ppage, false, m_scn);
        t_idleflag = (r_ppage->m_slotNum == r_ppage->m_idleNum);
        if (t_idleflag == true)
        {
            //��ҳ��Ϊ��,��黹����ռ�
            r_pdescpages->freePageIdle(this, r_pagepos.getOffSet(), r_ppage);
        }
    }
    catch (Mdb_Exception& e)
    {
#ifdef _DEBUG_
        e.toString();
        cout << __FILE__ << __LINE__ << endl;
#endif
        //modified by gaojf 2010/11/19 12:51:09
        //t_descpageinfo->m_lockinfo=0;
        t_descpageinfo->unlock();
        throw e;
    }
    //modified by gaojf 2010/11/19 12:51:09
    //t_descpageinfo->m_lockinfo=0;
    t_descpageinfo->unlock();
    if (t_idleflag == true)
    {
        //1. ����ҳ�Ӹñ����������ͷ�
        r_pdescpages->freePage(this, r_pagepos.getOffSet(), r_ppage);
        //2. ����ҳ�����ռ���ж���
        m_dpagesMgr.freePage(m_memmgr, r_pagepos.getOffSet(), false);
    }
}
//����slotλ�û�ȡҳ��λ��
bool UndoMemMgr::getPageBySlot(const ShmPosition& r_slotPos, ShmPosition& r_pagepos, PageInfo * &r_ppageinfo)
{
    r_ppageinfo = NULL;
    size_t t_slotPos = r_slotPos.getOffSet();
    size_t t_pagepos;
    if (t_slotPos < m_spinfo->m_pageoffset || t_slotPos >= m_spinfo->m_redooffset)
    {
#ifdef _DEBUG_
        cout << "r_slotPos=" << r_slotPos << " > " << m_spinfo->m_pageoffset
             << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    if (m_dpagesMgr.getPagePosBySlotPos(t_slotPos, t_pagepos) == false) return false;
    r_ppageinfo = (PageInfo*)(m_spaceHeader.m_shmAddr + t_pagepos);
    r_pagepos.setValue(r_slotPos.getSpaceCode(), t_pagepos);
    return true;
}
bool UndoMemMgr::getDescbySlot(const ShmPosition& r_slotPos, TableDesc * &r_pundodesc)
{
    PageInfo* t_ppage = NULL;
    ShmPosition t_pagepos;
    if (getPageBySlot(r_slotPos, t_pagepos, t_ppage) == false)
    {
        return false;
    }
    t_ppage = (PageInfo*)(m_spaceHeader.m_shmAddr + t_pagepos.getOffSet());
    r_pundodesc = m_tbDescMgr.getElement(t_ppage->m_descpos);
    return true;
}
void UndoMemMgr::initUndoFixDesc(TableDef t_undotbdefs[T_UNDO_TYPE_UNDEF])
{
    for (int i = 0; i < T_UNDO_TYPE_UNDEF; ++i)
    {
        if ((T_UNDO_DESCTYPE)i == T_UNDO_TYPE_TABLEDATA) continue;
        TableDef& t_tbdef = t_undotbdefs[i];
        memset(&t_tbdef, 0, sizeof(TableDef));
        strcpy(t_tbdef.m_tableName, _UNDO_TABLE[i]);
        t_tbdef.m_tableType = UNDO_TABLE;
        t_tbdef.m_spaceNum = 1;
        strcpy(t_tbdef.m_spaceList[0], m_spaceHeader.m_spaceName);
        sprintf(t_tbdef.m_columnList[0].m_name, "value");
        t_tbdef.m_columnList[0].m_type = VAR_TYPE_VSTR;
        t_tbdef.m_columnList[0].m_len  = _UNDO_TABLE_VLEN[i];
        t_tbdef.m_columnNum = 1;
    }
}

//����Ϊ֧��Undo���������� ������Ʊ� ��Ҫ֧��
bool UndoMemMgr::createHashIndex(IndexDesc* r_idxDesc, ShmPosition& r_shmPos)
{
    bool   t_bRet = true;
    size_t t_offset = 0;
    SpaceMgrBase::lock();
    t_bRet = m_indexSegsMgr.createHashIndex(r_idxDesc, t_offset, m_scn);
    r_shmPos.setValue(m_spaceHeader.m_spaceCode, t_offset);
    SpaceMgrBase::unlock();
    return t_bRet;
}
//r_shmPos��ָ��Hash��ShmPositioin[]���׵�ַ
bool UndoMemMgr::dropHashIdex(const ShmPosition& r_shmPos)
{
    bool t_bRet = true;
    SpaceMgrBase::lock();
    t_bRet = m_indexSegsMgr.dropHashIdex(r_shmPos.getOffSet(), m_scn);
    SpaceMgrBase::unlock();
    return t_bRet;
}
//r_shmPos��ָ��Hash��ShmPositioin[]���׵�ַ
void UndoMemMgr::initHashSeg(const ShmPosition& r_shmPos)
{
    m_indexSegsMgr.initHashSeg(r_shmPos.getOffSet(), m_scn);
}

bool UndoMemMgr::getIndexDescByName(const char* r_indexname, IndexDesc* &r_pundoIdesc)
{
    IndexDesc t_undoIDesc;
    strcpy(t_undoIDesc.m_indexDef.m_indexName, r_indexname);
    SpaceMgrBase::lock();
    r_pundoIdesc = NULL;
    if (m_indexDescMgr.getElement(t_undoIDesc, r_pundoIdesc) == true)
    {
        SpaceMgrBase::unlock();
        return true;
    }
    SpaceMgrBase::unlock();
    return false;
}
bool UndoMemMgr::getTbDescByname(const char* r_undotablename, TableDesc* &r_pundodesc)
{
    TableDesc t_undotableDesc;
    strcpy(t_undotableDesc.m_tableDef.m_tableName, r_undotablename);
    SpaceMgrBase::lock();
    if (m_tbDescMgr.getElement(t_undotableDesc, r_pundodesc) == true)
    {
        SpaceMgrBase::unlock();
        return true;
    }
    SpaceMgrBase::unlock();
    return false;
}
void UndoMemMgr::createTable(const TableDef& r_undotableDef, TableDesc* &r_undotableDesc)
{
    TableDesc  t_undotableDesc;
    //1. ���ݱ�ṹ���壬����һ��TableDesc����
    t_undotableDesc.initByTableDef(r_undotableDef);
    //2. �жϸñ��Ƿ��Ѿ�����
    if (getTbDescByname(r_undotableDef.m_tableName, r_undotableDesc) == true)
    {
#ifdef _DEBUG_
        cout << "��" << r_undotableDef.m_tableName << " �Ѿ�����!����ʧ��!"
             << __FILE__ << __LINE__ << endl;
#endif
        throw Mdb_Exception(__FILE__, __LINE__, "�������Ѿ�����!");
    }
    //2. ��TableDesc��������������ռ���
    createUndoTbDesc(t_undotableDesc, r_undotableDesc);
}
void UndoMemMgr::dropTable(const char* r_tableName)
{
    TableDesc*  t_pundoDesc;
    T_NAMEDEF   t_tbname[3];
    if (strncasecmp(r_tableName, _UNDO_TBPREFIX, strlen(_UNDO_TBPREFIX)) == 0)   //undo cmp
    {
        strcpy(t_tbname[0], r_tableName);
    }
    else
    {
        sprintf(t_tbname[0], "%s%s", _UNDO_TBPREFIX, r_tableName);
    }
    if (strncasecmp(r_tableName, _TX_TBPREFIX, strlen(_TX_TBPREFIX)) == 0)   //tx cmp
    {
        strcpy(t_tbname[1], r_tableName);
    }
    else
    {
        sprintf(t_tbname[1], "%s%s", _TX_TBPREFIX, r_tableName);
    }
    char t_indexundo[50];
    sprintf(t_indexundo, "%s%s\0", _UNDO_TBPREFIX, _UNDO_TABLE[T_UNDO_TYPE_INDEXDATA]);
    if (strncasecmp(r_tableName, t_indexundo, strlen(t_indexundo)) == 0)   //node index  undo  cmp
    {
        strcpy(t_tbname[2], r_tableName);
    }
    else
    {
        sprintf(t_tbname[2], "%s%s%s", _UNDO_TBPREFIX, _UNDO_TABLE[T_UNDO_TYPE_INDEXDATA], r_tableName);
    }
    for (int i = 0; i < 3; i++)
    {
        //1. �жϸñ��Ƿ��Ѿ�����
        if (getTbDescByname(t_tbname[i], t_pundoDesc) == false)
        {
#ifdef _DEBUG_
            cout << "��" << t_tbname[i] << " ������!dropʧ��!"
                 << __FILE__ << __LINE__ << endl;
#endif
            throw Mdb_Exception(__FILE__, __LINE__, "��%s������!", t_tbname[i]);
        }
        TableDesc t_tabledesc;
        memcpy(&t_tabledesc, t_pundoDesc, sizeof(TableDesc));
        //2.�����������ɾ������
        for (int j = 0; j < t_tabledesc.m_indexNum; j++)
        {
            dropIndex(t_tabledesc.m_indexNameList[j]);
        }
        //2.truncateTable
        truncateTable(t_tbname[i]);
        //3.ɾ����������
        deleteUndoTbDesc(*t_pundoDesc);
    }
}
void UndoMemMgr::truncateTable(const char* r_tableName)
{
    T_NAMEDEF  t_undotablename;
    if (strncasecmp(r_tableName, _UNDO_TBPREFIX, strlen(_UNDO_TBPREFIX)) == 0 ||
            strncasecmp(r_tableName, _TX_TBPREFIX, strlen(_TX_TBPREFIX)) == 0)
    {
        strcpy(t_undotablename, r_tableName);
    }
    else
    {
        sprintf(t_undotablename, "%s%s", _UNDO_TBPREFIX, r_tableName);
    }
    //1. ȡ��Ӧ�ı�������
    //1. ��ȡ��������������������
    TableDesc* t_pTableDesc = NULL;
    if (getTbDescByname(t_undotablename, t_pTableDesc) == false)
    {
        throw Mdb_Exception(__FILE__, __LINE__, "������!");
    }
    //2. truncate��������
    for (int i = 0; i < t_pTableDesc->m_indexNum; i++)
    {
        truncateIndex(t_pTableDesc->m_indexNameList[i]);
    }
    //3. �ͷű�PAGE
    ShmPosition  t_pagePos, t_nextPagePos;
    PageInfo* t_pPageInfo;
    char*      t_phAddr;
    for (int j = 0; j < MDB_PARL_NUM; ++j)
    {
        t_pagePos = t_pTableDesc->m_pageInfo[j].m_firstPage;
        while (!(t_pagePos == NULL_SHMPOS))
        {
            t_pPageInfo = (PageInfo*)(m_spaceHeader.m_shmAddr + t_pagePos.getOffSet());
            t_nextPagePos = t_pPageInfo->m_next;
            if (m_dpagesMgr.freePage(m_memmgr, t_pagePos.getOffSet(), false) == false)
            {
                throw Mdb_Exception(__FILE__, __LINE__, "�ͷ�PAGEʧ��!");
            }
            t_pagePos = t_nextPagePos;
        }
        //4. ������������е�λ����Ϣ
        t_pTableDesc->m_pageInfo[j].initPos();
    }
    //5. m_recordNum���� add by chenm 2008-5-23
    t_pTableDesc->m_recordNum = 0;
}
void UndoMemMgr::createIndex(const IndexDef& r_idxDef, IndexDesc* &r_idxDesc)
{
    //������������Ƿ����
    if (getIndexDescByName(r_idxDef.m_indexName, r_idxDesc) == true)
    {
#ifdef _DEBUG_
        cout << "������" << r_idxDef.m_indexName << " �Ѿ�����!����ʧ��!"
             << __FILE__ << __LINE__ << endl;
#endif
        throw Mdb_Exception(__FILE__, __LINE__, "���������Ѿ�����!����ʧ��!");
    }
    bool t_bRet = true;
    IndexDesc t_indexDesc;
    //0.ȡ��Ӧ�ı�������
    TableDesc* t_pTableDesc = NULL;
    getTbDescByname(r_idxDef.m_tableName, t_pTableDesc);
    //1. ��������������
    t_indexDesc.initByIndeDef(r_idxDef);
    if (m_indexDescMgr.addElement(t_indexDesc, r_idxDesc, 1) == false)
    {
#ifdef _DEBUG_
        cout << "m_ctlMemMgr.addIndexDesc(t_tableDesc,r_idxDesc) false!"
             << __FILE__ << __LINE__ << endl;
#endif
        throw Mdb_Exception(__FILE__, __LINE__, "��������������ʧ��!");
    }
    r_idxDesc->setdescinfo('2', m_indexDescMgr.getOffset(r_idxDesc));
    //2. �����Hash�������򴴽�Hash�ṹ
    if (r_idxDef.m_indexType == HASH_INDEX ||
            r_idxDef.m_indexType == HASH_INDEX_NP || //add by gaojf 2012/5/8 16:52:50
            r_idxDef.m_indexType == HASH_TREE)
    {
        //r_shmPos��ָ��Hash��ShmPositioin[]���׵�ַ
        size_t t_idxhoffset;
        if (m_indexSegsMgr.createHashIndex(r_idxDesc, t_idxhoffset, m_scn) == false)
        {
            m_indexDescMgr.deleteElement(t_indexDesc, 1);
            throw Mdb_Exception(__FILE__, __LINE__, "����Hash������ʧ��!");
        }
        r_idxDesc->m_header.setValue(SpaceMgrBase::getSpaceCode(), t_idxhoffset);
    }
    else
    {
        r_idxDesc->m_header = NULL_SHMPOS;
    }
    //3. �ڶ�Ӧ�ı������������Ӹ�������Ϣ
    size_t  t_indexOffset = ((const char*)r_idxDesc) - getSpaceAddr();
    r_idxDesc->m_tablePos = ((const char*)t_pTableDesc) - getSpaceAddr();
    t_pTableDesc->addIndex(t_indexOffset, r_idxDef.m_indexName, m_scn);
}
void UndoMemMgr::dropIndex(const char* r_idxName)
{
    T_NAMEDEF t_tableName;
    IndexDesc* t_pIndexDesc = NULL;
    if (getIndexDescByName(r_idxName, t_pIndexDesc) == false)
    {
        throw Mdb_Exception(__FILE__, __LINE__, "����������!dropʧ��!");
    }
    strcpy(t_tableName, t_pIndexDesc->m_indexDef.m_tableName);
    //0. truncateIndex
    truncateIndex(r_idxName);
    //1. ��ȡ��������������������
    TableDesc* t_pTableDesc = NULL;
    getTbDescByname(t_tableName, t_pTableDesc);
    size_t t_indexOffSet = -1;
    for (int i = 0; i < t_pTableDesc->m_indexNum; i++)
    {
        if (strcasecmp(t_pTableDesc->m_indexNameList[i], r_idxName) == 0)
        {
            t_indexOffSet = t_pTableDesc->m_indexPosList[i];
        }
    }
    if (t_indexOffSet == -1)
    {
#ifdef _DEBUG_
        cout << "ȡ��Ӧ��������ʶ��ʧ��!" << endl;
#endif
        throw Mdb_Exception(__FILE__, __LINE__, "ȡ��Ӧ��������ʶ��ʧ��!");
    }
    //2. ɾ�����ж�Ӧ������λ����Ϣ
    if (t_pTableDesc->deleteIndex(t_indexOffSet, m_scn) == false)
    {
        throw Mdb_Exception(__FILE__, __LINE__, "��ɾ����Ӧ��������Ϣʧ��!");
    }
    //3. �����Hash����,�ͷ�Hash�ṹ�ռ�
    if (t_pIndexDesc->m_indexDef.m_indexType == HASH_INDEX ||
            t_pIndexDesc->m_indexDef.m_indexType == HASH_INDEX_NP || //add by gaojf 2012/5/8 16:53:44
            t_pIndexDesc->m_indexDef.m_indexType == HASH_TREE)
    {
        if (m_indexSegsMgr.dropHashIdex(t_pIndexDesc->m_header.getOffSet(), m_scn) == false)
        {
            // add by chenm 2009-1-8 23:29:13 ֱ��ɾ������������
            t_pIndexDesc->m_header = NULL_SHMPOS;
            m_indexDescMgr.deleteElement(*t_pIndexDesc, 1);
            throw Mdb_Exception(__FILE__, __LINE__, "����������ɾ���ɹ�,���ͷ�Hash�����ռ�ʧ��!");
        }
        t_pIndexDesc->m_header = NULL_SHMPOS;
    }
    //4. ɾ������������
    if (m_indexDescMgr.deleteElement(*t_pIndexDesc, 1) == false)
    {
        throw Mdb_Exception(__FILE__, __LINE__, "ɾ������������ʧ��!");
    }
}
void UndoMemMgr::truncateIndex(const char* r_idxName)
{
    IndexDesc* t_pIndexDesc = NULL;
    //0.��ȡ����������
    if (getIndexDescByName(r_idxName, t_pIndexDesc) == false)
    {
        throw Mdb_Exception(__FILE__, __LINE__, "����������!dropʧ��!");
    }
    //1. �ͷŶ�Ӧ����������Page����ռ�
    ShmPosition  t_pagePos, t_nextPagePos;
    PageInfo* t_pPageInfo;
    char*      t_phAddr;
    for (int i = 0; i < MDB_PARL_NUM; ++i)
    {
        t_pagePos = t_pIndexDesc->m_pageInfo[i].m_firstPage;
        while (!(t_pagePos == NULL_SHMPOS))
        {
            t_pPageInfo = (PageInfo*)(m_spaceHeader.m_shmAddr + t_pagePos.getOffSet());
            t_nextPagePos = t_pPageInfo->m_next;
            if (m_dpagesMgr.freePage(m_memmgr, t_pagePos.getOffSet(), false) == false)
            {
                throw Mdb_Exception(__FILE__, __LINE__, "�ͷ�PAGEʧ��!");
            }
            t_pagePos = t_nextPagePos;
        }
        //2. ��������������е�λ����Ϣ
        t_pIndexDesc->m_pageInfo[i].initPos();
    }
    //3. ���Hash�����е�Hash�ṹ��Ϣ
    if (t_pIndexDesc->m_indexDef.m_indexType == HASH_INDEX ||
            t_pIndexDesc->m_indexDef.m_indexType == HASH_INDEX_NP || //add by gaojf 2012/5/8 16:54:21
            t_pIndexDesc->m_indexDef.m_indexType == HASH_TREE)
    {
        m_indexSegsMgr.initHashSeg(t_pIndexDesc->m_header.getOffSet(), m_scn);
    }
    else
    {
        t_pIndexDesc->m_header = NULL_SHMPOS;
    }
}
void UndoMemMgr::allocateTableMem(TableDesc* &r_tableDesc, MdbAddress& r_addr)
{
    allocateSlot(r_tableDesc, r_addr);
}
void UndoMemMgr::freeTableMem(TableDesc* &r_tableDesc, const MdbAddress& r_addr)
{
    deleteSlot(r_tableDesc, r_addr.m_pos);
}
void UndoMemMgr::allocateIdxMem(IndexDesc* &r_pIndexDesc, MdbAddress& r_addr)
{
    allocateSlot(r_pIndexDesc, r_addr);
}
void UndoMemMgr::freeIdxMem(IndexDesc* &r_pindexDesc, const MdbAddress& r_addr)
{
    deleteSlot(r_pindexDesc, r_addr.m_pos);
}

bool UndoMemMgr::getTableDefList(vector<TableDef> &r_tabledefs)
{
    bool t_bRet = true;
    try
    {
        SpaceMgrBase::lock();
    }
    catch (Mdb_Exception& e)
    {
        t_bRet = false;
        return t_bRet;
    }
    r_tabledefs.clear();
    vector<TableDesc> r_tableDescList;
    if (m_tbDescMgr.getElements(r_tableDescList) == false)
    {
        t_bRet = false;
    }
    for (int i = 0; i < r_tableDescList.size(); i++)
    {
        r_tabledefs.push_back(r_tableDescList[i].m_tableDef);
    }
    SpaceMgrBase::unlock();
    return t_bRet;
}

bool UndoMemMgr::getIndexDefList(vector<IndexDef> &r_indexdefs)
{
    bool t_bRet = true;
    try
    {
        SpaceMgrBase::lock();
    }
    catch (Mdb_Exception& e)
    {
        t_bRet = false;
        return t_bRet;
    }
    r_indexdefs.clear();
    vector<IndexDesc> r_indexDescList;
    if (m_indexDescMgr.getElements(r_indexDescList) == false)
    {
        t_bRet = false;
    }
    for (int i = 0; i < r_indexDescList.size(); i++)
    {
        r_indexdefs.push_back(r_indexDescList[i].m_indexDef);
    }
    SpaceMgrBase::unlock();
    return t_bRet;
}

void UndoMemMgr::deleteUndoSpace()
{
    deleteSpace(m_spaceHeader);
}

float UndoMemMgr::getTableSpaceUsedPercent()
{
    size_t t_totalsize, t_freesize;
    m_dpagesMgr.getPagesUseInfo(t_totalsize, t_freesize);
    return 1.0 - (float)t_freesize / t_totalsize;
}

void UndoMemMgr::getTableSpaceUsedPercent(map<string, float> &vUserdPercent, const char* cTableSpaceName)
{
    float fPercent;
    fPercent = getTableSpaceUsedPercent();
    vUserdPercent.insert(map<string, float>::value_type(getSpaceName(), fPercent));
}

bool UndoMemMgr::dumpSpaceInfo(ostream& r_os)
{
    r_os << "Undo TableSpace Info: " << endl;
    SpaceMgrBase::dumpSpaceInfo(r_os);
    m_spinfo->dumpInfo(r_os);
    r_os << "---����������Ϣ-------" << endl;
    m_tbDescMgr.dumpInfo(r_os);
    r_os << "---������������Ϣ-------" << endl;
    m_indexDescMgr.dumpInfo(r_os);
    m_indexSegsMgr.dumpInfo(r_os);
    m_dpagesMgr.dumpPagesInfo(r_os);
    return true;
}

