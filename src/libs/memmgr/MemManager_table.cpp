#include "MemManager.h"
#include <iostream>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include "InstanceFactory.h"

bool MemManager::createTable(const TableDef& r_tableDef, TableDesc* &r_tableDesc)
{
    bool t_existFlag = false;
    try
    {
        //0. ����Ƿ���ͬ�������
        if (getTableDescByName(r_tableDef.m_tableName, r_tableDesc) == true)
        {
#ifdef _DEBUG_
            cout << "��" << r_tableDef.m_tableName << " �Ѿ�����!����ʧ��!"
                 << __FILE__ << __LINE__ << endl;
#endif
            t_existFlag = true;
            throw Mdb_Exception(__FILE__, __LINE__, "�������Ѿ�����!");
        }
    }
    catch (Mdb_Exception& e)
    {
        if (t_existFlag == true)
        {
            throw e;
        }
        else
        {
            //������Ϊ���������
        }
    }
    lockddl();
    //1. ���ݱ�ṹ���壬����һ��TableDesc����
    TableDesc  t_tableDesc;
    try
    {
        t_tableDesc.initByTableDef(r_tableDef);
        //2. ��TableDesc��������������ռ���
        T_SCN t_scn;
        getscn(t_scn);
        if (m_ctlMemMgr.addTableDesc(t_tableDesc, r_tableDesc, t_scn) == false)
        {
#ifdef _DEBUG_
            cout << "m_ctlMemMgr.addTableDesc(t_tableDesc,r_tableDesc) false!"
                 << __FILE__ << __LINE__ << endl;
#endif
            throw Mdb_Exception(__FILE__, __LINE__, "������������ʧ��!");
        }
        r_tableDesc->setdescinfo('1', getdescOffset('1', r_tableDesc));
        m_undoMemMgr.createUndoDesc(*r_tableDesc);
        m_undoMemMgr.createUndoIdxDesc(*r_tableDesc);
        m_undoMemMgr.createTxDesc(*r_tableDesc);
    }
    catch (Mdb_Exception& e2)
    {
        unlockddl();
        throw e2;
    }
    unlockddl();
    return true;
}
bool MemManager::getTableDescList(vector<TableDesc>& r_tableDescList)
{
    if (m_ctlMemMgr.getTableDescList(r_tableDescList) == false)
    {
        throw Mdb_Exception(__FILE__, __LINE__, "ȡ���������б�ʧ��!");
        return false;
    }
    return true;
}
bool MemManager::getTableDefList(vector<TableDef>& r_tableDefList)
{
    vector<TableDesc> t_tableDescList;
    r_tableDefList.clear();
    if (m_ctlMemMgr.getTableDescList(t_tableDescList) == false)
    {
        throw Mdb_Exception(__FILE__, __LINE__, "ȡ�����б�ʧ��!");
        return false;
    }
    for (vector<TableDesc>::iterator t_itr = t_tableDescList.begin();
            t_itr != t_tableDescList.end(); t_itr++)
    {
        r_tableDefList.push_back(t_itr->m_tableDef);
    }
    return true;
}
bool MemManager::getTableDescByName(const char* r_tableName, TableDesc* &r_pTableDesc)
{
    if (m_ctlMemMgr.getTableDescByName(r_tableName, r_pTableDesc) == false)
    {
        char t_errMsg[256];
        sprintf(t_errMsg, "���ݱ���{%s}ȡ��������ʧ��!", r_tableName);
        throw Mdb_Exception(__FILE__, __LINE__, t_errMsg);
        return false;
    }
    return true;
}

