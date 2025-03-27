#ifndef _PAGEINFO_H_INCLUDE_20080425_
#define _PAGEINFO_H_INCLUDE_20080425_
#include "MdbConstDef.h"
#include "MdbConstDef2.h"
#include "MdbAddress.h"
#include "Mdb_Exception.h"

class MemManager;
class SpaceInfo;

//PAGE头的槽信息 mdb2.0 gaojf 2010-5-17 9:23
class Pageitl
{
    public:
        int     m_sid;
        size_t  m_tid;
        char    m_state;
        T_SCN   m_scn;
};
class PageInfo
{
    public:
        unsigned int m_spaceCode;   //表空间代码
        size_t       m_pos;         //该page位置信息
        int          m_parlpos;     //对应列表号
        int          m_descparlpos; //对应列表号
        char         m_useflag;     //是否已分配:0 未分配，1 未满，2已满
        char         m_desctype;    //描述符类型：表、索引
        size_t       m_descpos;     //描述符位置(CHKPT需要用到)
        ShmPosition  m_next;        //下个页面
        ShmPosition  m_nIdlePage;   //下个空闲页面
        size_t       m_pageSize;
        int          m_slotSize;
        size_t       m_slotNum;
        size_t       m_idleNum;
        size_t       m_fIdleSlot;
        size_t       m_lIdleSlot;
        //MDB2.0 add by gaojf 2010-5-17 8:34
        Pageitl      m_itls[PAGE_ITLS_NUM]; //槽
        T_SCN        m_scn_page;            //PAGE的时间戳
        T_SCN        m_scn_slot;            //SLOT的时间戳
        UndoPosition m_undopos;             //影子信息
    protected:
        volatile int     m_lacthinfo;           //页面头信息相关锁（影子对应）
        time_t       m_locktime;          ///<LATCH锁心跳时间
    public:
        void lock();
        void unlock();
        void* getFirstSlot(size_t& r_offset);
        void init(const unsigned int r_spacecode, const size_t r_pagepos,
                  const int r_parlpos, const size_t r_pagesize);
        //申请r_num个元素，r_slotAddrList初始化在外部，该处只push_back
        void allocateNode(const int& r_num, vector<MdbAddress>& r_slotAddrList, int& r_newNum);
        //初始化
        void initSlotList(const int& r_slotSize, const T_SCN& r_scn);
        //释放Slot
        void freeNode(const size_t& r_slotPos, const T_SCN& r_scn);
        //add by gaojf 2010-5-19 10:25
        void writeUndoInfo(MemManager* r_memMgr) throw(Mdb_Exception); //写影子信息,包括更新时间戳和影子位置信息
        void getundovalue(char* r_undovalue);
        void setundovalue(const char* r_undovalue);
};


class PageListInfo
{
    public:
        size_t         m_pos;
        size_t         m_pageSize;
        size_t         m_pageNum;
        size_t         m_idleNum;
        size_t         m_fIdlePage;
        size_t         m_lIdlePage;

        //MDB2.0 add by gaojf 2010-5-17 8:34
        T_SCN          m_scn;               ///<SCN 信息
        UndoPosition   m_undopos;           ///<影子位置
    public:
        void  init(const size_t r_pos, const size_t r_pagesize);
        void  addpage(char* r_spaceaddr, const unsigned int& r_spaceCode, const size_t r_pos);
        //add by gaojf 2010-5-17 8:34
        //申请一个页面  true OK，false 未申请到
        bool  new_page(MemManager* r_memMgr,
                       char*      r_spaceaddr,
                       const unsigned int r_spacecode,
                       size_t&    r_pagepos,
                       PageInfo* &r_ppage,
                       const bool& r_undoflag);
        //释放一个页面 true OK，false 失败
        bool  free_page(MemManager* r_memMgr,
                        char* r_spaceaddr,
                        const unsigned int r_spacecode,
                        const size_t& r_pagepos,
                        const bool& r_undoflag) ;
        void getundovalue(char* r_undovalue);
        void setundovalue(const char* r_undovalue);
    protected:
        //写影子信息,包括更新时间戳和影子位置信息
        void writeUndoInfo(const unsigned int r_spacecode,
                           MemManager* r_memMgr) throw(Mdb_Exception);
};

#endif //_PAGEINFO_H_INCLUDE_20080425_
