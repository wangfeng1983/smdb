#ifndef _TableOnUndo_H_
#define _TableOnUndo_H_


#include "RecordConvert.h"
#include "MemManager.h"
#include "BaseTable.h"

/**
* TableOnUndo:UNDO空间上的表,目前主要是事务管理器的数据,提供简单的增删改查功能。
  至支持单字段索引操作
*/

class SrcLatchInfo;
class TransResource;

class TableOnUndo: public BaseTable
{
    public:

        /**
        * TableOnUndo:构造函数
        * @param	tableName	表名称
        * @param	memManager	数据库内存管理实例的指针
        * @return
        */
        TableOnUndo(const char* tableName, MemManager* memManager);

        /**
        * ~TableOnUndo:析构函数
        * @param
        * @return
        */
        virtual ~TableOnUndo();

        /**
        * initialize:表初始化函数
        * @param
        * @return
        */
        virtual void initialize();

        /**
        * create:表创建函数
        * @param	tableDef	表的定义描述
        * @return
        */
        virtual void create(const TableDef& tableDef);

        /**
        * drop:表删除函数
        * @param
        * @return
        */
        virtual void drop();

        /**
        * truncate:表清空函数
        * @param
        * @return
        */
        virtual void truncate();

        /**
        * createIndex:表索引创建函数
        * @param	indexDef	索引定义描述符
        * @return
        */
        virtual void createIndex(const IndexDef& indexDef, TransResource* pTransRes = NULL);

        /**
        * dropIndex:表索引删除函数
        * @param	indexName	索引名称
        * @return
        */
        virtual void dropIndex(const char* indexName);

        /**
        * select:根据索引名称＋索引字段值，获取记录信息
        * @param	indexName			索引名称
        * @param	value				索引值
        * @return	vector<TransData>	记录结果集
        */
        void select(const char* indexName, const char* idxKey , vector<MdbAddress>& recordVector);

        /**
        * deleteRec:根据transDatas删除记录
        * @param	indexName			索引名称
        * @param	value				索引值
        * @return
        */
        void deleteRec(const char* indexName, const void* idxKey);

        /**
        * insert:根据transDatas插入记录
        * @param	value			数据
        * @return
        */
        void insert(void* pRecord);

        /**
        * update:根据transDatas更新记录
        * @param	indexName			索引名称
        * @param	value				索引值
        * @param	columnName			更新字段名称
        * @param	columValue			更新字段值

        */
        void update(const char* indexName, const void* idxKey, const char* columnName, const void* columValue);

        SrcLatchInfo* getSrcLatchInfo()
        {
            return m_pSrcLatchInfo;
        }

    protected:
        /**
        * getIndexByName:根据Index的名字获取Index指针
        * @param
        * @return Index指针
        */
        Index* getIndexByName(const char* indexName);

    private:

        /**
        * deleteAllIndex:根据一条记录,删除其所有索引
        * @param	pIdxKey			待删除索引信息
        * @return
        */
        void deleteAllIndex(const MdbAddress& address);

        /**
        * insertAllIndex:根据一条记录,插入其所有索引
        * @param	pIdxKey			待插入索引信息
        * @return
        */
        void insertAllIndex(const MdbAddress& addr);

        /**
        * updateAllIndex:根据一条记录,更新其所有索引
        * @param	pOldIdxKey			待删除索引信息
        * @param	pOldIdxKey			待插入索引信息
        * @return
        */
        void updateAllIndex(const MdbAddress& addr, const char* columnName, const void* columValue);

    protected:
        UndoMemMgr*			m_pUndoMemMgr;						//UNDO内存管理指针
        SrcLatchInfo*        m_pSrcLatchInfo;                   //属于事务管理器表的latch
};


#endif

