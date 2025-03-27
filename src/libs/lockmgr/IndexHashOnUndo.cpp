// ############################################
// Source file : IndexHashOnUndo.cpp
// Version     : 2.0
// Language	   : ANSI C++
// OS Platform : UNIX
// Authors     : chen min
// E_mail      : chenm@lianchuang.com
// Create      : 2010-06-13 14:41:56
// Update      : 2010-06-13 14:41:59
// Copyright(C): chen min, Linkage.
// ############################################

#include "IndexHashOnUndo.h"
#include "UndoMemMgr.h"
#include "IndexHash.h"

class IndexDef;

IndexHashOnUndo::IndexHashOnUndo(const char* cTableName, const char* cIndexName, MemManager* memMgr, RecordConvert* pRecordConvert)
    : Index(cTableName, cIndexName, memMgr, pRecordConvert)
{
    m_size = 0;
    m_hashTable = NULL;
    m_pUndoMemMgr = memMgr->getUndoMemMgr();
}

// 创建一个新索引
bool IndexHashOnUndo::create(const IndexDef& theIndexDef)
{
    //未分配过并且hash桶大于0
    if ((m_size == 0)
            && (theIndexDef.m_hashSize > 0)
       )
    {
        m_size = theIndexDef.m_hashSize;
        m_pUndoMemMgr->createIndex(theIndexDef, m_indexDesc);
        m_memMgr->getPhAddr(m_indexDesc->m_header, (char *&)m_hashTable); // 获取指向hash桶的首地址
        T_SCN t_scn;
        for (int i = 0; i < m_size; ++i)
        {
            m_hashTable[i].m_nodepos = NULL_SHMPOS;               //初始化
            m_hashTable[i].m_sid     = 0;
            m_hashTable[i].m_lockinfo = BUCKET_FREE;
            m_hashTable[i].m_scn     = t_scn;
        }
        getHashKeyLenth();
        return true;
    }
    else
    {
        if (m_size != 0)
            throw Mdb_Exception(__FILE__, __LINE__, "索引 [%s]  已建!", m_indexName.c_str());
        else
            throw Mdb_Exception(__FILE__, __LINE__, "索引 [%s]  未指定hash桶大小!", m_indexName.c_str());
    }
}

// 初始化一个索引（attach内存）
bool IndexHashOnUndo::initialization()
{
    m_pUndoMemMgr->getIndexDescByName(m_indexName.c_str(), m_indexDesc);
    m_memMgr->getPhAddr(m_indexDesc->m_header, (char *&)m_hashTable); // 获取指向hash桶的首地址
    m_size      = m_indexDesc->m_indexDef.m_hashSize;
    getHashKeyLenth();
    return true;
}

int IndexHashOnUndo::select(const InputParams* pInputParams
                            , vector<MdbAddress> &results
                            , int iRowCount
                            , bool isTransaction
                            , const TransResource* pTransRes)
{
    BucketIdxOnUndo* ptr1, *ptr2;
    void*   pRecord = 0;
    int i = 0;
    MdbAddress result;
    // 如果是获取全表数据,输入的查询hashke为空,则返回全表地址集
    if (pInputParams == NULL
            || pInputParams->m_iNum == 0)
    {
        if (iRowCount == -1) // 全表返回
            return this->selectALL(results);
        // add by chenm 2009-5-18
        else if (iRowCount == SELECT_ROW_BY_ROW || iRowCount == SELECT_ROW_BY_ROW_INTERNAL)
            return this->selectRowByRow(results, iRowCount);
        // over 2009-5-18 22:56:28
        else
            return this->selectLimited(results, iRowCount);
    }
    getSearchKey(pInputParams);
    unsigned val = HashCode::hash(m_cSearchKey, m_iHashKeyLenth) % m_size;
    results.clear();
    if (NULL_SHMPOS == m_hashTable[val].m_nodepos)
        return 0;
    m_memMgr->getPhAddr(m_hashTable[val].m_nodepos, (char *&)ptr1);  // 获取hash表指向的bucket的实际地址
    while (ptr1 != NULL)
    {
        m_memMgr->getPhAddr(ptr1->m_data, (char *&)pRecord); // 获取数据记录的实际地址
        if (memcmp(m_cSearchKey, this->getIndexKey(pRecord), m_iHashKeyLenth) == 0)
        {
            ptr2 = ptr1;
            while (ptr2 != NULL)
            {
                result.m_pos = ptr2->m_data;
                m_memMgr->getPhAddr(result);           // 根据相对地址,得到实际地址
                results.push_back(result);             // 选出所有匹配索引字段的记录
                ++i;
                m_memMgr->getPhAddr(ptr2->m_sameNext, (char *&)ptr2); // 遍历横向列表
            }
            break;
        }
        m_memMgr->getPhAddr(ptr1->m_diffNext, (char *&)ptr1); // 遍历纵向列表
    }
    return i;
}

