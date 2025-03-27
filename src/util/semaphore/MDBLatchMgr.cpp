#include "MDBLatchMgr.h"
#include <algorithm>
#include "MDBLatchShmMgr.h"
#include <iostream>
#include <fstream>

list<MDBLatchMgr*>   MDBLatchMgr::m_latchmgrlist;
pthread_mutex_t      MDBLatchMgr::m_innerMutex = PTHREAD_MUTEX_INITIALIZER;

MDBLatchMgr* MDBLatchMgr::getInstance(const char* r_dbname) throw(Mdb_Exception)
{
    pid_t      t_pid   = getpid();
    pthread_t  t_thId  = pthread_self();
    //加进程内，线程互斥锁  begin
    pthread_mutex_lock(&(MDBLatchMgr::m_innerMutex));
    bool t_bflag = false;
    for (list<MDBLatchMgr*>::iterator t_itr = MDBLatchMgr::m_latchmgrlist.begin();
            t_itr != MDBLatchMgr::m_latchmgrlist.end(); ++t_itr)
    {
        if ((*t_itr)->m_pid  == t_pid  &&
                (*t_itr)->m_thid == t_thId &&
                strcasecmp((*t_itr)->m_latchshminfo.m_dbName, r_dbname) == 0)
        {
            pthread_mutex_unlock(&(MDBLatchMgr::m_innerMutex));
            return (*t_itr);
        }
    }
    MDBLatchMgr* t_platchmgr = new MDBLatchMgr();
    sprintf(t_platchmgr->m_latchshminfo.m_dbName, r_dbname);
    t_platchmgr->m_pid = t_pid;
    t_platchmgr->m_thid = t_thId;
    if (MDBLatchShmMgr::getLatchShmAddr(t_platchmgr->m_latchshminfo, t_platchmgr->m_procinfo) == false)
    {
        pthread_mutex_unlock(&(MDBLatchMgr::m_innerMutex));
        throw Mdb_Exception(__FILE__, __LINE__, "getLatchShmAddr false!");
    }
    MDBLatchMgr::m_latchmgrlist.push_back(t_platchmgr);
    //加进程内，线程互斥锁  end
    pthread_mutex_unlock(&(MDBLatchMgr::m_innerMutex));
    return t_platchmgr;
}

