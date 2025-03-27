#ifndef _MDBADDRESS_H_INCLUDE_20080424_
#define _MDBADDRESS_H_INCLUDE_20080424_
#include "config-all.h"
#include <string>
#include <cstring>
#include <vector>
USING_NAMESPACE(std)
#include <iostream>
#include <pthread.h>

#include "MdbConstDef.h"

class MdbAddress
{
    public:
        ShmPosition  m_pos;
        char*        m_addr;
    public:
        MdbAddress()
        {
            m_addr = NULL;
        }
        MdbAddress(const MdbAddress& right)
        {
            memcpy(this, &right, sizeof(MdbAddress));
        }
        MdbAddress& operator=(const MdbAddress& right)
        {
            memcpy(this, &right, sizeof(MdbAddress));
            return *this;
        }
        friend bool  operator<(const MdbAddress& left, const MdbAddress& right)
        {
            return (memcmp(&(left.m_pos), &(right.m_pos), sizeof(ShmPosition)) < 0);
        }

        friend bool operator == (const MdbAddress& left, const MdbAddress& right)
        {
            return (memcmp(&(left.m_pos), &(right.m_pos), sizeof(ShmPosition)) == 0);
        }

        //依据起始地址和偏移量，设置物理地址
        void setAddr(char* r_spAddr);
        //依据起始地址和物理地址设置偏移量
        bool setShmPos(const unsigned int& r_spCode, char* r_spAddr);
};

class SpaceAddress
{
    public:
        unsigned int m_spaceCode;
        size_t  m_size;
        char*   m_sAddr;
        char*   m_eAddr;
    public:
        friend int operator<(const SpaceAddress& left, const SpaceAddress& right)
        {
            return (left.m_spaceCode < right.m_spaceCode);
        }

        friend int operator==(const SpaceAddress& left, const SpaceAddress& right)
        {
            return (left.m_spaceCode == right.m_spaceCode);
        }
};

class MdbAddressMgr
{
    public:
        MdbAddressMgr()
        {
            m_spAddrList.clear();
        }
        ~MdbAddressMgr()
        {
            m_spAddrList.clear();
        }
    public:
        void initialize(vector<SpaceAddress> &spAddrList);
        //要求对于NULL_SHMPOS时，不判为异常
        bool getPhAddr(MdbAddress& r_mdbAddr);
        //要求对于NULL_SHMPOS时，不判为异常
        bool getPhAddr(const ShmPosition& r_shmPos, char * &r_phAddr);
        bool getShmPos(MdbAddress& r_mdbAddr);
        void dump()
        {
            cout << "--------------------spAddrLit-------------" << endl;
            for (vector<SpaceAddress>::iterator r_itr = m_spAddrList.begin();
                    r_itr != m_spAddrList.end(); r_itr++)
            {
                cout << " " << r_itr->m_spaceCode << " "
                     << r_itr->m_size << " " << (void*)(r_itr->m_sAddr) << endl;
            }
            cout << "------------------------------------------" << endl;
        }
    private:
        vector<SpaceAddress> m_spAddrList;
};

//共享内存管理器
class MdbShmAdrrMgr
{
    public:
        class MdbShmAddrInfo
        {
            public:
                int   m_shmId;
                int   m_attachNum;
                char* m_shmAddr;
            public:
                friend int operator==(const MdbShmAddrInfo& r_left, const MdbShmAddrInfo& r_right)
                {
                    return (r_left.m_shmId == r_right.m_shmId);
                }

                friend int operator<(const MdbShmAddrInfo& r_left, const MdbShmAddrInfo& r_right)
                {
                    return (r_left.m_shmId < r_right.m_shmId);
                }
        };
    public:
        static vector<MdbShmAddrInfo> * getShmAddrInfoList();
        //根据SHMID取对应的地址.NULL表示空
        static char* getShmAddrByShmId(const int& r_shmId);
        //加入SHM地址信息
        static void setShmAddrInfo(const int& r_shmId, char* r_shmAddr);
        //从SHM地址信息中删除(将连接数-1)
        static bool delShmAddrInfo(const int& r_shmId);

    private:
        static pthread_mutex_t m_mutex;
};
#endif //_MDBADDRESS_H_INCLUDE_20080424_
