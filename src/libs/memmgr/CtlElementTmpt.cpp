#include <iostream>
#include "CtlElementTmpt.h"
#include <assert.h>

//重新初始化该列表 2009-3-2 4:16 gaojf
template<class T>
void ListManager<T>::reInit()
{
    initElements(m_pShmAddr, m_sOffSet, m_pNodeListInfo->m_maxNum);
    return ;
}

template<class T>
bool ListManager<T>::attach_init(char* r_shmAddr, const size_t& r_offSet)
{
    initElements(r_shmAddr, r_offSet);
    int     t_maxNum = m_pNodeListInfo->m_maxNum;
    m_pIndexInfo = (size_t*)((char*)r_shmAddr + r_offSet + sizeof(NodeListInfo) +
                             t_maxNum * sizeof(NodeTmpt<T>));
    return true;
}

//初始化元素列表:所有元素均为从未使用过
template<class T>
bool ListManager<T>::initElements(char* r_shmAddr, const size_t& r_offSet, const int& r_maxNum)
{
    initElements(r_shmAddr, r_offSet);
    size_t  t_offSet = r_offSet;
    NodeTmpt<T>   *t_pNode;
    m_pNodeListInfo->m_maxNum = r_maxNum;
    m_pIndexInfo = (size_t*)((char*)r_shmAddr + r_offSet + sizeof(NodeListInfo) +
                             m_pNodeListInfo->m_maxNum * sizeof(NodeTmpt<T>));
    m_pNodeListInfo->m_idleNum   = m_pNodeListInfo->m_maxNum;
    m_pNodeListInfo->m_lastNode  = m_pNodeListInfo->m_firstNode = 0;
    m_pNodeListInfo->m_fIdle2Node = m_pNodeListInfo->m_lIdle2Node = 0;
    m_pNodeListInfo->m_nodeNum   = 0;
    t_offSet += sizeof(NodeListInfo);
    m_pNodeListInfo->m_fIdleNode = t_offSet;
    m_pNodeListInfo->m_lIdleNode = t_offSet + (r_maxNum - 1) * sizeof(NodeTmpt<T>);
    for (int i = 0; i < r_maxNum; i++)
    {
        t_pNode = (NodeTmpt<T>*)((char*)m_pShmAddr + t_offSet);
        t_pNode->initElement();
        t_pNode->m_pos = t_offSet;
        if (i == 0) t_pNode->m_prev = 0;
        else   t_pNode->m_prev = t_offSet - sizeof(NodeTmpt<T>);
        if (i == r_maxNum - 1) t_pNode->m_next = 0;
        else   t_pNode->m_next = t_offSet + sizeof(NodeTmpt<T>);
        t_offSet += sizeof(NodeTmpt<T>);
    }
    return true;
}

//初始化(内部成员信息：不负责共享内存信息的初始化)
template<class T>
void ListManager<T>::initElements(char* r_shmAddr, const size_t& r_offSet)
{
    m_sOffSet = r_offSet;
    m_pShmAddr = r_shmAddr;
    m_pNodeListInfo = (NodeListInfo*)(m_pShmAddr + r_offSet);
    m_pNodeList = (NodeTmpt<T>*)((char*)r_shmAddr + r_offSet + sizeof(NodeListInfo));
}

//计算整个列表占用的内存空间
template<class T>
size_t ListManager<T>::getSize() const
{
    size_t t_size = 0;
    t_size += sizeof(NodeListInfo);
    t_size += m_pNodeListInfo->m_maxNum * sizeof(NodeTmpt<T>);
    t_size += m_pNodeListInfo->m_maxNum * sizeof(size_t); //索引信息大小
    return t_size;
}
//清除所有元素
template<class T>
void ListManager<T>::clear(const int& t_flag)
{
    vector<T> r_elements;
    getElements(r_elements);
    //for(vector<T>::iterator r_itr=r_elements.begin();
    //    r_itr!=r_elements.end();r_itr++) by chenm 2008-6-26 9:59:44 linux 上编译报错
    for (int i = 0; i < r_elements.size(); ++i)
    {
        //deleteElement(*r_itr,t_flag);
        deleteElement(r_elements[i], t_flag);
    }
}

