#include "MDBLatchShmMgr.h"
#include "Mdb_Exception.h"

#include <sys/errno.h>
#include <errno.h>
#include <algorithm>


vector<LatchShmInfo> MDBLatchShmMgr::m_latchShmList;
pthread_mutex_t      MDBLatchShmMgr::m_innerMutex = PTHREAD_MUTEX_INITIALIZER;
MDBLatchShmMgr::MDBLatchShmMgr()
{
}
MDBLatchShmMgr::~MDBLatchShmMgr()
{
}
bool MDBLatchShmMgr::getLatchShmAddr(LatchShmInfo& r_shminfo, ProcessLatchInfo* &r_procInfo)
{
    bool       t_bRet = true;
    //�ӽ����ڣ��̻߳�����  begin
    //1. ��m_mutexShmList����r_dbName��Ӧ����Ϣ
    pthread_mutex_lock(&(MDBLatchShmMgr::m_innerMutex));
    try
    {
        vector<LatchShmInfo>::iterator t_itr;
        t_itr =::lower_bound(m_latchShmList.begin(), m_latchShmList.end(), r_shminfo);
        if (t_itr == m_latchShmList.end() || !(*t_itr == r_shminfo))
        {
            if (createLatchShm(r_shminfo) == false)
            {
#ifdef _DEBUG_
                cout << "createLatchShm false!" << __FILE__ << __LINE__ << endl;
#endif
                t_bRet = false;
            }
            else
            {
                if (setProcInfo(r_shminfo, r_procInfo) == false)
                {
                    t_bRet = false;
#ifdef _DEBUG_
                    cout << "setProcInfo false!" << __FILE__ << __LINE__ << endl;
#endif
                }
                m_latchShmList.insert(t_itr, r_shminfo);
            }
        }
        else
        {
            r_shminfo.setaddress(t_itr->m_shmaddr);
            if (getProcInfo(r_shminfo, r_procInfo) == false)
            {
                if (setProcInfo(r_shminfo, r_procInfo) == false)
                {
                    t_bRet = false;
#ifdef _DEBUG_
                    cout << "setProcInfo false!" << __FILE__ << __LINE__ << endl;
#endif
                }
            }
        }
    }
    catch (Mdb_Exception& e)
    {
#ifdef _DEBUG_
        e.toString();
        cout << __FILE__ << __LINE__ << endl;
#endif
        pthread_mutex_unlock(&(MDBLatchShmMgr::m_innerMutex));
        return false;
    }
    //�ӽ����ڣ��̻߳�����  end
    pthread_mutex_unlock(&(MDBLatchShmMgr::m_innerMutex));
    return t_bRet;
}

