#ifndef _TABLEDESCRIPT_H_INCLUDE_20080411_
#define _TABLEDESCRIPT_H_INCLUDE_20080411_

#include "TableDefParam.h"
#include "MdbConstDef2.h"  //add by gaojf 2010-5-14 16:58
#include "Mdb_Exception.h"
#include "MdbAddress.h"

class MemManager;
class UndoMemMgr;
class PageInfo;
//DescPageInfo �ĸ��£���DescPageInfos�м���
class DescPageInfo
{
    public:
        int            m_slotSize;          ///<ÿ���ڵ�Ĵ�С�������ڵ��������Ϣ��
        //���ڴ��ַ��Ϣ
        ShmPosition    m_firstPage;         ///<��һҳ
        ShmPosition    m_lastPage;          ///<���һҳ
        ShmPosition    m_fIdlePage;         ///<��һ��δ��ҳ
        ShmPosition    m_lIdlePage;         ///<���һ��δ��ҳ
        //MDB2.0 add by gaojf 2010-5-11 16:37
        size_t         m_idleSlots;         ///<���м�¼��
        T_SCN          m_scn;               ///<SCN ��Ϣ
        UndoPosition   m_undopos;           ///<Ӱ��λ��
        //private: //��ʱ�޸� ��ֹ�޸Ĳ�ȫ
        volatile  int      m_lockinfo;          ///<LOCK����Ϣ
    public:
        time_t         m_locktime;          ///<LATCH������ʱ��
        char           m_desctype;          ///<���������
        size_t         m_pos;               ///<������λ��
    public:
        void  initPos();
        void  initPos(MemManager* r_memMgr, const bool& r_undoflag); //��ʼ�������Ϣ
        //����һ���µ�PAGE ע
        void  addNewPage(MemManager* r_memMgr, const int& r_descparlpos,
                         const ShmPosition& r_pagepos,
                         PageInfo* r_page, const bool& r_undoflag);
        //��PAGE����δ�������� (PAGE���Ƕ�����Ԫ��)
        void  addIdlePage(MemManager* r_memMgr,
                          const ShmPosition& r_pagepos,
                          PageInfo* r_ppage,
                          const bool& r_undoflag);
        void writeUndoInfo(MemManager* r_memMgr) throw(Mdb_Exception);
        void clearUndoInfo(MemManager* r_memMgr, const T_SCN& r_scn) throw(Mdb_Exception);
        void getundovalue(char* r_undovalue);
        void setundovalue(const char* r_undovalue);

        //��δ������������Slot
        // ��������� r_num, ���뵽������r_retnum, ���뵽�ĵ�ַr_slotAddrList
        // r_undoflag Desc�����Ƿ���ҪдUndo��Ϣ
        void  new_mem(MemManager* r_memMgr, const int& r_num, int& r_retnum,
                      vector<MdbAddress> &r_slotAddrList, const bool& r_undoflag) throw(Mdb_Exception);
        void  free_mem(MemManager* r_memMgr, const ShmPosition& r_slotaddr,
                       const ShmPosition& r_pagepos, PageInfo* r_pPage,
                       const bool& r_undoflag,
                       const T_SCN& r_scn) throw(Mdb_Exception);
        friend ostream& operator<<(ostream& r_os, const DescPageInfo& r_obj);
    public: //add by gaojf 2010/11/19 11:26:55
        bool lock(); //true �����ɹ�  false ����ʧ��
        void unlock(); //����
        bool islock();//true ����  false����

};