//增加元素r_flag=0表示不区分空闲和已使用过的空闲
//         r_flag=1需区分
template<class T>
bool ListManager<T>::addElement(const T& r_element, const int& r_flag)
{
    T*     t_pElement = NULL;
    return addElement(r_element, t_pElement, r_flag);
}
//增加元素:r_flag=0表示不区分空闲和已使用过的空闲
//         r_flag=1需区分
template<class T>
bool ListManager<T>::addElement(const T& r_element, T* &r_pElement, const int& r_flag)
{
    NodeTmpt<T>  t_Node;
    t_Node.setVaue(r_element);
    return addElement(t_Node, r_pElement, r_flag);
}

//增加元素:r_flag=0表示不区分空闲和已使用过的空闲
//         r_flag=1需区分
// 并将该元素所在的地址信息保留返回
template<class T>
bool ListManager<T>::addElement(NodeTmpt<T> &r_node, const int& r_flag)
{
    T*     t_pElement = NULL;
    return addElement(r_node, t_pElement, r_flag);
}
//增加元素:r_flag=0表示不区分空闲和已使用过的空闲
//         r_flag=1需区分
// 并将该元素所在的地址信息保留返回
template<class T>
bool ListManager<T>::addElement(NodeTmpt<T> &r_node, T* &r_pElement, const int& r_flag)
{
    NodeTmpt<T> *t_pNode, *t_pPrevNode, *t_pNextNode;
    bool         t_flag = false;
    if (r_flag != 0) //r_flag==1
    {
        t_flag = false;
        //1.从已使用过且空闲的列表中查找
        if (m_pNodeListInfo->m_fIdle2Node != 0)
        {
            t_pNode = (NodeTmpt<T>*)(m_pShmAddr + m_pNodeListInfo->m_fIdle2Node);
            while (1)
            {
                if (*t_pNode == r_node)
                {
                    t_flag = true;
                    break;
                }
                if (t_pNode->m_next == 0) break;
                t_pNode = (NodeTmpt<T>*)(m_pShmAddr + t_pNode->m_next);
            };
        }
    }
    if (t_flag == false)
    {
        //1.从空闲列表中获取一个元素
        if (m_pNodeListInfo->m_fIdleNode == 0)
        {
            //无空闲元素
#ifdef _DEBUG_
            cout << "无空闲元素!" << __FILE__ << __LINE__ << endl;
#endif
            return false;
        }
        t_pNode = (NodeTmpt<T>*)(m_pShmAddr + m_pNodeListInfo->m_fIdleNode);
        if (m_pNodeListInfo->m_fIdleNode == m_pNodeListInfo->m_lIdleNode)
        {
            //最后一个空闲节点
            m_pNodeListInfo->m_fIdleNode = m_pNodeListInfo->m_lIdleNode = 0;
        }
        else
        {
            //不是最后一个空闲节点
            m_pNodeListInfo->m_fIdleNode = t_pNode->m_next;
            t_pNextNode = (NodeTmpt<T>*)(m_pShmAddr + t_pNode->m_next);
            t_pNextNode->m_prev = 0;
        }
        //2.空闲资源-1
        m_pNodeListInfo->m_idleNum = m_pNodeListInfo->m_idleNum - 1;
    }
    else  //需要增加的元素属于重新启用
    {
        //将该元素从已使用队列中取出
        if (m_pNodeListInfo->m_fIdle2Node == t_pNode->m_pos)
        {
            //该元素为第一个节点
            m_pNodeListInfo->m_fIdle2Node = t_pNode->m_next;
        }
        if (m_pNodeListInfo->m_lIdle2Node == t_pNode->m_pos)
        {
            //该元素为最后一个节点
            m_pNodeListInfo->m_lIdle2Node = t_pNode->m_prev;
        }
        if (t_pNode->m_prev != 0)
        {
            //调整前一个节点m_next指针
            t_pPrevNode = (NodeTmpt<T>*)(m_pShmAddr + t_pNode->m_prev);
            t_pPrevNode->m_next = t_pNode->m_next;
        }
        if (t_pNode->m_next != 0)
        {
            //调整后一个节点m_prev指针
            t_pNextNode = (NodeTmpt<T>*)(m_pShmAddr + t_pNode->m_next);
            t_pNextNode->m_prev = t_pNode->m_prev;
        }
    }
    //2.设置该节点值
    t_pNode->setVaue(r_node.m_element);
    t_pNode->m_next = 0;
    t_pNode->m_prev = m_pNodeListInfo->m_lastNode;
    t_pNode->m_useFlag = MEMUSE_USED;
    //3.调整m_pNodeListInfo->m_firstNode和m_pNodeListInfo->m_lastNode值
    //  已经最后节点的m_next位置
    if (m_pNodeListInfo->m_lastNode == 0)
    {
        //第一个元素
        m_pNodeListInfo->m_firstNode = m_pNodeListInfo->m_lastNode = t_pNode->m_pos;
    }
    else
    {
        t_pPrevNode = (NodeTmpt<T>*)(m_pShmAddr + m_pNodeListInfo->m_lastNode);
        m_pNodeListInfo->m_lastNode = t_pPrevNode->m_next = t_pNode->m_pos;
    }
    //返回位置信息
    r_node.m_pos  = t_pNode->m_pos;
    r_node.m_prev = t_pNode->m_prev;
    r_node.m_next = t_pNode->m_next;
    r_node.m_useFlag = t_pNode->m_useFlag;
    //4.将元素在索引中增加
    int t_indexPos = lowerBound(r_node.m_element, t_pNextNode);
    for (int i = m_pNodeListInfo->m_nodeNum; i > t_indexPos; i--)
    {
        m_pIndexInfo[i] = m_pIndexInfo[i - 1];
    }
    m_pIndexInfo[t_indexPos] = t_pNode->m_pos;
    m_pNodeListInfo->m_nodeNum = m_pNodeListInfo->m_nodeNum + 1;
    r_pElement = &(t_pNode->m_element);
    return true;
}

