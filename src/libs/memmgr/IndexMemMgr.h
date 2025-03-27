#ifndef _INDEXMEMMGR_H_INCLUDE_20080425_
#define _INDEXMEMMGR_H_INCLUDE_20080425_
#include "SpaceMgrBase.h"
#include "MdbConstDef2.h"
#include "IndexSegsMgr.h"

class IndexMemMgr: public SpaceMgrBase
{
    public:
        IndexMemMgr();
        virtual ~IndexMemMgr();
    public:
        bool initialize(SpaceInfo& r_spaceInfo);
        //���ݱ�ռ䶨����Ϣ,������ռ�
        bool createIdxSpace(SpaceInfo& r_spaceInfo, const int& r_flag);
        bool deleteIdxSpace();
        //��ʼ����ռ�
        bool initSpaceInfo(const int& r_flag, const char* r_path);
        //attach��ʽinit
        bool attach_init(SpaceInfo& r_spaceInfo);
        void afterStartMdb(); //startmdb������scn��undoָ��
    public:
        //r_shmPos��ָ��Hash��ShmPositioin[]���׵�ַ
        bool createHashIndex(IndexDesc* r_idxDesc, ShmPosition& r_shmPos, const T_SCN& r_scn);
        //r_shmPos��ָ��Hash��ShmPositioin[]���׵�ַ
        bool dropHashIdex(const ShmPosition& r_shmPos, const T_SCN& r_scn);
        //r_shmPos��ָ��Hash��ShmPositioin[]���׵�ַ
        void initHashSeg(const ShmPosition& r_shmPos, const T_SCN& r_scn);
        IndexSegInfo* getSegInfos();
    protected:
        bool f_init();
    private:
        IndexSegsMgr  m_indexSegsMgr;
    public:
        //���Խӿ�
        virtual bool dumpSpaceInfo(ostream& r_os);
        virtual void getTableSpaceUsedPercent(map<string, float> &vUserdPercent, const char* cTableSpaceName);
};

#endif //_INDEXMEMMGR_H_INCLUDE_20080425_