// 逻辑和上一个select一样,只是传入参数的形式不同
int IndexHashOnUndo::select(const char* hashkey
                            , vector<MdbAddress> &results
                            , int iRowCount
                            , bool isTransaction
                            , const TransResource* pTransRes)
{
    BucketIdxOnUndo* ptr1, *ptr2;
    void*   pRecord = 0;
    int i = 0;
    MdbAddress result;
    // 如果是获取全表数据,输入的查询hashke为空,则返回全表地址集
    if (hashkey == NULL)
    {
        if (iRowCount == -1) // 全表返回
            return this->selectALL(results);
        else
            return this->selectLimited(results, iRowCount);
    }
    unsigned val = HashCode::hash(hashkey, m_iHashKeyLenth) % m_size;
    results.clear();
    if (NULL_SHMPOS == m_hashTable[val].m_nodepos)
        return 0;
    m_memMgr->getPhAddr(m_hashTable[val].m_nodepos, (char *&)ptr1);  // 获取hash表指向的bucket的实际地址
    while (ptr1 != NULL)
    {
        m_memMgr->getPhAddr(ptr1->m_data, (char *&)pRecord); // 获取数据记录的实际地址
        if (memcmp(hashkey, this->getIndexKey(pRecord), m_iHashKeyLenth) == 0)
        {
            ptr2 = ptr1;
            while (ptr2 != NULL)
            {
                result.m_pos = ptr2->m_data;
                m_memMgr->getPhAddr(result);           // 根据相对地址,得到实际地址
                results.push_back(result);             // 选出所有匹配索引字段的记录
                ++i;
                m_memMgr->getPhAddr(ptr2->m_sameNext, (char *&)ptr2); // 遍历横向列表
            }
            break;
        }
        m_memMgr->getPhAddr(ptr1->m_diffNext, (char *&)ptr1); // 遍历纵向列表
    }
    return i;
}

//插入一个节点
bool IndexHashOnUndo::insert(const char* hashkey       //查询关键字
                             , const MdbAddress& theMdbAddress
                             , TransResource* pTransRes
                             , TableOnUndo* pTransTable)
{
    unsigned val;
    BucketIdxOnUndo* ptr1;
    MdbAddress newMdbAddr;
    void* pRecord = 0;
    val = HashCode::hash(hashkey, m_iHashKeyLenth) % m_size;
    // 如果hash表中该下标对应的值为空,则直接插入
    if (NULL_SHMPOS == m_hashTable[val].m_nodepos)
    {
        m_pUndoMemMgr->allocateIdxMem(m_indexDesc, newMdbAddr);
        ((BucketIdxOnUndo*)(newMdbAddr.m_addr))->m_diffNext = NULL_SHMPOS;
        ((BucketIdxOnUndo*)(newMdbAddr.m_addr))->m_sameNext = NULL_SHMPOS;
        ((BucketIdxOnUndo*)(newMdbAddr.m_addr))->m_data = theMdbAddress.m_pos;
        m_hashTable[val].m_nodepos = newMdbAddr.m_pos;
        return true;
    }
    // 如果hash表中的该下标对应的值不为空
    m_memMgr->getPhAddr(m_hashTable[val].m_nodepos, (char *&)ptr1);  // 获取hash表指向的bucket的实际地址
    while (ptr1 != NULL)
    {
        m_memMgr->getPhAddr(ptr1->m_data, (char *&)pRecord); // 获取数据记录的实际地址
        if (memcmp(hashkey, this->getIndexKey(pRecord), m_iHashKeyLenth) == 0)
        {
            if (m_indexDesc->m_indexDef.m_isUnique)
            {
                // 如果是主键,则抛出主键重复异常
                throw Mdb_Exception(__FILE__, __LINE__, "值 [%s] 主键重复", hashkey);
            }
            m_pUndoMemMgr->allocateIdxMem(m_indexDesc, newMdbAddr);
            ((BucketIdxOnUndo*)(newMdbAddr.m_addr))->m_diffNext = NULL_SHMPOS;
            //相同的key 增加横向链表 插入横向链表的第二个节点。不直接插入首节点的原因，是兼顾纵向链表的完整
            ((BucketIdxOnUndo*)(newMdbAddr.m_addr))->m_sameNext = ptr1->m_sameNext;
            ((BucketIdxOnUndo*)(newMdbAddr.m_addr))->m_data = theMdbAddress.m_pos;
            ptr1->m_sameNext = newMdbAddr.m_pos;
            return true;
        }
        m_memMgr->getPhAddr(ptr1->m_diffNext, (char *&)ptr1); // 遍历纵向列表
    }
    //hash值相同,但却是不同的key 增加纵向链表
    m_pUndoMemMgr->allocateIdxMem(m_indexDesc, newMdbAddr);
    ((BucketIdxOnUndo*)(newMdbAddr.m_addr))->m_diffNext = m_hashTable[val].m_nodepos;
    ((BucketIdxOnUndo*)(newMdbAddr.m_addr))->m_sameNext = NULL_SHMPOS;
    ((BucketIdxOnUndo*)(newMdbAddr.m_addr))->m_data = theMdbAddress.m_pos;
    m_hashTable[val].m_nodepos = newMdbAddr.m_pos;
    return true;
}