class DescPageInfos
{
    public:
        DescPageInfos()
        {
            m_lockinfo = 0;
        }
    public:
        void startmdb_init(); //����SCN��Ӱ��ָ����Ϣ
        //MDB2.0 add by gaojf 2010-5-28 10:45
        //����ʱ���շ����㷨��ȡĿ���б�ָ�� t_descpginfo
        //-1ȫæ��Χ��ȴ�   0�޿���SLOT������PAGE  1OK
        int getNewDescPageInfo(DescPageInfo* &t_descpginfo, int& r_pos);
        //���ӿ���SLOTʱ���շ����㷨��ȡĿ���б�ָ�� t_descpginfo
        //ȡ��æ�Ŀ���ֵ��С��һ��  falseȫæ  true OK
        bool getFreeDescPageInfo(PageInfo* r_ppage, DescPageInfo* &t_descpginfo);
        //��һ���յ�ҵ����б����ͷ�
        void freePageIdle(UndoMemMgr* r_undomgr, const size_t& r_pagepos, PageInfo* r_ppage) throw(Mdb_Exception);
        //��һ���յ�ҵ����б����ͷ�
        void freePage(UndoMemMgr* r_undomgr, const size_t& r_pagepos, PageInfo* r_ppage) throw(Mdb_Exception);
        void setdescinfo(const char& r_type, const size_t& r_offset);
        void clearUndoInfo(MemManager* r_memMgr, const T_SCN& r_scn) throw(Mdb_Exception);
        void initpageinfoscn(const T_SCN& r_scn);
    protected:
        void lock();
        void unlock();
        DescPageInfo* getFreeDescPageInfo(const int& r_pos);
    public:
        volatile  int      m_lockinfo;                    ///<LOCK����Ϣ
        time_t         m_locktime;                    ///<LATCH������ʱ��
        DescPageInfo   m_pageInfo[MDB_PARL_NUM];      ///<ҳ���б�
        T_SCN          m_scn;                         ///<SCN ��Ϣ
};
class TableDesc: public DescPageInfos //������Ϣ
{
    public:
        TableDef       m_tableDef;                    ///<��ṹ����
        int            m_indexNum;                    ///<������
        size_t         m_indexPosList[MAX_INDEX_NUM]; ///<���Index�б�
        T_NAMEDEF      m_indexNameList[MAX_INDEX_NUM];///<��������
        size_t         m_recordNum;                   ///<�ܼ�¼��
        int            m_fixSize;                     ///<�̶�����
        int            m_valueSize;                   ///<ֵ����
        int            m_slotSize;                    ///<Slot��С
    public:
        TableDesc()
        {
            m_recordNum = 0;
        }
        //virtual ~TableDesc(){}
    public:
        friend int operator<(const TableDesc& left, const TableDesc& right)
        {
            return (left.m_tableDef < right.m_tableDef);
        }

        friend int operator==(const TableDesc& left, const TableDesc& right)
        {
            return (left.m_tableDef == right.m_tableDef);
        }
        friend ostream& operator<<(ostream& os, const TableDesc& r_obj);
    public:
        //���ܣ�����ռ�ͱ���������
        bool addSpace(const char* r_spaceName, const T_SCN& r_scn);
        void getSpaceList(vector<string> &r_spaceList)
        {
            m_tableDef.getSpaceList(r_spaceList);
        }
        bool setSlotSize();
        bool addIndex(const size_t& r_indexPos, const char* r_indexName, const T_SCN& r_scn);
        bool deleteIndex(const size_t& r_indexPos, const T_SCN& r_scn);
        void initByTableDef(const TableDef& r_tableDef);
    public:
        void dumpInfo(ostream& r_os);
};

class IndexDesc: public DescPageInfos
{
    public:
        IndexDef       m_indexDef;
        size_t         m_tablePos;   ///<������λ��
        ShmPosition    m_header;     ///<����ͷ��hash����Ϊhashͷ��T-TreeΪ��
        int            m_fixSize;    ///<�̶�����
        int            m_valueSize;  ///<ֵ����
        int            m_slotSize;   ///<Slot��С
    public:
        friend int operator<(const IndexDesc& left, const IndexDesc& right)
        {
            return (left.m_indexDef < right.m_indexDef);
        }

        friend int operator==(const IndexDesc& left, const IndexDesc& right)
        {
            return (left.m_indexDef == right.m_indexDef);
        }
        friend ostream& operator<<(ostream& os, const IndexDesc& r_obj);
    public:
        IndexDesc()
        {
            m_lockinfo = 0;
        }
        //virtual ~IndexDesc(){}
        //ǰ�᣺��ռ��Ѿ�����
        //���ܣ�����ռ�ͱ���������
        bool addSpace(const char* r_spaceName, const T_SCN& r_scn);
        void getSpaceList(vector<string> &r_spaceList)
        {
            m_indexDef.getSpaceList(r_spaceList);
        }
        bool setSlotSize();
        void initByIndeDef(const IndexDef& r_indexDef);
    public:
        void dumpInfo(ostream& r_os);
};


#endif //_TABLEDESCRIPT_H_INCLUDE_20080411_


