#ifndef _DATAPAGESMGR_H_INCLUDE_20100527
#define _DATAPAGESMGR_H_INCLUDE_20100527

#include "PageInfo.h"

class SpaceMgrBase;
class DataPagesMgr
{
    public:
        DataPagesMgr();
        virtual ~DataPagesMgr();
    public:
        //r_spacemgr  对应表空间管理对象
        //r_spaceaddr 表空间起始地址
        //r_offset    pages段起始偏移量
        //r_segsize   段的总大小  (包含段列表)
        //r_pagesize  每个PAGE大小
        //r_flag  1 第一次初始化，需要初始化段列表信息
        bool initialize(SpaceMgrBase* r_spacemgr,
                        char* r_spaceaddr, const size_t& r_offset,
                        const size_t& r_segsize, const size_t& r_pagesize,
                        const int r_flag = 0);
        void startmdb_init();
        //申请页面
        bool allocatePage(MemManager* r_memMgr, PageInfo* &r_pPage ,
                          size_t& r_pagePos, const bool& r_undoflag);
        //回收本Space内的page
        bool freePage(MemManager* r_memMgr, const size_t& r_pagePos, const bool& r_undoflag);
        //通过Slot位置求取对应页面位置
        bool getPagePosBySlotPos(const size_t& r_slotPos, size_t& r_pagePos);

        PageListInfo* getpagelistinfo(const int r_i)
        {
            return m_pagelist[r_i];
        }
    private:
        //第一次初始化表空间
        bool f_initTbSpace();
    public:
        size_t          m_firstpagepos;
    private:
        PageListInfo*   m_pagelist[MDB_PARL_NUM]; //每个list的管理
        char*           m_spaceaddr;              //表空间起始地址
        SpaceMgrBase*   m_spacemgr;               //该页面信息所在的段管理器
        size_t          m_pagesize;               //每个页面大小
        size_t          m_segsize;                //页面段大小(只是页面部分)
        unsigned int    m_spaceCode;              //表空间编码
    public:
        //调试接口
        virtual bool dumpPagesInfo(ostream& r_os);
        // 内存使用率 add by chenm
        // 默认返回所有表空间使用率
        virtual bool getPagesUseInfo(size_t& r_totalsize, size_t& r_freesize);
};


#endif //_DATAPAGESMGR_H_INCLUDE_20100527