//根据hashkey 删除一个节点
bool IndexHashOnUndo::deleteIdx(const char* hashkey       //查询关键字
                                , const MdbAddress& theMdbAddress)
{
    BucketIdxOnUndo* ptr1, *ptr2, *pLastVertical = NULL, *pLastParallel = NULL;
    MdbAddress ptr2MdbAddress ;
    void* pRecord = 0;
    unsigned val = HashCode::hash(hashkey, m_iHashKeyLenth) % m_size;
    //没有对应的节点
    if (NULL_SHMPOS == m_hashTable[val].m_nodepos)
        return false;
    for (m_memMgr->getPhAddr(m_hashTable[val].m_nodepos, (char *&)ptr1)
            ; NULL != ptr1
            ; pLastVertical = ptr1, m_memMgr->getPhAddr(ptr1->m_diffNext, (char *&)ptr1)
        )
    {
        m_memMgr->getPhAddr(ptr1->m_data, (char *&)pRecord); // 获取数据记录的实际地址
        if (memcmp(hashkey, this->getIndexKey(pRecord), m_iHashKeyLenth) == 0) // 如果hash值一样，并且查找内容相符时，才删除
        {
            //cout<<"------------------------------------------"<<endl;
            // 记录横向节点首节点
            BucketIdxOnUndo* pFirstSameKeyElement = ptr1;
            // 遍历横向列表
            for (ptr2 = ptr1, pLastParallel = NULL
                                              ; NULL != ptr2
                    ; pLastParallel = ptr2, m_memMgr->getPhAddr(ptr2->m_sameNext, (char *&)ptr2)
                )
            {
                if ((ptr2->m_data) == theMdbAddress.m_pos)   // 记录的相对地址相同时 删除
                {
                    // 如果是横向节点首节点
                    if (pLastParallel == NULL || pFirstSameKeyElement == ptr2)
                    {
                        // 删除第一行第一列,且第一行只有唯一一列
                        if (pLastVertical == NULL && ptr2->m_sameNext == NULL_SHMPOS)
                        {
                            m_hashTable[val].m_nodepos = ptr2->m_diffNext;
                        }
                        // 删除第一行第一列,但第一行中不止一列
                        else if (pLastVertical == NULL && ptr2->m_sameNext != NULL_SHMPOS)
                        {
                            m_hashTable[val].m_nodepos = ptr2->m_sameNext;
                            m_memMgr->getPhAddr(ptr2->m_sameNext, (char *&)pFirstSameKeyElement);
                            pFirstSameKeyElement->m_diffNext = ptr2->m_diffNext;
                        }
                        // 删除非第一行的第一列,且这一行只有唯一一列
                        else if (pLastVertical != NULL && ptr2->m_sameNext == NULL_SHMPOS)
                        {
                            pLastVertical->m_diffNext = ptr2->m_diffNext;
                        }
                        // 删除非第一行的第一列,但这一行中不止一列
                        else
                        {
                            pLastVertical->m_diffNext = ptr2->m_sameNext;
                            m_memMgr->getPhAddr(ptr2->m_sameNext, (char *&)pFirstSameKeyElement);
                            pFirstSameKeyElement->m_diffNext = ptr2->m_diffNext;
                        }
                    }
                    else  // 如果不是横向节点首节点
                    {
                        pLastParallel->m_sameNext = ptr2->m_sameNext;
                    }
                    ptr2MdbAddress.m_addr = (char*)ptr2;
                    m_memMgr->getShmPos(ptr2MdbAddress);
                    m_pUndoMemMgr->freeIdxMem(m_indexDesc, ptr2MdbAddress);  //释放index节点空间
                    return 1;
                }
            }
        }
    }
    return 0;
}