void MDBLatchMgr::unregister(const char* r_dbname, const pthread_t r_threadid)
{
    pid_t      t_pid   = getpid();
    //加进程内，线程互斥锁  begin
    pthread_mutex_lock(&(MDBLatchMgr::m_innerMutex));
    for (list<MDBLatchMgr*>::iterator t_itr = MDBLatchMgr::m_latchmgrlist.begin();
            t_itr != MDBLatchMgr::m_latchmgrlist.end(); ++t_itr)
    {
        if ((*t_itr)->m_pid  == t_pid  &&
                (*t_itr)->m_thid == r_threadid &&
                strcasecmp((*t_itr)->m_latchshminfo.m_dbName, r_dbname) == 0)
        {
            //将该线程资源注销
            MDBLatchShmMgr::delProcInfo(r_dbname, r_threadid);
            m_latchmgrlist.erase(t_itr);
            break;
        }
    }
    //加进程内，线程互斥锁  end
    pthread_mutex_unlock(&(MDBLatchMgr::m_innerMutex));
}
void MDBLatchMgr::mlatch_lock(const int r_srctype, const char* r_srcname) throw(Mdb_Exception)
{
    SrcLatchInfo* t_pLatchInfo = getSrcLatchInfo(r_srctype, r_srcname);
    return mlatch_lock(t_pLatchInfo);
}
void MDBLatchMgr::mlatch_unlock(const int r_srctype, const char* r_srcname) throw(Mdb_Exception)
{
    SrcLatchInfo* t_pLatchInfo = getSrcLatchInfo(r_srctype, r_srcname);
    return mlatch_unlock(t_pLatchInfo);
}
void MDBLatchMgr::slatch_lock(const size_t& r_lvalue) throw(Mdb_Exception)
{
    int t_hashval = hash_value(r_lvalue, LATCH_CONST::MAX_SLATCH_NUM);
    LatchNode* t_platchnode = m_latchshminfo.m_slatchaddr + t_hashval;
    int t_ret = 0;
    do
    {
        t_ret = MDBLatchShmMgr::slatch_lock(t_platchnode, t_hashval, m_procinfo);
        if (t_ret < 0) //异常
        {
            throw Mdb_Exception(__FILE__, __LINE__, "SLATCH不能同时占用多个!");
        }
        else if (t_ret > 0)
        {
            return ;
        }
        else continue;
    }
    while (1);
}
void MDBLatchMgr::slatch_unlock(const size_t& r_lvalue)
{
    int t_hashval = hash_value(r_lvalue, LATCH_CONST::MAX_SLATCH_NUM);
    LatchNode* t_platchnode = m_latchshminfo.m_slatchaddr + t_hashval;
    MDBLatchShmMgr::slatch_unlock(t_platchnode, t_hashval, m_procinfo);
}
void MDBLatchMgr::slatch_lock(const ShmPosition& r_shmpos) throw(Mdb_Exception)
{
    return slatch_lock(r_shmpos.getOffSet());
}
void MDBLatchMgr::slatch_unlock(const ShmPosition& r_shmpos)
{
    return slatch_unlock(r_shmpos.getOffSet());
}
SrcLatchInfo* MDBLatchMgr::getSrcLatchInfo(const int r_srctype, const char* r_srcname)
{
    InnerSrcinfo t_srcinfo(r_srctype, r_srcname);
    map<InnerSrcinfo, SrcLatchInfo>::iterator t_itr = m_srclatchlist.find(t_srcinfo);
    if (t_itr == m_srclatchlist.end())
    {
        SrcLatchInfo t_srclatchinfo;
        t_srclatchinfo.m_srctype = r_srctype;
        strcpy(t_srclatchinfo.m_srcname, r_srcname);
        t_srclatchinfo.m_srcid   = getInnerSrcId(r_srctype, r_srcname);
        t_srclatchinfo.m_latchnode = m_latchshminfo.m_mlatchaddr + t_srclatchinfo.m_srcid;
        m_srclatchlist.insert(map<InnerSrcinfo, SrcLatchInfo>::value_type(t_srcinfo, t_srclatchinfo));
        return &(m_srclatchlist[t_srcinfo]);
    }
    else
    {
        return &(t_itr->second);
    }
}
int MDBLatchMgr::getInnerSrcId(const int r_srctype, const char* r_srcname)
{
    //该函数一台主机只允许同时，只有一个进程中的一个线程执行
    T_NAMEDEF    t_lockFile;
    sprintf(t_lockFile, "%s/ctl/%s.lock", getenv(MDB_HOMEPATH.c_str()),
            m_latchshminfo.m_dbName);
    int t_innerId = -1;
    //加进程内，线程互斥锁  begin
    pthread_mutex_lock(&(MDBLatchMgr::m_innerMutex));
    //加写锁
    try
    {
        //1. 如果文件不存在,则创建文件
        int file_des;
        if (access(t_lockFile, F_OK) != 0)
        {
            file_des = creat(t_lockFile, S_IRUSR | S_IWUSR);
            if (access(t_lockFile, F_OK) != 0)
            {
                throw Mdb_Exception(__FILE__, __LINE__, "文件{%s}不存在并无法创建!", t_lockFile);
            }
            if (fchmod(file_des, 0666) != 0)
            {
                throw Mdb_Exception(__FILE__, __LINE__, "修改文件{%s}权限失败!", t_lockFile);
            }
            close(file_des);
        }
        int t_fd;
        if ((t_fd =::open(t_lockFile, O_RDWR)) < 0)
        {
            throw Mdb_Exception(__FILE__, __LINE__, "打开文件{%s}失败!", t_lockFile);
        }
        struct flock* ret ;
        ret = new struct flock();//add struct key word,by chenzm for hp-ux
        ret->l_type = F_WRLCK;
        if (fcntl(t_fd, F_SETLKW, ret) != 0) //如果他人加上文件锁，则阻塞等待他人解锁
        {
            delete 	ret;
            throw Mdb_Exception(__FILE__, __LINE__, "锁文件{%s}失败!", t_lockFile);
        }
        SrcLatchInfo t_innsrc;
        int          t_maxInnerSrcId = 1;
        while (::read(t_fd, &t_innsrc, sizeof(SrcLatchInfo)) == sizeof(SrcLatchInfo))
        {
            if (t_innsrc.m_srctype == r_srctype &&
                    strcasecmp(t_innsrc.m_srcname, r_srcname) == 0)
            {
                t_innerId = t_innsrc.m_srcid;
                break;
            }
            else
            {
                if (t_innsrc.m_srcid > t_maxInnerSrcId)
                    t_maxInnerSrcId = t_innsrc.m_srcid;
            }
        };
        if (t_innerId < 0)
        {
            //如果不存在，则记录最大的内部资源ID，并将新的资源加入
            t_innerId = t_maxInnerSrcId + 1;
            if (lseek(t_fd, 0, SEEK_END) < 0)
            {
                close(t_fd);
                throw Mdb_Exception(__FILE__, __LINE__, "lseek{%s}失败!", t_lockFile);
            }
            t_innsrc.m_srctype = r_srctype;
            strcpy(t_innsrc.m_srcname, r_srcname);
            t_innsrc.m_srcid = t_innerId;
            if (write(t_fd, &t_innsrc, sizeof(SrcLatchInfo)) != sizeof(SrcLatchInfo))
            {
                close(t_fd);
                throw Mdb_Exception(__FILE__, __LINE__, "write{%s}失败!", t_lockFile);
            }
        }
        delete 	ret;
        close(t_fd);
    }
    catch (LINKAGE_MDB::Mdb_Exception e)
    {
        pthread_mutex_unlock(&(MDBLatchMgr::m_innerMutex));
        t_innerId = -1;
#ifdef _DEBUG_
        cout << e.GetString() << endl;
#endif
        throw e;
    }
    pthread_mutex_unlock(&(MDBLatchMgr::m_innerMutex));
    return t_innerId;
}