bool MemManager::getIndexDescByName(const char* r_indexName, IndexDesc* &r_pIndexDesc)
{
    if (m_ctlMemMgr.getIndexDescByName(r_indexName, r_pIndexDesc) == false)
    {
        throw Mdb_Exception(__FILE__, __LINE__, "����������ȡ����������ʧ��!");
        return false;
    }
    return true;
}
bool MemManager::createIndex(const IndexDef& r_idxDef, IndexDesc* &r_idxDesc)
{
    bool t_existFlag = false;
    try
    {
        //������������Ƿ����
        getIndexDescByName(r_idxDef.m_indexName, r_idxDesc);
#ifdef _DEBUG_
        cout << "������" << r_idxDef.m_indexName << " �Ѿ�����!����ʧ��!"
             << __FILE__ << __LINE__ << endl;
#endif
        t_existFlag = true;
        throw Mdb_Exception(__FILE__, __LINE__, "���������Ѿ�����!����ʧ��!");
    }
    catch (Mdb_Exception& e)
    {
        //�������������ڣ���ʾ�������
        if (t_existFlag == true)
        {
            throw e;
        }
        else
        {
            //������Ϊ���������
        }
    }
    lockddl();
    T_SCN t_scn;
    bool t_bRet = true;
    IndexDesc t_indexDesc;
    //0.ȡ��Ӧ�ı�������
    TableDesc* t_pTableDesc = NULL;
    getTableDescByName(r_idxDef.m_tableName, t_pTableDesc);
    //1. ��������������
    t_indexDesc.initByIndeDef(r_idxDef);
    try
    {
        getscn(t_scn);
        if (m_ctlMemMgr.addIndexDesc(t_indexDesc, r_idxDesc, t_scn) == false)
        {
#ifdef _DEBUG_
            cout << "m_ctlMemMgr.addIndexDesc(t_tableDesc,r_idxDesc) false!"
                 << __FILE__ << __LINE__ << endl;
#endif
            t_bRet = false;
            throw Mdb_Exception(__FILE__, __LINE__, "��������������ʧ��!");
        }
        r_idxDesc->setdescinfo('2', getdescOffset('2', r_idxDesc));
        //2. �����Hash�������򴴽�Hash�ṹ
        if (r_idxDef.m_indexType == HASH_INDEX ||
                r_idxDef.m_indexType == HASH_INDEX_NP || //add by gaojf 2012/5/8 16:42:58
                r_idxDef.m_indexType == HASH_TREE ||
                r_idxDef.m_indexType == BITMAP_INDEX) // add by chenm 2010-09-09 12:43:39
        {
            //r_shmPos��ָ��Hash��ShmPositioin[]���׵�ַ
            if (m_indexMemMgr.createHashIndex(r_idxDesc, r_idxDesc->m_header, t_scn) == false)
            {
                m_ctlMemMgr.deleteIndexDesc(t_indexDesc, t_scn);
#ifdef _DEBUG_
                cout << "m_indexMemMgr.createHashIndex(r_idxDesc,t_IndexHashPos) false!"
                     << __FILE__ << __LINE__ << endl;
#endif
                t_bRet = false;
                throw Mdb_Exception(__FILE__, __LINE__, "����Hash������ʧ��!");
            }
        }
        else
        {
            r_idxDesc->m_header = NULL_SHMPOS;
        }
        //3. �ڶ�Ӧ�ı������������Ӹ�������Ϣ
        size_t  t_indexOffset = m_ctlMemMgr.getOffset((char*)r_idxDesc);
        r_idxDesc->m_tablePos = m_ctlMemMgr.getOffset((char*)t_pTableDesc);
        t_pTableDesc->addIndex(t_indexOffset, r_idxDef.m_indexName, t_scn);
    }
    catch (Mdb_Exception& e)
    {
        unlockddl();
        throw e;
    }
    unlockddl();
    return t_bRet;
}

