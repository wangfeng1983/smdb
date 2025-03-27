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

// ͬBucket�� ���˳�Աm_usedSlot ֻ��Ϊ��ʹ�÷���
class BucketIdxOnUndo
{
    public:
        Undo_Slot   m_undoSlot; // �˱���ʵ��ʹ�����岻��,��Ҫ��Ϊ�����������ShmPosi��������
        ShmPosition m_data;
        ShmPosition m_diffNext;//��ͬ��key������ ������
        ShmPosition m_sameNext;//��ͬ��key������ ������

};

// IndexHashOnUndo ����hash��������ṹʵ��
class IndexHashOnUndo : public Index
{
    public:
        IndexHashOnUndo(const char* cTableName, const char* cIndexName, MemManager* memMgr, RecordConvert* pRecordConvert);
        virtual ~IndexHashOnUndo() {};

        virtual bool create(const IndexDef& theIndexDef) ;   // ����һ��������
        virtual bool drop()                        ;
        virtual bool truncate()                    ;
        virtual bool initialization()              ; //��ʼ��һ��������attach�ڴ棩

        virtual int select(const InputParams* pInputParams   //��ѯ�ؼ���
                           , vector<MdbAddress> &result
                           , int iRowCount
                           , bool isTransaction = false
                                   , const TransResource* pTransRes = NULL) ;

        virtual int select(const char* hashkey               //��ѯ�ؼ���
                           , vector<MdbAddress> &result
                           , int iRowCount
                           , bool isTransaction = false
                                   , const TransResource* pTransRes = NULL) ;

        virtual bool deleteIdx(const char* hashkey       //��ѯ�ؼ���
                               , const MdbAddress& theMdbAddress);

        virtual bool insert(const char* hashkey       //��ѯ�ؼ���
                            , const MdbAddress& theMdbAddress
                            , TransResource* pTransRes = NULL
                                    , TableOnUndo* pTransTable = NULL)  ;

        virtual bool getHashPercent(int& iHashSize
                                    , int& iUsedHash
                                    , int& iRecordCount
                                    , map<int, int>& mHashCal); //HashͰ��Ԫ�ظ�����������HashͰ�ĸ���
        //virtual bool reIndex(Index * r_destIndex,TransResource *pTransRes,TableOnUndo *pTransTable);//���¶�	r_descIndex ����

    protected:
        // ƴ�����ֶ���ɵ�������ֵ ���ô����� ����ʵ��
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
        int            m_size;        //hash������С
        HashIndexNode* m_hashTable;   //�����ڴ�hash���������׵�ַ
        int            m_iHashPosi;   // by chenm 2009-5-18 23:07:02 ����ɨ��ļ�����
        UndoMemMgr*  	m_pUndoMemMgr;
};

#endif //INDEXHASHONUNDO_H_INCLUDED