bool IndexHashOnUndo::truncate()
{
    m_pUndoMemMgr->truncateIndex(m_indexName.c_str());
    return true;
}

bool IndexHashOnUndo::drop()
{
    m_pUndoMemMgr->dropIndex(m_indexName.c_str());
    return true;
}

//全部选取
int IndexHashOnUndo::selectALL(vector<MdbAddress> &results)
{
    BucketIdxOnUndo* ptr1, *ptr2, *next1, *next2;
    MdbAddress result;
    results.clear();
    for (int i = 0; i < m_size; i++)
    {
        if (NULL_SHMPOS != m_hashTable[i].m_nodepos)
        {
            // 遍历纵向列表
            for (m_memMgr->getPhAddr(m_hashTable[i].m_nodepos, (char *&)ptr1)
                    ; NULL != ptr1
                    ; ptr1 = next1
                )
            {
                m_memMgr->getPhAddr(ptr1->m_diffNext, (char *&)next1);
                // 遍历横向列表
                for (ptr2 = ptr1
                            ; NULL != ptr2
                        ; ptr2 = next2
                    )
                {
                    m_memMgr->getPhAddr(ptr2->m_sameNext, (char *&)next2);
                    result.m_pos = ptr2->m_data;
                    m_memMgr->getPhAddr(result);           // 根据相对地址,得到实际地址
                    results.push_back(result);
                }
            }
        }
    }
    return results.size();
}

