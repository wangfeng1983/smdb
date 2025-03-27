#ifndef MDBLATCHMGR_H_INCLUDE_20100519
#define MDBLATCHMGR_H_INCLUDE_20100519
#include "MdbConstDef.h"
#include "Mdb_Exception.h"
#include "MDBLatchInfo.h"
#include <list>
USING_NAMESPACE(std)
/*
      MLATCH ʹ��˵��:
  ����1��   ʹ�÷��㣬Ч�ʵ�
    static MDBLatchMgr * getInstance(const char * r_dbname) throw(Mdb_Exception);
    void mlatch_lock(const int r_srctype,const char *r_srcname) throw(Mdb_Exception);
    void mlatch_unlock(const int r_srctype,const char *r_srcname) throw(Mdb_Exception);
  ����2��   Ч�ʸ�
    static MDBLatchMgr * getInstance(const char * r_dbname) throw(Mdb_Exception);
    SrcLatchInfo* getSrcLatchInfo(const int r_srctype,const char *r_srcname);
    void mlatch_lock(const SrcLatchInfo *r_latchinfo)   throw(Mdb_Exception);
    void mlatch_unlock(const SrcLatchInfo *r_latchinfo) throw(Mdb_Exception);
*/

class InnerSrcinfo
{
        //��Դ����---  0Ϊ���ݿ�,1Ϊ��,2Ϊtablespace��
        //    UNDO���� 11 UNDO��
    public:
        int         m_srctype;   //��Դ����
        T_NAMEDEF   m_srcname;   //��Դ����
    public:
        InnerSrcinfo(const int r_srctype, const char* r_srcname)
        {
            m_srctype = r_srctype;
            strcpy(m_srcname, r_srcname);
        }
    public:
        friend int operator<(const InnerSrcinfo& r_left, const InnerSrcinfo& r_right)
        {
            if (!(r_left.m_srctype == r_right.m_srctype))
                return (r_left.m_srctype < r_right.m_srctype);
            return (strcasecmp(r_left.m_srcname, r_right.m_srcname) < 0);
        }
        friend int operator==(const InnerSrcinfo& r_left, const InnerSrcinfo& r_right)
        {
            return ((r_left.m_srctype == r_right.m_srctype) &&
                    (strcasecmp(r_left.m_srcname, r_right.m_srcname) == 0));
        }
};
class SrcLatchInfo
{
    public:
        int         m_srctype;   //��Դ����
        T_NAMEDEF   m_srcname;   //��Դ����
        int         m_srcid;     //��ԴID
        LatchNode*  m_latchnode; //��Ӧ��LATCHNODE
    public:
        friend int operator<(const SrcLatchInfo& r_left, const SrcLatchInfo& r_right)
        {
            if (!(r_left.m_srctype == r_right.m_srctype))
                return (r_left.m_srctype < r_right.m_srctype);
            return (strcasecmp(r_left.m_srcname, r_right.m_srcname) < 0);
        }
        friend int operator==(const SrcLatchInfo& r_left, const SrcLatchInfo& r_right)
        {
            return ((r_left.m_srctype == r_right.m_srctype) &&
                    (strcasecmp(r_left.m_srcname, r_right.m_srcname) == 0));
        }
};
//ÿ���߳�һ�����Ӧһ��MDBLatchMgrʵ��
class MDBLatchMgr
{
    public:
        static MDBLatchMgr* getInstance(const char* r_dbname) throw(Mdb_Exception);
        static void unregister(const char* r_dbname, const pthread_t r_threadid);
        void mlatch_lock(const int r_srctype, const char* r_srcname) throw(Mdb_Exception);
        void mlatch_unlock(const int r_srctype, const char* r_srcname) throw(Mdb_Exception);
        void slatch_lock(const size_t& r_lvalue) throw(Mdb_Exception);
        void slatch_unlock(const size_t& r_lvalue);
        void slatch_lock(const ShmPosition& r_shmpos) throw(Mdb_Exception);
        void slatch_unlock(const ShmPosition& r_shmpos);
    public:
        SrcLatchInfo* getSrcLatchInfo(const int r_srctype, const char* r_srcname);
        void mlatch_lock(const SrcLatchInfo* r_latchinfo)   throw(Mdb_Exception);
        void mlatch_unlock(const SrcLatchInfo* r_latchinfo) throw(Mdb_Exception);
        bool mlatch_trylock(const SrcLatchInfo* r_latchinfo)throw(Mdb_Exception);
    protected:
        int getInnerSrcId(const int r_srctype, const char* r_srcname);
        unsigned hash_value(size_t r_lvalue, size_t r_hashsize)
        {
            //��0�����á�
            return (r_lvalue % (r_hashsize - 1) + 1);
        }
    public:
        LatchShmInfo      m_latchshminfo; //��Ӧ�Ĺ����ڴ���Ϣ
        pid_t             m_pid;          //����ID
        pthread_t         m_thid;         //�߳�ID
        ProcessLatchInfo* m_procinfo;     //��ǰ�̵߳ĵ�ַ��Ϣ
    private:
        MDBLatchMgr() {};
    private:
        map<InnerSrcinfo, SrcLatchInfo>  m_srclatchlist;
        static list<MDBLatchMgr*> m_latchmgrlist;
        static pthread_mutex_t    m_innerMutex;
};
#endif //MDBLATCHMGR_H_INCLUDE_20100519
