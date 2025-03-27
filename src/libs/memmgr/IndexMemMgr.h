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
        //根据表空间定义信息,创建表空间
        bool createIdxSpace(SpaceInfo& r_spaceInfo, const int& r_flag);
        bool deleteIdxSpace();
        //初始化表空间
        bool initSpaceInfo(const int& r_flag, const char* r_path);
        //attach方式init
        bool attach_init(SpaceInfo& r_spaceInfo);
        void afterStartMdb(); //startmdb后，清理scn和undo指针
    public:
        //r_shmPos是指向Hash中ShmPositioin[]的首地址
        bool createHashIndex(IndexDesc* r_idxDesc, ShmPosition& r_shmPos, const T_SCN& r_scn);
        //r_shmPos是指向Hash中ShmPositioin[]的首地址
        bool dropHashIdex(const ShmPosition& r_shmPos, const T_SCN& r_scn);
        //r_shmPos是指向Hash中ShmPositioin[]的首地址
        void initHashSeg(const ShmPosition& r_shmPos, const T_SCN& r_scn);
        IndexSegInfo* getSegInfos();
    protected:
        bool f_init();
    private:
        IndexSegsMgr  m_indexSegsMgr;
    public:
        //调试接口
        virtual bool dumpSpaceInfo(ostream& r_os);
        virtual void getTableSpaceUsedPercent(map<string, float> &vUserdPercent, const char* cTableSpaceName);
};

#endif //_INDEXMEMMGR_H_INCLUDE_20080425_