//限定返回记录数的select * from table指令
int IndexHashOnUndo::selectLimited(vector<MdbAddress> &results, int iRowCount)
{
    BucketIdxOnUndo* ptr1, *ptr2, *next1, *next2;
    MdbAddress result;
    results.clear();
    for (int i = 0; i < m_size; i++)
    {
        if (NULL_SHMPOS != m_hashTable[i].m_nodepos)
        {
            // 遍历纵向列表
            for (m_memMgr->getPhAddr(m_hashTable[i].m_nodepos, (char *&)ptr1)
                    ; NULL != ptr1
                    ; ptr1 = next1
                )
            {
                m_memMgr->getPhAddr(ptr1->m_diffNext, (char *&)next1);
                // 遍历横向列表
                for (ptr2 = ptr1
                            ; NULL != ptr2
                        ; ptr2 = next2
                    )
                {
                    m_memMgr->getPhAddr(ptr2->m_sameNext, (char *&)next2);
                    result.m_pos = ptr2->m_data;
                    m_memMgr->getPhAddr(result);           // 根据相对地址,得到实际地址
                    results.push_back(result);
                    if (results.size() >= iRowCount)
                    {
                        break; // 如果记录数大于指定的rowcount,则返回,不再选取
                    }
                }
                if (results.size() >= iRowCount)
                {
                    break; // 如果记录数大于指定的rowcount,则返回,不再选取
                }
            }
        }
        if (results.size() >= iRowCount)
        {
            break; // 如果记录数大于指定的rowcount,则返回,不再选取
        }
    }
    return results.size();
}
bool IndexHashOnUndo::getHashPercent(int& iHashSize
                                     , int& iUsedHash
                                     , int& iRecordCount
                                     , map<int, int>& mHashCal)
{
    iHashSize = m_size; //总数
    iUsedHash = 0;
    iRecordCount = 0;
    //统计已经使用的Hash桶的个数
    for (int i = 0; i < m_size; i++)
    {
        if (m_hashTable[i].m_nodepos == NULL_SHMPOS)
            continue;
        iUsedHash ++;
    }
    //统计Hash的分布
    mHashCal.clear();
    BucketIdxOnUndo* ptr1;
    BucketIdxOnUndo* ptr2;
    int iCount;
    for (int i = 0; i < m_size; i++)
    {
        if (m_hashTable[i].m_nodepos == NULL_SHMPOS)
            continue;
        iCount = 0;
        m_memMgr->getPhAddr(m_hashTable[i].m_nodepos, (char *&)ptr1);
        while (ptr1 != NULL)
        {
            ptr2 = ptr1;
            while (ptr2 != NULL)
            {
                iCount ++;
                iRecordCount ++;
                m_memMgr->getPhAddr(ptr2->m_sameNext, (char *&)ptr2);
            }
            m_memMgr->getPhAddr(ptr1->m_diffNext, (char *&)ptr1);
        }
        if (iCount == 0)
            continue;
        if (mHashCal.find(iCount) != mHashCal.end())
            mHashCal[iCount]++;
        else
        {
            mHashCal.insert(map<int, int>::value_type(iCount, 1));
        }
    }
    return true;
}
/*
bool IndexHashOnUndo::reIndex(Index * r_destIndex,TransResource *pTransRes,TableOnUndo *pTransTable)
{
	IndexHashOnUndo* t_destIndex = (IndexHashOnUndo*)r_destIndex;
	char t_indexKey[MAX_COLUMN_LEN];
	cout<<"清空索引......"<<endl;
	t_destIndex->initialization();
	t_destIndex->truncate();//先清空

	BucketIdxOnUndo *ptr1,*ptr2,*next1,*next2;
	MdbAddress result;
	IndexDesc * t_destIndexdesc=t_destIndex->getIndexDesc();
	this->initialization();

	long lRowCount=0;
	for (int i=0;i<m_size;i++)
	{
		if (NULL_SHMPOS != m_hashTable[i].m_nodepos)
		{
			// 遍历纵向列表
			for (m_memMgr->getPhAddr(m_hashTable[i].m_nodepos,(char *&)ptr1)
					;NULL!=ptr1
					;ptr1=next1
				 )
			{
				m_memMgr->getPhAddr(ptr1->m_diffNext,(char *&)next1);

				// 遍历横向列表
				for( ptr2=ptr1
						;NULL!=ptr2
						;ptr2=next2
					)
				{
					m_memMgr->getPhAddr(ptr2->m_sameNext,(char *&)next2);
					memset(&result,0,sizeof(MdbAddress));
					result.m_pos = ptr2->m_data;
					m_memMgr->getPhAddr( result );         // 根据相对地址,得到实际地址
					memset(t_indexKey,0,MAX_COLUMN_LEN);
					memcpy(t_indexKey,t_destIndex->getIndexKey(result.m_addr),MAX_COLUMN_LEN);
					t_destIndex->insert(t_indexKey,result);
					lRowCount ++;
					if (lRowCount % 100000 == 0)
						cout<<lRowCount<<" records reindexed."<<endl;
				}
			}
		}
	}
	cout<<"total "<<lRowCount<<" records reindexed."<<endl;
	return true;
}
*/
// add by chenm 2009-5-18 23:07:28 逐条全表扫描,暂时实现把同一个hash节点下的记录都返回
int IndexHashOnUndo::selectRowByRow(vector<MdbAddress> &results, int iRowCount)
{
    if (iRowCount == SELECT_ROW_BY_ROW)
    {
        // 首次调用,从表第一条记录开始返回,内部计数器清零
        m_iHashPosi = 0;
        //m_iXPosi    = 0;  // 横向节点计数器
        //m_iYPosi    = 0;  // 纵向节点计数器
    }
    BucketIdxOnUndo* ptr1, *ptr2, *next1, *next2;
    MdbAddress result;
    results.clear();
    for (; m_iHashPosi < m_size; ++m_iHashPosi)
    {
        if (NULL_SHMPOS != m_hashTable[m_iHashPosi].m_nodepos)
        {
            // 遍历纵向列表
            for (m_memMgr->getPhAddr(m_hashTable[m_iHashPosi].m_nodepos, (char *&)ptr1)
                    ; NULL != ptr1
                    ; ptr1 = next1
                )
            {
                m_memMgr->getPhAddr(ptr1->m_diffNext, (char *&)next1);
                // 遍历横向列表
                for (ptr2 = ptr1
                            ; NULL != ptr2
                        ; ptr2 = next2
                    )
                {
                    m_memMgr->getPhAddr(ptr2->m_sameNext, (char *&)next2);
                    result.m_pos = ptr2->m_data;
                    m_memMgr->getPhAddr(result);           // 根据相对地址,得到实际地址
                    results.push_back(result);
                }
            }
            // 暂时实现把同一个hash节点下的记录都返回
            ++m_iHashPosi;
            break;
        }
    }
    return results.size();
}
