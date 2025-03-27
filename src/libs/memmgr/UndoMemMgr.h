#ifndef UNDOMEMMGR_H_INCLUDE_20100507
#define UNDOMEMMGR_H_INCLUDE_20100507

#include "SpaceMgrBase.h"
#include "UndoStructdef.h"
#include "CtlElementTmpt.h"
#include "IndexSegsMgr.h"
#include "DataPagesMgr.h"

class MemManager;
//备注：Undo DDL的操作直接用表空间锁（为了简化算法）
class UndoMemMgr: public SpaceMgrBase
{
    public:
        UndoMemMgr()
        {
            m_scn = T_SCN();
        };
        virtual ~UndoMemMgr() {};
        bool initialize(SpaceInfo& r_spaceInfo);
        void setMemMgr(MemManager* r_mgr)
        {
            m_memmgr = r_mgr;
        }
    public:
        /**
         * insert  插入影子信息: 返回UNDO记录的偏移量.
         * @param  r_pundodesc    : UNDO表指针
         * @param  r_doffset      : 原始数据的偏移量
         * @param  r_scn          : 时间戳
         * @param  r_value        : 值
         * @param  r_prevUndopos  : 前个版本的影子偏移(空表示无)
         * @return 正常返回UNDO记录的偏移量。非正常，则抛出异常
         */
        UndoPosition insert(TableDesc*              r_pundodesc,
                            const ShmPosition&      r_doffset,
                            const T_SCN&            r_scn    ,
                            const char*             r_value  ,
                            const UndoPosition&     r_prevUndopos) throw(Mdb_Exception);
        /**
         * insert  插入影子信息: 返回UNDO记录的偏移量.
         * @param  r_type         : 数据类型
         * @param  r_tbname       : 原数据表名(只有是插入表数据SLOT有用)
         * @param  r_doffset      : 原始数据的偏移量
         * @param  r_scn          : 时间戳
         * @param  r_value        : 值
         * @param  r_prevUndopos  : 前个版本的影子偏移(空表示无)
         * @return 正常返回UNDO记录的偏移量。非正常，则抛出异常
         */
        UndoPosition insert(const T_UNDO_DESCTYPE&  r_type   ,
                            const T_NAMEDEF&        r_tbname ,
                            const ShmPosition&      r_doffset,
                            const T_SCN&            r_scn    ,
                            const char*             r_value  ,
                            const UndoPosition&     r_prevUndopos) throw(Mdb_Exception);
        /**
         * free    回收影子信息：对指定的节点对应的影子进行回收
         *         r_scn之前的。r_flag为1 最新的不回收
         * @param  r_type         : 数据类型
         * @param  r_tbname       : 原数据表名(只有是插入表数据SLOT有用)
         * @param  r_scn          : 时间戳
         * @param  r_flag         : 最新一个是否保留0 否,1是
         * @param  r_undopos      : 影子偏移量
         */
        void free(const T_UNDO_DESCTYPE&  r_type   ,
                  const T_NAMEDEF&        r_tbname ,
                  const T_SCN&            r_scn    ,
                  const int&              r_flag   ,
                  UndoPosition&     r_undopos) throw(Mdb_Exception);
        void free(TableDesc*              r_tabledesc,
                  const T_SCN&            r_scn    ,
                  const int&              r_flag   ,
                  UndoPosition&     r_undopos) throw(Mdb_Exception);
        /**
         * getUndoInfo    获取影子信息
         * @param  r_undopos      : 影子偏移量
         */
        char* getUndoInfo(const UndoPosition&     r_undopos) throw(Mdb_Exception);
        /**
         * getUndoInfo    获取影子信息
         * @param  r_scn          : 时间戳
         * @param  r_undopos      : 影子偏移量
         */
        char* getUndoInfo(const T_SCN&            r_scn    ,
                          const UndoPosition&     r_undopos) throw(Mdb_Exception);
        /**
         * getUndoInfo    获取影子信息
         * @param  r_scn1/r_scn2  : 时间戳(r_scn1,r_scn2)之间的
         * @param  r_undopos      : 影子偏移量
         */
        char* getUndoInfo(const T_SCN&            r_scn1    ,
                          const T_SCN&            r_scn2   ,
                          const UndoPosition&     r_undopos) throw(Mdb_Exception);
        //获取REDO内存的信息。REDO的内存管理在REDO内部管理
        void getRedobufInfo(char * &r_addr, size_t& r_size);
        void getPageInfo(const size_t& r_pgoffset, PageInfo *&r_pPage)
        {
            r_pPage = (PageInfo*)(m_spaceHeader.m_shmAddr + r_pgoffset);
        }
    public:
        void createTable(const TableDef& r_undotableDef, TableDesc* &r_undotableDesc)  ;
        void dropTable(const char* r_tableName)  ;
        void truncateTable(const char* r_tableName)  ;
        void createIndex(const IndexDef& r_idxDef, IndexDesc* &r_idxDesc)  ;
        void dropIndex(const char* r_idxName)  ;
        void truncateIndex(const char* r_idxName)  ;
        void allocateTableMem(TableDesc* &r_tableDesc, MdbAddress& r_addr);
        void freeTableMem(TableDesc* &r_tableDesc, const MdbAddress& r_addr);
        void allocateIdxMem(IndexDesc* &r_pIndexDesc, MdbAddress& r_addr)  ;
        void freeIdxMem(IndexDesc* &r_pindexDesc, const MdbAddress& r_addr)  ;
        bool getIndexDescByName(const char* r_indexname, IndexDesc* &r_pundoIdesc);
        bool getTbDescByname(const char* r_tablename, TableDesc* &r_pundodesc);
        virtual float getTableSpaceUsedPercent();
        void getTableSpaceUsedPercent(map<string, float> &vUserdPercent, const char* cTableSpaceName);
    protected:
        /**
         * getUndoInfo    获取影子信息
         * @param  r_type         : 数据类型
         * @param  r_tbname       : 原数据表名(只有是插入表数据SLOT有用)
         * @param  r_scn          : 时间戳
         * @param  r_undopos      : 影子偏移量
         */
        char* getUndoInfo(const T_SCN&            r_scn    ,
                          const UndoPosition&     r_undopos,
                          Undo_Slot*       &r_pundo) throw(Mdb_Exception);
        //申请一个节点的内存
        void allocateSlot(DescPageInfos*         r_pdescpages,
                          MdbAddress&       r_addr) throw(Mdb_Exception);
        //释放一个节点的内存:如果页面已经全部空闲,释放给表空间
        void deleteSlot(DescPageInfos*        r_pdescpages,
                        const ShmPosition&    r_addr) throw(Mdb_Exception);
        void deleteSlot(DescPageInfos* r_pdescpages,
                        const ShmPosition&    r_addr,
                        ShmPosition&    r_pagepos,
                        PageInfo*    &r_ppage) throw(Mdb_Exception);
        //根据slot位置获取页面位置
        bool getPageBySlot(const ShmPosition& r_slotPos, ShmPosition& r_pagepos, PageInfo * &r_ppageinfo);
        bool getDescbySlot(const ShmPosition& r_slotPos, TableDesc * &r_pundodesc);
    public:
        //根据表空间定义信息,创建表空间
        bool createUndoSpace(SpaceInfo& r_spaceInfo);
        void deleteUndoSpace();
        //初始化表空间r_descnum 描述符最大个数
        //r_redosize redo日志空间大小
        //r_indexsize 索引段大小
        bool initSpaceInfo(const size_t& r_descnum , const size_t& r_redosize,
                           const size_t& r_indexsize);
        //根据原数据表创建索引slot对应的Undo表 2012/9/18 7:51:53 gaojf
        bool createUndoIdxDesc(const TableDesc& r_tabledesc);
        //插入UNDO描述符信息:根据原数据表创建表描述符
        bool createUndoDescs(const vector<TableDesc> &r_tabledescs);
        //根据原数据表创建对应的Undo表
        bool createUndoDesc(const TableDesc& r_tabledesc);
        //删除一个原数据表对应的Undo表
        bool deleteUndoDesc(const TableDesc& r_tabledesc);
        bool createTxDesc(const TableDesc& r_tabledesc);
        bool deleteTxDesc(const TableDesc& r_tabledesc);
        //attach方式init
        bool attach_init(SpaceInfo& r_spaceInfo);
        //
        bool getTableDefList(vector<TableDef> &r_tabledefs);
        bool getIndexDefList(vector<IndexDef> &r_indexdefs);

