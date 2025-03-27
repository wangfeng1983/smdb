#include "IndexSegsMgr.h"
#include "TableDescript.h"

IndexSegsMgr::IndexSegsMgr()
{
}
IndexSegsMgr::~IndexSegsMgr()
{
}

//r_spaceaddr 表空间起始地址     r_offset index段起始偏移量
//索引段的总大小 r_idxsegsize (包含段列表)
//r_flag  1 第一次初始化，需要初始化段列表信息
bool IndexSegsMgr::initialize(char*   r_spaceaddr,
                              const size_t& r_offset,
                              const size_t& r_idxsegsize,
                              const int r_flag)
{
    m_spaceaddr   = r_spaceaddr;
    m_pIdxSegInfo = (IndexSegInfo*)(m_spaceaddr + r_offset);
    if (r_flag == 1)
    {
        //需要初始化第一段表空间
        size_t t_offSet = r_offset + sizeof(IndexSegInfo);
        m_pIdxSegInfo->f_init(r_offset, t_offSet);
        //3.初始化段信息
        HashIndexSeg* t_pSegInfo;
        t_pSegInfo = (HashIndexSeg*)(m_spaceaddr + t_offSet);
        t_pSegInfo->m_sOffSet = t_offSet;
        t_pSegInfo->m_segSize = r_idxsegsize - (t_offSet - r_offset);
        t_pSegInfo->m_prev = t_pSegInfo->m_next = 0;
        t_pSegInfo->m_scn = T_SCN(); //初始化
    }
    return true;
}



