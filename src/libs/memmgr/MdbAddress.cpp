#include "MdbAddress.h"
#include <algorithm>

pthread_mutex_t MdbShmAdrrMgr::m_mutex = PTHREAD_MUTEX_INITIALIZER;

//依据起始地址和偏移量，设置物理地址
void MdbAddress::setAddr(char* r_spAddr)
{
    m_addr = r_spAddr + m_pos.getOffSet();
}
//依据起始地址和物理地址设置偏移量
bool MdbAddress::setShmPos(const unsigned int& r_spCode, char* r_spAddr)
{
    size_t t_offSet = m_addr - r_spAddr;
    if ((m_pos.setSpaceCode(r_spCode) == false) ||
            (m_pos.setOffSet(t_offSet) == false))
    {
        return false;
    }
    return true;
}

void MdbAddressMgr::initialize(vector<SpaceAddress> &r_spAddrList)
{
    m_spAddrList = r_spAddrList;
    ::sort(m_spAddrList.begin(), m_spAddrList.end());
}
bool MdbAddressMgr::getPhAddr(MdbAddress& r_mdbAddr)
{
    if (NULL_SHMPOS == r_mdbAddr.m_pos)
    {
        r_mdbAddr.m_addr = NULL;
        return true;
    }
    SpaceAddress t_spAddr;
    t_spAddr.m_spaceCode = r_mdbAddr.m_pos.getSpaceCode();
    //1.找到对应的表空间地址信息
    vector<SpaceAddress>::iterator t_itr;
    t_itr = ::lower_bound(m_spAddrList.begin(), m_spAddrList.end(), t_spAddr);
    if (t_itr == m_spAddrList.end())
    {
        r_mdbAddr.m_addr = NULL;
#ifdef _DEBUG_
        cout << "No valid address r_mdbAddr.m_pos={"
             << r_mdbAddr.m_pos << "}!" << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    if (!(*t_itr == t_spAddr))
    {
        r_mdbAddr.m_addr = NULL;
#ifdef _DEBUG_
        cout << "No valid address r_mdbAddr.m_pos={"
             << r_mdbAddr.m_pos << "}!" << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    //2.根据偏移量和表空间首地址,设置物理地址
    r_mdbAddr.setAddr(t_itr->m_sAddr);
    return true;
}
bool MdbAddressMgr::getPhAddr(const ShmPosition& r_shmPos, char * &r_phAddr)
{
    SpaceAddress t_spAddr;
    t_spAddr.m_spaceCode = r_shmPos.getSpaceCode();
    if (NULL_SHMPOS == r_shmPos)
    {
        r_phAddr = NULL;
        return true;
    }
    r_phAddr = NULL;
    //1.找到对应的表空间地址信息
    vector<SpaceAddress>::iterator t_itr;
    t_itr = ::lower_bound(m_spAddrList.begin(), m_spAddrList.end(), t_spAddr);
    if (t_itr == m_spAddrList.end())
    {
#ifdef _DEBUG_
        cout << "No valid address!" << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    if (!(*t_itr == t_spAddr))
    {
#ifdef _DEBUG_
        cout << "No valid address!" << __FILE__ << __LINE__ << endl;
        cout << "r_shmPos:" << r_shmPos << endl;
        cout << "SpaceAddress:" << t_itr->m_spaceCode << endl;
#endif
        return false;
    }
    //2.根据偏移量和表
    r_phAddr = t_itr->m_sAddr + r_shmPos.getOffSet();
    return true;
}
bool MdbAddressMgr::getShmPos(MdbAddress& r_mdbAddr)
{
    //1.找到对应的表空间地址信息
    for (vector<SpaceAddress>::iterator t_itr = m_spAddrList.begin();
            t_itr != m_spAddrList.end(); t_itr++)
    {
        if (t_itr->m_sAddr <= r_mdbAddr.m_addr &&
                t_itr->m_eAddr >= r_mdbAddr.m_addr)
        {
            //2.根据偏移量和表
            return r_mdbAddr.setShmPos(t_itr->m_spaceCode, t_itr->m_sAddr);
        }
    }
    return false;
}


vector<MdbShmAdrrMgr::MdbShmAddrInfo> * MdbShmAdrrMgr::getShmAddrInfoList()
{
    static vector<MdbShmAddrInfo> m_shmAddrInfoList;
    return &m_shmAddrInfoList;
}

char* MdbShmAdrrMgr::getShmAddrByShmId(const int& r_shmId)
{
    pthread_mutex_lock(&m_mutex);
    try
    {
        MdbShmAddrInfo t_shmAddrInfo;
        vector<MdbShmAddrInfo> &t_shmAdrrInfoList = *getShmAddrInfoList();
        vector<MdbShmAddrInfo>::iterator t_itr;
        t_shmAddrInfo.m_shmId   = r_shmId ;
        t_shmAddrInfo.m_shmAddr = NULL ;
        t_itr = ::lower_bound(t_shmAdrrInfoList.begin(),
                              t_shmAdrrInfoList.end(),
                              t_shmAddrInfo);
        if (t_itr == t_shmAdrrInfoList.end() ||
                !(*t_itr == t_shmAddrInfo))
        {
            pthread_mutex_unlock(&m_mutex);
            return NULL;
        }
        else
        {
            char* addr = t_itr->m_shmAddr;
            pthread_mutex_unlock(&m_mutex);
            return addr;
        }
    }
    catch (...)
    {
        pthread_mutex_unlock(&m_mutex);
        throw;
    }
}

void MdbShmAdrrMgr::setShmAddrInfo(const int& r_shmId, char* r_shmAddr)
{
    pthread_mutex_lock(&m_mutex);
    try
    {
        MdbShmAddrInfo t_shmAddrInfo;
        vector<MdbShmAddrInfo> &t_shmAdrrInfoList = *getShmAddrInfoList();
        vector<MdbShmAddrInfo>::iterator t_itr;
        t_shmAddrInfo.m_shmId     = r_shmId ;
        t_shmAddrInfo.m_attachNum = 1;
        t_shmAddrInfo.m_shmAddr   = r_shmAddr ;
        t_itr = ::lower_bound(t_shmAdrrInfoList.begin(),
                              t_shmAdrrInfoList.end(),
                              t_shmAddrInfo);
        if (t_itr == t_shmAdrrInfoList.end() ||
                !(*t_itr == t_shmAddrInfo))
        {
            t_shmAdrrInfoList.insert(t_itr, t_shmAddrInfo);
        }
        else
        {
            t_itr->m_attachNum += 1;
        }
    }
    catch (...)
    {
        pthread_mutex_unlock(&m_mutex);
        throw;
    }
    pthread_mutex_unlock(&m_mutex);
    return ;
}

bool MdbShmAdrrMgr::delShmAddrInfo(const int& r_shmId)
{
    pthread_mutex_lock(&m_mutex);
    try
    {
        MdbShmAddrInfo t_shmAddrInfo;
        vector<MdbShmAddrInfo> &t_shmAdrrInfoList = *getShmAddrInfoList();
        vector<MdbShmAddrInfo>::iterator t_itr;
        t_shmAddrInfo.m_shmId   = r_shmId ;
        t_shmAddrInfo.m_shmAddr = NULL ;
        t_itr = ::lower_bound(t_shmAdrrInfoList.begin(),
                              t_shmAdrrInfoList.end(),
                              t_shmAddrInfo);
        if (t_itr == t_shmAdrrInfoList.end() ||
                !(*t_itr == t_shmAddrInfo))
        {
            pthread_mutex_unlock(&m_mutex);
            return true;
        }
        else
        {
            if (t_itr->m_attachNum == 1)
            {
                t_shmAdrrInfoList.erase(t_itr);
                pthread_mutex_unlock(&m_mutex);
                return true;
            }
            else
            {
                t_itr->m_attachNum -= 1;
                pthread_mutex_unlock(&m_mutex);
                return false;
            }
        }
    }
    catch (...)
    {
        pthread_mutex_unlock(&m_mutex);
        throw;
    }
}

