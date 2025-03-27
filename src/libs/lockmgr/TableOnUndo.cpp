#include "TableOnUndo.h"
#include "Index.h"
#include "InstanceFactory.h"
#include "MDBLatchMgr.h"
#include "IndexHashOnUndo.h"

TableOnUndo::TableOnUndo(const char* tableName, MemManager* memManager): BaseTable(tableName, memManager)
{
}


TableOnUndo::~TableOnUndo()
{
    m_pUndoMemMgr = NULL;
}


void TableOnUndo::initialize()
{
    m_pUndoMemMgr = m_memManager->getUndoMemMgr();
    if (m_pUndoMemMgr->getTbDescByname(m_tableName, m_tableDesc) == false)
    {
        throw Mdb_Exception(__FILE__, __LINE__, "�� %s! ������", m_tableName);
    }
    m_recordConvert = new RecordConvert(m_tableDesc);
    m_pSrcLatchInfo = (MDBLatchMgr::getInstance(m_dbName))->getSrcLatchInfo(T_DBLK_TABLE, m_tableName);
    return;
}


void TableOnUndo::create(const TableDef& tableDef)
{
    m_pUndoMemMgr->createTable(tableDef, m_tableDesc);
    m_tableDesc->m_recordNum = 0;
    m_recordConvert = new RecordConvert(m_tableDesc);
}


void TableOnUndo::drop()
{
    for (int i = 0; i < m_tableDesc->m_indexNum; ++i)
    {
        IndexDesc* indexDesc = NULL;
        m_pUndoMemMgr->getIndexDescByName(m_tableDesc->m_indexNameList[i], indexDesc);
        Index* tmpIndex = new IndexHashOnUndo(m_tableName, m_tableDesc->m_indexNameList[i], m_memManager, m_recordConvert);
        try
        {
            tmpIndex->initialization();
            tmpIndex->drop();
        }
        catch (Mdb_Exception& e)
        {
            if (tmpIndex != NULL)
            {
                delete tmpIndex;
            }
            throw e;
        }
        if (tmpIndex != NULL)
        {
            delete tmpIndex;
        }
    }
    m_tableDesc->m_recordNum = 0;
    m_pUndoMemMgr->dropTable(m_tableName);
}


void TableOnUndo::truncate()
{
    for (int i = 0; i < m_tableDesc->m_indexNum; ++i)
    {
        IndexDesc* indexDesc = NULL;
        m_pUndoMemMgr->getIndexDescByName(m_tableDesc->m_indexNameList[i], indexDesc);
        Index* tmpIndex = new IndexHashOnUndo(m_tableName, m_tableDesc->m_indexNameList[i], m_memManager, m_recordConvert);
        try
        {
            tmpIndex->initialization();
            tmpIndex->truncate();
        }
        catch (Mdb_Exception& e)
        {
            if (tmpIndex != NULL)
            {
                delete tmpIndex;
            }
            throw e;
        }
        if (tmpIndex != NULL)
        {
            delete tmpIndex;
        }
    }
    m_tableDesc->m_recordNum = 0;
    m_pUndoMemMgr->truncateTable(m_tableName);
}


void TableOnUndo::createIndex(const IndexDef& indexDef, TransResource* pTransRes)
{
    //��������ɾ�������ؽ������.�Ȳ�ѯ,�����ѯ����ԭ����������ؽ�;�����ѯ�����½�.
    Index* tmpIndex = new IndexHashOnUndo(m_tableName, indexDef.m_indexName, m_memManager, m_recordConvert);
    try
    {
        tmpIndex->create(indexDef);
    }
    catch (Mdb_Exception& e)
    {
        if (tmpIndex != NULL)
        {
            delete tmpIndex;
        }
        throw e;
    }
    if (tmpIndex != NULL)
    {
        delete tmpIndex;
    }
}


void TableOnUndo::dropIndex(const char* indexName)
{
    IndexDesc* indexDesc = NULL;
    m_pUndoMemMgr->getIndexDescByName(indexName, indexDesc);
    Index* tmpIndex = new IndexHashOnUndo(m_tableName, indexName, m_memManager, m_recordConvert);
    try
    {
        tmpIndex->initialization();
        tmpIndex->drop();
    }
    catch (Mdb_Exception& e)
    {
        if (tmpIndex != NULL)
        {
            delete tmpIndex;
        }
        throw e;
    }
    if (tmpIndex != NULL)
    {
        delete tmpIndex;
    }
}

