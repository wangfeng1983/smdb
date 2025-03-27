#ifndef _TBMEMINFO_H_INCLUDE_20080507_
#define _TBMEMINFO_H_INCLUDE_20080507_
#include "MdbConstDef.h"

class PageInfo;
class TbPageInfo
{
    public:
        ShmPosition m_pagePos;
        size_t      m_pageSize;
        size_t      m_slotNum;
        size_t      m_idleNum;
};

class TbMemInfo
{
    public:
        T_NAMEDEF           m_tbName;
        T_TABLETYPE         m_tbType;
        int                 m_slotSize;
        size_t              m_pageNum;
        vector<TbPageInfo>  m_tbPagesInfo;
        vector<ShmPosition> m_notFullPages;
        size_t              m_totalSlotNum;
        size_t              m_idleSlotNum;
        size_t              m_usedSlotNum;
    public:
        void clear();
        void setPageInfo(const PageInfo* r_pPage);
    public:
        //µ÷ÊÔ½Ó¿Ú
        virtual bool dumpInfo(ostream& r_os);
};

#endif //_TBMEMINFO_H_INCLUDE_20080507_
