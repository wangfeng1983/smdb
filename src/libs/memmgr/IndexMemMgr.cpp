#include "IndexMemMgr.h"
#include "TableDescript.h"

IndexMemMgr::IndexMemMgr()
{
}
IndexMemMgr::~IndexMemMgr()
{
}

bool IndexMemMgr::initialize(SpaceInfo& r_spaceInfo)
{
    SpaceMgrBase::initialize(r_spaceInfo);
    return true;
}
//根据表空间定义信息,创建表空间
bool IndexMemMgr::createIdxSpace(SpaceInfo& r_spaceInfo, const int& r_flag)
{
    if (createSpace(m_spaceHeader) == false)
    {
#ifdef _DEBUG_
        cout << "createSpace false!" << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    r_spaceInfo = *(getSpaceInfo());
    return true;
}
bool IndexMemMgr::deleteIdxSpace()
{
    return deleteSpace(m_spaceHeader);
}
//初始化表空间
bool IndexMemMgr::initSpaceInfo(const int& r_flag, const char* r_path)
{
    if (r_flag == 0)
    {
        return f_init();
    }
    //否则从文件中读取
    if (loadDataFromFile(r_path) == false)
    {
#ifdef _DEBUG
        cout << "loadDataFromFile() false!" << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    size_t  t_offSet = 0;
    //1. 跳过块头信息m_spaceHeader
    t_offSet += sizeof(SpaceInfo);
    //2. 初始化索引段管理对象
    m_indexSegsMgr.initialize(m_spaceHeader.m_shmAddr, t_offSet, m_spaceHeader.m_size - t_offSet, 0);
    return true;
}

bool IndexMemMgr::f_init()
{
    if (m_spaceHeader.m_size < sizeof(SpaceInfo) + sizeof(IndexSegInfo) + sizeof(HashIndexSeg))
    {
#ifdef _DEBUG_
        cout << "索引表空间数值太小:" << m_spaceHeader.m_size
             << " " << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    //初始化为一个大段
    size_t  t_offSet = 0;
    //1. 初始化块头信息m_spaceHeader
    memcpy(m_pSpHeader, &m_spaceHeader, sizeof(SpaceInfo));
    t_offSet += sizeof(SpaceInfo);
    //2.设置段列表信息
    m_indexSegsMgr.initialize(m_spaceHeader.m_shmAddr, t_offSet, m_spaceHeader.m_size - t_offSet, 1);
    return true;
}
//attach方式init
bool IndexMemMgr::attach_init(SpaceInfo& r_spaceInfo)
{
    initialize(r_spaceInfo);
    if (SpaceMgrBase::attach() == false)
    {
        return false;
    }
    size_t  t_offSet = 0;
    //1. 跳过块头信息m_spaceHeader
    t_offSet += sizeof(SpaceInfo);
    //2. 初始化索引段管理对象
    m_indexSegsMgr.initialize(m_spaceHeader.m_shmAddr, t_offSet, m_spaceHeader.m_size - t_offSet, 0);
    r_spaceInfo = m_spaceHeader;
    return true;
}

//r_shmPos是指向Hash中ShmPositioin[]的首地址
bool IndexMemMgr::createHashIndex(IndexDesc* r_idxDesc, ShmPosition& r_shmPos, const T_SCN& r_scn)
{
    bool   t_bRet = true;
    size_t t_offset = 0;
    SpaceMgrBase::lock();
    t_bRet = m_indexSegsMgr.createHashIndex(r_idxDesc, t_offset, r_scn);
    r_shmPos.setValue(m_spaceHeader.m_spaceCode, t_offset);
    SpaceMgrBase::unlock();
    return t_bRet;
}
//r_shmPos是指向Hash中ShmPositioin[]的首地址
void IndexMemMgr::initHashSeg(const ShmPosition& r_shmPos, const T_SCN& r_scn)
{
    m_indexSegsMgr.initHashSeg(r_shmPos.getOffSet(), r_scn);
}

//r_shmPos是指向Hash中ShmPositioin[]的首地址
bool IndexMemMgr::dropHashIdex(const ShmPosition& r_shmPos, const T_SCN& r_scn)
{
    bool t_bRet = true;
    SpaceMgrBase::lock();
    t_bRet = m_indexSegsMgr.dropHashIdex(r_shmPos.getOffSet(), r_scn);
    SpaceMgrBase::unlock();
    return t_bRet;
}
bool IndexMemMgr::dumpSpaceInfo(ostream& r_os)
{
    r_os << "---------索引表空间:" << m_pSpHeader->m_spaceName << "内容 起始!---------" << endl;
    SpaceMgrBase::dumpSpaceInfo(r_os);
    m_indexSegsMgr.dumpInfo(r_os);
    r_os << "---------索引表空间:" << m_pSpHeader->m_spaceName << "内容 终止!---------" << endl;
    return true;
}

void IndexMemMgr::getTableSpaceUsedPercent(map<string, float> &vUserdPercent, const char* cTableSpaceName)
{
    HashIndexSeg* pHashIndexSeg;
    size_t totalFreeSegSize = 0;
    float fPercent;
    totalFreeSegSize = m_indexSegsMgr.getUsedSize();
    fPercent = 1 - (float)totalFreeSegSize / m_spaceHeader.m_size;
    vUserdPercent.insert(map<string, float>::value_type(m_spaceHeader.m_spaceName, fPercent));
    return;
}
IndexSegInfo* IndexMemMgr::getSegInfos()
{
    return m_indexSegsMgr.getSegInfos();
}
//startmdb后，清理scn和undo指针
void IndexMemMgr::afterStartMdb()
{
    m_indexSegsMgr.startmdb_init();
}
