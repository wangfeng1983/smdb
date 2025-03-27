#include "IndexSegsMgr.h"
#include "TableDescript.h"

IndexSegsMgr::IndexSegsMgr()
{
}
IndexSegsMgr::~IndexSegsMgr()
{
}

//r_spaceaddr ��ռ���ʼ��ַ     r_offset index����ʼƫ����
//�����ε��ܴ�С r_idxsegsize (�������б�)
//r_flag  1 ��һ�γ�ʼ������Ҫ��ʼ�����б���Ϣ
bool IndexSegsMgr::initialize(char*   r_spaceaddr,
                              const size_t& r_offset,
                              const size_t& r_idxsegsize,
                              const int r_flag)
{
    m_spaceaddr   = r_spaceaddr;
    m_pIdxSegInfo = (IndexSegInfo*)(m_spaceaddr + r_offset);
    if (r_flag == 1)
    {
        //��Ҫ��ʼ����һ�α�ռ�
        size_t t_offSet = r_offset + sizeof(IndexSegInfo);
        m_pIdxSegInfo->f_init(r_offset, t_offSet);
        //3.��ʼ������Ϣ
        HashIndexSeg* t_pSegInfo;
        t_pSegInfo = (HashIndexSeg*)(m_spaceaddr + t_offSet);
        t_pSegInfo->m_sOffSet = t_offSet;
        t_pSegInfo->m_segSize = r_idxsegsize - (t_offSet - r_offset);
        t_pSegInfo->m_prev = t_pSegInfo->m_next = 0;
        t_pSegInfo->m_scn = T_SCN(); //��ʼ��
    }
    return true;
}



