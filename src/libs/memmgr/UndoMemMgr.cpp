#include "UndoMemMgr.h"
#include "lockMgr/TransactionMgr.h"

bool UndoMemMgr::initialize(SpaceInfo& r_spaceInfo)
{
    SpaceMgrBase::initialize(r_spaceInfo);
    return true;
}
/**
 * insert  插入影子信息: 返回UNDO记录的偏移量.
 * @param  r_type         : 数据类型
 * @param  r_tbname       : 原数据表名(只有是插入表数据SLOT有用)
 * @param  r_doffset      : 原始数据的偏移量
 * @param  r_scn          : 时间戳
 * @param  r_value        : 值
 * @param  r_prevUndopos  : 前个版本的影子偏移(空表示无)
 * @return 正常返回UNDO记录的偏移量。非正常，则抛出异常
 */
UndoPosition UndoMemMgr::insert(const T_UNDO_DESCTYPE&  r_type   ,
                                const T_NAMEDEF&        r_tbname ,
                                const ShmPosition&      r_doffset,
                                const T_SCN&            r_scn    ,
                                const char*             r_value  ,
                                const UndoPosition&     r_prevUndopos) throw(Mdb_Exception)
{
    TableDesc*    t_pundodesc = NULL;
    //1. 获取表描述符
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
        //1. 判断影子信息是否和前一个一致，如果一致则不需再重写
        size_t t_preoffset = r_prevUndopos.getOffSet();
        if (t_preoffset != 0)
        {
            Undo_Slot* t_preslot = (Undo_Slot*)(m_spaceHeader.m_shmAddr + t_preoffset);
            if (memcmp((char*)(t_preslot) + UNDO_VAL_OFFSET, r_value, r_pundodesc->m_valueSize) == 0)
            {
                return r_prevUndopos; //不需要重写直接返回上次影子信息
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
    //2.设置值
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
 * free  回收影子信息：对指定的节点对应的影子进行回收.
 * @param  r_type         : 数据类型
 * @param  r_tbname       : 原数据表名(只有是插入表数据SLOT有用)
 * @param  r_undopos      : 影子偏移量
 */
void UndoMemMgr::free(const T_UNDO_DESCTYPE&  r_type   ,
                      const T_NAMEDEF&        r_tbname ,
                      const T_SCN&            r_scn    ,
                      const int&              r_flag   ,
                      UndoPosition&     r_undopos) throw(Mdb_Exception)
{
    TableDesc*    t_pundodesc = NULL;
    //1. 获取表描述符
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
                //越界了
                break;
            }
            t_pundoslot = (Undo_Slot*)(m_spaceHeader.m_shmAddr + t_slotoffset);
            t_nextslot = t_pundoslot->m_next;
            if ((t_pundoslot->m_lockinfo & 0x80) == 0) break; //该SLOT未使用
            if (t_flag == 2)
            {
                //前面已经有删除的了
                t_slotPos_shm.setValue(m_spaceHeader.m_spaceCode, t_slotoffset);
                deleteSlot(r_tabledesc, t_slotPos_shm);
                --(r_tabledesc->m_recordNum);
            }
            else if (t_flag == 1 && t_fisrtflag == true)
            {
                //第一个节点 不删除
                t_preslot_pos = &(t_pundoslot->m_next);
                t_flag = 0; //后面可能需要删除
            }
            else if (t_pundoslot->m_scn > r_scn)
            {
                //不需要删除
                t_preslot_pos = &(t_pundoslot->m_next);
            }
            else
            {
                //该节点需要删除
                t_slotPos_shm.setValue(m_spaceHeader.m_spaceCode, t_slotoffset);
                deleteSlot(r_tabledesc, t_slotPos_shm);
                --(r_tabledesc->m_recordNum);
                t_flag = 2;
                *t_preslot_pos = NULL_UNDOPOS; //只有删除的第一次需要将上个节点的位置清楚
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
 * getUndoInfo    获取影子信息
 * @param  r_type         : 数据类型
 * @param  r_tbname       : 原数据表名(只有是插入表数据SLOT有用)
 * @param  r_scn          : 时间戳
 * @param  r_undopos      : 影子偏移量
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
                //越界了
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
 * getUndoInfo    获取影子信息
 * @param  r_scn1/r_scn2  : 时间戳(r_scn1,r_scn2)之间的
 * @param  r_undopos      : 影子偏移量
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

//根据表空间定义信息,创建表空间
bool UndoMemMgr::createUndoSpace(SpaceInfo& r_spaceInfo)
{
    if (createSpace(m_spaceHeader) == false) //不需要创建对应的文件
    {
#ifdef _DEBUG_
        cout << "createSpace false!" << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    r_spaceInfo = *(getSpaceInfo());
    return true;
}

//初始化表空间
bool UndoMemMgr::initSpaceInfo(const size_t& r_descnum,
                               const size_t& r_redosize,
                               const size_t& r_indexsize)
{
    //初始化
    size_t  t_offSet = 0;
    size_t  t_spinfo_offset = 0;
    //1. 初始化控制区表空间头m_spaceHeader信息
    memcpy(m_pSpHeader, &m_spaceHeader, sizeof(SpaceInfo));
    t_offSet += sizeof(SpaceInfo);
    //1-2 预留m_spinfo空间
    m_spinfo = (Undo_spaceinfo*)(m_spaceHeader.m_shmAddr + t_offSet);
    m_spinfo->m_tablenums = r_descnum;
    m_spinfo->m_redosize  = r_redosize;
    m_spinfo->m_indexsize = r_indexsize;
    if (m_pSpHeader->m_size <= m_spinfo->m_pageoffset + m_spinfo->m_redosize + m_spinfo->m_indexsize)
    {
#ifdef _DEBUG_
        cout << "m_pSpHeader->m_size过小!" << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    t_offSet += sizeof(Undo_spaceinfo);
    //2.初始化UNDO表描述符信息
    if (m_tbDescMgr.initElements(m_spaceHeader.m_shmAddr, t_offSet, m_spinfo->m_tablenums) == false)
    {
#ifdef _DEBUG_
        cout << "m_tbDescMgr.initElements false!" << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    t_offSet += m_tbDescMgr.getSize();
    //初始化Undo索引描述符
    if (m_indexDescMgr.initElements(m_spaceHeader.m_shmAddr, t_offSet, m_spinfo->m_tablenums) == false)
    {
#ifdef _DEBUG_
        cout << "m_indexDescMgr.initElements false!" << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    t_offSet += m_indexDescMgr.getSize();
    //初始化m_spinfo信息
    m_spinfo->m_pageoffset = t_offSet;
    m_spinfo->m_pagetsize = m_pSpHeader->m_size - m_spinfo->m_pageoffset
                            - m_spinfo->m_redosize - m_spinfo->m_indexsize;
    if (m_spinfo->m_pagetsize <= 0)
    {
#ifdef _DEBUG_
        cout << "page空间不够，请调整配置大小" << __FILE__ << __LINE__ << endl;
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

//获取REDO内存的信息。REDO的内存管理在REDO内部管理
void UndoMemMgr::getRedobufInfo(char * &r_addr, size_t& r_size)
{
    r_addr = m_redoaddr;
    r_size = m_spinfo->m_redosize;
}

//attach方式init
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
    //1. 跳过表空间头m_spaceHeader信息
    t_offSet += sizeof(SpaceInfo);
    //2. attach初始化m_spinfo
    m_spinfo = (Undo_spaceinfo*)(m_spaceHeader.m_shmAddr + t_offSet);
    t_offSet += sizeof(Undo_spaceinfo);
    //3.初始化表描述符信息
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
    //初始化
    T_NAMEDEF  t_undotbname;
    for (int i = (int)T_UNDO_TYPE_DESC; i < (int)T_UNDO_TYPE_UNDEF; ++i)
    {
        //if ((i == (int)T_UNDO_TYPE_TABLEDATA) || (i == (int)T_UNDO_TYPE_INDEXDATA))
        if (i == (int)T_UNDO_TYPE_TABLEDATA)
        {
            //数据表或索引slot对应的undo表，不在此创建
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

//根据表定义初始化对应的Undo表描述符
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

//根据表定义初始话对应的TX表描述符
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

//根据原数据表创建索引slot对应的Undo表 2012/9/18 7:51:53 gaojf
bool UndoMemMgr::createUndoIdxDesc(const TableDesc& r_tabledesc)
{
    TableDesc  t_undodesc;
    TableDef   t_undotbdefs[T_UNDO_TYPE_UNDEF]; ///<固定部分描述符结构
    TableDesc* t_pundodesc = NULL;
    //1. 初始化 固定部分描述符结构 ht_undotbdefs
    initUndoFixDesc(t_undotbdefs);
    sprintf(t_undotbdefs[T_UNDO_TYPE_INDEXDATA].m_tableName, "%s%s", _UNDO_TABLE[T_UNDO_TYPE_INDEXDATA],
            r_tabledesc.m_tableDef.m_tableName);
    initUndoDesc(t_undotbdefs[T_UNDO_TYPE_INDEXDATA], t_undodesc);
    createUndoTbDesc(t_undodesc, t_pundodesc);
    return true;
}

//插入UNDO描述符信息
bool UndoMemMgr::createUndoDescs(const vector<TableDesc> &r_tabledescs)
{
    TableDesc  t_undodesc;
    TableDef   t_undotbdefs[T_UNDO_TYPE_UNDEF];///<固定部分描述符结构
    TableDesc* t_pundodesc = NULL;
    //1. 初始化 固定部分描述符结构 t_undotbdefs
    initUndoFixDesc(t_undotbdefs);
    try
    {
        //创建固定部分UNDO表描述符
        for (int i = (int)T_UNDO_TYPE_DESC; i < (int)T_UNDO_TYPE_UNDEF; ++i)
        {
            if ((i == (int)T_UNDO_TYPE_TABLEDATA) || (i == (int)T_UNDO_TYPE_INDEXDATA))
            {
                //数据表 索引表，不在此创建
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
        //创建数据部分表描述符
        for (vector<TableDesc>::const_iterator t_itr = r_tabledescs.begin();
                t_itr != r_tabledescs.end(); ++t_itr)
        {
            initUndoDesc(t_itr->m_tableDef, t_undodesc);
            createUndoTbDesc(t_undodesc, t_pundodesc);
            createTxDesc(*t_itr);
            createUndoIdxDesc(*t_itr); //创建索引slot在Undo中的表：每个实体表一个undo表 2012/9/18 7:35:54
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
//插入一个表描述符
bool UndoMemMgr::createUndoDesc(const TableDesc& r_tabledesc)
{
    TableDesc  t_undodesc, *t_pundodesc;
    initUndoDesc(r_tabledesc.m_tableDef, t_undodesc);
    createUndoTbDesc(t_undodesc, t_pundodesc);
    return true;
}
//插入一个表对应的TX表 并创建索引
bool UndoMemMgr::createTxDesc(const TableDesc& r_tabledesc)
{
    TableDesc t_txdesc, *t_ptxdesc;
    initTxDesc(r_tabledesc.m_tableDef, t_txdesc);
    createUndoTbDesc(t_txdesc, t_ptxdesc);
    //创建第一个索引
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

//删除一个表描述符
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
//删除一个表描述符对应的tx表 并drop索引
bool UndoMemMgr::deleteTxDesc(const TableDesc& r_tabledesc)
{
    return true;
}
//插入一个表描述符
void UndoMemMgr::createUndoTbDesc(const TableDesc& r_undodesc, TableDesc* &r_pundodesc)
{
    SpaceMgrBase::lock();
    try
    {
        //cout<<"---表描述符信息-------"<<endl;
        //m_tbDescMgr.dumpInfo(cout);
        if (m_tbDescMgr.getElement(r_undodesc, r_pundodesc) == true)
        {
#ifdef _DEBUG_
            cout << "UNDO描述符已经存在{" << r_undodesc << "}!" << __FILE__ << __LINE__ << endl;
#endif
            throw Mdb_Exception(__FILE__, __LINE__, "UNDO描述符{%s}已经存在!",
                                r_undodesc.m_tableDef.m_tableName);
        }
        int t_flag = 1; //支持表描述符重用
        if (m_tbDescMgr.addElement(r_undodesc, r_pundodesc, t_flag) == false)
        {
            throw Mdb_Exception(__FILE__, __LINE__, "m_tbDescMgr.addElement失败!");
        }
        r_pundodesc->setdescinfo('1', m_tbDescMgr.getOffset(r_pundodesc));
        //cout<<"---表描述符信息-------"<<endl;
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
//删除一个表描述符
void UndoMemMgr::deleteUndoTbDesc(const TableDesc& r_undodesc)
{
    TableDesc* t_pundodesc = NULL;
    SpaceMgrBase::lock();
    try
    {
        int t_flag = 1; //支持表描述符重用
        if (m_tbDescMgr.deleteElement(r_undodesc, t_flag) == false)
        {
            throw Mdb_Exception(__FILE__, __LINE__, "m_tbDescMgr.deleteElement失败!");
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

//获取一个表描述符
void UndoMemMgr::getUndoDesc(const T_UNDO_DESCTYPE&  r_type, const char* r_tablename, TableDesc* &r_pundodesc)
{
    TableDesc t_undodesc;
    if (r_type == T_UNDO_TYPE_TABLEDATA)
    {
        sprintf(t_undodesc.m_tableDef.m_tableName, "_Undo_%s", r_tablename);
    }
    //索引描述符根据表名_Undo__i_+表名获取 2012/9/18 7:39:56 gaojf
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
        throw Mdb_Exception(__FILE__, __LINE__, "UNDO描述符类型{%d}不存在!",
                            (int)r_type);
    }
    SpaceMgrBase::lock();
    if (m_tbDescMgr.getElement(t_undodesc, r_pundodesc) == false)
    {
        SpaceMgrBase::unlock();
        throw Mdb_Exception(__FILE__, __LINE__, "UNDO描述符类型{%s}不存在!",
                            t_undodesc.m_tableDef.m_tableName);
    }
    SpaceMgrBase::unlock();
}

//申请一个节点的内存
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
            usleep(100000); //休眠100ms
        }
    }
    while (t_iRet == -1);
    try
    {
        do
        {
            //1. 从表中已有页面申请SLot
            t_descpageinfo->new_mem(m_memmgr, 1, t_newnum, t_addrList, false);
            if (t_newnum != 1)
            {
                //2.向表空间申请page
                size_t        t_pagepos;
                PageInfo*     t_ppage = NULL;
                if (m_dpagesMgr.allocatePage(m_memmgr, t_ppage, t_pagepos, false) == false)
                {
                    throw Mdb_Exception(__FILE__, __LINE__, "Undo表空间已满,无空闲PAGE!");
                }
                else
                {
                    ShmPosition t_pageShmpos(m_spaceHeader.m_spaceCode, t_pagepos);
                    //3.将申请到得page加入r_descPageInfo
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
    { //该页面为空,则归还给表空间
      //1. 将该页从该表描述符中释放
      r_pdescpages->freePage(this,t_pagepos.getOffSet(),t_ppage);
      //2. 将该页加入表空间空闲队列
      m_dpagesMgr.freePage(m_memmgr,t_pagepos.getOffSet(),false);
    }
    */
}

//释放一个节点的内存:如果页面已经全部空闲,释放给表空间
void UndoMemMgr::deleteSlot(DescPageInfos* r_pdescpages,
                            const ShmPosition&   r_addr,
                            ShmPosition&   r_pagepos,
                            PageInfo*     &r_ppage) throw(Mdb_Exception)
{
    DescPageInfo* t_descpageinfo = NULL;
    bool t_idleflag = false;
    if (getPageBySlot(r_addr, r_pagepos, r_ppage) == false)
    {
        throw Mdb_Exception(__FILE__, __LINE__, "根据SLot获取页面位置失败!");
    }
    bool t_bRet = true;
    do
    {
        t_bRet = r_pdescpages->getFreeDescPageInfo(r_ppage, t_descpageinfo);
        if (t_bRet == false)
        {
            usleep(100000); //休眠100ms
        }
    }
    while (t_bRet == false);
    try
    {
        t_descpageinfo->free_mem(m_memmgr, r_addr, r_pagepos, r_ppage, false, m_scn);
        t_idleflag = (r_ppage->m_slotNum == r_ppage->m_idleNum);
        if (t_idleflag == true)
        {
            //该页面为空,则归还给表空间
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
        //1. 将该页从该表描述符中释放
        r_pdescpages->freePage(this, r_pagepos.getOffSet(), r_ppage);
        //2. 将该页加入表空间空闲队列
        m_dpagesMgr.freePage(m_memmgr, r_pagepos.getOffSet(), false);
    }
}
//根据slot位置获取页面位置
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

//以下为支持Undo索引而增加 事务控制表 需要支持
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
//r_shmPos是指向Hash中ShmPositioin[]的首地址
bool UndoMemMgr::dropHashIdex(const ShmPosition& r_shmPos)
{
    bool t_bRet = true;
    SpaceMgrBase::lock();
    t_bRet = m_indexSegsMgr.dropHashIdex(r_shmPos.getOffSet(), m_scn);
    SpaceMgrBase::unlock();
    return t_bRet;
}
//r_shmPos是指向Hash中ShmPositioin[]的首地址
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
    //1. 根据表结构定义，生成一个TableDesc对象
    t_undotableDesc.initByTableDef(r_undotableDef);
    //2. 判断该表是否已经存在
    if (getTbDescByname(r_undotableDef.m_tableName, r_undotableDesc) == true)
    {
#ifdef _DEBUG_
        cout << "表：" << r_undotableDef.m_tableName << " 已经存在!创建失败!"
             << __FILE__ << __LINE__ << endl;
#endif
        throw Mdb_Exception(__FILE__, __LINE__, "重名表已经存在!");
    }
    //2. 将TableDesc对象加入控制区表空间中
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
        //1. 判断该表是否已经存在
        if (getTbDescByname(t_tbname[i], t_pundoDesc) == false)
        {
#ifdef _DEBUG_
            cout << "表：" << t_tbname[i] << " 不存在!drop失败!"
                 << __FILE__ << __LINE__ << endl;
#endif
            throw Mdb_Exception(__FILE__, __LINE__, "表%s不存在!", t_tbname[i]);
        }
        TableDesc t_tabledesc;
        memcpy(&t_tabledesc, t_pundoDesc, sizeof(TableDesc));
        //2.如果有索引，删除索引
        for (int j = 0; j < t_tabledesc.m_indexNum; j++)
        {
            dropIndex(t_tabledesc.m_indexNameList[j]);
        }
        //2.truncateTable
        truncateTable(t_tbname[i]);
        //3.删除表描述符
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
    //1. 取对应的表描述符
    //1. 获取表描述符和索引描述符
    TableDesc* t_pTableDesc = NULL;
    if (getTbDescByname(t_undotablename, t_pTableDesc) == false)
    {
        throw Mdb_Exception(__FILE__, __LINE__, "表不存在!");
    }
    //2. truncate所有索引
    for (int i = 0; i < t_pTableDesc->m_indexNum; i++)
    {
        truncateIndex(t_pTableDesc->m_indexNameList[i]);
    }
    //3. 释放表PAGE
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
                throw Mdb_Exception(__FILE__, __LINE__, "释放PAGE失败!");
            }
            t_pagePos = t_nextPagePos;
        }
        //4. 清除表描述符中的位置信息
        t_pTableDesc->m_pageInfo[j].initPos();
    }
    //5. m_recordNum置零 add by chenm 2008-5-23
    t_pTableDesc->m_recordNum = 0;
}
void UndoMemMgr::createIndex(const IndexDef& r_idxDef, IndexDesc* &r_idxDesc)
{
    //检查重名索引是否存在
    if (getIndexDescByName(r_idxDef.m_indexName, r_idxDesc) == true)
    {
#ifdef _DEBUG_
        cout << "索引：" << r_idxDef.m_indexName << " 已经存在!创建失败!"
             << __FILE__ << __LINE__ << endl;
#endif
        throw Mdb_Exception(__FILE__, __LINE__, "重名索引已经存在!创建失败!");
    }
    bool t_bRet = true;
    IndexDesc t_indexDesc;
    //0.取对应的表描述符
    TableDesc* t_pTableDesc = NULL;
    getTbDescByname(r_idxDef.m_tableName, t_pTableDesc);
    //1. 创建索引描述符
    t_indexDesc.initByIndeDef(r_idxDef);
    if (m_indexDescMgr.addElement(t_indexDesc, r_idxDesc, 1) == false)
    {
#ifdef _DEBUG_
        cout << "m_ctlMemMgr.addIndexDesc(t_tableDesc,r_idxDesc) false!"
             << __FILE__ << __LINE__ << endl;
#endif
        throw Mdb_Exception(__FILE__, __LINE__, "创建索引描述符失败!");
    }
    r_idxDesc->setdescinfo('2', m_indexDescMgr.getOffset(r_idxDesc));
    //2. 如果是Hash索引，则创建Hash结构
    if (r_idxDef.m_indexType == HASH_INDEX ||
            r_idxDef.m_indexType == HASH_INDEX_NP || //add by gaojf 2012/5/8 16:52:50
            r_idxDef.m_indexType == HASH_TREE)
    {
        //r_shmPos是指向Hash中ShmPositioin[]的首地址
        size_t t_idxhoffset;
        if (m_indexSegsMgr.createHashIndex(r_idxDesc, t_idxhoffset, m_scn) == false)
        {
            m_indexDescMgr.deleteElement(t_indexDesc, 1);
            throw Mdb_Exception(__FILE__, __LINE__, "创建Hash索引区失败!");
        }
        r_idxDesc->m_header.setValue(SpaceMgrBase::getSpaceCode(), t_idxhoffset);
    }
    else
    {
        r_idxDesc->m_header = NULL_SHMPOS;
    }
    //3. 在对应的表描述符中增加该索引信息
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
        throw Mdb_Exception(__FILE__, __LINE__, "索引不存在!drop失败!");
    }
    strcpy(t_tableName, t_pIndexDesc->m_indexDef.m_tableName);
    //0. truncateIndex
    truncateIndex(r_idxName);
    //1. 获取表描述符和索引描述符
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
        cout << "取对应的索引标识符失败!" << endl;
#endif
        throw Mdb_Exception(__FILE__, __LINE__, "取对应的索引标识符失败!");
    }
    //2. 删除表中对应的索引位置信息
    if (t_pTableDesc->deleteIndex(t_indexOffSet, m_scn) == false)
    {
        throw Mdb_Exception(__FILE__, __LINE__, "表删除对应的索引信息失败!");
    }
    //3. 如果是Hash索引,释放Hash结构空间
    if (t_pIndexDesc->m_indexDef.m_indexType == HASH_INDEX ||
            t_pIndexDesc->m_indexDef.m_indexType == HASH_INDEX_NP || //add by gaojf 2012/5/8 16:53:44
            t_pIndexDesc->m_indexDef.m_indexType == HASH_TREE)
    {
        if (m_indexSegsMgr.dropHashIdex(t_pIndexDesc->m_header.getOffSet(), m_scn) == false)
        {
            // add by chenm 2009-1-8 23:29:13 直接删除索引描述符
            t_pIndexDesc->m_header = NULL_SHMPOS;
            m_indexDescMgr.deleteElement(*t_pIndexDesc, 1);
            throw Mdb_Exception(__FILE__, __LINE__, "索引描述符删除成功,但释放Hash索引空间失败!");
        }
        t_pIndexDesc->m_header = NULL_SHMPOS;
    }
    //4. 删除索引描述符
    if (m_indexDescMgr.deleteElement(*t_pIndexDesc, 1) == false)
    {
        throw Mdb_Exception(__FILE__, __LINE__, "删除索引描述符失败!");
    }
}
void UndoMemMgr::truncateIndex(const char* r_idxName)
{
    IndexDesc* t_pIndexDesc = NULL;
    //0.获取索引描述符
    if (getIndexDescByName(r_idxName, t_pIndexDesc) == false)
    {
        throw Mdb_Exception(__FILE__, __LINE__, "索引不存在!drop失败!");
    }
    //1. 释放对应的索引数据Page给表空间
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
                throw Mdb_Exception(__FILE__, __LINE__, "释放PAGE失败!");
            }
            t_pagePos = t_nextPagePos;
        }
        //2. 清除索引描述符中的位置信息
        t_pIndexDesc->m_pageInfo[i].initPos();
    }
    //3. 清除Hash索引中的Hash结构信息
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
    r_os << "---表描述符信息-------" << endl;
    m_tbDescMgr.dumpInfo(r_os);
    r_os << "---索引描述符信息-------" << endl;
    m_indexDescMgr.dumpInfo(r_os);
    m_indexSegsMgr.dumpInfo(r_os);
    m_dpagesMgr.dumpPagesInfo(r_os);
    return true;
}

