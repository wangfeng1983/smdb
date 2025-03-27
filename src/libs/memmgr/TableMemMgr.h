#ifndef _TABLEMEMMGR_H_INCLUDE_20080425_
#define _TABLEMEMMGR_H_INCLUDE_20080425_
#include "TableSpaceMgr.h"
#include <pthread.h>

class TableMemMgr
{
    public:
        friend class ChkptMgr;
        TableMemMgr();
        virtual ~TableMemMgr();
    public:
        //���ݱ�ռ䶨����Ϣ,������ռ�
        bool createTbSpace(SpaceInfo& r_spaceInfo, const int& r_flag);
        bool attachAllTbSpace();
        bool detachAllTbSpace();
        bool deleteAllTbSpace();
        bool attach_int(SpaceInfo& r_spaceInfo);
        void afterStartMdb();//startmdb �� ����scn��Ӱ��ָ�����Ϣ
        //��ʼ����ռ�
        //r_status: -1 ��ʼ(һ��δ��); 0��checkpoint����; 1��stop database ����  add by gaojf 2012/5/8 14:26:25
        bool initSpaceInfo(const int& r_flag, const char* r_path, const int& r_status);
        bool dumpDataIntoFile(const char* r_path);
        //����һ����ռ�Ĺ���
        bool addSpaceMgr(TableSpaceMgr* t_tbSpaceMgr)
        {
            m_tableSpaceMgrList.push_back(t_tbSpaceMgr);
            return true;
        }
        void getSpaceInfoList(vector<SpaceInfo> &r_spaceInofList)
        {
            r_spaceInofList.clear();
            for (vector<TableSpaceMgr*>::iterator r_itr = m_tableSpaceMgrList.begin();
                    r_itr != m_tableSpaceMgrList.end(); r_itr++)
            {
                r_spaceInofList.push_back(*((*r_itr)->getSpaceInfo()));
            }
        }
        // add by chenm 2010-09-10
        void getSpaceMgr(vector<SpaceMgrBase*> &rSpaceMgrBases)
        {
            for (vector<TableSpaceMgr*>::iterator r_itr = m_tableSpaceMgrList.begin();
                    r_itr != m_tableSpaceMgrList.end(); ++r_itr)
            {
                rSpaceMgrBases.push_back(*r_itr);
            }
        }
        // over 2010-09-10
    public:
        //����ҳ��
        bool allocatePage(MemManager* r_memMgr, const char* r_spaceName, PageInfo* &r_pPage,
                          ShmPosition& r_pagePos, const bool& r_undoflag);
        //�ͷ�ҳ��
        bool freePage(MemManager* r_memMgr, const ShmPosition& r_pagePos, const bool& r_undoflag);
        //����Slot��ַ��ȡ��Ӧ��ҳ����Ϣ��ҳ���ַ
        bool getPageBySlot(const ShmPosition& r_slot, ShmPosition& r_pagePos, PageInfo* &r_pPage);
    protected:
        typedef struct
        {
            pthread_t     m_threadId;	    ///< �߳�ID.
            bool          m_bRet;		      ///< ����ֵ.
            T_FILENAMEDEF m_path;         ///< Ŀ¼
            TableSpaceMgr* m_pSpaceMgr;   ///< ��ռ�������ָ��
        } ThreadParam;
        static void* dumpDataInfoFile_r(ThreadParam& r_param);
    protected:
        vector<TableSpaceMgr*> m_tableSpaceMgrList;
        vector<TableSpaceMgr*>::iterator m_tbMgrItr;
    public:
        //���Խӿ�
        virtual bool dumpSpaceInfo(ostream& r_os);
        // �ڴ�ʹ���� add by chenm
        // Ĭ�Ϸ������б�ռ�ʹ����
        void getTableSpaceUsedPercent(map<string, float> &vUserdPercent, const char* cTableSpaceName);
};

#endif //_TABLEMEMMGR_H_INCLUDE_20080425_
