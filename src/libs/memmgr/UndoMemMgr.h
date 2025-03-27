#ifndef UNDOMEMMGR_H_INCLUDE_20100507
#define UNDOMEMMGR_H_INCLUDE_20100507

#include "SpaceMgrBase.h"
#include "UndoStructdef.h"
#include "CtlElementTmpt.h"
#include "IndexSegsMgr.h"
#include "DataPagesMgr.h"

class MemManager;
//��ע��Undo DDL�Ĳ���ֱ���ñ�ռ�����Ϊ�˼��㷨��
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
         * insert  ����Ӱ����Ϣ: ����UNDO��¼��ƫ����.
         * @param  r_pundodesc    : UNDO��ָ��
         * @param  r_doffset      : ԭʼ���ݵ�ƫ����
         * @param  r_scn          : ʱ���
         * @param  r_value        : ֵ
         * @param  r_prevUndopos  : ǰ���汾��Ӱ��ƫ��(�ձ�ʾ��)
         * @return ��������UNDO��¼��ƫ�����������������׳��쳣
         */
        UndoPosition insert(TableDesc*              r_pundodesc,
                            const ShmPosition&      r_doffset,
                            const T_SCN&            r_scn    ,
                            const char*             r_value  ,
                            const UndoPosition&     r_prevUndopos) throw(Mdb_Exception);
        /**
         * insert  ����Ӱ����Ϣ: ����UNDO��¼��ƫ����.
         * @param  r_type         : ��������
         * @param  r_tbname       : ԭ���ݱ���(ֻ���ǲ��������SLOT����)
         * @param  r_doffset      : ԭʼ���ݵ�ƫ����
         * @param  r_scn          : ʱ���
         * @param  r_value        : ֵ
         * @param  r_prevUndopos  : ǰ���汾��Ӱ��ƫ��(�ձ�ʾ��)
         * @return ��������UNDO��¼��ƫ�����������������׳��쳣
         */
        UndoPosition insert(const T_UNDO_DESCTYPE&  r_type   ,
                            const T_NAMEDEF&        r_tbname ,
                            const ShmPosition&      r_doffset,
                            const T_SCN&            r_scn    ,
                            const char*             r_value  ,
                            const UndoPosition&     r_prevUndopos) throw(Mdb_Exception);
        /**
         * free    ����Ӱ����Ϣ����ָ���Ľڵ��Ӧ��Ӱ�ӽ��л���
         *         r_scn֮ǰ�ġ�r_flagΪ1 ���µĲ�����
         * @param  r_type         : ��������
         * @param  r_tbname       : ԭ���ݱ���(ֻ���ǲ��������SLOT����)
         * @param  r_scn          : ʱ���
         * @param  r_flag         : ����һ���Ƿ���0 ��,1��
         * @param  r_undopos      : Ӱ��ƫ����
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
         * getUndoInfo    ��ȡӰ����Ϣ
         * @param  r_undopos      : Ӱ��ƫ����
         */
        char* getUndoInfo(const UndoPosition&     r_undopos) throw(Mdb_Exception);
        /**
         * getUndoInfo    ��ȡӰ����Ϣ
         * @param  r_scn          : ʱ���
         * @param  r_undopos      : Ӱ��ƫ����
         */
        char* getUndoInfo(const T_SCN&            r_scn    ,
                          const UndoPosition&     r_undopos) throw(Mdb_Exception);
        /**
         * getUndoInfo    ��ȡӰ����Ϣ
         * @param  r_scn1/r_scn2  : ʱ���(r_scn1,r_scn2)֮���
         * @param  r_undopos      : Ӱ��ƫ����
         */
        char* getUndoInfo(const T_SCN&            r_scn1    ,
                          const T_SCN&            r_scn2   ,
                          const UndoPosition&     r_undopos) throw(Mdb_Exception);
        //��ȡREDO�ڴ����Ϣ��REDO���ڴ������REDO�ڲ�����
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
         * getUndoInfo    ��ȡӰ����Ϣ
         * @param  r_type         : ��������
         * @param  r_tbname       : ԭ���ݱ���(ֻ���ǲ��������SLOT����)
         * @param  r_scn          : ʱ���
         * @param  r_undopos      : Ӱ��ƫ����
         */
        char* getUndoInfo(const T_SCN&            r_scn    ,
                          const UndoPosition&     r_undopos,
                          Undo_Slot*       &r_pundo) throw(Mdb_Exception);
        //����һ���ڵ���ڴ�
        void allocateSlot(DescPageInfos*         r_pdescpages,
                          MdbAddress&       r_addr) throw(Mdb_Exception);
        //�ͷ�һ���ڵ���ڴ�:���ҳ���Ѿ�ȫ������,�ͷŸ���ռ�
        void deleteSlot(DescPageInfos*        r_pdescpages,
                        const ShmPosition&    r_addr) throw(Mdb_Exception);
        void deleteSlot(DescPageInfos* r_pdescpages,
                        const ShmPosition&    r_addr,
                        ShmPosition&    r_pagepos,
                        PageInfo*    &r_ppage) throw(Mdb_Exception);
        //����slotλ�û�ȡҳ��λ��
        bool getPageBySlot(const ShmPosition& r_slotPos, ShmPosition& r_pagepos, PageInfo * &r_ppageinfo);
        bool getDescbySlot(const ShmPosition& r_slotPos, TableDesc * &r_pundodesc);
    public:
        //���ݱ�ռ䶨����Ϣ,������ռ�
        bool createUndoSpace(SpaceInfo& r_spaceInfo);
        void deleteUndoSpace();
        //��ʼ����ռ�r_descnum ������������
        //r_redosize redo��־�ռ��С
        //r_indexsize �����δ�С
        bool initSpaceInfo(const size_t& r_descnum , const size_t& r_redosize,
                           const size_t& r_indexsize);
        //����ԭ���ݱ�������slot��Ӧ��Undo�� 2012/9/18 7:51:53 gaojf
        bool createUndoIdxDesc(const TableDesc& r_tabledesc);
        //����UNDO��������Ϣ:����ԭ���ݱ�����������
        bool createUndoDescs(const vector<TableDesc> &r_tabledescs);
        //����ԭ���ݱ�����Ӧ��Undo��
        bool createUndoDesc(const TableDesc& r_tabledesc);
        //ɾ��һ��ԭ���ݱ��Ӧ��Undo��
        bool deleteUndoDesc(const TableDesc& r_tabledesc);
        bool createTxDesc(const TableDesc& r_tabledesc);
        bool deleteTxDesc(const TableDesc& r_tabledesc);
        //attach��ʽinit
        bool attach_init(SpaceInfo& r_spaceInfo);
        //
        bool getTableDefList(vector<TableDef> &r_tabledefs);
        bool getIndexDefList(vector<IndexDef> &r_indexdefs);

        bool dumpSpaceInfo(ostream& r_os);
    protected:
        friend class ChkptMgr;
        friend class MemManager;
        friend class Table; //add by gaojf 2012/9/3 16:09:26
        //����һ����������
        void createUndoTbDesc(const TableDesc& r_undodesc, TableDesc* &r_pundodesc);
        //ɾ��һ����������
        void deleteUndoTbDesc(const TableDesc& r_undodesc);
        //��ȡһ����������
        void getUndoDesc(const T_UNDO_DESCTYPE&  r_type, const char* r_tablename, TableDesc* &r_pundodesc);
        //����ԭ���ݱ��ʼ����Ӧ��Undo��������
        void initUndoDesc(const TableDef& r_tabledef, TableDesc& r_undodesc);
        void initTxDesc(const TableDef& r_tabledef, TableDesc& r_txdesc);
        void updateTbDescTime(); //�����������޸�ʱ��
        void initUndoFixDesc(TableDef t_undotbdefs[T_UNDO_TYPE_UNDEF]);
        //����Ϊ֧��Undo���������� ������Ʊ� ��Ҫ֧��
        bool createHashIndex(IndexDesc* r_idxDesc, ShmPosition& r_shmPos);
        //r_shmPos��ָ��Hash��ShmPositioin[]���׵�ַ
        bool dropHashIdex(const ShmPosition& r_shmPos) ;
        //r_shmPos��ָ��Hash��ShmPositioin[]���׵�ַ
        void initHashSeg(const ShmPosition& r_shmPos);
    protected:
        //SpaceInfo     m_spaceHeader;         ///<��SpaceMgrBase�ж���
        Undo_spaceinfo*         m_spinfo;      ///<��ռ���Ϣ
        ListManager<TableDesc>  m_tbDescMgr;   ///<��������
        ListManager<IndexDesc>  m_indexDescMgr;///<����������
        char*                   m_redoaddr;    ///<redo��ʼ��ַ
        IndexSegsMgr            m_indexSegsMgr;///<�����������
        DataPagesMgr            m_dpagesMgr;   ///<����PAGE�������
        MemManager*             m_memmgr;      ///<�ڴ����ָ��
    private:
        T_SCN            m_scn;
        TableDesc*       m_undodesc[T_UNDO_TYPE_UNDEF];

};

#endif //UNDOMEMMGR_H_INCLUDE_20100507