//r_shmPos��ָ��Hash��ShmPositioin[]���׵�ַ
bool IndexSegsMgr::createHashIndex(IndexDesc* r_idxDesc, size_t& r_offset, const T_SCN& r_scn)
{
    m_pIdxSegInfo->m_scn = r_scn;
    bool t_bRet = true;
    try
    {
        bool t_flag = false; //�Ƿ����㹻�Ŀռ�
        if (m_pIdxSegInfo->m_fIdleSeg == 0)
        {
#ifdef _DEBUG_
            cout << "�����ռ���ж���Ϊ��!" << __FILE__ << __LINE__ << endl;
#endif
            t_bRet = false;
            throw Mdb_Exception(__FILE__, __LINE__, "�����ռ���ж���Ϊ��!");
        }
        size_t         t_needSize;
        HashIndexSeg* t_pSegInfo;
        HashIndexSeg* t_prevSegInfo;
        HashIndexSeg* t_nextSegInfo;
        //0.�������ε���С�ռ�t_needSize
        t_needSize = sizeof(HashIndexSeg) + r_idxDesc->m_indexDef.m_hashSize * sizeof(HashIndexNode);
        //1.�ҵ�������������Ŀ��ж�t_pSegInfo
        t_pSegInfo = (HashIndexSeg*)(m_spaceaddr + m_pIdxSegInfo->m_fIdleSeg);
        while (1)
        {
            if (t_pSegInfo->m_segSize >= t_needSize)
            {
                t_flag = true;
                break;
            }
            if (t_pSegInfo->m_next == 0) break;
            t_pSegInfo = (HashIndexSeg*)(m_spaceaddr + t_pSegInfo->m_next);
        };
        if (t_flag == false)
        {
#ifdef _DEBUG_
            cout << "�����ռ�û���㹻���������пռ�!" << __FILE__ << __LINE__ << endl;
#endif
            t_bRet = false;
            throw Mdb_Exception(__FILE__, __LINE__, "�����ռ�û���㹻���������пռ�!");
        }
        if (t_pSegInfo->m_segSize < (t_needSize + 2 * sizeof(HashIndexSeg)))
        {
            //2.����ռ�պ�(�˷ѿռ�<2*sizeof(HashIndexSeg)����ӿ��ж������Ƴ�
            if (t_pSegInfo->m_prev == 0)
            {
                //��һ��
                m_pIdxSegInfo->m_fIdleSeg = t_pSegInfo->m_next;
            }
            else
            {
                t_prevSegInfo = (HashIndexSeg*)(m_spaceaddr + t_pSegInfo->m_prev);
                t_prevSegInfo->m_next = t_pSegInfo->m_next;
                t_prevSegInfo->m_scn = r_scn;
            }
            if (t_pSegInfo->m_next == 0)
            {
                //���һ��
                m_pIdxSegInfo->m_lIdleSeg = t_pSegInfo->m_prev;
            }
            else
            {
                t_nextSegInfo = (HashIndexSeg*)(m_spaceaddr + t_pSegInfo->m_next);
                t_nextSegInfo->m_prev = t_pSegInfo->m_prev;
                t_nextSegInfo->m_scn  = r_scn;
            }
        }
        else
        {
            //3.���򣬷ָ�ö�
            t_prevSegInfo = t_pSegInfo;
            t_pSegInfo   = (HashIndexSeg*)(((char*)t_prevSegInfo) + (t_prevSegInfo->m_segSize - t_needSize));
            t_prevSegInfo->m_segSize = t_prevSegInfo->m_segSize - t_needSize;
            t_pSegInfo->m_sOffSet = t_prevSegInfo->m_sOffSet + t_prevSegInfo->m_segSize;
            t_pSegInfo->m_segSize = t_needSize;
            t_prevSegInfo->m_scn = r_scn; //��һ�θ������б���Ϣ
        }
        //����m_pIdxSegInfo��Ϣ
        t_pSegInfo->m_hashSize = r_idxDesc->m_indexDef.m_hashSize;
        t_pSegInfo->m_scn      = r_scn;
        memcpy(t_pSegInfo->m_idxName, r_idxDesc->m_indexDef.m_indexName, sizeof(T_NAMEDEF));
        //4.������Ķβ�����ʹ�ö�β��
        if (m_pIdxSegInfo->m_lUsedSeg == 0)
        {
            //��һ��
            m_pIdxSegInfo->m_lUsedSeg = m_pIdxSegInfo->m_fUsedSeg = t_pSegInfo->m_sOffSet;
            t_pSegInfo->m_prev = t_pSegInfo->m_next = 0;
        }
        else
        {
            t_pSegInfo->m_prev = m_pIdxSegInfo->m_lUsedSeg;
            t_pSegInfo->m_next = 0;
            t_prevSegInfo = (HashIndexSeg*)(m_spaceaddr + m_pIdxSegInfo->m_lUsedSeg);
            m_pIdxSegInfo->m_lUsedSeg = t_prevSegInfo->m_next = t_pSegInfo->m_sOffSet;
            t_prevSegInfo->m_scn     = r_scn; //�б���Ϣ�仯����Ҫ����ʱ���֪ͨchkp 2010/8/23 13:51:03
        }
        r_offset = t_pSegInfo->m_sOffSet + sizeof(HashIndexSeg);
        t_pSegInfo->initHash((char*)t_pSegInfo, r_scn);
    }
    catch (Mdb_Exception& e)
    {
#ifdef _DEBUG_
        cout << e.GetString() << endl;
#endif
        t_bRet = false;
    }
    return t_bRet;
}
//r_shmPos��ָ��Hash��ShmPositioin[]���׵�ַ
void IndexSegsMgr::initHashSeg(const size_t& r_offset, const T_SCN& r_scn)
{
    HashIndexSeg* t_pSegInfo;
    t_pSegInfo = (HashIndexSeg*)(m_spaceaddr + r_offset - sizeof(HashIndexSeg));
    t_pSegInfo->initHash((char*)t_pSegInfo, r_scn);
    t_pSegInfo->m_scn = r_scn;
}

