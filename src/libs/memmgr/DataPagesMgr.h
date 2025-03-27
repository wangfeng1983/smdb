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
        //r_spacemgr  ��Ӧ��ռ�������
        //r_spaceaddr ��ռ���ʼ��ַ
        //r_offset    pages����ʼƫ����
        //r_segsize   �ε��ܴ�С  (�������б�)
        //r_pagesize  ÿ��PAGE��С
        //r_flag  1 ��һ�γ�ʼ������Ҫ��ʼ�����б���Ϣ
        bool initialize(SpaceMgrBase* r_spacemgr,
                        char* r_spaceaddr, const size_t& r_offset,
                        const size_t& r_segsize, const size_t& r_pagesize,
                        const int r_flag = 0);
        void startmdb_init();
        //����ҳ��
        bool allocatePage(MemManager* r_memMgr, PageInfo* &r_pPage ,
                          size_t& r_pagePos, const bool& r_undoflag);
        //���ձ�Space�ڵ�page
        bool freePage(MemManager* r_memMgr, const size_t& r_pagePos, const bool& r_undoflag);
        //ͨ��Slotλ����ȡ��Ӧҳ��λ��
        bool getPagePosBySlotPos(const size_t& r_slotPos, size_t& r_pagePos);

        PageListInfo* getpagelistinfo(const int r_i)
        {
            return m_pagelist[r_i];
        }
    private:
        //��һ�γ�ʼ����ռ�
        bool f_initTbSpace();
    public:
        size_t          m_firstpagepos;
    private:
        PageListInfo*   m_pagelist[MDB_PARL_NUM]; //ÿ��list�Ĺ���
        char*           m_spaceaddr;              //��ռ���ʼ��ַ
        SpaceMgrBase*   m_spacemgr;               //��ҳ����Ϣ���ڵĶι�����
        size_t          m_pagesize;               //ÿ��ҳ���С
        size_t          m_segsize;                //ҳ��δ�С(ֻ��ҳ�沿��)
        unsigned int    m_spaceCode;              //��ռ����
    public:
        //���Խӿ�
        virtual bool dumpPagesInfo(ostream& r_os);
        // �ڴ�ʹ���� add by chenm
        // Ĭ�Ϸ������б�ռ�ʹ����
        virtual bool getPagesUseInfo(size_t& r_totalsize, size_t& r_freesize);
};


#endif //_DATAPAGESMGR_H_INCLUDE_20100527
