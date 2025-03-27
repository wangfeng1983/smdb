#include <sys/errno.h>
#include <errno.h>
#include "SpaceMgrBase.h"
#include "InstanceFactory.h"
#include "MdbAddress.h"
/**
 *initialize ��ʼ��.
 *@param   r_SpaceInfo:  ��ռ䶨�����
 *@return  true �ɹ�,false ʧ��
 */
bool  SpaceMgrBase::initialize(const SpaceInfo&  r_SpaceInfo)
{
    memcpy(&m_spaceHeader, &r_SpaceInfo, sizeof(SpaceInfo));
    if (m_file.initialize(m_spaceHeader.m_fileName, m_spaceHeader.m_size) == false)
    {
#ifdef _DEBUG_
        cout << "spaceFile init false!" << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    if (m_pdbLock == NULL)
    {
        try
        {
            m_pdbLock = MDBLatchMgr::getInstance(m_spaceHeader.m_dbName);
            for (int i = 0; i < MDB_PARL_NUM; ++i)
            {
                m_srclatchinfo[i] = m_pdbLock->getSrcLatchInfo(T_DBLK_SPACE + i,
                                    m_spaceHeader.m_spaceName);
            }
        }
        catch (Mdb_Exception& e)
        {
#ifdef _DEBUG_
            cout << "m_pdbLock open failed!" << __FILE__ << __LINE__ << endl;
#endif
            return false;
        }
    }
    return true;
}

/**
 *createSpace ������ռ�.
 *@param   r_SpaceInfo :  ��ռ������Ϣ
 *@param   r_diskFile  :  �����ļ���
 *@param   r_spaceSize :  ��ռ��С
 *@param   r_spaceType :  ��ռ����ͣ�����������������������
 *@param   r_shmKey    :  ��ռ��ShmKey
 *@param   r_shmId     :  ��ռ�ShmId
 *@return  true �����ɹ�,false ʧ��
 */
bool SpaceMgrBase::createSpace(SpaceInfo&  r_SpaceInfo)
{
    //1. �����ñ�ռ��Ӧ�Ŀ����ļ�
    //   $MDB_HOME/ctl/dbname.ctl
    T_FILENAMEDEF  t_ctlFileName;
    sprintf(t_ctlFileName, "%s/ctl/%s.ctl", getenv(MDB_HOMEPATH.c_str()), r_SpaceInfo.m_dbName);
    //2. ���ݸ��ļ����ɶ�Ӧ��
    //�ж��ļ��Ƿ���ڣ��������򴴽�r_spaceSize��С��ռ�ռ䣩���ļ�
    //�ж��ļ���С�Ƿ��r_spaceSizeһ��,��һ��,����
    //��ȡr_shmKey
    r_SpaceInfo.m_shmKey = ftok(t_ctlFileName, r_SpaceInfo.m_spaceCode);
    if (r_SpaceInfo.m_shmKey < 0)
    {
#ifdef _DEBUG_
        cout << "ftok(" << t_ctlFileName << ",";
        cout << r_SpaceInfo.m_spaceCode << ") false!" << endl;
#endif
        return false;
    }
    int t_shmId;
    /* the permission of shared memory is 'rw-rw-rw-' */
    t_shmId = shmget(r_SpaceInfo.m_shmKey, r_SpaceInfo.m_size, SHM_MODEFLAG | IPC_CREAT | IPC_EXCL);
    if (t_shmId == -1)
    {
#ifdef _DEBUG_
        cout << "createSpace Error! errno=" << errno << "," << strerror(errno)
             << " " << __FILE__ << ":" << __LINE__ << endl;
#endif
        return false;
    }
    return true;
}
bool SpaceMgrBase::deleteSpace(SpaceInfo&  r_SpaceInfo)
{
    //1. �����ñ�ռ��Ӧ�Ŀ����ļ�
    //   $MDB_HOME/ctl/dbname.ctl
    T_FILENAMEDEF  t_ctlFileName;
    key_t          t_shmKey;
    sprintf(t_ctlFileName, "%s/ctl/%s.ctl", getenv(MDB_HOMEPATH.c_str()), r_SpaceInfo.m_dbName);
    t_shmKey = ftok(t_ctlFileName, r_SpaceInfo.m_spaceCode);
    if (t_shmKey < 0)
    {
#ifdef _DEBUG_
        cout << "ftok(" << t_ctlFileName << ",";
        cout << r_SpaceInfo.m_spaceCode << ") false!" << endl;
#endif
        return false;
    }
    int r_shmId;
    r_shmId = shmget(t_shmKey, r_SpaceInfo.m_size, SHM_MODEFLAG);
    if (r_shmId == -1)
    {
#ifdef _DEBUG_
        cout << "shmget Error!t_shmKey=" << t_shmKey << " errno=" << errno << "," << strerror(errno)
             << " " << __FILE__ << ":" << __LINE__ << endl;
#endif
        return false;
    }
    return deleteSpace(r_shmId);
}

/**
 *deleteSpace ɾ����ռ�.
 *@param   r_shmId :  ��ռ�ShmId
 *@return  true �ɹ�,false ʧ��
 */
bool SpaceMgrBase::deleteSpace(const int& r_shmId)
{
    int t_iRet;
    t_iRet = shmctl(r_shmId, IPC_RMID, (struct shmid_ds*)0);
    if (t_iRet < 0)
    {
#ifdef _DEBUG_
        cout << "�ͷŹ����ڴ�ʧ��!r_shmId=" << r_shmId
             << " " << __FILE__ << ":" << __LINE__ << endl;
#endif
        return false;
    }
    return true;
}

/**
 *attachSpace ���ӱ�ռ�
 *@param   r_shmKey:  ��ռ�ShmKey
 *@param   r_shmId :  ��ռ�ShmId
 *@param   r_pShm  :  ���Ӻ�ĵ�ַ
 *@return  true �ɹ�,false ʧ��
 */
bool SpaceMgrBase::attachSpace(const key_t& r_shmKey, const size_t& r_size,
                               int& r_shmId, char * &r_pShm)
{
    r_shmId = shmget(r_shmKey, r_size, SHM_MODEFLAG); /* get shmid */
    if (r_shmId == -1)
    {
#ifdef _DEBUG_
        cout << "createSpace Error! shmKey=" << r_shmKey << " errno=" << strerror(errno)
             << " " << __FILE__ << ":" << __LINE__ << endl;
#endif
        return false;
    }
    return attachSpace(r_shmId, r_pShm);
}

/**
 *attachSpace ���ӱ�ռ�
 *@param   r_shmId :  ��ռ�ShmId
 *@param   r_pShm  :  ���Ӻ�ĵ�ַ
 *@return  true �ɹ�,false ʧ��
 */
bool SpaceMgrBase::attachSpace(const int& r_shmId, char * &r_pShm)
{
    //�Ѿ�ATTACH����,��������ATTACH
    r_pShm = MdbShmAdrrMgr::getShmAddrByShmId(r_shmId);
    if (r_pShm != NULL)
    {
        MdbShmAdrrMgr::setShmAddrInfo(r_shmId, r_pShm);
        return true;
    }
    //add by gaojf 2008-12-27 10:02
    r_pShm = (char*) shmat(r_shmId, (char*) 0, 0);
    if (r_pShm == (char*) - 1)
    {
#ifdef _DEBUG_
        cout << "attachSpace failed " << __FILE__ << __LINE__ << endl;
        cout << " errno = " << errno << endl;
        cout << "EINVAL = " << EINVAL << " ENOMEM=" << ENOMEM << " EACCES =" << EACCES << endl;
#endif
        return false;
    }
    //���Ѿ�ATTACH����Ϣ�����б���
    MdbShmAdrrMgr::setShmAddrInfo(r_shmId, r_pShm);
    return true;
}

/**
 *detachSpace ���ӱ�ռ�
 *@param   r_pShm  :  ��ַ
 *@return  true �ɹ�,false ʧ��
 */
bool SpaceMgrBase::detachSpace(char * &r_pShm)
{
    if (r_pShm != NULL)
    {
        shmdt(r_pShm);
        r_pShm = NULL;
    }
    return true;
}

bool SpaceMgrBase::dumpDataIntoFile(const char* r_path)
{
    return m_file.shmtofile(m_spaceHeader.m_shmAddr, r_path);
}
bool SpaceMgrBase::loadDataFromFile(const char* r_path)
{
    return m_file.filetoshm(m_spaceHeader.m_shmAddr, r_path);
}

bool  SpaceMgrBase::attach()
{
    int t_shmId;
    t_shmId = shmget(m_spaceHeader.m_shmKey, m_spaceHeader.m_size, SHM_MODEFLAG);
    if (t_shmId < 0)
    {
#ifdef _DEBUG_
        cout << "shmget false!m_spaceHeader.m_shmKey=" << m_spaceHeader.m_shmKey
             << " " << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    if (attachSpace(t_shmId, m_spaceHeader.m_shmAddr) == false)
    {
#ifdef _DEBUG_
        cout << "attachSpace false!" << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    m_pSpHeader = (SpaceInfo*)(m_spaceHeader.m_shmAddr);
    return true;
}
bool SpaceMgrBase::detach()
{
    int t_shmId;
    t_shmId = shmget(m_spaceHeader.m_shmKey, m_spaceHeader.m_size, SHM_MODEFLAG);
    if (t_shmId < 0)
    {
#ifdef _DEBUG_
        cout << "shmget false!m_spaceHeader.m_shmKey=" << m_spaceHeader.m_shmKey
             << " " << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    //���б���ɾ��SHM��ַ��Ϣ add by gaojf 2008-12-27 10:04
    if (MdbShmAdrrMgr::delShmAddrInfo(t_shmId) == true)
    {
        //��Ҫ�����ͷ�
        return detachSpace(m_spaceHeader.m_shmAddr);
    }
    else
    {
        //����Ҫ�����ͷ�
        m_spaceHeader.m_shmAddr = NULL;
        return true;
    }
}

SpaceInfo* SpaceMgrBase::getSpaceInfo()
{
    return &m_spaceHeader;
}

//���Խӿ�
bool SpaceMgrBase::dumpSpaceInfo(ostream& r_os)
{
    r_os << "---------��ռ�ͷ��Ϣ-------------------------" << endl;
    m_pSpHeader->dumpInfo(r_os);
    r_os << "----------------------------------------------" << endl;
    return true;
}
void SpaceMgrBase::lock(const int r_pos) throw(Mdb_Exception)
{
    m_pdbLock->mlatch_lock(m_srclatchinfo[r_pos]);
}
bool SpaceMgrBase::trylock(const int r_pos) throw(Mdb_Exception)
{
    return m_pdbLock->mlatch_trylock(m_srclatchinfo[r_pos]);
}
void SpaceMgrBase::unlock(const int r_pos)
{
    m_pdbLock->mlatch_unlock(m_srclatchinfo[r_pos]);
}