Index* TableOnUndo::getIndexByName(const char* indexName)
{
    MyString s;
    T_INDEXTYPE indexType;
    strcpy(s.m_pStr, indexName);
    INDEX_POOL_ITR itr = m_indexs.find(s);
    if (itr != m_indexs.end())
    {
        indexType = (((itr->second).m_pIndex)->getIndexDesc()->m_indexDef).m_indexType;
        if ((itr->second).m_indexType == indexType)
        {
            return (itr->second).m_pIndex;
        }
        else
        {
            //���Ͳ��ԣ����»�ȡ
            delete(itr->second).m_pIndex;
            (itr->second).m_pIndex = new IndexHashOnUndo(m_tableName, indexName, m_memManager, m_recordConvert);
            (itr->second).m_pIndex->initialization();
            return (itr->second).m_pIndex;
        }
    }
    else
    {
        IndexDesc* indexDesc = NULL;
        m_pUndoMemMgr->getIndexDescByName(indexName, indexDesc);
        indexType = (indexDesc->m_indexDef).m_indexType;
        Index* tmpIndex = new IndexHashOnUndo(m_tableName, indexName, m_memManager, m_recordConvert);
        IndexP_Type indexP_Type;
        indexP_Type.m_pIndex = tmpIndex;
        indexP_Type.m_indexType = indexType;
        m_indexs.insert(INDEX_POOL::value_type(s, indexP_Type));
        tmpIndex->initialization();
        return tmpIndex;
    }
}

// ���� ���ݼ�¼���追����˽���ڴ�
void TableOnUndo::select(const char* indexName, const char* idxKey , vector<MdbAddress>& recordVector)
{
    recordVector.clear();
    //Ѱ������
    Index* tmpIndex;
    tmpIndex = getIndexByName(indexName);
    //����������ѯ��¼
    recordVector.clear();
    tmpIndex->select(idxKey, recordVector, m_iRowCount);
}

void TableOnUndo::deleteRec(const char* indexName, const void* idxKey)
{
    vector<MdbAddress> addrs;
    addrs.clear();
    //(1)����
    select(indexName, (char*)idxKey, addrs);
    //(2)ɾ������
    for (int i = 0; i < addrs.size(); ++i)
    {
        //ɾ����¼ռ�õ�����,����ʱ�ָ��Բ���
        try
        {
            deleteAllIndex(addrs[i]);
        }
        catch (Mdb_Exception& e1)
        {
            for (int j = i - 1; j >= 0; --j)
            {
                insertAllIndex(addrs[i]);
            }
            throw e1;
        }
    }
    //(2)ɾ����¼;��¼���ɾ�����ɹ�,�ָ���Щ��¼����
    int iCount;
    try
    {
        for (iCount = 0; iCount < addrs.size(); ++iCount)
            m_pUndoMemMgr->freeTableMem(m_tableDesc, addrs[iCount]);
    }
    catch (Mdb_Exception& e2)
    {
        for (int i = addrs.size() - 1; i >= iCount; --i)
        {
            insertAllIndex(addrs[i]);
        }
        throw e2;
    }
    //(3)���¼������
    m_tableDesc->m_recordNum -= addrs.size();
}

// ���뵥����¼
void TableOnUndo::insert(void* pRecord)
{
    //(1)�����ڴ沢�Ҳ����¼
    MdbAddress addr;
    m_pUndoMemMgr->allocateTableMem(m_tableDesc, addr);
    memcpy(addr.m_addr + sizeof(Undo_Slot), pRecord, m_tableDesc->m_valueSize);
    //(2)��������;�����¼��Ӧ��������Ϣ,�������ʧ��,���лָ���ɾ������.
    try
    {
        insertAllIndex(addr);
    }
    catch (Mdb_Exception& e1)
    {
        m_pUndoMemMgr->freeTableMem(m_tableDesc, addr);
        deleteAllIndex(addr);
        throw e1;
    }
    //(3)���¼������
    m_tableDesc->m_recordNum += 1;
}

void TableOnUndo::update(const char* indexName, const void* idxKey, const char* columnName, const void* columValue)
{
    //(1)����
    vector<MdbAddress> addrs;
    addrs.clear();
    select(indexName, (char*)idxKey, addrs);
    for (int i = 0; i < addrs.size(); ++i)
    {
        //(2)��������;���¼�¼��Ӧ��������Ϣ,�������ʧ��,���лָ��Ը��¶���.
        updateAllIndex(addrs[i], columnName , columValue);
        //(3)������������
        m_recordConvert->setRecordValue(addrs[i].m_addr + sizeof(Undo_Slot));
        m_recordConvert->setColumnValue(columnName, columValue);
    }
    return;
}