//r_shmPos��ָ��Hash��ShmPositioin[]���׵�ַ
bool IndexSegsMgr::dropHashIdex(const size_t& r_offset, const T_SCN& r_scn)
{
    m_pIdxSegInfo->m_scn = r_scn;
    bool t_bRet = true;
    try
    {
        bool t_flag;
        //1. ����HashIndexSeg����ʼλ��
        size_t t_segOffSet;
        t_segOffSet  = r_offset;
        t_segOffSet -= sizeof(HashIndexSeg);
        //2.����ʹ�õĶ��б����ҵ���Ӧ�Ķ�
        if (m_pIdxSegInfo->m_fUsedSeg == 0)
        {
#ifdef _DEBUG_
            cout << "�����ռ���ʹ�ö���Ϊ��!" << __FILE__ << __LINE__ << endl;
#endif
            t_bRet = false;
            throw Mdb_Exception(__FILE__, __LINE__, "�����ռ���ʹ�ö���Ϊ��!");
        }
        HashIndexSeg* t_pSegInfo;
        t_pSegInfo = (HashIndexSeg*)(m_spaceaddr + m_pIdxSegInfo->m_fUsedSeg);
        t_flag = false;
        while (1)
        {
            if (t_pSegInfo->m_sOffSet == t_segOffSet)
            {
                t_flag = true;
                break;
            }
            if (t_pSegInfo->m_next == 0) break;
            t_pSegInfo = (HashIndexSeg*)(m_spaceaddr + t_pSegInfo->m_next);
        };
        if (t_flag == false)
        {
#ifdef _DEBUG_
            cout << "�����ռ���ʹ�ö���δ�ҵ���Ӧ�Ķ�!" << __FILE__ << __LINE__ << endl;
            cout << "r_offset=" << r_offset << " t_segOffSet=" << t_segOffSet << endl;
#endif
            t_bRet = false;
            throw Mdb_Exception(__FILE__, __LINE__, "�����ռ���ʹ�ö���δ�ҵ���Ӧ�Ķ�!");
        }
        HashIndexSeg* t_pPrevSegInfo, *t_pNextSegInfo;
        t_pSegInfo->m_scn = r_scn;
        //3.���öδ���ʹ�ü�¼���Ƴ�
        if (t_pSegInfo->m_sOffSet == m_pIdxSegInfo->m_fUsedSeg)
        {
            //��һ��
            m_pIdxSegInfo->m_fUsedSeg = t_pSegInfo->m_next;
        }
        if (t_pSegInfo->m_sOffSet == m_pIdxSegInfo->m_lUsedSeg)
        {
            //���һ��
            m_pIdxSegInfo->m_lUsedSeg = t_pSegInfo->m_prev;
        }
        if (t_pSegInfo->m_prev != 0)
        {
            t_pPrevSegInfo = (HashIndexSeg*)(m_spaceaddr + t_pSegInfo->m_prev);
            t_pPrevSegInfo->m_next = t_pSegInfo->m_next;
            t_pPrevSegInfo->m_scn = r_scn;
        }
        if (t_pSegInfo->m_next != 0)
        {
            t_pNextSegInfo = (HashIndexSeg*)(m_spaceaddr + t_pSegInfo->m_next);
            t_pNextSegInfo->m_prev = t_pSegInfo->m_prev;
            t_pNextSegInfo->m_scn = r_scn;
        }
        //4.���öμ�����ж���
        if (m_pIdxSegInfo->m_fIdleSeg == 0)
        {
            //��һ��
            m_pIdxSegInfo->m_lIdleSeg = m_pIdxSegInfo->m_fIdleSeg = t_pSegInfo->m_sOffSet;
            t_pSegInfo->m_prev = t_pSegInfo->m_next = 0;
        }
        else
        {
            //4.1 �ҵ�ƫ������t_pSegInfo��ĵ�һ����
            t_flag = false;
            t_pNextSegInfo = (HashIndexSeg*)(m_spaceaddr + m_pIdxSegInfo->m_fIdleSeg);
            while (1)
            {
                if (t_pSegInfo->m_sOffSet < t_pNextSegInfo->m_sOffSet)
                {
                    t_flag = true;
                    break;
                }
                if (t_pNextSegInfo->m_next == 0) break;
                t_pNextSegInfo = (HashIndexSeg*)(m_spaceaddr + t_pNextSegInfo->m_next);
            };
            if (t_flag == false)
            {
                //���Ƿ��ܺ����һ�κϲ�
                t_pPrevSegInfo = (HashIndexSeg*)(m_spaceaddr + m_pIdxSegInfo->m_lIdleSeg);
                if (t_pPrevSegInfo->m_sOffSet + t_pPrevSegInfo->m_segSize == t_pSegInfo->m_sOffSet)
                {
                    //t_pPrevSegInfo��t_pSegInfo�ϲ�������t_pSegInfo���ƺϲ������Ϣ
                    t_pPrevSegInfo->m_segSize = t_pPrevSegInfo->m_segSize + t_pSegInfo->m_segSize;
                    t_pSegInfo = t_pPrevSegInfo;
                    t_pPrevSegInfo->m_scn = r_scn;
                }
                else
                {
                    //׷�������һ��
                    t_pSegInfo->m_prev = m_pIdxSegInfo->m_lIdleSeg;
                    t_pSegInfo->m_next = 0;
                    t_pPrevSegInfo = (HashIndexSeg*)(m_spaceaddr + m_pIdxSegInfo->m_lIdleSeg);
                    m_pIdxSegInfo->m_lIdleSeg = t_pPrevSegInfo->m_next = t_pSegInfo->m_sOffSet;
                    t_pPrevSegInfo->m_scn = r_scn;
                }
            }
            else
            {
                //����t_pNextSegInfoǰ
                t_pSegInfo->m_prev = t_pNextSegInfo->m_prev;
                t_pSegInfo->m_next = t_pNextSegInfo->m_sOffSet;
                t_pNextSegInfo->m_prev = t_pSegInfo->m_sOffSet;
                t_pNextSegInfo->m_scn = r_scn;
                if (t_pSegInfo->m_prev != 0)
                {
                    //��t_pSegInfo֮ǰ���ж�
                    t_pPrevSegInfo = (HashIndexSeg*)(m_spaceaddr + t_pSegInfo->m_prev);
                    t_pPrevSegInfo->m_next = t_pSegInfo->m_sOffSet;
                    t_pPrevSegInfo->m_scn = r_scn;
                    //5.�ϲ����ж���
                    //5.1 ��t_pPrevSegInfo�ܷ��t_pSegInfo�ϲ�
                    if (t_pPrevSegInfo->m_sOffSet + t_pPrevSegInfo->m_segSize == t_pSegInfo->m_sOffSet)
                    {
                        //t_pPrevSegInfo��t_pSegInfo�ϲ�������t_pSegInfo���ƺϲ������Ϣ
                        t_pPrevSegInfo->m_segSize = t_pPrevSegInfo->m_segSize + t_pSegInfo->m_segSize;
                        t_pPrevSegInfo->m_next   = t_pSegInfo->m_next;
                        t_pSegInfo = t_pPrevSegInfo;
                        t_pNextSegInfo->m_prev = t_pSegInfo->m_sOffSet;
                    }
                }
                else
                {
                    //���ڵ�һ��ǰ����Ҫ���� m_pIdxSegInfo->m_fIdleSeg
                    m_pIdxSegInfo->m_fIdleSeg = t_pSegInfo->m_sOffSet;
                }
                //5.2 ��t_pSegInfo�ܷ��t_pNextSegInfo�ϲ�
                if (t_pSegInfo->m_sOffSet + t_pSegInfo->m_segSize == t_pNextSegInfo->m_sOffSet)
                {
                    //t_pPrevSegInfo��t_pSegInfo�ϲ�������t_pSegInfo���ƺϲ������Ϣ
                    t_pSegInfo->m_segSize = t_pSegInfo->m_segSize + t_pNextSegInfo->m_segSize;
                    t_pSegInfo->m_next   = t_pNextSegInfo->m_next;
                    if (t_pSegInfo->m_next == 0)
                    {
                        //����ϲ��������һ�Σ������m_lIdleSegֵ
                        m_pIdxSegInfo->m_lIdleSeg = t_pSegInfo->m_sOffSet;
                    }
                    else
                    {
                        //Ҫ�� t_pNextSegInfo->next �ڵ�� m_prevֵ�޸�Ϊ t_pSegInfo->m_sOffSet;
                        t_pNextSegInfo = (HashIndexSeg*)(m_spaceaddr + t_pSegInfo->m_next);
                        t_pNextSegInfo->m_prev = t_pSegInfo->m_sOffSet;
                        t_pNextSegInfo->m_scn = r_scn;
                    }
                }
            }
        }
    }
    catch (Mdb_Exception e)
    {
#ifdef _DEBUG_
        cout << e.GetString() << endl;
#endif
        t_bRet = false;
    }
    return t_bRet;
}
bool IndexSegsMgr::dumpInfo(ostream& r_os)
{
    r_os << "---------��������Ϣ----------------------" << endl;
    r_os << "m_fIdleSeg = " << m_pIdxSegInfo->m_fIdleSeg << endl;
    r_os << "m_lIdleSeg = " << m_pIdxSegInfo->m_lIdleSeg << endl;
    r_os << "m_fUsedSeg = " << m_pIdxSegInfo->m_fUsedSeg << endl;
    r_os << "m_lUsedSeg = " << m_pIdxSegInfo->m_lUsedSeg << endl;
    r_os << "------��ʹ����������Ϣ----------------" << endl;
    HashIndexSeg* t_phashIndex ;
    if (m_pIdxSegInfo->m_fUsedSeg != 0)
    {
        t_phashIndex = (HashIndexSeg*)(m_spaceaddr + m_pIdxSegInfo->m_fUsedSeg);
        while (1)
        {
            r_os << "--------������" << t_phashIndex->m_idxName << "----" << endl;
            r_os << "m_sOffSet =" << t_phashIndex->m_sOffSet << endl;
            r_os << "m_segSize =" << t_phashIndex->m_segSize << endl;
            r_os << "m_hashSize=" << t_phashIndex->m_hashSize << endl;
            r_os << "m_prev    =" << t_phashIndex->m_prev << endl;
            r_os << "m_next    =" << t_phashIndex->m_next << endl;
            if (t_phashIndex->m_next == 0) break;
            t_phashIndex = (HashIndexSeg*)(m_spaceaddr + t_phashIndex->m_next);
        };
    }
    r_os << "------������������Ϣ--����-------------" << endl;
    if (m_pIdxSegInfo->m_fIdleSeg != 0)
    {
        t_phashIndex = (HashIndexSeg*)(m_spaceaddr + m_pIdxSegInfo->m_fIdleSeg);
        while (1)
        {
            r_os << "--------������" << t_phashIndex->m_idxName << "----" << endl;
            r_os << "m_sOffSet =" << t_phashIndex->m_sOffSet << endl;
            r_os << "m_segSize =" << t_phashIndex->m_segSize << endl;
            r_os << "m_hashSize=" << t_phashIndex->m_hashSize << endl;
            r_os << "m_prev    =" << t_phashIndex->m_prev << endl;
            r_os << "m_next    =" << t_phashIndex->m_next << endl;
            if (t_phashIndex->m_next == 0) break;
            t_phashIndex = (HashIndexSeg*)(m_spaceaddr + t_phashIndex->m_next);
        };
    }
    r_os << "---------��������Ϣ���� ��ֹ!---------" << endl;
    return true;
}