        bool dumpSpaceInfo(ostream& r_os);
    protected:
        friend class ChkptMgr;
        friend class MemManager;
        friend class Table; //add by gaojf 2012/9/3 16:09:26
        //插入一个表描述符
        void createUndoTbDesc(const TableDesc& r_undodesc, TableDesc* &r_pundodesc);
        //删除一个表描述符
        void deleteUndoTbDesc(const TableDesc& r_undodesc);
        //获取一个表描述符
        void getUndoDesc(const T_UNDO_DESCTYPE&  r_type, const char* r_tablename, TableDesc* &r_pundodesc);
        //根据原数据表初始化对应的Undo表描述符
        void initUndoDesc(const TableDef& r_tabledef, TableDesc& r_undodesc);
        void initTxDesc(const TableDef& r_tabledef, TableDesc& r_txdesc);
        void updateTbDescTime(); //更新描述符修改时间
        void initUndoFixDesc(TableDef t_undotbdefs[T_UNDO_TYPE_UNDEF]);
        //以下为支持Undo索引而增加 事务控制表 需要支持
        bool createHashIndex(IndexDesc* r_idxDesc, ShmPosition& r_shmPos);
        //r_shmPos是指向Hash中ShmPositioin[]的首地址
        bool dropHashIdex(const ShmPosition& r_shmPos) ;
        //r_shmPos是指向Hash中ShmPositioin[]的首地址
        void initHashSeg(const ShmPosition& r_shmPos);
    protected:
        //SpaceInfo     m_spaceHeader;         ///<在SpaceMgrBase中定义
        Undo_spaceinfo*         m_spinfo;      ///<表空间信息
        ListManager<TableDesc>  m_tbDescMgr;   ///<表描述符
        ListManager<IndexDesc>  m_indexDescMgr;///<索引描述符
        char*                   m_redoaddr;    ///<redo起始地址
        IndexSegsMgr            m_indexSegsMgr;///<索引管理对象
        DataPagesMgr            m_dpagesMgr;   ///<数据PAGE管理对象
        MemManager*             m_memmgr;      ///<内存管理指针
    private:
        T_SCN            m_scn;
        TableDesc*       m_undodesc[T_UNDO_TYPE_UNDEF];

};

#endif //UNDOMEMMGR_H_INCLUDE_20100507