void TableOnUndo::deleteAllIndex(const MdbAddress& address)
{
    for (int i = 0; i < m_tableDesc->m_indexNum; ++i)
    {
        //(1)Ѱ�Ҷ�Ӧ��Index��
        Index* tmpIndex = NULL;
        tmpIndex = getIndexByName(m_tableDesc->m_indexNameList[i]);
        //(2)��ȡ��Ӧ��Index�ֶ�����
        IndexDesc* indexDesc = tmpIndex->getIndexDesc();
        m_recordConvert->setRecordValue(address.m_addr + sizeof(Undo_Slot));
        int iColumnLenth, iOffSize = 0;
        char value[MAX_COLUMN_LEN];
        char columnValue[MAX_COLUMN_LEN];
        for (int i = 0; i < (indexDesc->m_indexDef).m_columnNum; ++i)
        {
            iColumnLenth = 0;
            memset(columnValue, 0, sizeof(columnValue));
            m_recordConvert->getColumnValue((indexDesc->m_indexDef).m_columnList[i]
                                            , columnValue, iColumnLenth);
            memcpy(value + iOffSize, columnValue, iColumnLenth);
            iOffSize += iColumnLenth;
        }
        //(3)ɾ��������Ϣ,����¼ɾ����־
        tmpIndex->deleteIdx(value, address);
    }
    return;
}


void TableOnUndo::insertAllIndex(const MdbAddress& addr)
{
    for (int i = 0; i < m_tableDesc->m_indexNum; ++i)
    {
        //(1)Ѱ�Ҷ�Ӧ��Index��
        Index* tmpIndex = NULL;
        tmpIndex = getIndexByName(m_tableDesc->m_indexNameList[i]);
        //(2)��ȡ��Ӧ��Index�ֶ�����
        IndexDesc* indexDesc = tmpIndex->getIndexDesc();
        //(3)��ȡ�ֶζ�Ӧ��ֵ����(type:0-����,ʹ����ֵ 1-�ָ��Բ���,ʹ��ԭֵ)
        m_recordConvert->setRecordValue(addr.m_addr + sizeof(Undo_Slot));
        int iColumnLenth, iOffSize = 0;
        char value[MAX_COLUMN_LEN];
        char columnValue[MAX_COLUMN_LEN];
        for (int j = 0; j < (indexDesc->m_indexDef).m_columnNum; ++j)
        {
            iColumnLenth = 0;
            memset(columnValue, 0, sizeof(columnValue));
            m_recordConvert->getColumnValue((indexDesc->m_indexDef).m_columnList[j]
                                            , columnValue, iColumnLenth);
            memcpy(value + iOffSize, columnValue, iColumnLenth);
            iOffSize += iColumnLenth;
        }
        //(4)����������Ϣ
        tmpIndex->insert(value, addr);
    }
}


void TableOnUndo::updateAllIndex(const MdbAddress& addr, const char* columnName, const void* columValue)
{
    for (int i = 0; i < m_tableDesc->m_indexNum; ++i)
    {
        //(1)Ѱ�Ҷ�Ӧ��Index��
        Index* tmpIndex = NULL;
        tmpIndex = getIndexByName(m_tableDesc->m_indexNameList[i]);
        //(2)��ȡ��Ӧ��Index�ֶ�����  disabled by chenm 2008-6-24 9:39:11
        IndexDesc* indexDesc = tmpIndex->getIndexDesc();
        int iColumnLenth, iOffSize = 0;
        char oldValue[MAX_COLUMN_LEN];
        m_recordConvert->setRecordValue(addr.m_addr + sizeof(Undo_Slot));
        if (strcmp(columnName, (indexDesc->m_indexDef).m_columnList[0]) == 0)
        {
            m_recordConvert->getColumnValue((indexDesc->m_indexDef).m_columnList[0]
                                            , oldValue, iColumnLenth);
            //(3)����������Ϣ
            //ɾ��
            tmpIndex->deleteIdx(oldValue, addr);
            //����
            tmpIndex->insert((char*)columValue, addr);
        }
    }
}

