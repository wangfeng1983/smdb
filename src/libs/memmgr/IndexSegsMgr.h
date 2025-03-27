#ifndef _INDEXSEGSMGR_H_INCLUDE_20100527_
#define _INDEXSEGSMGR_H_INCLUDE_20100527_
#include "MdbConstDef.h"
#include "MdbConstDef2.h"

#if defined(hpux) || defined(__hpux__) || defined(__hpux)
#pragma pack 1
#elif defined(_AIX)
#pragma options align=packed
#elif defined(_WIN32)
#pragma pack(push, 1)
#else
#pragma pack(1)
#endif // by chenm 2010-09-06 指定字符对齐方式 HashIndexNode

class IndexDesc;
class IndexSegInfo
{
    public:
        size_t m_pos;     //IndexSegInfo本身起始偏移
        size_t m_fIdleSeg;
        size_t m_lIdleSeg;
        size_t m_fUsedSeg;
        size_t m_lUsedSeg;
        T_SCN  m_scn;
    public:
        //r_offset 段起始偏移量
        void f_init(const size_t& r_pos, const size_t& r_offset)
        {
            memset(this, 0, sizeof(IndexSegInfo));
            m_pos = r_pos;
            m_lIdleSeg = m_fIdleSeg = r_offset;
        }
};
class HashIndexNode
{
    public:
        ShmPosition  m_nodepos;
        int          m_sid;
        char         m_lockinfo; //0x00 非锁定,0x01 锁定
        T_SCN        m_scn;
    public:
        void lock()
        {
            m_lockinfo = 0x01;
        }
        void unlock()
        {
            m_lockinfo = 0x00;
        }
};
class HashIndexSeg
{
    public:
        size_t    m_sOffSet;  //HashIndexSeg本身起始偏移
        size_t    m_segSize;  //该段大小,包括HashIndexSeg
        size_t    m_hashSize;
        T_NAMEDEF m_idxName;
        size_t    m_prev;
        size_t    m_next;
        T_SCN     m_scn;
    public:
        //t_addr为指向HashIndexSeg的地址
        void initHash(char* t_addr, const T_SCN& r_scn)
        {
            memset(t_addr + sizeof(HashIndexSeg), 0, m_hashSize * sizeof(HashIndexNode));
            HashIndexNode* t_pnode = (HashIndexNode*)(t_addr + sizeof(HashIndexSeg));
            for (size_t t_i = 0; t_i < m_hashSize; ++t_i)
            {
                t_pnode->m_scn = r_scn;
                t_pnode++;
            }
            m_scn =  r_scn;
        }
};
class IndexSegsMgr
{
    public:
        IndexSegsMgr();
        virtual ~IndexSegsMgr();
    public:
        //r_spaceaddr 表空间起始地址     r_offset index段起始偏移量
        //索引段的总大小 r_idxsegsize (包含段列表)
        //r_flag  1 第一次初始化，需要初始化段列表信息
        bool initialize(char* r_spaceaddr, const size_t& r_offset,
                        const size_t& r_idxsegsize, const int r_flag = 0);
        void startmdb_init();
        //r_shmPos是指向Hash中ShmPositioin[]的首地址
        bool createHashIndex(IndexDesc* r_idxDesc, size_t& r_offset, const T_SCN& r_scn);
        //r_shmPos是指向Hash中ShmPositioin[]的首地址
        bool dropHashIdex(const size_t& r_offset, const T_SCN& r_scn);
        //r_shmPos是指向Hash中ShmPositioin[]的首地址
        void initHashSeg(const size_t& r_offset, const T_SCN& r_scn);
        IndexSegInfo* getSegInfos();
    protected:
        bool f_init();
    private:
        IndexSegInfo*  m_pIdxSegInfo;
        char*          m_spaceaddr;
    public:
        //调试接口
        virtual bool   dumpInfo(ostream& r_os);
        virtual size_t getUsedSize();
};
#if defined(hpux) || defined(__hpux__) || defined(__hpux)
#pragma pack()
#elif defined(_AIX)
#pragma options align=reset
#elif defined(_WIN32)
#pragma pack(pop)
#else
#pragma pack()
#endif
#endif //_INDEXSEGSMGR_H_INCLUDE_20100527_