bool MemManager::dropTable(const char* r_tableName)
{
    bool t_bRet = true;
    //0.��ȡ����������ϢTableDesc
    TableDesc* t_pTableDesc = NULL, t_tableDesc;
    getTableDescByName(r_tableName, t_pTableDesc);
    memcpy(&t_tableDesc, t_pTableDesc, sizeof(TableDesc));
    //1.�����������ɾ������
    for (int i = 0; i < t_tableDesc.m_indexNum; i++)
    {
        dropIndex(t_tableDesc.m_indexNameList[i], r_tableName);
    }
    //2.truncateTable
    try
    {
        truncateTable(r_tableName);
    }
    catch (Mdb_Exception& e)
    {
        cout << e.GetString() << endl;
        // do nothing ����drop�� by chenm 2010-08-26 16:17:08
    }
    lockddl();
    T_SCN t_scn;
    getscn(t_scn);
    //3.ɾ����������
    try
    {
        t_pTableDesc->clearUndoInfo(this, t_scn); //���Ӱ����Ϣ
    }
    catch (Mdb_Exception& e)
    {
        cout << e.GetString() << endl;
        // do nothing ����drop�� by chenm 2010-08-26 16:17:08
    }
    if (m_ctlMemMgr.deleteTableDesc(*t_pTableDesc, t_scn) == false)
    {
        t_bRet = false;
        unlockddl();
        throw Mdb_Exception(__FILE__, __LINE__, "ɾ����������ʧ��!");
    }
    T_NAMEDEF t_undotablename;
    sprintf(t_undotablename, "%s", r_tableName);
    try
    {
        m_undoMemMgr.dropTable(t_undotablename);
    }
    catch (Mdb_Exception& e)
    {
        unlockddl();
        throw e;
    }
    unlockddl();
    return t_bRet;
}

bool MemManager::dropIndex(const char* r_idxName, const char* r_tableName)
{
    bool t_bRet = true;
    T_NAMEDEF t_tableName;
    IndexDesc* t_pIndexDesc = NULL;
    getIndexDescByName(r_idxName, t_pIndexDesc);
    if (r_tableName == 0)
    {
        strcpy(t_tableName, t_pIndexDesc->m_indexDef.m_tableName);
    }
    else
    {
        strcpy(t_tableName, r_tableName);
    }
    lockddl();
    T_SCN t_scn;
    try
    {
        getscn(t_scn);
        //0. truncateIndex
        truncateIndex(r_idxName, r_tableName);
        //1. ��ȡ��������������������
        TableDesc* t_pTableDesc = NULL;
        getTableDescByName(t_tableName, t_pTableDesc);
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
            t_bRet = false;
            throw Mdb_Exception(__FILE__, __LINE__, "ȡ��Ӧ��������ʶ��ʧ��!");
        }
        //2. ɾ�����ж�Ӧ������λ����Ϣ
        if (t_pTableDesc->deleteIndex(t_indexOffSet, t_scn) == false)
        {
            t_bRet = false;
            throw Mdb_Exception(__FILE__, __LINE__, "��ɾ����Ӧ��������Ϣʧ��!");
        }
        //3. �����Hash����,�ͷ�Hash�ṹ�ռ�
        if (t_pIndexDesc->m_indexDef.m_indexType == HASH_INDEX ||
                t_pIndexDesc->m_indexDef.m_indexType == HASH_INDEX_NP || //add by gaojf 2012/5/8 16:44:45
                t_pIndexDesc->m_indexDef.m_indexType == HASH_TREE ||
                t_pIndexDesc->m_indexDef.m_indexType == BITMAP_INDEX) // add by chenm 2010-09-09 12:43:47
        {
            if (m_indexMemMgr.dropHashIdex(t_pIndexDesc->m_header, t_scn) == false)
            {
                t_bRet = false;
                // add by chenm 2009-1-8 23:29:13 ֱ��ɾ������������
                t_pIndexDesc->m_header = NULL_SHMPOS;
                t_pIndexDesc->clearUndoInfo(this, t_scn); //���Ӱ����Ϣ
                m_ctlMemMgr.deleteIndexDesc(*t_pIndexDesc, t_scn);
                throw Mdb_Exception(__FILE__, __LINE__, "����������ɾ���ɹ�,���ͷ�Hash�����ռ�ʧ��!");
            }
            t_pIndexDesc->m_header = NULL_SHMPOS;
        }
        t_pIndexDesc->clearUndoInfo(this, t_scn); //���Ӱ����Ϣ
        //4. ɾ������������
        if (m_ctlMemMgr.deleteIndexDesc(*t_pIndexDesc, t_scn) == false)
        {
            t_bRet = false;
            throw Mdb_Exception(__FILE__, __LINE__, "ɾ������������ʧ��!");
        }
    }
    catch (Mdb_Exception& e)
    {
        unlockddl();
        throw e;
    }
    unlockddl();
    return t_bRet;
}