bool MDBLatchShmMgr::createLatchShm(LatchShmInfo& r_shminfo)
{
    T_FILENAMEDEF  t_fileName;
    size_t         t_shmSize;
    bool           t_createFlag = false;
    int            file_des;
    sprintf(t_fileName, "%s/ctl/%s.lock", getenv(MDB_HOMEPATH.c_str()), r_shminfo.m_dbName);
    if (access(t_fileName, F_OK) != 0)
    {
        file_des = creat(t_fileName, S_IRUSR | S_IWUSR);
        if (access(t_fileName, F_OK) != 0)
        {
            throw Mdb_Exception(__FILE__, __LINE__, "�ļ�{%s}�����ڲ��޷�����!", t_fileName);
        }
        if (fchmod(file_des, 0666) != 0)
        {
            throw Mdb_Exception(__FILE__, __LINE__, "�޸��ļ�{%s}Ȩ��ʧ��!", t_fileName);
        }
        close(file_des);
    }
    r_shminfo.computeSize();
    key_t  t_pkey = ftok(t_fileName, LATCH_CONST::LATCHSH_FTOK_ID);
    // over 2010-5-12 10:33
    if (t_pkey < 0)
    {
#ifdef _DEBUG_
        cout << "t_pkey = " << t_pkey << endl;
        cout << "t_fileName is {" << t_fileName << "} " << __FILE__ << __LINE__ << endl;
        cout << "errno=" << errno << " " << strerror(errno) << endl;
#endif
        return false;
    }
    int    t_shmid;
    if ((t_shmid = shmget(t_pkey, r_shminfo.m_shmsize, SHM_MODEFLAG)) == -1)
    {
        // add by chenm 2009-2-19 15:40:21 ���mdb���Ʋ����� �򲻴������������ڴ�
        T_FILENAMEDEF  t_fileName;
        sprintf(t_fileName, "%s/config/%s.conf", getenv(MDB_HOMEPATH.c_str()), r_shminfo.m_dbName);
        if (access(t_fileName, F_OK) != 0)
        {
            throw Mdb_Exception(__FILE__, __LINE__, "���ݿ�{%s}������!", t_fileName);
        }
        // over 2009-2-19 15:40:29
        t_shmid = shmget(t_pkey, r_shminfo.m_shmsize, IPC_CREAT | 0666);
        if (t_shmid < 0)
        {
            if (errno == EEXIST)
            {
                sleep(1);
                if ((t_shmid = shmget(t_pkey, r_shminfo.m_shmsize, SHM_MODEFLAG)) == -1)
                {
#ifdef _DEBUG_
                    cout << "error here " << __FILE__ << __LINE__ << endl;
#endif
                    return false;
                }
            }
            else
            {
#ifdef _DEBUG_
                cout << "error here " << __FILE__ << __LINE__ << endl;
#endif
                return false;
            }
        }
        else
        {
            t_createFlag = true;
        }
    }
    r_shminfo.setaddress((char*)shmat(t_shmid, NULL, 0));
    if (r_shminfo.m_shmaddr == (void*) - 1)
    {
        r_shminfo.m_shmaddr = NULL;
#ifdef _DEBUG_
        cout << "error here " << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    //״̬�ж�
    if (t_createFlag == true)
    {
        //��ʼ��
        memset(r_shminfo.m_shmaddr, 0, r_shminfo.m_shmsize);
        //����״̬
        memcpy(r_shminfo.m_shmaddr, (char*)"1", 1);
    }
    else
    {
        while (memcmp(r_shminfo.m_shmaddr, (char*)"1", 1) != 0)
        {
            usleep(1);
        }
    }
    return true;
}

bool MDBLatchShmMgr::deleteLatchShm(LatchShmInfo& r_shminfo)
{
    vector<LatchShmInfo>::iterator t_itr;
    t_itr =::lower_bound(m_latchShmList.begin(), m_latchShmList.end(), r_shminfo);
    if (t_itr == m_latchShmList.end() || !(*t_itr == r_shminfo))
    {
        //û�ж�Ӧ�ĸ����ڴ���Ϣ,����detach
    }
    else
    {
        //��detach
        shmdt(t_itr->m_shmaddr);
        m_latchShmList.erase(t_itr);
    }
    key_t t_key;
    int   t_shmid, t_ret;
    //��ȡt_key
    T_FILENAMEDEF  t_fileName;
    sprintf(t_fileName, "%s/ctl/%s.lock", getenv(MDB_HOMEPATH.c_str()), r_shminfo.m_dbName);
    t_key = ftok(t_fileName, LATCH_CONST::LATCHSH_FTOK_ID);
    if (t_key < 0) return false;
    r_shminfo.computeSize();
    //��ȡt_shmid
    t_shmid = shmget(t_key, r_shminfo.m_shmsize, 0666);
    if (t_shmid < 0) return false;
    t_ret   = shmctl(t_shmid, IPC_RMID, (struct shmid_ds*)0);
    if (t_ret < 0)
    {
#ifdef _DEBUG_
        cout << "�ͷŹ����ڴ�ʧ��!"
             << " " << __FILE__ << ":" << __LINE__ << endl;
#endif
        return false;
    }
    return true;
}

bool MDBLatchShmMgr::setProcInfo(LatchShmInfo& r_shminfo, ProcessLatchInfo* &r_procInfo)
{
    LatchNode*         t_pshmlatch = r_shminfo.m_mlatchaddr;
    ProcessLatchInfo   t_procInfo;
    ProcessLatchInfo*  t_pProcInfo = r_shminfo.m_procAddr + 1;
    t_procInfo.init();
    while (latch_lock(t_pshmlatch->m_latchlock, LATCH_CONST::SLATCH_OUTTIME) == false)
    {
        latch_unlock(t_pshmlatch->m_latchlock);
    }
    for (int i = 1; i < LATCH_CONST::MAX_LATCHPROC_NUM; ++i)
    {
        if (t_pProcInfo->m_state == 0)
        {
            t_procInfo.m_pos = i;
            memcpy(t_pProcInfo, &t_procInfo, sizeof(ProcessLatchInfo));
            r_procInfo = t_pProcInfo;
            latch_unlock(t_pshmlatch->m_latchlock);
            return true;
        }
        else
        {
            t_pProcInfo++;
            continue;
        }
    }
    latch_unlock(t_pshmlatch->m_latchlock);
    return false;
}
bool MDBLatchShmMgr::getProcInfo(LatchShmInfo& r_shminfo, ProcessLatchInfo* &r_procInfo)
{
    pthread_t t_thid  = pthread_self();
    return getProcInfo(r_shminfo, t_thid, r_procInfo);
}

bool MDBLatchShmMgr::getProcInfo(LatchShmInfo& r_shminfo, const pthread_t r_threadid,
                                 ProcessLatchInfo* &r_procInfo)
{
    pid_t     t_pid   = getpid();
    LatchNode*         t_pshmlatch = r_shminfo.m_mlatchaddr;
    ProcessLatchInfo*  t_pProcInfo = r_shminfo.m_procAddr + 1;
    int t_trycount = 0;
    //100ms ����ס��ǿ�ƽ���
    while (latch_lock(t_pshmlatch->m_latchlock, LATCH_CONST::SLATCH_OUTTIME) == false)
    {
        latch_unlock(t_pshmlatch->m_latchlock);
    }
    for (int i = 1; i < LATCH_CONST::MAX_LATCHPROC_NUM; ++i)
    {
        if (t_pProcInfo->m_state != 0 &&
                t_pProcInfo->m_pid == t_pid &&
                t_pProcInfo->m_thid == r_threadid)
        {
            r_procInfo = t_pProcInfo;
            latch_unlock(t_pshmlatch->m_latchlock);
            return true;
        }
        else
        {
            t_pProcInfo++;
            continue;
        }
    }
    latch_unlock(t_pshmlatch->m_latchlock);
    return false;
}

bool MDBLatchShmMgr::delProcInfo(const char* r_dbname, const pthread_t r_threadid)
{
    bool          t_flag = false;
    //1. �ҵ����е��б����ҵ���Ӧ�ڴ���Ϣ
    pthread_mutex_lock(&(MDBLatchShmMgr::m_innerMutex));
    vector<LatchShmInfo>::iterator t_itr;
    for (t_itr = MDBLatchShmMgr::m_latchShmList.begin();
            t_itr != MDBLatchShmMgr::m_latchShmList.end(); ++t_itr)
    {
        if (strcasecmp(t_itr->m_dbName, r_dbname) == 0)
        {
            t_flag = true;
            break;
        }
    }
    pthread_mutex_unlock(&(MDBLatchShmMgr::m_innerMutex));
    if (t_flag == false) return false;
    ProcessLatchInfo* t_procInfo;
    if (getProcInfo(*t_itr, r_threadid, t_procInfo) == true)
    {
        LatchNode* t_latchnode = NULL;
        if (t_procInfo->m_slatchinfo > 0)
        {
            //slatch û���ͷŸɾ������ͷ�
            t_latchnode = t_itr->m_slatchaddr + t_procInfo->m_slatchinfo;
            slatch_unlock(t_latchnode, (int)(t_procInfo->m_slatchinfo), t_procInfo);
        }
        //mlatch û���ͷŸɾ������ͷ�
        for (int t_i = 0; t_i < t_procInfo->m_mlatchnum; ++t_i)
        {
            t_latchnode = t_itr->m_mlatchaddr + t_procInfo->m_mlatchinfo[t_procInfo->m_slatchinfo];
            mlatch_unlock(t_latchnode, (int)(t_procInfo->m_mlatchinfo[t_procInfo->m_slatchinfo]), t_procInfo);
        }
        //��״̬��Ϊ0��ʾ�ͷ�
        _clear_lock((atomic_p) & (t_procInfo->m_state), 0);
        return true;
    }
    return false;
}

int MDBLatchShmMgr::slatch_lock(LatchNode* r_platch, const int& r_srcid, ProcessLatchInfo* r_procInfo)
{
    if (r_procInfo->m_slatchinfo != 0)
    {
#ifdef _DEBUG_
        cout << "SLATCH����ͬʱռ�ö��!" << __FILE__ << __LINE__ << endl;
#endif
        return -1; //�쳣
    }
    if (latch_lock(r_platch->m_latchlock, LATCH_CONST::SLATCH_OUTTIME) == false)
    {
        //��ʱ���ܻ�ȡ����
        return 0;
    }
    r_platch->m_proclab = r_procInfo->m_pos;
    r_procInfo->m_slatchinfo = r_srcid;
    time(&(r_procInfo->m_time));
    return 1;
}
void MDBLatchShmMgr::slatch_unlock(LatchNode* r_platch, const int& r_srcid, ProcessLatchInfo* r_procInfo)
{
    if (r_procInfo->m_slatchinfo != r_srcid) return; //�Ƕ�Ӧ��Դ�������ܽ�
    r_platch->m_proclab = 0;
    latch_unlock(r_platch->m_latchlock);
    r_procInfo->m_slatchinfo = 0;
    time(&(r_procInfo->m_time));
}

int MDBLatchShmMgr::mlatch_lock(LatchNode* r_platch, const int& r_srcid, ProcessLatchInfo* r_procInfo,
                                const size_t t_outtime)
{
    if ((r_srcid == 0) || (r_procInfo->m_mlatchnum >= LATCH_CONST::MAX_LATCH_NUM))
    {
#ifdef _DEBUG_
        cout << "r_srcid=" << r_srcid << " r_procInfo->m_mlatchnum=" << r_procInfo->m_mlatchnum
             << " ����(idΪ0����Դռ�ù���)!" << __FILE__ << __LINE__ << endl;
#endif
        for (int t_s = 0; t_s < r_procInfo->m_mlatchnum; ++t_s)
        {
            cout << "���:" << t_s << " srcid=" << r_procInfo->m_mlatchinfo[t_s] << endl;
        }
        return -1; //r_srcid����Ϊ0 ��ͬʱռ����Դ���� �쳣
    }
    if (latch_lock(r_platch->m_latchlock, t_outtime) == false)
    {
        //��ʱ��ȡ������
        return 0;
    }
    pthread_mutex_lock(&(MDBLatchShmMgr::m_innerMutex));
    r_platch->m_proclab = r_procInfo->m_pos;
    for (int i = 0; i < r_procInfo->m_mlatchnum; ++i)
    {
        if (r_procInfo->m_mlatchinfo[i] == 0)
        {
            r_procInfo->m_mlatchinfo[i] = r_srcid;
            time(&(r_procInfo->m_time));
            pthread_mutex_unlock(&(MDBLatchShmMgr::m_innerMutex));
            return 1;
        }
    }
    r_procInfo->m_mlatchinfo[r_procInfo->m_mlatchnum] = r_srcid;
    time(&(r_procInfo->m_time));
    ++(r_procInfo->m_mlatchnum);
    pthread_mutex_unlock(&(MDBLatchShmMgr::m_innerMutex));
    return 1;
}
void MDBLatchShmMgr::mlatch_unlock(LatchNode* r_platch, const int& r_srcid, ProcessLatchInfo* r_procInfo)
{
    //�ͷŹ�����Դ��
    r_platch->m_proclab = 0;
    latch_unlock(r_platch->m_latchlock);
    pthread_mutex_lock(&(MDBLatchShmMgr::m_innerMutex));
    //�ͷŽ�����Ϣ
    int i = 0;
    for (; i < r_procInfo->m_mlatchnum; ++i)
    {
        if (r_procInfo->m_mlatchinfo[i] == r_srcid)
        {
            break;
        }
    }
    if (i >= r_procInfo->m_mlatchnum)
    {
        pthread_mutex_unlock(&(MDBLatchShmMgr::m_innerMutex));
        return;
    }
    for (; i < r_procInfo->m_mlatchnum - 1; ++i)
    {
        memcpy(&(r_procInfo->m_mlatchinfo[i]), &(r_procInfo->m_mlatchinfo[i + 1]), sizeof(int));
    }
    --(r_procInfo->m_mlatchnum);
    pthread_mutex_unlock(&(MDBLatchShmMgr::m_innerMutex));
}

//timeout Ϊѭ����΢����,lock�ɹ�����true ���򷵻�false
bool MDBLatchShmMgr::latch_lock(volatile int& r_lvalue, const size_t timeout)
{
    //Ϊ�����Ч�ʵ�һ�������Ͳ���Ҫ����t_trycount����
    if (_check_lock((atomic_p)&r_lvalue, 0, 1) == false) return true;
    size_t t_trycount = 0;
    do
    {
        if (t_trycount++ >= (timeout / 10)) return false;
        else usleep(10);
    }
    while (_check_lock((atomic_p)&r_lvalue, 0, 1) == true);
    return true;
}
void MDBLatchShmMgr::latch_unlock(volatile int& r_lvalue)
{
    _clear_lock((atomic_p)&r_lvalue, 0);
}
void MDBLatchShmMgr::dumpShmInfo(const LatchShmInfo& r_shminfo)
{
    cout << "--------------���������ڴ������Ϣ-------------" << endl;
    cout << "  r_shminfo.m_dbName   =" << r_shminfo.m_dbName << endl;
    cout << "  r_shminfo.m_mLatchNum=" << r_shminfo.m_mLatchNum << endl;
    cout << "  r_shminfo.m_sLatchNum=" << r_shminfo.m_sLatchNum << endl;
    cout << "  r_shminfo.m_shmsize  =" << r_shminfo.m_shmsize << endl;
    cout << "-------------MLATCH������Ϣ----------------" << endl;
    int i = 0;
    for (i = 0; i < r_shminfo.m_mLatchNum; ++i)
    {
        LatchNode* t_pmnode = r_shminfo.m_mlatchaddr + i;
        if (t_pmnode->m_proclab == 0) continue;
        cout << "  sid=" << i << " proclab=" << t_pmnode->m_proclab << endl;
    }
    cout << "------------SLATCH������Ϣ----------------" << endl;
    for (i = 0; i < r_shminfo.m_sLatchNum; ++i)
    {
        LatchNode* t_pnsode = r_shminfo.m_slatchaddr + i;
        if (t_pnsode->m_proclab == 0) continue;
        cout << "  sid=" << i << " proclab=" << t_pnsode->m_proclab << endl;
    }
    cout << "------------��������Ϣ----------------" << endl;
    for (i = 0; i < r_shminfo.m_procNum; ++i)
    {
        ProcessLatchInfo* t_procinfo = r_shminfo.m_procAddr + i;
        if (t_procinfo->m_state == 0) continue;
        cout << "  m_state=" << t_procinfo->m_state << " proclab=" << t_procinfo->m_pos << " m_pid=" << t_procinfo->m_pid
             << " m_time=" << t_procinfo->m_time << " m_slatchinfo=" << t_procinfo->m_slatchinfo << endl;
        for (int j = 0; j < t_procinfo->m_mlatchnum; ++j)
        {
            cout << "    m_mlatchinfo[" << j << "]=" << t_procinfo->m_mlatchinfo[j] << endl;
        }
    }
    cout << "--------------������Ϣ���------------------" << endl;
}

