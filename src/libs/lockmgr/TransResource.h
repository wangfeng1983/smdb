#ifndef TRANSRESOURCE_H_HEADER_INCLUDED_B3E0E918
#define TRANSRESOURCE_H_HEADER_INCLUDED_B3E0E918

// 用来存放事务过程中，变更过的信息，以用于回滚
// 一个本地会话一个TransResource实例
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
        //slot地址 DML操作类型
        TRANSRES_POOL m_lockedSlot;
        //hash桶地址 DML操作类型
        TRANSRES_POOL m_lockedIdxNode;
        // 已经处理过ITL的page,对于一个事务中,同一个page上有两条rec的情况下,只需要设置一次ITL信息,但需要知道ITL槽号
        //page地址指针 itl槽号
        TRANSRES_POOL m_lockedPage;

        //## 是否已经在事务管理器中注册过,同一个session中,对一张表多次事务,只注册一次
        IS_REGED_TX_MAP m_hasRegSid;
        int  m_sid;
        int  m_semNo;
        bool m_sessionQuit; // 会话退出标志,在session的析构函数中置true
        // 带入表达式判断逻辑 在锁定时 判断附加表达式是否满足条件
        Expression*    m_pExpression;
        RecordConvert* m_pRecConvert;
        void**         m_pCondValues;

        // 锁管理 放在这儿 纯粹是为了在不同实体间进程传递 减少接口函数的参数个数
        LockMgr* m_pSlotLockMgr  ;
        LockMgr* m_pBucketLockMgr ;

        vector<void*> m_index2del;  // 待置为无效的索引节点地址

};



#endif /* TRANSRESMGR_H_HEADER_INCLUDED_B3E0E918 */
