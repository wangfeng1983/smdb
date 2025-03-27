#include <iostream>
#include "CtlElementTmpt.h"
#include <assert.h>

//���³�ʼ�����б� 2009-3-2 4:16 gaojf
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

//��ʼ��Ԫ���б�:����Ԫ�ؾ�Ϊ��δʹ�ù�
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

//��ʼ��(�ڲ���Ա��Ϣ�����������ڴ���Ϣ�ĳ�ʼ��)
template<class T>
void ListManager<T>::initElements(char* r_shmAddr, const size_t& r_offSet)
{
    m_sOffSet = r_offSet;
    m_pShmAddr = r_shmAddr;
    m_pNodeListInfo = (NodeListInfo*)(m_pShmAddr + r_offSet);
    m_pNodeList = (NodeTmpt<T>*)((char*)r_shmAddr + r_offSet + sizeof(NodeListInfo));
}

//���������б�ռ�õ��ڴ�ռ�
template<class T>
size_t ListManager<T>::getSize() const
{
    size_t t_size = 0;
    t_size += sizeof(NodeListInfo);
    t_size += m_pNodeListInfo->m_maxNum * sizeof(NodeTmpt<T>);
    t_size += m_pNodeListInfo->m_maxNum * sizeof(size_t); //������Ϣ��С
    return t_size;
}
//�������Ԫ��
template<class T>
void ListManager<T>::clear(const int& t_flag)
{
    vector<T> r_elements;
    getElements(r_elements);
    //for(vector<T>::iterator r_itr=r_elements.begin();
    //    r_itr!=r_elements.end();r_itr++) by chenm 2008-6-26 9:59:44 linux �ϱ��뱨��
    for (int i = 0; i < r_elements.size(); ++i)
    {
        //deleteElement(*r_itr,t_flag);
        deleteElement(r_elements[i], t_flag);
    }
}

//����Ԫ��r_flag=0��ʾ�����ֿ��к���ʹ�ù��Ŀ���
//         r_flag=1������
template<class T>
bool ListManager<T>::addElement(const T& r_element, const int& r_flag)
{
    T*     t_pElement = NULL;
    return addElement(r_element, t_pElement, r_flag);
}
//����Ԫ��:r_flag=0��ʾ�����ֿ��к���ʹ�ù��Ŀ���
//         r_flag=1������
template<class T>
bool ListManager<T>::addElement(const T& r_element, T* &r_pElement, const int& r_flag)
{
    NodeTmpt<T>  t_Node;
    t_Node.setVaue(r_element);
    return addElement(t_Node, r_pElement, r_flag);
}

