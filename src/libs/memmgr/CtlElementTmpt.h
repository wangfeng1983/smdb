#ifndef _CTLELEMENT_H_INCLUDE_20080423
#define _CTLELEMENT_H_INCLUDE_20080423
#include "config-all.h"
#include "MdbConstDef.h"
#include "MdbConstDef2.h"
#include <vector>
USING_NAMESPACE(std)

template<class T>
class NodeTmpt
{
    public:
        size_t m_pos;       ///本节点偏移量
        size_t m_prev;      ///上一个位置
        size_t m_next;      ///下一个位置
        int    m_useFlag;   ///<0 未使用 ,1 占用,2 已使用过现未用
        T      m_element;   ///元素
    public:
        void initElement()
        {
            memset(&m_element, 0, sizeof(T));
            m_useFlag = MEMUSE_IDLE;
        }
        void setVaue(const T& r_element)
        {
            memcpy(&m_element, &r_element, sizeof(T));
        }
        friend int operator==(const NodeTmpt<T> &left, const NodeTmpt<T> &right)
        {
            return (left.m_element == right.m_element);
        }
        friend int operator<(const NodeTmpt<T> &left, const NodeTmpt<T> &right)
        {
            return (left.m_element < right.m_element);
        }
};

class NodeListInfo
{
    public:
        int       m_maxNum;      ///<最大元素个数
        int       m_nodeNum;     ///<现有元素个数
        int       m_idleNum;     ///<未使用过的元素个数
        size_t    m_firstNode;    ///<第一个元素位置
        size_t    m_lastNode;    ///<最后一个元素位置
        size_t    m_fIdleNode;   ///<第一个空闲位置(从未使用过)
        size_t    m_lIdleNode;   ///<最后一个空闲位置(从未使用过)
        size_t    m_fIdle2Node;  ///<第一个已使用过空闲位置
        size_t    m_lIdle2Node;  ///<最后一个已使用过空闲位置

        //Mdb2.0 add by gaojf 2010-5-17 9:30
        T_SCN     m_scn;         ///<时间戳，用于CHKPT
};

template<class T>
class ListManager
{
    public:
        friend class ChkptMgr;
        /*
        class CmpIdxValue
        {
          public:
            const char *            m_pShmAddr;      ///<表空间地址信息
          public:
            virtual bool operator() (const size_t &r_left, const size_t &r_right) const
            {
              if(m_pShmAddr == NULL) return false;
              const NodeTmpt<T>* t_lpNode=(NodeTmpt<T>*)(m_pShmAddr+r_left);
              const NodeTmpt<T>* t_rpNode=(NodeTmpt<T>*)(m_pShmAddr+r_right);
              return ((*t_lpNode)<(*t_rpNode));
            }
        };
        */
    protected:
        size_t         m_sOffSet;       ///<整个列表的起始偏移量
        char*          m_pShmAddr;      ///<表空间地址信息
        NodeListInfo*  m_pNodeListInfo; ///<列表信息
        NodeTmpt<T>   *m_pNodeList;     ///<列表元素信息
        size_t*        m_pIndexInfo;    ///<索引信息:每个元素指向NodeTmpt<T>
        //CmpIdxValue    ; ///<定义索引比较方法类
    public:
        void update_scn(const T_SCN& r_scn)
        {
            m_pNodeListInfo->m_scn = r_scn;
        }
        void clear(const int& t_flag); //清除所有元素
        void reInit(); //重新初始化该列表 2009-3-2 4:16 gaojf
        bool attach_init(char* r_shmAddr, const size_t& r_offSet);
        void startmdb_init(); //启动MDB后初始化:清理scn等信息
        //初始化元素列表(第一次初始化)
        bool initElements(char* r_shmAddr, const size_t& r_offSet, const int& r_maxNum);
        //初始化(内部成员信息：不负责共享内存信息的初始化)
        void initElements(char* r_shmAddr, const size_t& r_offSet);
        //计算整个列表占用的内存空间
        size_t getSize() const;
        //增加元素:r_flag=0表示不区分空闲和已使用过的空闲
        //         r_flag=1需区分
        bool addElement(const T& r_element, const int& r_flag = 0);
        //增加元素:r_flag=0表示不区分空闲和已使用过的空闲
        //         r_flag=1需区分
        bool addElement(const T& r_element, T* &r_pElement, const int& r_flag = 0);
        //增加元素:r_flag=0表示不区分空闲和已使用过的空闲
        //         r_flag=1需区分
        // 并将该元素所在的地址信息保留返回
        bool addElement(NodeTmpt<T> &r_node, const int& r_flag = 0);
        //增加元素:r_flag=0表示不区分空闲和已使用过的空闲
        //         r_flag=1需区分
        // 并将该元素所在的地址信息保留返回
        bool addElement(NodeTmpt<T> &r_node, T* &r_pElement, const int& r_flag = 0);

        //删除元素:r_flag=0表示不区分空闲和已使用过的空闲
        //         r_flag=1需区分
        bool deleteElement(const T& r_element, const int& r_flag = 0);
        //更新元素：非键值更新
        bool updateElement(const T& r_element);
        //查找一个元素
        bool getElement(const T& r_element, NodeTmpt<T> &r_result);
        //查找一个元素
        bool getElement(const T& r_element, NodeTmpt<T>* &r_result);
        //查找一个元素
        bool getElement(const T& r_element, T& r_result);
        //查找一个元素
        bool getElement(const T& r_element, T* &r_result);
        //取元素列表
        bool getElements(vector<T> &r_resultList);
        //根据元素的偏移量获取对应的对象指针 add by gaojf 2010-5-18 14:08
        //r_offset为NodeTmpt的起始位置
        T*     getElement(const size_t& r_offset);
        //根据node指针获取该NodeTmpt的起始位置 add by gaojf 2010-5-18 14:08
        size_t getOffset(const T* r_element);
    protected:
        //根据索引找元素的位置
        bool getNodeIndexPos(const T& r_element, int& r_indexPos, NodeTmpt<T>* &r_pNode);
        int lowerBound(const T& r_element, NodeTmpt<T>* &r_pNode) const;
    public:
        void dumpInfo(ostream& r_os) const;
};

#include "CtlElementTmpt.cpp"

#endif //_CTLELEMENT_H_INCLUDE_20080423
