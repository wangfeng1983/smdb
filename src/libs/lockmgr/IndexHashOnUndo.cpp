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

// ����һ��������
bool IndexHashOnUndo::create(const IndexDef& theIndexDef)
{
    //δ���������hashͰ����0
    if ((m_size == 0)
            && (theIndexDef.m_hashSize > 0)
       )
    {
        m_size = theIndexDef.m_hashSize;
        m_pUndoMemMgr->createIndex(theIndexDef, m_indexDesc);
        m_memMgr->getPhAddr(m_indexDesc->m_header, (char *&)m_hashTable); // ��ȡָ��hashͰ���׵�ַ
        T_SCN t_scn;
        for (int i = 0; i < m_size; ++i)
        {
            m_hashTable[i].m_nodepos = NULL_SHMPOS;               //��ʼ��
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
            throw Mdb_Exception(__FILE__, __LINE__, "���� [%s]  �ѽ�!", m_indexName.c_str());
        else
            throw Mdb_Exception(__FILE__, __LINE__, "���� [%s]  δָ��hashͰ��С!", m_indexName.c_str());
    }
}

// ��ʼ��һ��������attach�ڴ棩
bool IndexHashOnUndo::initialization()
{
    m_pUndoMemMgr->getIndexDescByName(m_indexName.c_str(), m_indexDesc);
    m_memMgr->getPhAddr(m_indexDesc->m_header, (char *&)m_hashTable); // ��ȡָ��hashͰ���׵�ַ
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
    // ����ǻ�ȡȫ������,����Ĳ�ѯhashkeΪ��,�򷵻�ȫ���ַ��
    if (pInputParams == NULL
            || pInputParams->m_iNum == 0)
    {
        if (iRowCount == -1) // ȫ����
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
    m_memMgr->getPhAddr(m_hashTable[val].m_nodepos, (char *&)ptr1);  // ��ȡhash��ָ���bucket��ʵ�ʵ�ַ
    while (ptr1 != NULL)
    {
        m_memMgr->getPhAddr(ptr1->m_data, (char *&)pRecord); // ��ȡ���ݼ�¼��ʵ�ʵ�ַ
        if (memcmp(m_cSearchKey, this->getIndexKey(pRecord), m_iHashKeyLenth) == 0)
        {
            ptr2 = ptr1;
            while (ptr2 != NULL)
            {
                result.m_pos = ptr2->m_data;
                m_memMgr->getPhAddr(result);           // ������Ե�ַ,�õ�ʵ�ʵ�ַ
                results.push_back(result);             // ѡ������ƥ�������ֶεļ�¼
                ++i;
                m_memMgr->getPhAddr(ptr2->m_sameNext, (char *&)ptr2); // ���������б�
            }
            break;
        }
        m_memMgr->getPhAddr(ptr1->m_diffNext, (char *&)ptr1); // ���������б�
    }
    return i;
}

// �߼�����һ��selectһ��,ֻ�Ǵ����������ʽ��ͬ
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
    // ����ǻ�ȡȫ������,����Ĳ�ѯhashkeΪ��,�򷵻�ȫ���ַ��
    if (hashkey == NULL)
    {
        if (iRowCount == -1) // ȫ����
            return this->selectALL(results);
        else
            return this->selectLimited(results, iRowCount);
    }
    unsigned val = HashCode::hash(hashkey, m_iHashKeyLenth) % m_size;
    results.clear();
    if (NULL_SHMPOS == m_hashTable[val].m_nodepos)
        return 0;
    m_memMgr->getPhAddr(m_hashTable[val].m_nodepos, (char *&)ptr1);  // ��ȡhash��ָ���bucket��ʵ�ʵ�ַ
    while (ptr1 != NULL)
    {
        m_memMgr->getPhAddr(ptr1->m_data, (char *&)pRecord); // ��ȡ���ݼ�¼��ʵ�ʵ�ַ
        if (memcmp(hashkey, this->getIndexKey(pRecord), m_iHashKeyLenth) == 0)
        {
            ptr2 = ptr1;
            while (ptr2 != NULL)
            {
                result.m_pos = ptr2->m_data;
                m_memMgr->getPhAddr(result);           // ������Ե�ַ,�õ�ʵ�ʵ�ַ
                results.push_back(result);             // ѡ������ƥ�������ֶεļ�¼
                ++i;
                m_memMgr->getPhAddr(ptr2->m_sameNext, (char *&)ptr2); // ���������б�
            }
            break;
        }
        m_memMgr->getPhAddr(ptr1->m_diffNext, (char *&)ptr1); // ���������б�
    }
    return i;
}

//����һ���ڵ�
bool IndexHashOnUndo::insert(const char* hashkey       //��ѯ�ؼ���
                             , const MdbAddress& theMdbAddress
                             , TransResource* pTransRes
                             , TableOnUndo* pTransTable)
{
    unsigned val;
    BucketIdxOnUndo* ptr1;
    MdbAddress newMdbAddr;
    void* pRecord = 0;
    val = HashCode::hash(hashkey, m_iHashKeyLenth) % m_size;
    // ���hash���и��±��Ӧ��ֵΪ��,��ֱ�Ӳ���
    if (NULL_SHMPOS == m_hashTable[val].m_nodepos)
    {
        m_pUndoMemMgr->allocateIdxMem(m_indexDesc, newMdbAddr);
        ((BucketIdxOnUndo*)(newMdbAddr.m_addr))->m_diffNext = NULL_SHMPOS;
        ((BucketIdxOnUndo*)(newMdbAddr.m_addr))->m_sameNext = NULL_SHMPOS;
        ((BucketIdxOnUndo*)(newMdbAddr.m_addr))->m_data = theMdbAddress.m_pos;
        m_hashTable[val].m_nodepos = newMdbAddr.m_pos;
        return true;
    }
    // ���hash���еĸ��±��Ӧ��ֵ��Ϊ��
    m_memMgr->getPhAddr(m_hashTable[val].m_nodepos, (char *&)ptr1);  // ��ȡhash��ָ���bucket��ʵ�ʵ�ַ
    while (ptr1 != NULL)
    {
        m_memMgr->getPhAddr(ptr1->m_data, (char *&)pRecord); // ��ȡ���ݼ�¼��ʵ�ʵ�ַ
        if (memcmp(hashkey, this->getIndexKey(pRecord), m_iHashKeyLenth) == 0)
        {
            if (m_indexDesc->m_indexDef.m_isUnique)
            {
                // ���������,���׳������ظ��쳣
                throw Mdb_Exception(__FILE__, __LINE__, "ֵ [%s] �����ظ�", hashkey);
            }
            m_pUndoMemMgr->allocateIdxMem(m_indexDesc, newMdbAddr);
            ((BucketIdxOnUndo*)(newMdbAddr.m_addr))->m_diffNext = NULL_SHMPOS;
            //��ͬ��key ���Ӻ������� �����������ĵڶ����ڵ㡣��ֱ�Ӳ����׽ڵ��ԭ���Ǽ���������������
            ((BucketIdxOnUndo*)(newMdbAddr.m_addr))->m_sameNext = ptr1->m_sameNext;
            ((BucketIdxOnUndo*)(newMdbAddr.m_addr))->m_data = theMdbAddress.m_pos;
            ptr1->m_sameNext = newMdbAddr.m_pos;
            return true;
        }
        m_memMgr->getPhAddr(ptr1->m_diffNext, (char *&)ptr1); // ���������б�
    }
    //hashֵ��ͬ,��ȴ�ǲ�ͬ��key ������������
    m_pUndoMemMgr->allocateIdxMem(m_indexDesc, newMdbAddr);
    ((BucketIdxOnUndo*)(newMdbAddr.m_addr))->m_diffNext = m_hashTable[val].m_nodepos;
    ((BucketIdxOnUndo*)(newMdbAddr.m_addr))->m_sameNext = NULL_SHMPOS;
    ((BucketIdxOnUndo*)(newMdbAddr.m_addr))->m_data = theMdbAddress.m_pos;
    m_hashTable[val].m_nodepos = newMdbAddr.m_pos;
    return true;
}

//����hashkey ɾ��һ���ڵ�
bool IndexHashOnUndo::deleteIdx(const char* hashkey       //��ѯ�ؼ���
                                , const MdbAddress& theMdbAddress)
{
    BucketIdxOnUndo* ptr1, *ptr2, *pLastVertical = NULL, *pLastParallel = NULL;
    MdbAddress ptr2MdbAddress ;
    void* pRecord = 0;
    unsigned val = HashCode::hash(hashkey, m_iHashKeyLenth) % m_size;
    //û�ж�Ӧ�Ľڵ�
    if (NULL_SHMPOS == m_hashTable[val].m_nodepos)
        return false;
    for (m_memMgr->getPhAddr(m_hashTable[val].m_nodepos, (char *&)ptr1)
            ; NULL != ptr1
            ; pLastVertical = ptr1, m_memMgr->getPhAddr(ptr1->m_diffNext, (char *&)ptr1)
        )
    {
        m_memMgr->getPhAddr(ptr1->m_data, (char *&)pRecord); // ��ȡ���ݼ�¼��ʵ�ʵ�ַ
        if (memcmp(hashkey, this->getIndexKey(pRecord), m_iHashKeyLenth) == 0) // ���hashֵһ�������Ҳ����������ʱ����ɾ��
        {
            //cout<<"------------------------------------------"<<endl;
            // ��¼����ڵ��׽ڵ�
            BucketIdxOnUndo* pFirstSameKeyElement = ptr1;
            // ���������б�
            for (ptr2 = ptr1, pLastParallel = NULL
                                              ; NULL != ptr2
                    ; pLastParallel = ptr2, m_memMgr->getPhAddr(ptr2->m_sameNext, (char *&)ptr2)
                )
            {
                if ((ptr2->m_data) == theMdbAddress.m_pos)   // ��¼����Ե�ַ��ͬʱ ɾ��
                {
                    // ����Ǻ���ڵ��׽ڵ�
                    if (pLastParallel == NULL || pFirstSameKeyElement == ptr2)
                    {
                        // ɾ����һ�е�һ��,�ҵ�һ��ֻ��Ψһһ��
                        if (pLastVertical == NULL && ptr2->m_sameNext == NULL_SHMPOS)
                        {
                            m_hashTable[val].m_nodepos = ptr2->m_diffNext;
                        }
                        // ɾ����һ�е�һ��,����һ���в�ֹһ��
                        else if (pLastVertical == NULL && ptr2->m_sameNext != NULL_SHMPOS)
                        {
                            m_hashTable[val].m_nodepos = ptr2->m_sameNext;
                            m_memMgr->getPhAddr(ptr2->m_sameNext, (char *&)pFirstSameKeyElement);
                            pFirstSameKeyElement->m_diffNext = ptr2->m_diffNext;
                        }
                        // ɾ���ǵ�һ�еĵ�һ��,����һ��ֻ��Ψһһ��
                        else if (pLastVertical != NULL && ptr2->m_sameNext == NULL_SHMPOS)
                        {
                            pLastVertical->m_diffNext = ptr2->m_diffNext;
                        }
                        // ɾ���ǵ�һ�еĵ�һ��,����һ���в�ֹһ��
                        else
                        {
                            pLastVertical->m_diffNext = ptr2->m_sameNext;
                            m_memMgr->getPhAddr(ptr2->m_sameNext, (char *&)pFirstSameKeyElement);
                            pFirstSameKeyElement->m_diffNext = ptr2->m_diffNext;
                        }
                    }
                    else  // ������Ǻ���ڵ��׽ڵ�
                    {
                        pLastParallel->m_sameNext = ptr2->m_sameNext;
                    }
                    ptr2MdbAddress.m_addr = (char*)ptr2;
                    m_memMgr->getShmPos(ptr2MdbAddress);
                    m_pUndoMemMgr->freeIdxMem(m_indexDesc, ptr2MdbAddress);  //�ͷ�index�ڵ�ռ�
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

//ȫ��ѡȡ
int IndexHashOnUndo::selectALL(vector<MdbAddress> &results)
{
    BucketIdxOnUndo* ptr1, *ptr2, *next1, *next2;
    MdbAddress result;
    results.clear();
    for (int i = 0; i < m_size; i++)
    {
        if (NULL_SHMPOS != m_hashTable[i].m_nodepos)
        {
            // ���������б�
            for (m_memMgr->getPhAddr(m_hashTable[i].m_nodepos, (char *&)ptr1)
                    ; NULL != ptr1
                    ; ptr1 = next1
                )
            {
                m_memMgr->getPhAddr(ptr1->m_diffNext, (char *&)next1);
                // ���������б�
                for (ptr2 = ptr1
                            ; NULL != ptr2
                        ; ptr2 = next2
                    )
                {
                    m_memMgr->getPhAddr(ptr2->m_sameNext, (char *&)next2);
                    result.m_pos = ptr2->m_data;
                    m_memMgr->getPhAddr(result);           // ������Ե�ַ,�õ�ʵ�ʵ�ַ
                    results.push_back(result);
                }
            }
        }
    }
    return results.size();
}

//�޶����ؼ�¼����select * from tableָ��
int IndexHashOnUndo::selectLimited(vector<MdbAddress> &results, int iRowCount)
{
    BucketIdxOnUndo* ptr1, *ptr2, *next1, *next2;
    MdbAddress result;
    results.clear();
    for (int i = 0; i < m_size; i++)
    {
        if (NULL_SHMPOS != m_hashTable[i].m_nodepos)
        {
            // ���������б�
            for (m_memMgr->getPhAddr(m_hashTable[i].m_nodepos, (char *&)ptr1)
                    ; NULL != ptr1
                    ; ptr1 = next1
                )
            {
                m_memMgr->getPhAddr(ptr1->m_diffNext, (char *&)next1);
                // ���������б�
                for (ptr2 = ptr1
                            ; NULL != ptr2
                        ; ptr2 = next2
                    )
                {
                    m_memMgr->getPhAddr(ptr2->m_sameNext, (char *&)next2);
                    result.m_pos = ptr2->m_data;
                    m_memMgr->getPhAddr(result);           // ������Ե�ַ,�õ�ʵ�ʵ�ַ
                    results.push_back(result);
                    if (results.size() >= iRowCount)
                    {
                        break; // �����¼������ָ����rowcount,�򷵻�,����ѡȡ
                    }
                }
                if (results.size() >= iRowCount)
                {
                    break; // �����¼������ָ����rowcount,�򷵻�,����ѡȡ
                }
            }
        }
        if (results.size() >= iRowCount)
        {
            break; // �����¼������ָ����rowcount,�򷵻�,����ѡȡ
        }
    }
    return results.size();
}
bool IndexHashOnUndo::getHashPercent(int& iHashSize
                                     , int& iUsedHash
                                     , int& iRecordCount
                                     , map<int, int>& mHashCal)
{
    iHashSize = m_size; //����
    iUsedHash = 0;
    iRecordCount = 0;
    //ͳ���Ѿ�ʹ�õ�HashͰ�ĸ���
    for (int i = 0; i < m_size; i++)
    {
        if (m_hashTable[i].m_nodepos == NULL_SHMPOS)
            continue;
        iUsedHash ++;
    }
    //ͳ��Hash�ķֲ�
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
	cout<<"�������......"<<endl;
	t_destIndex->initialization();
	t_destIndex->truncate();//�����

	BucketIdxOnUndo *ptr1,*ptr2,*next1,*next2;
	MdbAddress result;
	IndexDesc * t_destIndexdesc=t_destIndex->getIndexDesc();
	this->initialization();

	long lRowCount=0;
	for (int i=0;i<m_size;i++)
	{
		if (NULL_SHMPOS != m_hashTable[i].m_nodepos)
		{
			// ���������б�
			for (m_memMgr->getPhAddr(m_hashTable[i].m_nodepos,(char *&)ptr1)
					;NULL!=ptr1
					;ptr1=next1
				 )
			{
				m_memMgr->getPhAddr(ptr1->m_diffNext,(char *&)next1);

				// ���������б�
				for( ptr2=ptr1
						;NULL!=ptr2
						;ptr2=next2
					)
				{
					m_memMgr->getPhAddr(ptr2->m_sameNext,(char *&)next2);
					memset(&result,0,sizeof(MdbAddress));
					result.m_pos = ptr2->m_data;
					m_memMgr->getPhAddr( result );         // ������Ե�ַ,�õ�ʵ�ʵ�ַ
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
// add by chenm 2009-5-18 23:07:28 ����ȫ��ɨ��,��ʱʵ�ְ�ͬһ��hash�ڵ��µļ�¼������
int IndexHashOnUndo::selectRowByRow(vector<MdbAddress> &results, int iRowCount)
{
    if (iRowCount == SELECT_ROW_BY_ROW)
    {
        // �״ε���,�ӱ��һ����¼��ʼ����,�ڲ�����������
        m_iHashPosi = 0;
        //m_iXPosi    = 0;  // ����ڵ������
        //m_iYPosi    = 0;  // ����ڵ������
    }
    BucketIdxOnUndo* ptr1, *ptr2, *next1, *next2;
    MdbAddress result;
    results.clear();
    for (; m_iHashPosi < m_size; ++m_iHashPosi)
    {
        if (NULL_SHMPOS != m_hashTable[m_iHashPosi].m_nodepos)
        {
            // ���������б�
            for (m_memMgr->getPhAddr(m_hashTable[m_iHashPosi].m_nodepos, (char *&)ptr1)
                    ; NULL != ptr1
                    ; ptr1 = next1
                )
            {
                m_memMgr->getPhAddr(ptr1->m_diffNext, (char *&)next1);
                // ���������б�
                for (ptr2 = ptr1
                            ; NULL != ptr2
                        ; ptr2 = next2
                    )
                {
                    m_memMgr->getPhAddr(ptr2->m_sameNext, (char *&)next2);
                    result.m_pos = ptr2->m_data;
                    m_memMgr->getPhAddr(result);           // ������Ե�ַ,�õ�ʵ�ʵ�ַ
                    results.push_back(result);
                }
            }
            // ��ʱʵ�ְ�ͬһ��hash�ڵ��µļ�¼������
            ++m_iHashPosi;
            break;
        }
    }
    return results.size();
}
