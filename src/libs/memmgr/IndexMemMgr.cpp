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
//���ݱ�ռ䶨����Ϣ,������ռ�
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
//��ʼ����ռ�
bool IndexMemMgr::initSpaceInfo(const int& r_flag, const char* r_path)
{
    if (r_flag == 0)
    {
        return f_init();
    }
    //������ļ��ж�ȡ
    if (loadDataFromFile(r_path) == false)
    {
#ifdef _DEBUG
        cout << "loadDataFromFile() false!" << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    size_t  t_offSet = 0;
    //1. ������ͷ��Ϣm_spaceHeader
    t_offSet += sizeof(SpaceInfo);
    //2. ��ʼ�������ι������
    m_indexSegsMgr.initialize(m_spaceHeader.m_shmAddr, t_offSet, m_spaceHeader.m_size - t_offSet, 0);
    return true;
}

bool IndexMemMgr::f_init()
{
    if (m_spaceHeader.m_size < sizeof(SpaceInfo) + sizeof(IndexSegInfo) + sizeof(HashIndexSeg))
    {
#ifdef _DEBUG_
        cout << "������ռ���ֵ̫С:" << m_spaceHeader.m_size
             << " " << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    //��ʼ��Ϊһ�����
    size_t  t_offSet = 0;
    //1. ��ʼ����ͷ��Ϣm_spaceHeader
    memcpy(m_pSpHeader, &m_spaceHeader, sizeof(SpaceInfo));
    t_offSet += sizeof(SpaceInfo);
    //2.���ö��б���Ϣ
    m_indexSegsMgr.initialize(m_spaceHeader.m_shmAddr, t_offSet, m_spaceHeader.m_size - t_offSet, 1);
    return true;
}
//attach��ʽinit
bool IndexMemMgr::attach_init(SpaceInfo& r_spaceInfo)
{
    initialize(r_spaceInfo);
    if (SpaceMgrBase::attach() == false)
    {
        return false;
    }
    size_t  t_offSet = 0;
    //1. ������ͷ��Ϣm_spaceHeader
    t_offSet += sizeof(SpaceInfo);
    //2. ��ʼ�������ι������
    m_indexSegsMgr.initialize(m_spaceHeader.m_shmAddr, t_offSet, m_spaceHeader.m_size - t_offSet, 0);
    r_spaceInfo = m_spaceHeader;
    return true;
}

//r_shmPos��ָ��Hash��ShmPositioin[]���׵�ַ
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
//r_shmPos��ָ��Hash��ShmPositioin[]���׵�ַ
void IndexMemMgr::initHashSeg(const ShmPosition& r_shmPos, const T_SCN& r_scn)
{
    m_indexSegsMgr.initHashSeg(r_shmPos.getOffSet(), r_scn);
}

//r_shmPos��ָ��Hash��ShmPositioin[]���׵�ַ
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
    r_os << "---------������ռ�:" << m_pSpHeader->m_spaceName << "���� ��ʼ!---------" << endl;
    SpaceMgrBase::dumpSpaceInfo(r_os);
    m_indexSegsMgr.dumpInfo(r_os);
    r_os << "---------������ռ�:" << m_pSpHeader->m_spaceName << "���� ��ֹ!---------" << endl;
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
//startmdb������scn��undoָ��
void IndexMemMgr::afterStartMdb()
{
    m_indexSegsMgr.startmdb_init();
}
