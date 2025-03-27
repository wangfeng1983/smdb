// ############################################
// Source file : IndexHashOnUndo.h
// Version     : 2.0
// Language	   : ANSI C++
// OS Platform : UNIX
// Authors     : chen min
// E_mail      : chenm@lianchuang.com
// Create      : 2010-06-13 14:42:17
// Update      : 2010-06-13 14:42:19
// Copyright(C): chen min, Linkage.
// ############################################

#ifndef INDEXHASHONUNDO_H_INCLUDED
#define INDEXHASHONUNDO_H_INCLUDED

#include <vector>

#include "Index.h"
#include "Bucket.h"
#include "MdbConstDef2.h"

class UndoMemMgr;

// 同Bucket类 多了成员m_usedSlot 只是为了使用方便
class BucketIdxOnUndo
{
    public:
        Undo_Slot   m_undoSlot; // 此变量实际使用意义不大,主要是为了引用下面的ShmPosi变量方便
        ShmPosition m_data;
        ShmPosition m_diffNext;//不同的key的链表 纵链表
        ShmPosition m_sameNext;//相同的key的链表 横链表

};

// IndexHashOnUndo 基于hash表的索引结构实现
class IndexHashOnUndo : public Index
{
    public:
        IndexHashOnUndo(const char* cTableName, const char* cIndexName, MemManager* memMgr, RecordConvert* pRecordConvert);
        virtual ~IndexHashOnUndo() {};

        virtual bool create(const IndexDef& theIndexDef) ;   // 创建一个新索引
        virtual bool drop()                        ;
        virtual bool truncate()                    ;
        virtual bool initialization()              ; //初始化一个索引（attach内存）

        virtual int select(const InputParams* pInputParams   //查询关键字
                           , vector<MdbAddress> &result
                           , int iRowCount
                           , bool isTransaction = false
                                   , const TransResource* pTransRes = NULL) ;

        virtual int select(const char* hashkey               //查询关键字
                           , vector<MdbAddress> &result
                           , int iRowCount
                           , bool isTransaction = false
                                   , const TransResource* pTransRes = NULL) ;

        virtual bool deleteIdx(const char* hashkey       //查询关键字
                               , const MdbAddress& theMdbAddress);

        virtual bool insert(const char* hashkey       //查询关键字
                            , const MdbAddress& theMdbAddress
                            , TransResource* pTransRes = NULL
                                    , TableOnUndo* pTransTable = NULL)  ;

        virtual bool getHashPercent(int& iHashSize
                                    , int& iUsedHash
                                    , int& iRecordCount
                                    , map<int, int>& mHashCal); //Hash桶内元素个数，该类型Hash桶的个数
        //virtual bool reIndex(Index * r_destIndex,TransResource *pTransRes,TableOnUndo *pTransTable);//重新对	r_descIndex 索引

    protected:
        // 拼出多字段组成的索引键值 调用次数多 内联实现
        virtual inline const char* getIndexKey(void* pRecord, bool isNewHashNode = false)
        {
            char columnValue[MAX_COLUMN_LEN];
            int  iColumnLenth, iOffSize = 0 ;
            memset(m_indexKey, 0, m_iHashKeyLenth);
            m_pRecordConvert->setRecordValue((char*)pRecord + sizeof(Undo_Slot));
            for (int i = 0; i < (m_indexDesc->m_indexDef).m_columnNum; ++i)
            {
                iColumnLenth = 0;
                memset(columnValue, 0, sizeof(columnValue));
                m_pRecordConvert->getColumnValue((m_indexDesc->m_indexDef).m_columnList[i]
                                                 , columnValue, iColumnLenth);
                memcpy(m_indexKey + iOffSize, columnValue, iColumnLenth);
                iOffSize += iColumnLenth;
            }
            return  m_indexKey;
        }
    private:
        int selectALL(vector<MdbAddress> &results);
        int selectLimited(vector<MdbAddress> &results, int iRowCount);
        int selectRowByRow(vector<MdbAddress> &results, int iRowCount);

    protected:
        int            m_size;        //hash表分配大小
        HashIndexNode* m_hashTable;   //共享内存hash索引区的首地址
        int            m_iHashPosi;   // by chenm 2009-5-18 23:07:02 逐条扫描的计数器
        UndoMemMgr*  	m_pUndoMemMgr;
};

#endif //INDEXHASHONUNDO_H_INCLUDED