size_t IndexSegsMgr::getUsedSize()
{
    HashIndexSeg* pHashIndexSeg;
    size_t totalFreeSegSize = 0;
    if (m_pIdxSegInfo->m_fIdleSeg != 0)
    {
        pHashIndexSeg = (HashIndexSeg*)(m_spaceaddr + m_pIdxSegInfo->m_fIdleSeg);
        while (1)
        {
            totalFreeSegSize += pHashIndexSeg->m_segSize ;
            if (pHashIndexSeg->m_next == 0)
                break;
            pHashIndexSeg = (HashIndexSeg*)(m_spaceaddr + pHashIndexSeg->m_next);
        }
    }
    return totalFreeSegSize;
}

IndexSegInfo* IndexSegsMgr::getSegInfos()
{
    return m_pIdxSegInfo;
}
void IndexSegsMgr::startmdb_init()
{
    m_pIdxSegInfo->m_scn.setOffSet(0);
    HashIndexSeg* t_phashIndex = NULL ;
    HashIndexNode* t_pnode = NULL;
    if (m_pIdxSegInfo->m_fUsedSeg != 0)
    {
        t_phashIndex = (HashIndexSeg*)(m_spaceaddr + m_pIdxSegInfo->m_fUsedSeg);
        while (1)
        {
            t_phashIndex->m_scn.setOffSet(0); //����Hash������scn
            t_pnode = (HashIndexNode*)(((char*)t_phashIndex) + sizeof(HashIndexSeg));
            //����Hash������Ͱ�ڵ��scn
            for (size_t t_l = 0; t_l < t_phashIndex->m_hashSize; ++t_l)
            {
                t_pnode->m_scn.setOffSet(0);
                t_pnode->unlock();
                ++t_pnode;
            }
            if (t_phashIndex->m_next == 0) break;
            t_phashIndex = (HashIndexSeg*)(m_spaceaddr + t_phashIndex->m_next);
        };
    }
    if (m_pIdxSegInfo->m_fIdleSeg != 0)
    {
        t_phashIndex = (HashIndexSeg*)(m_spaceaddr + m_pIdxSegInfo->m_fIdleSeg);
        while (1)
        {
            t_phashIndex->m_scn.setOffSet(0);
            if (t_phashIndex->m_next == 0) break;
            t_phashIndex = (HashIndexSeg*)(m_spaceaddr + t_phashIndex->m_next);
        };
    }
}



