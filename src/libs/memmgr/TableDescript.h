#ifndef _TABLEDESCRIPT_H_INCLUDE_20080411_
#define _TABLEDESCRIPT_H_INCLUDE_20080411_

#include "TableDefParam.h"
#include "MdbConstDef2.h"  //add by gaojf 2010-5-14 16:58
#include "Mdb_Exception.h"
#include "MdbAddress.h"

class MemManager;
class UndoMemMgr;
class PageInfo;
//DescPageInfo 的更新，在DescPageInfos中加锁
class DescPageInfo
{
    public:
        int            m_slotSize;          ///<每个节点的大小（包含节点的所有信息）
        //表内存地址信息
        ShmPosition    m_firstPage;         ///<第一页
        ShmPosition    m_lastPage;          ///<最后一页
        ShmPosition    m_fIdlePage;         ///<第一个未满页
        ShmPosition    m_lIdlePage;         ///<最后一个未满页
        //MDB2.0 add by gaojf 2010-5-11 16:37
        size_t         m_idleSlots;         ///<空闲记录数
        T_SCN          m_scn;               ///<SCN 信息
        UndoPosition   m_undopos;           ///<影子位置
        //private: //临时修改 防止修改不全
        volatile  int      m_lockinfo;          ///<LOCK锁信息
    public:
        time_t         m_locktime;          ///<LATCH锁心跳时间
        char           m_desctype;          ///<表或者索引
        size_t         m_pos;               ///<描述符位置
    public:
        void  initPos();
        void  initPos(MemManager* r_memMgr, const bool& r_undoflag); //初始化类表信息
        //增加一个新的PAGE 注
        void  addNewPage(MemManager* r_memMgr, const int& r_descparlpos,
                         const ShmPosition& r_pagepos,
                         PageInfo* r_page, const bool& r_undoflag);
        //将PAGE加入未满队列中 (PAGE已是队列中元素)
        void  addIdlePage(MemManager* r_memMgr,
                          const ShmPosition& r_pagepos,
                          PageInfo* r_ppage,
                          const bool& r_undoflag);
        void writeUndoInfo(MemManager* r_memMgr) throw(Mdb_Exception);
        void clearUndoInfo(MemManager* r_memMgr, const T_SCN& r_scn) throw(Mdb_Exception);
        void getundovalue(char* r_undovalue);
        void setundovalue(const char* r_undovalue);

        //从未满队列中申请Slot
        // 申请的数量 r_num, 申请到的数量r_retnum, 申请到的地址r_slotAddrList
        // r_undoflag Desc本身是否需要写Undo信息
        void  new_mem(MemManager* r_memMgr, const int& r_num, int& r_retnum,
                      vector<MdbAddress> &r_slotAddrList, const bool& r_undoflag) throw(Mdb_Exception);
        void  free_mem(MemManager* r_memMgr, const ShmPosition& r_slotaddr,
                       const ShmPosition& r_pagepos, PageInfo* r_pPage,
                       const bool& r_undoflag,
                       const T_SCN& r_scn) throw(Mdb_Exception);
        friend ostream& operator<<(ostream& r_os, const DescPageInfo& r_obj);
    public: //add by gaojf 2010/11/19 11:26:55
        bool lock(); //true 锁定成功  false 锁定失败
        void unlock(); //解锁
        bool islock();//true 有锁  false无锁

};

class DescPageInfos
{
    public:
        DescPageInfos()
        {
            m_lockinfo = 0;
        }
    public:
        void startmdb_init(); //清理SCN和影子指针信息
        //MDB2.0 add by gaojf 2010-5-28 10:45
        //申请时按照分配算法：取目标列表指针 t_descpginfo
        //-1全忙外围需等待   0无空闲SLOT需申请PAGE  1OK
        int getNewDescPageInfo(DescPageInfo* &t_descpginfo, int& r_pos);
        //增加空闲SLOT时按照分配算法：取目标列表指针 t_descpginfo
        //取非忙的空闲值最小的一个  false全忙  true OK
        bool getFreeDescPageInfo(PageInfo* r_ppage, DescPageInfo* &t_descpginfo);
        //将一个空的业务从列表中释放
        void freePageIdle(UndoMemMgr* r_undomgr, const size_t& r_pagepos, PageInfo* r_ppage) throw(Mdb_Exception);
        //将一个空的业务从列表中释放
        void freePage(UndoMemMgr* r_undomgr, const size_t& r_pagepos, PageInfo* r_ppage) throw(Mdb_Exception);
        void setdescinfo(const char& r_type, const size_t& r_offset);
        void clearUndoInfo(MemManager* r_memMgr, const T_SCN& r_scn) throw(Mdb_Exception);
        void initpageinfoscn(const T_SCN& r_scn);
    protected:
        void lock();
        void unlock();
        DescPageInfo* getFreeDescPageInfo(const int& r_pos);
    public:
        volatile  int      m_lockinfo;                    ///<LOCK锁信息
        time_t         m_locktime;                    ///<LATCH锁心跳时间
        DescPageInfo   m_pageInfo[MDB_PARL_NUM];      ///<页面列表集
        T_SCN          m_scn;                         ///<SCN 信息
};
class TableDesc: public DescPageInfos //表定义信息
{
    public:
        TableDef       m_tableDef;                    ///<表结构定义
        int            m_indexNum;                    ///<索引数
        size_t         m_indexPosList[MAX_INDEX_NUM]; ///<组成Index列表
        T_NAMEDEF      m_indexNameList[MAX_INDEX_NUM];///<索引名称
        size_t         m_recordNum;                   ///<总记录数
        int            m_fixSize;                     ///<固定长度
        int            m_valueSize;                   ///<值长度
        int            m_slotSize;                    ///<Slot大小
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
        //功能：将表空间和表描述符绑定
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
        size_t         m_tablePos;   ///<归属表位置
        ShmPosition    m_header;     ///<索引头：hash索引为hash头，T-Tree为根
        int            m_fixSize;    ///<固定长度
        int            m_valueSize;  ///<值长度
        int            m_slotSize;   ///<Slot大小
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
        //前提：表空间已经创建
        //功能：将表空间和表描述符绑定
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