//r_shmPos是指向Hash中ShmPositioin[]的首地址
bool IndexSegsMgr::createHashIndex(IndexDesc* r_idxDesc, size_t& r_offset, const T_SCN& r_scn)
{
    m_pIdxSegInfo->m_scn = r_scn;
    bool t_bRet = true;
    try
    {
        bool t_flag = false; //是否有足够的空间
        if (m_pIdxSegInfo->m_fIdleSeg == 0)
        {
#ifdef _DEBUG_
            cout << "索引空间空闲队列为空!" << __FILE__ << __LINE__ << endl;
#endif
            t_bRet = false;
            throw Mdb_Exception(__FILE__, __LINE__, "索引空间空闲队列为空!");
        }
        size_t         t_needSize;
        HashIndexSeg* t_pSegInfo;
        HashIndexSeg* t_prevSegInfo;
        HashIndexSeg* t_nextSegInfo;
        //0.计算分配段的最小空间t_needSize
        t_needSize = sizeof(HashIndexSeg) + r_idxDesc->m_indexDef.m_hashSize * sizeof(HashIndexNode);
        //1.找到满足分配条件的空闲段t_pSegInfo
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
            cout << "索引空间没有足够的连续空闲空间!" << __FILE__ << __LINE__ << endl;
#endif
            t_bRet = false;
            throw Mdb_Exception(__FILE__, __LINE__, "索引空间没有足够的连续空闲空间!");
        }
        if (t_pSegInfo->m_segSize < (t_needSize + 2 * sizeof(HashIndexSeg)))
        {
            //2.如果空间刚好(浪费空间<2*sizeof(HashIndexSeg)，则从空闲队列中移出
            if (t_pSegInfo->m_prev == 0)
            {
                //第一段
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
                //最后一段
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
            //3.否则，分割该段
            t_prevSegInfo = t_pSegInfo;
            t_pSegInfo   = (HashIndexSeg*)(((char*)t_prevSegInfo) + (t_prevSegInfo->m_segSize - t_needSize));
            t_prevSegInfo->m_segSize = t_prevSegInfo->m_segSize - t_needSize;
            t_pSegInfo->m_sOffSet = t_prevSegInfo->m_sOffSet + t_prevSegInfo->m_segSize;
            t_pSegInfo->m_segSize = t_needSize;
            t_prevSegInfo->m_scn = r_scn; //上一段更新了列表信息
        }
        //设置m_pIdxSegInfo信息
        t_pSegInfo->m_hashSize = r_idxDesc->m_indexDef.m_hashSize;
        t_pSegInfo->m_scn      = r_scn;
        memcpy(t_pSegInfo->m_idxName, r_idxDesc->m_indexDef.m_indexName, sizeof(T_NAMEDEF));
        //4.将分配的段插入已使用段尾部
        if (m_pIdxSegInfo->m_lUsedSeg == 0)
        {
            //第一段
            m_pIdxSegInfo->m_lUsedSeg = m_pIdxSegInfo->m_fUsedSeg = t_pSegInfo->m_sOffSet;
            t_pSegInfo->m_prev = t_pSegInfo->m_next = 0;
        }
        else
        {
            t_pSegInfo->m_prev = m_pIdxSegInfo->m_lUsedSeg;
            t_pSegInfo->m_next = 0;
            t_prevSegInfo = (HashIndexSeg*)(m_spaceaddr + m_pIdxSegInfo->m_lUsedSeg);
            m_pIdxSegInfo->m_lUsedSeg = t_prevSegInfo->m_next = t_pSegInfo->m_sOffSet;
            t_prevSegInfo->m_scn     = r_scn; //列表信息变化，需要更新时间戳通知chkp 2010/8/23 13:51:03
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
//r_shmPos是指向Hash中ShmPositioin[]的首地址
void IndexSegsMgr::initHashSeg(const size_t& r_offset, const T_SCN& r_scn)
{
    HashIndexSeg* t_pSegInfo;
    t_pSegInfo = (HashIndexSeg*)(m_spaceaddr + r_offset - sizeof(HashIndexSeg));
    t_pSegInfo->initHash((char*)t_pSegInfo, r_scn);
    t_pSegInfo->m_scn = r_scn;
}

//r_shmPos是指向Hash中ShmPositioin[]的首地址
bool IndexSegsMgr::dropHashIdex(const size_t& r_offset, const T_SCN& r_scn)
{
    m_pIdxSegInfo->m_scn = r_scn;
    bool t_bRet = true;
    try
    {
        bool t_flag;
        //1. 计算HashIndexSeg的起始位置
        size_t t_segOffSet;
        t_segOffSet  = r_offset;
        t_segOffSet -= sizeof(HashIndexSeg);
        //2.从已使用的段列表中找到对应的段
        if (m_pIdxSegInfo->m_fUsedSeg == 0)
        {
#ifdef _DEBUG_
            cout << "索引空间已使用队列为空!" << __FILE__ << __LINE__ << endl;
#endif
            t_bRet = false;
            throw Mdb_Exception(__FILE__, __LINE__, "索引空间已使用队列为空!");
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
            cout << "索引空间已使用队列未找到相应的段!" << __FILE__ << __LINE__ << endl;
            cout << "r_offset=" << r_offset << " t_segOffSet=" << t_segOffSet << endl;
#endif
            t_bRet = false;
            throw Mdb_Exception(__FILE__, __LINE__, "索引空间已使用队列未找到相应的段!");
        }
        HashIndexSeg* t_pPrevSegInfo, *t_pNextSegInfo;
        t_pSegInfo->m_scn = r_scn;
        //3.将该段从已使用记录中移出
        if (t_pSegInfo->m_sOffSet == m_pIdxSegInfo->m_fUsedSeg)
        {
            //第一段
            m_pIdxSegInfo->m_fUsedSeg = t_pSegInfo->m_next;
        }
        if (t_pSegInfo->m_sOffSet == m_pIdxSegInfo->m_lUsedSeg)
        {
            //最后一段
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
        //4.将该段加入空闲队列
        if (m_pIdxSegInfo->m_fIdleSeg == 0)
        {
            //第一段
            m_pIdxSegInfo->m_lIdleSeg = m_pIdxSegInfo->m_fIdleSeg = t_pSegInfo->m_sOffSet;
            t_pSegInfo->m_prev = t_pSegInfo->m_next = 0;
        }
        else
        {
            //4.1 找到偏移量比t_pSegInfo大的第一个段
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
                //看是否能和最后一段合并
                t_pPrevSegInfo = (HashIndexSeg*)(m_spaceaddr + m_pIdxSegInfo->m_lIdleSeg);
                if (t_pPrevSegInfo->m_sOffSet + t_pPrevSegInfo->m_segSize == t_pSegInfo->m_sOffSet)
                {
                    //t_pPrevSegInfo和t_pSegInfo合并，并用t_pSegInfo来计合并后的信息
                    t_pPrevSegInfo->m_segSize = t_pPrevSegInfo->m_segSize + t_pSegInfo->m_segSize;
                    t_pSegInfo = t_pPrevSegInfo;
                    t_pPrevSegInfo->m_scn = r_scn;
                }
                else
                {
                    //追到到最后一段
                    t_pSegInfo->m_prev = m_pIdxSegInfo->m_lIdleSeg;
                    t_pSegInfo->m_next = 0;
                    t_pPrevSegInfo = (HashIndexSeg*)(m_spaceaddr + m_pIdxSegInfo->m_lIdleSeg);
                    m_pIdxSegInfo->m_lIdleSeg = t_pPrevSegInfo->m_next = t_pSegInfo->m_sOffSet;
                    t_pPrevSegInfo->m_scn = r_scn;
                }
            }
            else
            {
                //加在t_pNextSegInfo前
                t_pSegInfo->m_prev = t_pNextSegInfo->m_prev;
                t_pSegInfo->m_next = t_pNextSegInfo->m_sOffSet;
                t_pNextSegInfo->m_prev = t_pSegInfo->m_sOffSet;
                t_pNextSegInfo->m_scn = r_scn;
                if (t_pSegInfo->m_prev != 0)
                {
                    //在t_pSegInfo之前还有段
                    t_pPrevSegInfo = (HashIndexSeg*)(m_spaceaddr + t_pSegInfo->m_prev);
                    t_pPrevSegInfo->m_next = t_pSegInfo->m_sOffSet;
                    t_pPrevSegInfo->m_scn = r_scn;
                    //5.合并空闲队列
                    //5.1 看t_pPrevSegInfo能否和t_pSegInfo合并
                    if (t_pPrevSegInfo->m_sOffSet + t_pPrevSegInfo->m_segSize == t_pSegInfo->m_sOffSet)
                    {
                        //t_pPrevSegInfo和t_pSegInfo合并，并用t_pSegInfo来计合并后的信息
                        t_pPrevSegInfo->m_segSize = t_pPrevSegInfo->m_segSize + t_pSegInfo->m_segSize;
                        t_pPrevSegInfo->m_next   = t_pSegInfo->m_next;
                        t_pSegInfo = t_pPrevSegInfo;
                        t_pNextSegInfo->m_prev = t_pSegInfo->m_sOffSet;
                    }
                }
                else
                {
                    //加在第一段前，需要调整 m_pIdxSegInfo->m_fIdleSeg
                    m_pIdxSegInfo->m_fIdleSeg = t_pSegInfo->m_sOffSet;
                }
                //5.2 看t_pSegInfo能否和t_pNextSegInfo合并
                if (t_pSegInfo->m_sOffSet + t_pSegInfo->m_segSize == t_pNextSegInfo->m_sOffSet)
                {
                    //t_pPrevSegInfo和t_pSegInfo合并，并用t_pSegInfo来计合并后的信息
                    t_pSegInfo->m_segSize = t_pSegInfo->m_segSize + t_pNextSegInfo->m_segSize;
                    t_pSegInfo->m_next   = t_pNextSegInfo->m_next;
                    if (t_pSegInfo->m_next == 0)
                    {
                        //如果合并后是最后一段，则调整m_lIdleSeg值
                        m_pIdxSegInfo->m_lIdleSeg = t_pSegInfo->m_sOffSet;
                    }
                    else
                    {
                        //要将 t_pNextSegInfo->next 节点的 m_prev值修改为 t_pSegInfo->m_sOffSet;
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
    r_os << "---------索引段信息----------------------" << endl;
    r_os << "m_fIdleSeg = " << m_pIdxSegInfo->m_fIdleSeg << endl;
    r_os << "m_lIdleSeg = " << m_pIdxSegInfo->m_lIdleSeg << endl;
    r_os << "m_fUsedSeg = " << m_pIdxSegInfo->m_fUsedSeg << endl;
    r_os << "m_lUsedSeg = " << m_pIdxSegInfo->m_lUsedSeg << endl;
    r_os << "------已使用索引段信息----------------" << endl;
    HashIndexSeg* t_phashIndex ;
    if (m_pIdxSegInfo->m_fUsedSeg != 0)
    {
        t_phashIndex = (HashIndexSeg*)(m_spaceaddr + m_pIdxSegInfo->m_fUsedSeg);
        while (1)
        {
            r_os << "--------索引：" << t_phashIndex->m_idxName << "----" << endl;
            r_os << "m_sOffSet =" << t_phashIndex->m_sOffSet << endl;
            r_os << "m_segSize =" << t_phashIndex->m_segSize << endl;
            r_os << "m_hashSize=" << t_phashIndex->m_hashSize << endl;
            r_os << "m_prev    =" << t_phashIndex->m_prev << endl;
            r_os << "m_next    =" << t_phashIndex->m_next << endl;
            if (t_phashIndex->m_next == 0) break;
            t_phashIndex = (HashIndexSeg*)(m_spaceaddr + t_phashIndex->m_next);
        };
    }
    r_os << "------空闲索引段信息--－－-------------" << endl;
    if (m_pIdxSegInfo->m_fIdleSeg != 0)
    {
        t_phashIndex = (HashIndexSeg*)(m_spaceaddr + m_pIdxSegInfo->m_fIdleSeg);
        while (1)
        {
            r_os << "--------索引：" << t_phashIndex->m_idxName << "----" << endl;
            r_os << "m_sOffSet =" << t_phashIndex->m_sOffSet << endl;
            r_os << "m_segSize =" << t_phashIndex->m_segSize << endl;
            r_os << "m_hashSize=" << t_phashIndex->m_hashSize << endl;
            r_os << "m_prev    =" << t_phashIndex->m_prev << endl;
            r_os << "m_next    =" << t_phashIndex->m_next << endl;
            if (t_phashIndex->m_next == 0) break;
            t_phashIndex = (HashIndexSeg*)(m_spaceaddr + t_phashIndex->m_next);
        };
    }
    r_os << "---------索引段信息内容 终止!---------" << endl;
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
            t_phashIndex->m_scn.setOffSet(0); //清理Hash索引的scn
            t_pnode = (HashIndexNode*)(((char*)t_phashIndex) + sizeof(HashIndexSeg));
            //清理Hash索引中桶节点的scn
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



