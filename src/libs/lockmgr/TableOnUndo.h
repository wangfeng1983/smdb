#ifndef _TableOnUndo_H_
#define _TableOnUndo_H_


#include "RecordConvert.h"
#include "MemManager.h"
#include "BaseTable.h"

/**
* TableOnUndo:UNDO�ռ��ϵı�,Ŀǰ��Ҫ�����������������,�ṩ�򵥵���ɾ�Ĳ鹦�ܡ�
  ��֧�ֵ��ֶ���������
*/

class SrcLatchInfo;
class TransResource;

class TableOnUndo: public BaseTable
{
    public:

        /**
        * TableOnUndo:���캯��
        * @param	tableName	������
        * @param	memManager	���ݿ��ڴ����ʵ����ָ��
        * @return
        */
        TableOnUndo(const char* tableName, MemManager* memManager);

        /**
        * ~TableOnUndo:��������
        * @param
        * @return
        */
        virtual ~TableOnUndo();

        /**
        * initialize:���ʼ������
        * @param
        * @return
        */
        virtual void initialize();

        /**
        * create:��������
        * @param	tableDef	��Ķ�������
        * @return
        */
        virtual void create(const TableDef& tableDef);

        /**
        * drop:��ɾ������
        * @param
        * @return
        */
        virtual void drop();

        /**
        * truncate:����պ���
        * @param
        * @return
        */
        virtual void truncate();

        /**
        * createIndex:��������������
        * @param	indexDef	��������������
        * @return
        */
        virtual void createIndex(const IndexDef& indexDef, TransResource* pTransRes = NULL);

        /**
        * dropIndex:������ɾ������
        * @param	indexName	��������
        * @return
        */
        virtual void dropIndex(const char* indexName);

        /**
        * select:�����������ƣ������ֶ�ֵ����ȡ��¼��Ϣ
        * @param	indexName			��������
        * @param	value				����ֵ
        * @return	vector<TransData>	��¼�����
        */
        void select(const char* indexName, const char* idxKey , vector<MdbAddress>& recordVector);

        /**
        * deleteRec:����transDatasɾ����¼
        * @param	indexName			��������
        * @param	value				����ֵ
        * @return
        */
        void deleteRec(const char* indexName, const void* idxKey);

        /**
        * insert:����transDatas�����¼
        * @param	value			����
        * @return
        */
        void insert(void* pRecord);

        /**
        * update:����transDatas���¼�¼
        * @param	indexName			��������
        * @param	value				����ֵ
        * @param	columnName			�����ֶ�����
        * @param	columValue			�����ֶ�ֵ

        */
        void update(const char* indexName, const void* idxKey, const char* columnName, const void* columValue);

        SrcLatchInfo* getSrcLatchInfo()
        {
            return m_pSrcLatchInfo;
        }

    protected:
        /**
        * getIndexByName:����Index�����ֻ�ȡIndexָ��
        * @param
        * @return Indexָ��
        */
        Index* getIndexByName(const char* indexName);

    private:

        /**
        * deleteAllIndex:����һ����¼,ɾ������������
        * @param	pIdxKey			��ɾ��������Ϣ
        * @return
        */
        void deleteAllIndex(const MdbAddress& address);

        /**
        * insertAllIndex:����һ����¼,��������������
        * @param	pIdxKey			������������Ϣ
        * @return
        */
        void insertAllIndex(const MdbAddress& addr);

        /**
        * updateAllIndex:����һ����¼,��������������
        * @param	pOldIdxKey			��ɾ��������Ϣ
        * @param	pOldIdxKey			������������Ϣ
        * @return
        */
        void updateAllIndex(const MdbAddress& addr, const char* columnName, const void* columValue);

    protected:
        UndoMemMgr*			m_pUndoMemMgr;						//UNDO�ڴ����ָ��
        SrcLatchInfo*        m_pSrcLatchInfo;                   //����������������latch
};


#endif