bool MemManager::truncateIndex(const char* r_idxName, const char* r_tableName)
{
    bool t_bRet = true;
    IndexDesc* t_pIndexDesc = NULL;
    //0.��ȡ����������
    getIndexDescByName(r_idxName, t_pIndexDesc);
    //1. �ͷŶ�Ӧ����������Page����ռ�
    ShmPosition  t_pagePos, t_nextPagePos;
    PageInfo* t_pPageInfo;
    char*      t_phAddr;
    size_t     t_slotoffset = 0;
    UsedSlot*  t_pidxslot = NULL;
    TableDesc* t_pundodesc = NULL;
    T_SCN      t_scn;
    getscn(t_scn);
    m_undoMemMgr.getUndoDesc(T_UNDO_TYPE_INDEXDATA, r_tableName, t_pundodesc);
    for (int i = 0; i < MDB_PARL_NUM; ++i)
    {
        t_pagePos = t_pIndexDesc->m_pageInfo[i].m_firstPage;
        while (!(t_pagePos == NULL_SHMPOS))
        {
            getPhAddr(t_pagePos, t_phAddr);
            t_pPageInfo = (PageInfo*)t_phAddr;
            t_nextPagePos = t_pPageInfo->m_next;
            //��������SLOT��������slot��Ӱ��ȫ���ͷ� 2010-7-19 11:43 gaojf
            t_pidxslot = (UsedSlot*)(t_pPageInfo->getFirstSlot(t_slotoffset));
            for (size_t t_pos = 0; t_pos < t_pPageInfo->m_slotNum; ++t_pos,
                    t_pidxslot = (UsedSlot*)(((char*)t_pidxslot) + t_pPageInfo->m_slotSize))
            {
                if (!(t_pidxslot->isUnUsed()))
                {
                    if (t_pidxslot->m_undopos == NULL_UNDOPOS) continue;
                    else
                    {
                        //�ͷŸ�Ӱ��
                        m_undoMemMgr.free(t_pundodesc, t_scn, 0, t_pidxslot->m_undopos);
                    }
                }
            }
            if (m_tableMemMgr.freePage(this, t_pagePos, true) == false)
            {
#ifdef _DEBUG_
                cout << "freePage(" << t_pagePos << ") false!"
                     << __FILE__ << __LINE__ << endl;
#endif
                t_bRet = false;
                throw Mdb_Exception(__FILE__, __LINE__, "�ͷ�PAGEʧ��!");
            }
            t_pagePos = t_nextPagePos;
        }
        //2. ��������������е�λ����Ϣ
        t_pIndexDesc->m_pageInfo[i].initPos(this, true);
    }
    //3. ���Hash�����е�Hash�ṹ��Ϣ
    if (t_pIndexDesc->m_indexDef.m_indexType == HASH_INDEX ||
            t_pIndexDesc->m_indexDef.m_indexType == HASH_INDEX_NP || //add by gaojf 2012/5/8 16:46:23
            t_pIndexDesc->m_indexDef.m_indexType == HASH_TREE ||
            t_pIndexDesc->m_indexDef.m_indexType == BITMAP_INDEX) // add by chenm 2010-09-09 12:43:27
    {
        m_indexMemMgr.initHashSeg(t_pIndexDesc->m_header, t_scn);
    }
    else
    {
        t_pIndexDesc->m_header = NULL_SHMPOS;
    }
    return t_bRet;
}