//����Ԫ��:r_flag=0��ʾ�����ֿ��к���ʹ�ù��Ŀ���
//         r_flag=1������
// ������Ԫ�����ڵĵ�ַ��Ϣ��������
template<class T>
bool ListManager<T>::addElement(NodeTmpt<T> &r_node, const int& r_flag)
{
    T*     t_pElement = NULL;
    return addElement(r_node, t_pElement, r_flag);
}
//����Ԫ��:r_flag=0��ʾ�����ֿ��к���ʹ�ù��Ŀ���
//         r_flag=1������
// ������Ԫ�����ڵĵ�ַ��Ϣ��������
template<class T>
bool ListManager<T>::addElement(NodeTmpt<T> &r_node, T* &r_pElement, const int& r_flag)
{
    NodeTmpt<T> *t_pNode, *t_pPrevNode, *t_pNextNode;
    bool         t_flag = false;
    if (r_flag != 0) //r_flag==1
    {
        t_flag = false;
        //1.����ʹ�ù��ҿ��е��б��в���
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
        //1.�ӿ����б��л�ȡһ��Ԫ��
        if (m_pNodeListInfo->m_fIdleNode == 0)
        {
            //�޿���Ԫ��
#ifdef _DEBUG_
            cout << "�޿���Ԫ��!" << __FILE__ << __LINE__ << endl;
#endif
            return false;
        }
        t_pNode = (NodeTmpt<T>*)(m_pShmAddr + m_pNodeListInfo->m_fIdleNode);
        if (m_pNodeListInfo->m_fIdleNode == m_pNodeListInfo->m_lIdleNode)
        {
            //���һ�����нڵ�
            m_pNodeListInfo->m_fIdleNode = m_pNodeListInfo->m_lIdleNode = 0;
        }
        else
        {
            //�������һ�����нڵ�
            m_pNodeListInfo->m_fIdleNode = t_pNode->m_next;
            t_pNextNode = (NodeTmpt<T>*)(m_pShmAddr + t_pNode->m_next);
            t_pNextNode->m_prev = 0;
        }
        //2.������Դ-1
        m_pNodeListInfo->m_idleNum = m_pNodeListInfo->m_idleNum - 1;
    }
    else  //��Ҫ���ӵ�Ԫ��������������
    {
        //����Ԫ�ش���ʹ�ö�����ȡ��
        if (m_pNodeListInfo->m_fIdle2Node == t_pNode->m_pos)
        {
            //��Ԫ��Ϊ��һ���ڵ�
            m_pNodeListInfo->m_fIdle2Node = t_pNode->m_next;
        }
        if (m_pNodeListInfo->m_lIdle2Node == t_pNode->m_pos)
        {
            //��Ԫ��Ϊ���һ���ڵ�
            m_pNodeListInfo->m_lIdle2Node = t_pNode->m_prev;
        }
        if (t_pNode->m_prev != 0)
        {
            //����ǰһ���ڵ�m_nextָ��
            t_pPrevNode = (NodeTmpt<T>*)(m_pShmAddr + t_pNode->m_prev);
            t_pPrevNode->m_next = t_pNode->m_next;
        }
        if (t_pNode->m_next != 0)
        {
            //������һ���ڵ�m_prevָ��
            t_pNextNode = (NodeTmpt<T>*)(m_pShmAddr + t_pNode->m_next);
            t_pNextNode->m_prev = t_pNode->m_prev;
        }
    }
    //2.���øýڵ�ֵ
    t_pNode->setVaue(r_node.m_element);
    t_pNode->m_next = 0;
    t_pNode->m_prev = m_pNodeListInfo->m_lastNode;
    t_pNode->m_useFlag = MEMUSE_USED;
    //3.����m_pNodeListInfo->m_firstNode��m_pNodeListInfo->m_lastNodeֵ
    //  �Ѿ����ڵ��m_nextλ��
    if (m_pNodeListInfo->m_lastNode == 0)
    {
        //��һ��Ԫ��
        m_pNodeListInfo->m_firstNode = m_pNodeListInfo->m_lastNode = t_pNode->m_pos;
    }
    else
    {
        t_pPrevNode = (NodeTmpt<T>*)(m_pShmAddr + m_pNodeListInfo->m_lastNode);
        m_pNodeListInfo->m_lastNode = t_pPrevNode->m_next = t_pNode->m_pos;
    }
    //����λ����Ϣ
    r_node.m_pos  = t_pNode->m_pos;
    r_node.m_prev = t_pNode->m_prev;
    r_node.m_next = t_pNode->m_next;
    r_node.m_useFlag = t_pNode->m_useFlag;
    //4.��Ԫ��������������
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

//ɾ��Ԫ��r_flag=0��ʾ�����ֿ��к���ʹ�ù��Ŀ���
//        r_flag=1������
template<class T>
bool ListManager<T>::deleteElement(const T& r_element, const int& r_flag)
{
    NodeTmpt<T> *t_pNode, *t_pPrevNode, *t_pNextNode;
    bool         t_flag = false;
    size_t* t_pNodeIndexPos = 0;
    //0. �ҵ�������Ϣt_pNodeIndexPos  �ͽڵ�λ����Ϣ t_pNode
    int   t_indexPos;
    //���ݽڵ��ҵ�����λ�úͽڵ�λ��
    t_flag = getNodeIndexPos(r_element, t_indexPos, t_pNode);
    if (t_flag == false)
    {
#ifdef _DEBUG_
        cout << "����Ӧ����Ԫ��!" << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    //2. ����ʹ�ö�����ɾ��
    if (m_pNodeListInfo->m_firstNode == t_pNode->m_pos)
    {
        //��Ԫ��Ϊ��һ���ڵ�
        m_pNodeListInfo->m_firstNode = t_pNode->m_next;
    }
    if (m_pNodeListInfo->m_lastNode == t_pNode->m_pos)
    {
        //��Ԫ��Ϊ���һ���ڵ�
        m_pNodeListInfo->m_lastNode = t_pNode->m_prev;
    }
    if (t_pNode->m_prev != 0)
    {
        //����ǰһ���ڵ�m_nextָ��
        t_pPrevNode = (NodeTmpt<T>*)(m_pShmAddr + t_pNode->m_prev);
        t_pPrevNode->m_next = t_pNode->m_next;
    }
    if (t_pNode->m_next != 0)
    {
        //������һ���ڵ�m_prevָ��
        t_pNextNode = (NodeTmpt<T>*)(m_pShmAddr + t_pNode->m_next);
        t_pNextNode->m_prev = t_pNode->m_prev;
    }
    t_pNode->m_next = 0;
    if (r_flag == 0)
    {
        //3. ������ж���
        t_pNode->m_prev = m_pNodeListInfo->m_lIdleNode;
        t_pNode->m_useFlag = MEMUSE_IDLE;
        if (m_pNodeListInfo->m_lIdleNode == 0)
        {
            //��һ��Ԫ��
            m_pNodeListInfo->m_fIdleNode = m_pNodeListInfo->m_lIdleNode = t_pNode->m_pos;
        }
        else
        {
            t_pPrevNode = (NodeTmpt<T>*)(m_pShmAddr + m_pNodeListInfo->m_lIdleNode);
            m_pNodeListInfo->m_lIdleNode = t_pPrevNode->m_next = t_pNode->m_pos;
        }
        //2.������Դ+1
        m_pNodeListInfo->m_idleNum = m_pNodeListInfo->m_idleNum + 1;
    }
    else  //r_flag==1
    {
        //4. ������ʹ�ÿ��ж���
        t_pNode->m_prev = m_pNodeListInfo->m_lIdle2Node;
        t_pNode->m_useFlag = MEMUSE_USEDIDLE;
        if (m_pNodeListInfo->m_lIdle2Node == 0)
        {
            //��һ��Ԫ��
            m_pNodeListInfo->m_fIdle2Node = m_pNodeListInfo->m_lIdle2Node = t_pNode->m_pos;
        }
        else
        {
            t_pPrevNode = (NodeTmpt<T>*)(m_pShmAddr + m_pNodeListInfo->m_lIdle2Node);
            m_pNodeListInfo->m_lIdle2Node = t_pPrevNode->m_next = t_pNode->m_pos;
        }
    }
    //5. �������ڵ�t_indexPos�������б���ɾ��
    for (; t_indexPos < m_pNodeListInfo->m_nodeNum; t_indexPos++)
    {
        m_pIndexInfo[t_indexPos] = m_pIndexInfo[t_indexPos + 1];
    }
    m_pNodeListInfo->m_nodeNum = m_pNodeListInfo->m_nodeNum - 1;
    return true;
}

//����Ԫ�أ��Ǽ�ֵ����
template<class T>
bool ListManager<T>::updateElement(const T& r_element)
{
    NodeTmpt<T> *t_pNode;
    bool         t_flag = false;
    int   t_indexPos;
    //���ݽڵ��ҵ�����λ�úͽڵ�λ��
    t_flag = getNodeIndexPos(r_element, t_indexPos, t_pNode);
    if (t_flag == false)
    {
#ifdef _DEBUG_
        cout << "����Ӧ����Ԫ��!" << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    memcpy(&(t_pNode->m_element), &r_element, sizeof(T));
    return true;
}

//����һ��Ԫ��
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
//����һ��Ԫ��
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

//����һ��Ԫ��
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
    //���ݽڵ��ҵ�����λ�úͽڵ�λ��
    t_flag = getNodeIndexPos(r_element, t_indexPos, t_pNode);
    if (t_flag == false)
    {
        /*
        #ifdef _DEBUG_
          cout<<"����Ӧ����Ԫ��!"<<__FILE__<<__LINE__<<endl;
        #endif
        */
        return false;
    }
    r_result = t_pNode;
    return true;
}

//ȡԪ���б�
template<class T>
bool ListManager<T>:: getElements(vector<T> &r_resultList)
{
    NodeTmpt<T> *t_pNode;
    r_resultList.clear();
    //1. ����ʹ�ö������ҳ�
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

//����������Ԫ�ص�λ��
template<class T>
bool ListManager<T>::getNodeIndexPos(const T& r_element, int& r_indexPos, NodeTmpt<T>* &r_pNode)
{
    size_t* t_pIdx;;
    r_indexPos = lowerBound(r_element, r_pNode);
    if (r_indexPos == m_pNodeListInfo->m_nodeNum)
    {
        /*
        #ifdef _DEBUG_
          cout<<"û�ж�Ӧ��Ԫ��!"<<__FILE__<<__LINE__<<endl;
        #endif
        */
        return false;
    }
    return (r_pNode->m_element == r_element);
}

//1. �����Ƚ�,�ҵ���һ����С�ڲ�����Ӧ������λ��idxPos
template<class T>
int ListManager<T>::lowerBound(const T& r_element, NodeTmpt<T>* &r_pNode) const
{
    long low = -1, high = 0, mid = 0;
    int  t_nodeNum;
    memcpy(&t_nodeNum, &(m_pNodeListInfo->m_nodeNum), sizeof(int));
    size_t* t_pIdx;
    high = t_nodeNum;
    //���ö��ַ�����
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
    //��ʱhigh ��Ϊ��һ����С��r_element��λ��
    r_pNode = (NodeTmpt<T>*)(m_pShmAddr + m_pIndexInfo[high]);
    return high;
}

template<class T>
void ListManager<T>::dumpInfo(ostream& r_os) const
{
    NodeTmpt<T> *t_pNode;
    //1.����б���ϢNodeListInfo
    r_os << "---------�б���Ϣ-----------------" << endl;
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
    //2.�����������Ԫ����Ϣ
    r_os << "--------����Ԫ�� ��ʼ------------------" << endl;
    if (m_pNodeListInfo->m_firstNode != 0)
    {
        t_pNode = (NodeTmpt<T>*)(m_pShmAddr + m_pNodeListInfo->m_firstNode);
        while (1)
        {
            r_os << "m_pos    =" << t_pNode->m_pos << endl;
            r_os << "m_prev   =" << t_pNode->m_prev << endl;
            r_os << "m_next   =" << t_pNode->m_next << endl;
            r_os << "m_useFlag=" << t_pNode->m_useFlag << endl;
            r_os << "------m_element��Ϣ----" << endl;
            t_pNode->m_element.dumpInfo(r_os);
            r_os << "------------------------------" << endl;
            if (t_pNode->m_next == 0) break;
            t_pNode = (NodeTmpt<T>*)(m_pShmAddr + t_pNode->m_next);
        };
    }
    r_os << "--------����Ԫ�� ��ֹ------------------" << endl;
    //3.���������ʹ�ù����е�Ԫ����Ϣ
    r_os << "--------��ʹ�ù�����Ԫ�� ��ʼ------------------" << endl;
    if (m_pNodeListInfo->m_fIdle2Node != 0)
    {
        t_pNode = (NodeTmpt<T>*)(m_pShmAddr + m_pNodeListInfo->m_fIdle2Node);
        while (1)
        {
            r_os << "m_pos    =" << t_pNode->m_pos << endl;
            r_os << "m_prev   =" << t_pNode->m_prev << endl;
            r_os << "m_next   =" << t_pNode->m_next << endl;
            r_os << "m_useFlag=" << t_pNode->m_useFlag << endl;
            r_os << "------m_element��Ϣ----" << endl;
            t_pNode->m_element.dumpInfo(r_os);
            r_os << "------------------------------" << endl;
            if (t_pNode->m_next == 0) break;
            t_pNode = (NodeTmpt<T>*)(m_pShmAddr + t_pNode->m_next);
        };
    }
    r_os << "--------��ʹ�ù�����Ԫ�� ��ֹ------------------" << endl;
    //4.������п��е�Ԫ��λ����Ϣ
    r_os << "--------����Ԫ�� ��ʼ------------------" << endl;
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
    r_os << "--------����Ԫ�� ��ֹ------------------" << endl;
}

//����Ԫ�ص�ƫ������ȡ��Ӧ�Ķ���ָ�� add by gaojf 2010-5-18 14:08
//r_offsetΪT����ʼλ��
template<class T>
T* ListManager<T>::getElement(const size_t& r_offset)
{
    return (T*)((char*)m_pShmAddr + r_offset);
}
template<class T>
//����nodeָ���ȡ��T����ʼλ�� add by gaojf 2010-5-18 14:08
size_t ListManager<T>::getOffset(const T* r_element)
{
    return (((char*)r_element) - m_pShmAddr);
}

//����MDB���ʼ��:����scn����Ϣ
template<class T>
void ListManager<T>::startmdb_init()
{
    NodeTmpt<T>* t_pNode = NULL;
    m_pNodeListInfo->m_scn.setOffSet(0); //������б��ϵ�scn��Ϣ
    for (int t_i = 0; t_i < m_pNodeListInfo->m_maxNum; ++t_i)
    {
        t_pNode = m_pNodeList + t_i;
        t_pNode->m_element.startmdb_init(); //����ԭʼ���������scn��Ӱ����Ϣ��
    }
}

