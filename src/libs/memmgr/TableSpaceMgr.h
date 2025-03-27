#ifndef _TableSpaceMgr_H_INCLUDE_20080425
#define _TableSpaceMgr_H_INCLUDE_20080425
#include "SpaceMgrBase.h"
#include "PageInfo.h"
#include "DataPagesMgr.h"

class TableSpaceMgr: public SpaceMgrBase
{
    public:
        TableSpaceMgr(SpaceInfo& r_spaceInfo);
        virtual ~TableSpaceMgr();
    public:
        //���ݱ�ռ䶨����Ϣ,������ռ�
        bool createTbSpace(const int& r_flag);
        bool deleteTbSpace();
        //����ʽ��ʼ����ռ�
        bool initTbSpace(const int& r_flag, const char* r_path);
        //attach��ʽ��ʼ����ռ�
        bool attach_init();
        void afterStartMdb(); //startmdb �� ����scn��Ӱ��ָ�����Ϣ
        //����ҳ��
        bool allocatePage(MemManager* r_memMgr, PageInfo* &r_pPage,
                          ShmPosition& r_pagePos, const bool& r_undoflag);
        //���ձ�Space�ڵ�page
        bool freePage(MemManager* r_memMgr, const ShmPosition& r_pagePos, const bool& r_undoflag);
        //ͨ��Slotλ����ȡ��Ӧҳ��λ��
        bool getPagePosBySlotPos(const size_t& r_slotPos, size_t& r_pagePos);
        DataPagesMgr* getpagesmgr()
        {
            return &m_pagesmgr;
        }
    private:
        //��һ�γ�ʼ����ռ�
        bool f_initTbSpace();
    private:
        DataPagesMgr    m_pagesmgr; //PAGE�б�Ĺ���
    public:
        //���Խӿ�
        virtual bool dumpSpaceInfo(ostream& r_os);
        // �ڴ�ʹ���� add by chenm
        // Ĭ�Ϸ������б�ռ�ʹ����
        virtual float getTableSpaceUsedPercent();
};


#endif //_TableSpaceMgr_H_INCLUDE_20080425