bool MemManager::truncateTable(const char* r_tableName)
{
    m_undoMemMgr.truncateTable(r_tableName);
    bool t_bRet = true;
    //1. ȡ��Ӧ�ı�������
    //1. ��ȡ��������������������
    TableDesc* t_pTableDesc = NULL;
    getTableDescByName(r_tableName, t_pTableDesc);
    lockddl();
    try
    {
        //2. truncate��������
        for (int i = 0; i < t_pTableDesc->m_indexNum; i++)
        {
            truncateIndex(t_pTableDesc->m_indexNameList[i], r_tableName);
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
                getPhAddr(t_pagePos, t_phAddr);
                t_pPageInfo = (PageInfo*)t_phAddr;
                t_nextPagePos = t_pPageInfo->m_next;
                if (m_tableMemMgr.freePage(this, t_pagePos, true) == false)
                {
#ifdef _DEBUG_
                    cout << "m_tableMemMgr.freePage(" << t_pagePos << ") false!"
                         << " " << __FILE__ << __LINE__ << endl;
#endif
                    t_bRet = false;
                    throw Mdb_Exception(__FILE__, __LINE__, "�ͷ�PAGEʧ��!");
                }
                t_pagePos = t_nextPagePos;
            }
            //4. ������������е�λ����Ϣ
            t_pTableDesc->m_pageInfo[j].initPos(this, true);
        }
        //5. m_recordNum���� add by chenm 2008-5-23
        t_pTableDesc->m_recordNum = 0;
    }
    catch (Mdb_Exception& e)
    {
        unlockddl();
        throw e;
    }
    unlockddl();
    return t_bRet;
}

bool MemManager::allocateTableMem(TableDesc* &r_tableDesc, const int& r_num,
                                  vector<MdbAddress>& r_slotAddrList)
{
    allocateMem(r_tableDesc->m_tableDef.m_spaceNum, r_tableDesc->m_tableDef.m_spaceList,
                r_tableDesc, r_num, r_slotAddrList, 0);
    return true;
}
bool MemManager::allocateIdxMem(IndexDesc& r_indexDesc, const int& r_num,
                                vector<MdbAddress> &r_addrList)
{
    allocateMem(r_indexDesc.m_indexDef.m_spaceNum, r_indexDesc.m_indexDef.m_spaceList,
                &r_indexDesc, r_num, r_addrList, 1);
    return true;
}
//Ҫ��r_slotAddrList�м��������ַ��Ҳ��ShmPosition
bool MemManager::freeTableMem(TableDesc*  r_tableDesc,
                              const vector<MdbAddress>& r_slotAddrList,
                              const T_SCN& r_scn)
{
    bool t_bRet = true;
    try
    {
        for (vector<MdbAddress>::const_iterator t_itr = r_slotAddrList.begin();
                t_itr != r_slotAddrList.end(); t_itr++)
        {
            freeMem(r_tableDesc, t_itr->m_pos, 0, r_scn);
        }
    }
    catch (Mdb_Exception& e)
    {
        //�ͷ�t_descpageinfo
        throw e;
    }
    return t_bRet;
}

//Ҫ��r_slotAddrList�м��������ַ��Ҳ��ShmPosition
// by chenm 2008-6-25 �����ͷ�
bool MemManager::freeTableMem(TableDesc* r_tableDesc, const MdbAddress& r_slotAddr, const T_SCN& r_scn)
{
    return freeMem(r_tableDesc, r_slotAddr.m_pos, 0, r_scn);
}

bool MemManager::freeIdxMem(IndexDesc& r_indexDesc, const vector<ShmPosition> &r_addrList, const T_SCN& r_scn)
{
    bool t_bRet = true;
    for (vector<ShmPosition>::const_iterator t_itr = r_addrList.begin();
            t_itr != r_addrList.end(); t_itr++)
    {
        freeMem(&r_indexDesc, *t_itr, 1, r_scn);
    }
    return true;
}
bool MemManager::freeIdxMem(IndexDesc& r_indexDesc, const ShmPosition& r_addr, const T_SCN& r_scn)
{
    bool t_bRet = true;
    freeMem(&r_indexDesc, r_addr, 1, r_scn);
    return true;
}

