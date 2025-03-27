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
        size_t m_pos;       ///���ڵ�ƫ����
        size_t m_prev;      ///��һ��λ��
        size_t m_next;      ///��һ��λ��
        int    m_useFlag;   ///<0 δʹ�� ,1 ռ��,2 ��ʹ�ù���δ��
        T      m_element;   ///Ԫ��
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
        int       m_maxNum;      ///<���Ԫ�ظ���
        int       m_nodeNum;     ///<����Ԫ�ظ���
        int       m_idleNum;     ///<δʹ�ù���Ԫ�ظ���
        size_t    m_firstNode;    ///<��һ��Ԫ��λ��
        size_t    m_lastNode;    ///<���һ��Ԫ��λ��
        size_t    m_fIdleNode;   ///<��һ������λ��(��δʹ�ù�)
        size_t    m_lIdleNode;   ///<���һ������λ��(��δʹ�ù�)
        size_t    m_fIdle2Node;  ///<��һ����ʹ�ù�����λ��
        size_t    m_lIdle2Node;  ///<���һ����ʹ�ù�����λ��

        //Mdb2.0 add by gaojf 2010-5-17 9:30
        T_SCN     m_scn;         ///<ʱ���������CHKPT
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
            const char *            m_pShmAddr;      ///<��ռ��ַ��Ϣ
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
        size_t         m_sOffSet;       ///<�����б����ʼƫ����
        char*          m_pShmAddr;      ///<��ռ��ַ��Ϣ
        NodeListInfo*  m_pNodeListInfo; ///<�б���Ϣ
        NodeTmpt<T>   *m_pNodeList;     ///<�б�Ԫ����Ϣ
        size_t*        m_pIndexInfo;    ///<������Ϣ:ÿ��Ԫ��ָ��NodeTmpt<T>
        //CmpIdxValue    ; ///<���������ȽϷ�����
    public:
        void update_scn(const T_SCN& r_scn)
        {
            m_pNodeListInfo->m_scn = r_scn;
        }
        void clear(const int& t_flag); //�������Ԫ��
        void reInit(); //���³�ʼ�����б� 2009-3-2 4:16 gaojf
        bool attach_init(char* r_shmAddr, const size_t& r_offSet);
        void startmdb_init(); //����MDB���ʼ��:����scn����Ϣ
        //��ʼ��Ԫ���б�(��һ�γ�ʼ��)
        bool initElements(char* r_shmAddr, const size_t& r_offSet, const int& r_maxNum);
        //��ʼ��(�ڲ���Ա��Ϣ�����������ڴ���Ϣ�ĳ�ʼ��)
        void initElements(char* r_shmAddr, const size_t& r_offSet);
        //���������б�ռ�õ��ڴ�ռ�
        size_t getSize() const;
        //����Ԫ��:r_flag=0��ʾ�����ֿ��к���ʹ�ù��Ŀ���
        //         r_flag=1������
        bool addElement(const T& r_element, const int& r_flag = 0);
        //����Ԫ��:r_flag=0��ʾ�����ֿ��к���ʹ�ù��Ŀ���
        //         r_flag=1������
        bool addElement(const T& r_element, T* &r_pElement, const int& r_flag = 0);
        //����Ԫ��:r_flag=0��ʾ�����ֿ��к���ʹ�ù��Ŀ���
        //         r_flag=1������
        // ������Ԫ�����ڵĵ�ַ��Ϣ��������
        bool addElement(NodeTmpt<T> &r_node, const int& r_flag = 0);
        //����Ԫ��:r_flag=0��ʾ�����ֿ��к���ʹ�ù��Ŀ���
        //         r_flag=1������
        // ������Ԫ�����ڵĵ�ַ��Ϣ��������
        bool addElement(NodeTmpt<T> &r_node, T* &r_pElement, const int& r_flag = 0);

        //ɾ��Ԫ��:r_flag=0��ʾ�����ֿ��к���ʹ�ù��Ŀ���
        //         r_flag=1������
        bool deleteElement(const T& r_element, const int& r_flag = 0);
        //����Ԫ�أ��Ǽ�ֵ����
        bool updateElement(const T& r_element);
        //����һ��Ԫ��
        bool getElement(const T& r_element, NodeTmpt<T> &r_result);
        //����һ��Ԫ��
        bool getElement(const T& r_element, NodeTmpt<T>* &r_result);
        //����һ��Ԫ��
        bool getElement(const T& r_element, T& r_result);
        //����һ��Ԫ��
        bool getElement(const T& r_element, T* &r_result);
        //ȡԪ���б�
        bool getElements(vector<T> &r_resultList);
        //����Ԫ�ص�ƫ������ȡ��Ӧ�Ķ���ָ�� add by gaojf 2010-5-18 14:08
        //r_offsetΪNodeTmpt����ʼλ��
        T*     getElement(const size_t& r_offset);
        //����nodeָ���ȡ��NodeTmpt����ʼλ�� add by gaojf 2010-5-18 14:08
        size_t getOffset(const T* r_element);
    protected:
        //����������Ԫ�ص�λ��
        bool getNodeIndexPos(const T& r_element, int& r_indexPos, NodeTmpt<T>* &r_pNode);
        int lowerBound(const T& r_element, NodeTmpt<T>* &r_pNode) const;
    public:
        void dumpInfo(ostream& r_os) const;
};

#include "CtlElementTmpt.cpp"

#endif //_CTLELEMENT_H_INCLUDE_20080423
