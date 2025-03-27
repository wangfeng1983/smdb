#ifndef TRANSRESOURCE_H_HEADER_INCLUDED_B3E0E918
#define TRANSRESOURCE_H_HEADER_INCLUDED_B3E0E918

// ���������������У����������Ϣ�������ڻع�
// һ�����ػỰһ��TransResourceʵ��
//##ModelId=4C0DEC160022
#include <map>
#include <vector>
#include <algorithm>

#include "base/config-all.h"
#include "MdbAddress.h"


class Expression;
class RecordConvert;
class LockMgr;

template<class T>
class Pred
{
    public:
        bool   operator()(const T& left, const T& right)
        {
            return left.first < right.first;
        }
};

template<class Key, class T>
class MyMap
{
    public:
        typedef pair<Key, T> value_type;
        class MyIterator
        {
            public:
                MyIterator() {}
                MyIterator(const typename vector< value_type >::iterator& itr)
                {
                    m_itr = itr;
                }
                int operator = (const MyIterator& MyiItr)
                {
                    m_itr = MyiItr.m_itr;
                    return 1;
                }
                const value_type* operator ->()
                {
                    return &(*m_itr);
                }
                MyIterator operator ++ ()
                {
                    return ++m_itr;
                }
                bool operator != (const MyIterator& MyiItr)
                {
                    return 	m_itr != MyiItr.m_itr;
                }
                bool operator == (const MyIterator& MyiItr)
                {
                    return 	m_itr == MyiItr.m_itr;
                }
            protected:
                typename vector< value_type >::iterator m_itr;
        };

        MyMap()
        {
            m_elements.reserve(5000);
            m_elements.clear();
        }
        ~MyMap()
        {
            m_elements.clear();
        }
        inline void insert(const value_type& r_p)
        {
            typename vector< value_type >::iterator itr = ::lower_bound(m_elements.begin(), m_elements.end(), r_p, Pred<value_type>());
            if (itr == m_elements.end())
            {
                m_elements.push_back(r_p);
            }
            else if (!((*itr).first == r_p.first))
            {
                m_elements.insert(itr, r_p);
            }
        }
        void clear()
        {
            m_elements.clear();
        }
        inline MyIterator find(Key& key)
        {
            value_type t_p;
            t_p.first = key;
            typename vector< value_type >::iterator itr = ::lower_bound(m_elements.begin(), m_elements.end(), t_p, Pred<value_type>());
            if (itr == m_elements.end()
                    || !((*itr).first == key))
            {
                return MyIterator(m_elements.end());
            }
            else
            {
                return MyIterator(itr);
            }
        }
        MyIterator begin()
        {
            return MyIterator(m_elements.begin());
        }
        MyIterator end()
        {
            return MyIterator(m_elements.end());
        }


    protected:
        vector< value_type >	m_elements;
};

//typedef MyMap<MdbAddress,int> TRANSRES_POOL;
typedef map<MdbAddress, int> TRANSRES_POOL;
//typedef TRANSRES_POOL::MyIterator TRANSRES_POOL_ITR;
typedef TRANSRES_POOL::iterator TRANSRES_POOL_ITR;

typedef map<string, bool> IS_REGED_TX_MAP;
typedef IS_REGED_TX_MAP::iterator IS_REGED_TX_MAP_ITR;


class TransResource
{
    public:
        //##ModelId=4C1EC129016D
        TransResource(const int iSid, const int iSemNo)
        {
            m_sid          = iSid;
            m_semNo        = iSemNo;
            m_sessionQuit  = false;
            m_index2del.reserve(5000);
            m_hasRegSid.clear();
            this->clear();
        }

        void setLockMgrPt(LockMgr* pSlotLockMgr, LockMgr* pBucketLockMgr)
        {
            m_pSlotLockMgr = pSlotLockMgr;
            m_pBucketLockMgr = pBucketLockMgr ;
        }

        void clear()
        {
            m_lockedSlot.clear();
            m_lockedIdxNode.clear();
            m_lockedPage.clear();
            m_index2del.clear();
            m_pExpression = NULL;
            m_pCondValues = NULL;
            m_pRecConvert = NULL;
            return;
        }

    public:
        //slot��ַ DML��������
        TRANSRES_POOL m_lockedSlot;
        //hashͰ��ַ DML��������
        TRANSRES_POOL m_lockedIdxNode;
        // �Ѿ������ITL��page,����һ��������,ͬһ��page��������rec�������,ֻ��Ҫ����һ��ITL��Ϣ,����Ҫ֪��ITL�ۺ�
        //page��ַָ�� itl�ۺ�
        TRANSRES_POOL m_lockedPage;

        //## �Ƿ��Ѿ��������������ע���,ͬһ��session��,��һ�ű�������,ֻע��һ��
        IS_REGED_TX_MAP m_hasRegSid;
        int  m_sid;
        int  m_semNo;
        bool m_sessionQuit; // �Ự�˳���־,��session��������������true
        // ������ʽ�ж��߼� ������ʱ �жϸ��ӱ��ʽ�Ƿ���������
        Expression*    m_pExpression;
        RecordConvert* m_pRecConvert;
        void**         m_pCondValues;

        // ������ ������� ������Ϊ���ڲ�ͬʵ�����̴��� ���ٽӿں����Ĳ�������
        LockMgr* m_pSlotLockMgr  ;
        LockMgr* m_pBucketLockMgr ;

        vector<void*> m_index2del;  // ����Ϊ��Ч�������ڵ��ַ

};



#endif /* TRANSRESMGR_H_HEADER_INCLUDED_B3E0E918 */