bool MemManager::allocateMem(const int& r_spaceNum, const T_NAMEDEF r_spaceList[MAX_SPACE_NUM],
                             DescPageInfos* r_descpageinfogs, const int& r_num,
                             vector<MdbAddress>& r_slotAddrList,
                             const int& r_flag)
{
    int          t_needNum = r_num, t_newNum = 0, t_pos = 0;
    ShmPosition  t_pagePos;
    PageInfo*    t_pPage = NULL;
    bool         t_undoflag = true;
    DescPageInfo* t_descPageInfo;
    int          t_iRet = -1;
    updateMgrInfo();//���±�ռ���Ϣ,��Ϊ��ռ��п��ܱ仯
    r_slotAddrList.clear();
    if (r_num > r_slotAddrList.capacity())
    {
        r_slotAddrList.reserve(r_num);
    }
    do
    {
        t_iRet = r_descpageinfogs->getNewDescPageInfo(t_descPageInfo, t_pos);
        if (-1 == t_iRet) usleep(10000); //����10ms
    }
    while (-1 == t_iRet);
    try
    {
        while (t_needNum > 0)
        {
            t_undoflag = true;
            t_descPageInfo->new_mem(this, t_needNum, t_newNum, r_slotAddrList, t_undoflag);
            t_needNum = t_needNum - t_newNum;
            t_newNum  = 0;
            if (t_needNum > 0)
            {
                //2.���û�з��䵽�㹻�Ŀռ䣬��ӱ��Ӧ�ı�ռ��б��з���
                bool t_newpageflag = true;
                t_newpageflag = false;
                for (int i = 0; i < r_spaceNum; i++)
                {
                    //����һҳ��
                    t_undoflag = true;
                    if (m_tableMemMgr.allocatePage(this, r_spaceList[i], t_pPage, t_pagePos, t_undoflag) == true)
                    {
                        t_descPageInfo->addNewPage(this, t_pos, t_pagePos, t_pPage, t_undoflag);
                        t_newpageflag = true;
                        break;
                    }
                    else
                    {
                        //��ռ�����
                        continue;
                    }
                }
                if (t_newpageflag == false)
                {
                    //��ռ�����
                    throw Mdb_Exception(__FILE__, __LINE__, "û���㹻�ı�ռ����!");
                }
            }
        }
    }
    catch (Mdb_Exception& e)
    {
        //modified by gaojf 2010/11/19 12:52:14
        //t_descPageInfo->m_lockinfo=0;
        t_descPageInfo->unlock();
        T_SCN t_scn;
        getscn(t_scn);
        //�������뵽�Ľڵ��ͷŵ�
        for (vector<MdbAddress>::iterator t_addrItr = r_slotAddrList.begin();
                t_addrItr != r_slotAddrList.end(); t_addrItr++)
        {
            freeMem(r_descpageinfogs, t_addrItr->m_pos, r_flag, t_scn);
        }
        r_slotAddrList.clear();
        throw e;
    }
    //modified by gaojf 2010/11/19 12:52:14
    //t_descPageInfo->m_lockinfo=0;
    t_descPageInfo->unlock();
    return true;
}

bool MemManager::freeMem(DescPageInfos*     r_descpageinfogs,
                         const ShmPosition& r_addr,
                         const int&         r_flag,
                         const T_SCN&       r_scn)
{
    PageInfo*      t_pPage = NULL;
    DescPageInfo*  t_descPageInfo = NULL;
    ShmPosition    t_pagePos;
    updateMgrInfo(); //���±�ռ���Ϣ,��Ϊ��ռ��п��ܱ仯
    if (getPageBySlot(r_addr, t_pagePos, t_pPage) == false)
    {
        throw Mdb_Exception(__FILE__, __LINE__, "����ҳ��λ��ȡҳ��ʧ��!");
    }
    bool t_bret = true;
    do
    {
        t_bret = r_descpageinfogs->getFreeDescPageInfo(t_pPage, t_descPageInfo);
        if (t_bret == false) usleep(100);
    }
    while (t_bret == false);
    try
    {
        t_descPageInfo->free_mem(this, r_addr, t_pagePos, t_pPage, true, r_scn);
    }
    catch (Mdb_Exception& e)
    {
        //modified by gaojf 2010/11/19 12:52:14
        //t_descPageInfo->m_lockinfo=0;
        t_descPageInfo->unlock();
        throw e;
    }
    //modified by gaojf 2010/11/19 12:52:14
    //t_descPageInfo->m_lockinfo=0;
    t_descPageInfo->unlock();
    return true;
}