void MDBLatchMgr::mlatch_lock(const SrcLatchInfo* r_latchinfo) throw(Mdb_Exception)
{
    int t_ret = 0;
    do
    {
        t_ret = MDBLatchShmMgr::mlatch_lock(r_latchinfo->m_latchnode, r_latchinfo->m_srcid, m_procinfo);
        if (t_ret < 0) //异常
        {
            //为差异常情况锁失败,增加一些错误信息输出gaojf 2012/4/12 17:46:14 begin
            char t_errlogFilename[256];
            struct tm* t_stNowTime = NULL;
            time_t t_tNowTime;
            time(&t_tNowTime);
            t_stNowTime = localtime(&t_tNowTime);
            sprintf(t_errlogFilename, "%s/ctl/%s.errlog", getenv(MDB_HOMEPATH.c_str()), m_latchshminfo.m_dbName);
            fstream t_errstream;
            t_errstream.open(t_errlogFilename, ios::out);
            t_errstream << "[" << t_stNowTime->tm_year + 1900 << "/"
                        << t_stNowTime->tm_mon + 1 << "/" << t_stNowTime->tm_mday << " "
                        << t_stNowTime->tm_hour << ":" << t_stNowTime->tm_min << ":"
                        << t_stNowTime->tm_sec << "]" << endl;
            t_errstream << "现有mlatch 信息----------------------" << __FILE__ << __LINE__ << endl;
            t_errstream << "m_procinfo->m_mlatchnum   =" << m_procinfo->m_mlatchnum << endl
                        << "LATCH_CONST::MAX_LATCH_NUM=" << LATCH_CONST::MAX_LATCH_NUM << endl;
            for (int t_s = 0; t_s < m_procinfo->m_mlatchnum; ++t_s)
            {
                t_errstream << "序号:" << t_s << " srcid=" << m_procinfo->m_mlatchinfo[t_s] << endl;
            }
            t_errstream << "尝试失败的信息:r_latchinfo->m_srcid=" << r_latchinfo->m_srcid << "  或资源占用过多" << endl;
            t_errstream.close();
            t_errstream.clear();
            //为差异常情况锁失败,增加一些错误信息输出gaojf 2012/4/12 17:46:14 end
            throw Mdb_Exception(__FILE__, __LINE__, "错误(id为0或资源占用过多)!");
        }
        else if (t_ret > 0)
        {
            return ;
        }
        else continue;
    }
    while (1);
}
bool MDBLatchMgr::mlatch_trylock(const SrcLatchInfo* r_latchinfo)throw(Mdb_Exception)
{
    int t_ret = t_ret = MDBLatchShmMgr::mlatch_lock(r_latchinfo->m_latchnode, r_latchinfo->m_srcid, m_procinfo, 0);
    if (t_ret < 0) //异常
    {
        throw Mdb_Exception(__FILE__, __LINE__, "错误(id为0或资源占用过多)!");
    }
    else if (t_ret > 0)
    {
        return true;
    }
    else return false;
}
void MDBLatchMgr::mlatch_unlock(const SrcLatchInfo* r_latchinfo) throw(Mdb_Exception)
{
    MDBLatchShmMgr::mlatch_unlock(r_latchinfo->m_latchnode, r_latchinfo->m_srcid, m_procinfo);
}

