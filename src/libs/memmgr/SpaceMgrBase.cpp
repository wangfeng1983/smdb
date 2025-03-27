#include <sys/errno.h>
#include <errno.h>
#include "SpaceMgrBase.h"
#include "InstanceFactory.h"
#include "MdbAddress.h"
/**
 *initialize 初始化.
 *@param   r_SpaceInfo:  表空间定义参数
 *@return  true 成功,false 失败
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
 *createSpace 创建表空间.
 *@param   r_SpaceInfo :  表空间基本信息
 *@param   r_diskFile  :  磁盘文件名
 *@param   r_spaceSize :  表空间大小
 *@param   r_spaceType :  表空间类型：控制区、数据区、索引区
 *@param   r_shmKey    :  表空间的ShmKey
 *@param   r_shmId     :  表空间ShmId
 *@return  true 创建成功,false 失败
 */
bool SpaceMgrBase::createSpace(SpaceInfo&  r_SpaceInfo)
{
    //1. 创建该表空间对应的控制文件
    //   $MDB_HOME/ctl/dbname.ctl
    T_FILENAMEDEF  t_ctlFileName;
    sprintf(t_ctlFileName, "%s/ctl/%s.ctl", getenv(MDB_HOMEPATH.c_str()), r_SpaceInfo.m_dbName);
    //2. 根据该文件生成对应的
    //判断文件是否存在？不存在则创建r_spaceSize大小（占空间）的文件
    //判断文件大小是否和r_spaceSize一致,不一致,报错
    //获取r_shmKey
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
    //1. 创建该表空间对应的控制文件
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
 *deleteSpace 删除表空间.
 *@param   r_shmId :  表空间ShmId
 *@return  true 成功,false 失败
 */
bool SpaceMgrBase::deleteSpace(const int& r_shmId)
{
    int t_iRet;
    t_iRet = shmctl(r_shmId, IPC_RMID, (struct shmid_ds*)0);
    if (t_iRet < 0)
    {
#ifdef _DEBUG_
        cout << "释放共享内存失败!r_shmId=" << r_shmId
             << " " << __FILE__ << ":" << __LINE__ << endl;
#endif
        return false;
    }
    return true;
}

/**
 *attachSpace 连接表空间
 *@param   r_shmKey:  表空间ShmKey
 *@param   r_shmId :  表空间ShmId
 *@param   r_pShm  :  连接后的地址
 *@return  true 成功,false 失败
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
 *attachSpace 连接表空间
 *@param   r_shmId :  表空间ShmId
 *@param   r_pShm  :  连接后的地址
 *@return  true 成功,false 失败
 */
bool SpaceMgrBase::attachSpace(const int& r_shmId, char * &r_pShm)
{
    //已经ATTACH过的,不再重新ATTACH
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
    //将已经ATTACH的信息加入列表中
    MdbShmAdrrMgr::setShmAddrInfo(r_shmId, r_pShm);
    return true;
}

/**
 *detachSpace 连接表空间
 *@param   r_pShm  :  地址
 *@return  true 成功,false 失败
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
    //从列表中删除SHM地址信息 add by gaojf 2008-12-27 10:04
    if (MdbShmAdrrMgr::delShmAddrInfo(t_shmId) == true)
    {
        //需要彻底释放
        return detachSpace(m_spaceHeader.m_shmAddr);
    }
    else
    {
        //不需要彻底释放
        m_spaceHeader.m_shmAddr = NULL;
        return true;
    }
}

SpaceInfo* SpaceMgrBase::getSpaceInfo()
{
    return &m_spaceHeader;
}

//调试接口
bool SpaceMgrBase::dumpSpaceInfo(ostream& r_os)
{
    r_os << "---------表空间头信息-------------------------" << endl;
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