bool MemManager::getIndexListByTable(const TableDesc& r_tablDesc, vector<IndexDesc*> &r_pIndexList)
{
    TableDesc* t_pTableDesc = NULL;
    IndexDesc* t_pIndexDesc = NULL;
    r_pIndexList.clear();
    //1. �ҵ�TableDesc
    if (m_ctlMemMgr.getTableDescByName(r_tablDesc.m_tableDef.m_tableName, t_pTableDesc) == false)
    {
        throw Mdb_Exception(__FILE__, __LINE__, "������!");
        return false;
    }
    for (int i = 0; i < t_pTableDesc->m_indexNum; i++)
    {
        m_ctlMemMgr.getIndexDescByPos(t_pTableDesc->m_indexPosList[i], t_pIndexDesc);
        r_pIndexList.push_back(t_pIndexDesc);
    }
    return true;
}
bool MemManager::getIndexListByTable(const char* r_tableName, vector<IndexDef> &r_pIndexList)
{
    TableDesc           t_tableDesc;
    vector<IndexDesc*>  t_pIndexDescList;
    r_pIndexList.clear();
    strcpy(t_tableDesc.m_tableDef.m_tableName, r_tableName);
    getIndexListByTable(t_tableDesc, t_pIndexDescList);
    for (vector<IndexDesc*>::iterator t_itr = t_pIndexDescList.begin();
            t_itr != t_pIndexDescList.end(); t_itr++)
    {
        r_pIndexList.push_back((*t_itr)->m_indexDef);
    }
    return true;
}

bool MemManager::addTableSpace(const char* r_spaceName, const char* r_tableName, const T_TABLETYPE& r_tableType)
{
    lockddl();
    T_SCN t_scn;
    getscn(t_scn);
    //1.����Ǳ����ҵ���������
    if (r_tableType == SYSTEM_TABLE ||
            r_tableType == DATA_TABLE   ||
            r_tableType == DATA_TABLE_NP) //add by gaojf 2012/5/8 16:34:17
    {
        TableDesc* t_pTableDesc = NULL;
        getTableDescByName(r_tableName, t_pTableDesc);
        m_ctlMemMgr.lock();
        if (t_pTableDesc->addSpace(r_spaceName, t_scn) == false)
        {
            m_ctlMemMgr.unlock();
            unlockddl();
            throw Mdb_Exception(__FILE__, __LINE__, "�Ա����ӱ�ռ�ʧ��!");
            return false;
        }
        m_ctlMemMgr.unlock();
        unlockddl();
        return true;
    }
    else if (r_tableType == INDEX_TABLE)
    {
        IndexDesc* t_pIndexDesc = NULL;
        //2.��������������ҵ�����������
        getIndexDescByName(r_tableName, t_pIndexDesc);
        m_ctlMemMgr.lock();
        if (t_pIndexDesc->addSpace(r_spaceName, t_scn) == false)
        {
            m_ctlMemMgr.unlock();
            unlockddl();
            throw Mdb_Exception(__FILE__, __LINE__, "���������ӱ�ռ�ʧ��!");
            return false;
        }
        m_ctlMemMgr.unlock();
        unlockddl();
        return true;
    }
    else
    {
#ifdef _DEBUG_
        cout << "�����ʹ���!r_tableType=" << r_tableType
             << " " << __FILE__ << __LINE__ << endl;
#endif
        unlockddl();
        throw Mdb_Exception(__FILE__, __LINE__, "�����ʹ���!");
        return false;
    }
    unlockddl();
    return true;
}
bool MemManager::getSpaceListByTable(const char* r_tableName, vector<string> &r_spaceList)
{
    r_spaceList.clear();
    //1. �ҵ���������
    TableDesc* t_pTableDesc = NULL;
    getTableDescByName(r_tableName, t_pTableDesc);
    t_pTableDesc->getSpaceList(r_spaceList);
    return true;
}
bool MemManager::getSpaceListByIndex(const char* r_indexName, vector<string> &r_spaceList)
{
    r_spaceList.clear();
    //1. �ҵ���������
    IndexDesc* t_pIndexDesc = NULL;
    //2.��������������ҵ�����������
    getIndexDescByName(r_indexName, t_pIndexDesc);
    t_pIndexDesc->getSpaceList(r_spaceList);
    return true;
}