//删除元素r_flag=0表示不区分空闲和已使用过的空闲
//        r_flag=1需区分
template<class T>
bool ListManager<T>::deleteElement(const T& r_element, const int& r_flag)
{
    NodeTmpt<T> *t_pNode, *t_pPrevNode, *t_pNextNode;
    bool         t_flag = false;
    size_t* t_pNodeIndexPos = 0;
    //0. 找到索引信息t_pNodeIndexPos  和节点位置信息 t_pNode
    int   t_indexPos;
    //根据节点找到索引位置和节点位置
    t_flag = getNodeIndexPos(r_element, t_indexPos, t_pNode);
    if (t_flag == false)
    {
#ifdef _DEBUG_
        cout << "无相应在用元素!" << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    //2. 从已使用队列中删除
    if (m_pNodeListInfo->m_firstNode == t_pNode->m_pos)
    {
        //该元素为第一个节点
        m_pNodeListInfo->m_firstNode = t_pNode->m_next;
    }
    if (m_pNodeListInfo->m_lastNode == t_pNode->m_pos)
    {
        //该元素为最后一个节点
        m_pNodeListInfo->m_lastNode = t_pNode->m_prev;
    }
    if (t_pNode->m_prev != 0)
    {
        //调整前一个节点m_next指针
        t_pPrevNode = (NodeTmpt<T>*)(m_pShmAddr + t_pNode->m_prev);
        t_pPrevNode->m_next = t_pNode->m_next;
    }
    if (t_pNode->m_next != 0)
    {
        //调整后一个节点m_prev指针
        t_pNextNode = (NodeTmpt<T>*)(m_pShmAddr + t_pNode->m_next);
        t_pNextNode->m_prev = t_pNode->m_prev;
    }
    t_pNode->m_next = 0;
    if (r_flag == 0)
    {
        //3. 加入空闲队列
        t_pNode->m_prev = m_pNodeListInfo->m_lIdleNode;
        t_pNode->m_useFlag = MEMUSE_IDLE;
        if (m_pNodeListInfo->m_lIdleNode == 0)
        {
            //第一个元素
            m_pNodeListInfo->m_fIdleNode = m_pNodeListInfo->m_lIdleNode = t_pNode->m_pos;
        }
        else
        {
            t_pPrevNode = (NodeTmpt<T>*)(m_pShmAddr + m_pNodeListInfo->m_lIdleNode);
            m_pNodeListInfo->m_lIdleNode = t_pPrevNode->m_next = t_pNode->m_pos;
        }
        //2.空闲资源+1
        m_pNodeListInfo->m_idleNum = m_pNodeListInfo->m_idleNum + 1;
    }
    else  //r_flag==1
    {
        //4. 加入已使用空闲队列
        t_pNode->m_prev = m_pNodeListInfo->m_lIdle2Node;
        t_pNode->m_useFlag = MEMUSE_USEDIDLE;
        if (m_pNodeListInfo->m_lIdle2Node == 0)
        {
            //第一个元素
            m_pNodeListInfo->m_fIdle2Node = m_pNodeListInfo->m_lIdle2Node = t_pNode->m_pos;
        }
        else
        {
            t_pPrevNode = (NodeTmpt<T>*)(m_pShmAddr + m_pNodeListInfo->m_lIdle2Node);
            m_pNodeListInfo->m_lIdle2Node = t_pPrevNode->m_next = t_pNode->m_pos;
        }
    }
    //5. 将索引节点t_indexPos从索引列表中删除
    for (; t_indexPos < m_pNodeListInfo->m_nodeNum; t_indexPos++)
    {
        m_pIndexInfo[t_indexPos] = m_pIndexInfo[t_indexPos + 1];
    }
    m_pNodeListInfo->m_nodeNum = m_pNodeListInfo->m_nodeNum - 1;
    return true;
}

//更新元素：非键值更新
template<class T>
bool ListManager<T>::updateElement(const T& r_element)
{
    NodeTmpt<T> *t_pNode;
    bool         t_flag = false;
    int   t_indexPos;
    //根据节点找到索引位置和节点位置
    t_flag = getNodeIndexPos(r_element, t_indexPos, t_pNode);
    if (t_flag == false)
    {
#ifdef _DEBUG_
        cout << "无相应在用元素!" << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    memcpy(&(t_pNode->m_element), &r_element, sizeof(T));
    return true;
}

//查找一个元素
template<class T>
bool ListManager<T>::getElement(const T& r_element, NodeTmpt<T> &r_result)
{
    NodeTmpt<T>*  t_pNode;
    if (getElement(r_element, t_pNode) == false)
    {
        return false;
    }
    memcpy(&r_result, t_pNode, sizeof(NodeTmpt<T>));
    return true;
}
//查找一个元素
template<class T>
bool ListManager<T>::getElement(const T& r_element, T& r_result)
{
    NodeTmpt<T>*  t_pNode;
    if (getElement(r_element, t_pNode) == false)
    {
        return false;
    }
    memcpy(&r_result, &(t_pNode->m_element), sizeof(T));
    return true;
}

//查找一个元素
template<class T>
bool ListManager<T>::getElement(const T& r_element, T* &r_result)
{
    NodeTmpt<T>*  t_pNode;
    if (getElement(r_element, t_pNode) == false)
    {
        return false;
    }
    r_result = &(t_pNode->m_element);
    return true;
}
template<class T>
bool ListManager<T>::getElement(const T& r_element, NodeTmpt<T>* &r_result)
{
    NodeTmpt<T> *t_pNode;
    bool         t_flag = false;
    int   t_indexPos;
    //根据节点找到索引位置和节点位置
    t_flag = getNodeIndexPos(r_element, t_indexPos, t_pNode);
    if (t_flag == false)
    {
        /*
        #ifdef _DEBUG_
          cout<<"无相应在用元素!"<<__FILE__<<__LINE__<<endl;
        #endif
        */
        return false;
    }
    r_result = t_pNode;
    return true;
}

//取元素列表
template<class T>
bool ListManager<T>:: getElements(vector<T> &r_resultList)
{
    NodeTmpt<T> *t_pNode;
    r_resultList.clear();
    //1. 从已使用队列中找出
    if (m_pNodeListInfo->m_firstNode != 0)
    {
        t_pNode = (NodeTmpt<T>*)(m_pShmAddr + m_pNodeListInfo->m_firstNode);
        while (1)
        {
            r_resultList.push_back(t_pNode->m_element);
            if (t_pNode->m_next == 0) break;
            t_pNode = (NodeTmpt<T>*)(m_pShmAddr + t_pNode->m_next);
        };
    }
    return true;
}

//根据索引找元素的位置
template<class T>
bool ListManager<T>::getNodeIndexPos(const T& r_element, int& r_indexPos, NodeTmpt<T>* &r_pNode)
{
    size_t* t_pIdx;;
    r_indexPos = lowerBound(r_element, r_pNode);
    if (r_indexPos == m_pNodeListInfo->m_nodeNum)
    {
        /*
        #ifdef _DEBUG_
          cout<<"没有对应的元素!"<<__FILE__<<__LINE__<<endl;
        #endif
        */
        return false;
    }
    return (r_pNode->m_element == r_element);
}

//1. 索引比较,找到第一个不小于参数对应的所以位置idxPos
template<class T>
int ListManager<T>::lowerBound(const T& r_element, NodeTmpt<T>* &r_pNode) const
{
    long low = -1, high = 0, mid = 0;
    int  t_nodeNum;
    memcpy(&t_nodeNum, &(m_pNodeListInfo->m_nodeNum), sizeof(int));
    size_t* t_pIdx;
    high = t_nodeNum;
    //采用二分发查找
    while (low < high - 1)
    {
        mid = (low + high) / 2;
        t_pIdx = m_pIndexInfo + mid;
        r_pNode = (NodeTmpt<T>*)(m_pShmAddr + *t_pIdx);
        if (r_pNode->m_element < r_element)
        {
            low  = mid;
        }
        else
        {
            high = mid;
        }
    }
    //此时high 即为第一个不小于r_element的位置
    r_pNode = (NodeTmpt<T>*)(m_pShmAddr + m_pIndexInfo[high]);
    return high;
}

template<class T>
void ListManager<T>::dumpInfo(ostream& r_os) const
{
    NodeTmpt<T> *t_pNode;
    //1.输出列表信息NodeListInfo
    r_os << "---------列表信息-----------------" << endl;
    r_os << "m_sOffSet   =" << m_sOffSet << endl;
    r_os << "m_size      =" << getSize() << endl;
    r_os << "m_maxNum    =" << m_pNodeListInfo->m_maxNum << endl;
    r_os << "m_nodeNum   =" << m_pNodeListInfo->m_nodeNum << endl;
    r_os << "m_idleNum   =" << m_pNodeListInfo->m_idleNum << endl;
    r_os << "m_firstNode  =" << m_pNodeListInfo->m_firstNode << endl;
    r_os << "m_lastNode  =" << m_pNodeListInfo->m_lastNode << endl;
    r_os << "m_fIdleNode =" << m_pNodeListInfo->m_fIdleNode << endl;
    r_os << "m_lIdleNode =" << m_pNodeListInfo->m_lIdleNode << endl;
    r_os << "m_fIdle2Node=" << m_pNodeListInfo->m_fIdle2Node << endl;
    r_os << "m_lIdle2Node=" << m_pNodeListInfo->m_lIdle2Node << endl;
    for (int t_i = 0; t_i < m_pNodeListInfo->m_nodeNum; ++t_i)
    {
        r_os << "m_pIndexInfo[" << t_i << "]=" << m_pIndexInfo[t_i] << endl;
    }
    //2.输出所有在用元素信息
    r_os << "--------在用元素 起始------------------" << endl;
    if (m_pNodeListInfo->m_firstNode != 0)
    {
        t_pNode = (NodeTmpt<T>*)(m_pShmAddr + m_pNodeListInfo->m_firstNode);
        while (1)
        {
            r_os << "m_pos    =" << t_pNode->m_pos << endl;
            r_os << "m_prev   =" << t_pNode->m_prev << endl;
            r_os << "m_next   =" << t_pNode->m_next << endl;
            r_os << "m_useFlag=" << t_pNode->m_useFlag << endl;
            r_os << "------m_element信息----" << endl;
            t_pNode->m_element.dumpInfo(r_os);
            r_os << "------------------------------" << endl;
            if (t_pNode->m_next == 0) break;
            t_pNode = (NodeTmpt<T>*)(m_pShmAddr + t_pNode->m_next);
        };
    }
    r_os << "--------在用元素 终止------------------" << endl;
    //3.输出所有已使用过空闲的元素信息
    r_os << "--------已使用过空闲元素 起始------------------" << endl;
    if (m_pNodeListInfo->m_fIdle2Node != 0)
    {
        t_pNode = (NodeTmpt<T>*)(m_pShmAddr + m_pNodeListInfo->m_fIdle2Node);
        while (1)
        {
            r_os << "m_pos    =" << t_pNode->m_pos << endl;
            r_os << "m_prev   =" << t_pNode->m_prev << endl;
            r_os << "m_next   =" << t_pNode->m_next << endl;
            r_os << "m_useFlag=" << t_pNode->m_useFlag << endl;
            r_os << "------m_element信息----" << endl;
            t_pNode->m_element.dumpInfo(r_os);
            r_os << "------------------------------" << endl;
            if (t_pNode->m_next == 0) break;
            t_pNode = (NodeTmpt<T>*)(m_pShmAddr + t_pNode->m_next);
        };
    }
    r_os << "--------已使用过空闲元素 终止------------------" << endl;
    //4.输出所有空闲的元素位置信息
    r_os << "--------空闲元素 起始------------------" << endl;
    if (m_pNodeListInfo->m_fIdleNode != 0)
    {
        t_pNode = (NodeTmpt<T>*)(m_pShmAddr + m_pNodeListInfo->m_fIdleNode);
        while (1)
        {
            r_os << "m_pos    =" << t_pNode->m_pos << endl;
            r_os << "m_prev   =" << t_pNode->m_prev << endl;
            r_os << "m_next   =" << t_pNode->m_next << endl;
            r_os << "m_useFlag=" << t_pNode->m_useFlag << endl;
            r_os << "------------------------------" << endl;
            if (t_pNode->m_next == 0) break;
            t_pNode = (NodeTmpt<T>*)(m_pShmAddr + t_pNode->m_next);
        };
    }
    r_os << "--------空闲元素 终止------------------" << endl;
}

//根据元素的偏移量获取对应的对象指针 add by gaojf 2010-5-18 14:08
//r_offset为T的起始位置
template<class T>
T* ListManager<T>::getElement(const size_t& r_offset)
{
    return (T*)((char*)m_pShmAddr + r_offset);
}
template<class T>
//根据node指针获取该T的起始位置 add by gaojf 2010-5-18 14:08
size_t ListManager<T>::getOffset(const T* r_element)
{
    return (((char*)r_element) - m_pShmAddr);
}

//启动MDB后初始化:清理scn等信息
template<class T>
void ListManager<T>::startmdb_init()
{
    NodeTmpt<T>* t_pNode = NULL;
    m_pNodeListInfo->m_scn.setOffSet(0); //清管理列表上的scn信息
    for (int t_i = 0; t_i < m_pNodeListInfo->m_maxNum; ++t_i)
    {
        t_pNode = m_pNodeList + t_i;
        t_pNode->m_element.startmdb_init(); //清理原始自身上面的scn和影子信息等
    }
}

