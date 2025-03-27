#ifndef _PAGEINFO_H_INCLUDE_20080425_
#define _PAGEINFO_H_INCLUDE_20080425_
#include "MdbConstDef.h"
#include "MdbConstDef2.h"
#include "MdbAddress.h"
#include "Mdb_Exception.h"

class MemManager;
class SpaceInfo;

//PAGEͷ�Ĳ���Ϣ mdb2.0 gaojf 2010-5-17 9:23
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
        unsigned int m_spaceCode;   //��ռ����
        size_t       m_pos;         //��pageλ����Ϣ
        int          m_parlpos;     //��Ӧ�б��
        int          m_descparlpos; //��Ӧ�б��
        char         m_useflag;     //�Ƿ��ѷ���:0 δ���䣬1 δ����2����
        char         m_desctype;    //���������ͣ�������
        size_t       m_descpos;     //������λ��(CHKPT��Ҫ�õ�)
        ShmPosition  m_next;        //�¸�ҳ��
        ShmPosition  m_nIdlePage;   //�¸�����ҳ��
        size_t       m_pageSize;
        int          m_slotSize;
        size_t       m_slotNum;
        size_t       m_idleNum;
        size_t       m_fIdleSlot;
        size_t       m_lIdleSlot;
        //MDB2.0 add by gaojf 2010-5-17 8:34
        Pageitl      m_itls[PAGE_ITLS_NUM]; //��
        T_SCN        m_scn_page;            //PAGE��ʱ���
        T_SCN        m_scn_slot;            //SLOT��ʱ���
        UndoPosition m_undopos;             //Ӱ����Ϣ
    protected:
        volatile int     m_lacthinfo;           //ҳ��ͷ��Ϣ�������Ӱ�Ӷ�Ӧ��
        time_t       m_locktime;          ///<LATCH������ʱ��
    public:
        void lock();
        void unlock();
        void* getFirstSlot(size_t& r_offset);
        void init(const unsigned int r_spacecode, const size_t r_pagepos,
                  const int r_parlpos, const size_t r_pagesize);
        //����r_num��Ԫ�أ�r_slotAddrList��ʼ�����ⲿ���ô�ֻpush_back
        void allocateNode(const int& r_num, vector<MdbAddress>& r_slotAddrList, int& r_newNum);
        //��ʼ��
        void initSlotList(const int& r_slotSize, const T_SCN& r_scn);
        //�ͷ�Slot
        void freeNode(const size_t& r_slotPos, const T_SCN& r_scn);
        //add by gaojf 2010-5-19 10:25
        void writeUndoInfo(MemManager* r_memMgr) throw(Mdb_Exception); //дӰ����Ϣ,��������ʱ�����Ӱ��λ����Ϣ
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
        T_SCN          m_scn;               ///<SCN ��Ϣ
        UndoPosition   m_undopos;           ///<Ӱ��λ��
    public:
        void  init(const size_t r_pos, const size_t r_pagesize);
        void  addpage(char* r_spaceaddr, const unsigned int& r_spaceCode, const size_t r_pos);
        //add by gaojf 2010-5-17 8:34
        //����һ��ҳ��  true OK��false δ���뵽
        bool  new_page(MemManager* r_memMgr,
                       char*      r_spaceaddr,
                       const unsigned int r_spacecode,
                       size_t&    r_pagepos,
                       PageInfo* &r_ppage,
                       const bool& r_undoflag);
        //�ͷ�һ��ҳ�� true OK��false ʧ��
        bool  free_page(MemManager* r_memMgr,
                        char* r_spaceaddr,
                        const unsigned int r_spacecode,
                        const size_t& r_pagepos,
                        const bool& r_undoflag) ;
        void getundovalue(char* r_undovalue);
        void setundovalue(const char* r_undovalue);
    protected:
        //дӰ����Ϣ,��������ʱ�����Ӱ��λ����Ϣ
        void writeUndoInfo(const unsigned int r_spacecode,
                           MemManager* r_memMgr) throw(Mdb_Exception);
};

#endif //_PAGEINFO_H_INCLUDE_20080425_