//���ݱ������ȡ��Ӧ���ڴ�ʹ��״��
bool MemManager::getTableMemInfo(const char* r_tableName, TbMemInfo& r_tbMemInfo)
{
    r_tbMemInfo.clear();
    T_TABLETYPE t_tableType;
    TableDesc* t_pTableDesc = NULL;
    IndexDesc* t_pIndexDesc = NULL;
    DescPageInfo* t_pDescPageInfo = NULL;
    PageInfo*      t_pPageInfo = NULL;
    try
    {
        getTableDescByName(r_tableName, t_pTableDesc);
        t_tableType = t_pTableDesc->m_tableDef.m_tableType;
        t_pDescPageInfo = t_pTableDesc->m_pageInfo;
    }
    catch (Mdb_Exception& e)
    {
        try
        {
            //2.��������������ҵ�����������
            getIndexDescByName(r_tableName, t_pIndexDesc);
            t_tableType = INDEX_TABLE;
            t_pDescPageInfo = t_pIndexDesc->m_pageInfo;
        }
        catch (Mdb_Exception& e)
        {
            throw Mdb_Exception(__FILE__, __LINE__, "������������������!");
            return false;
        }
    }
    strcpy(r_tbMemInfo.m_tbName, r_tableName);
    r_tbMemInfo.m_tbType = t_tableType;
    for (int i = 0; i < PAGE_ITLS_NUM; ++i)
    {
        r_tbMemInfo.m_slotSize = t_pDescPageInfo[i].m_slotSize;
        //1.ȡռ��ҳ���״̬
        ShmPosition  t_pagePos;
        char*      t_phAddr;
        t_pagePos = t_pDescPageInfo[i].m_firstPage;
        while (!(t_pagePos == NULL_SHMPOS))
        {
            getPhAddr(t_pagePos, t_phAddr);
            t_pPageInfo = (PageInfo*)t_phAddr;
            r_tbMemInfo.setPageInfo(t_pPageInfo);
            t_pagePos = t_pPageInfo->m_next;
        }
        //2.ȡδ��ҳ��״̬
        t_pagePos = t_pDescPageInfo[i].m_fIdlePage;
        while (!(t_pagePos == NULL_SHMPOS))
        {
            getPhAddr(t_pagePos, t_phAddr);
            t_pPageInfo = (PageInfo*)t_phAddr;
            r_tbMemInfo.m_notFullPages.push_back(t_pagePos);
            t_pagePos = t_pPageInfo->m_nIdlePage;
        }
    }
    return true;
}

bool MemManager::allocateIdxMem(IndexDesc* &r_pIndexDesc, MdbAddress& r_addr)
{
    int t_num = 1;
    vector<MdbAddress> t_addrList;
    allocateMem(r_pIndexDesc->m_indexDef.m_spaceNum, r_pIndexDesc->m_indexDef.m_spaceList,
                r_pIndexDesc, t_num, t_addrList, 1);
    r_addr = t_addrList[0];
    return true;
}



