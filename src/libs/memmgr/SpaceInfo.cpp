#include "SpaceInfo.h"
#include <sys/time.h>
#include <unistd.h>


///<首次创建时初始化
//r_flag 0 首次创建初始化，其他 启动时初始化
void MDbGlobalInfo::init(const int r_flag)
{
    unsigned int t_nextspcode = m_nextSpCode;
    memset(this, 0, sizeof(MDbGlobalInfo));
    updateDbTime();
    updateSpTime();
    updateTbTime();
    updateIdxTime();
    //m_nextSpCode=2;
    //m_curSessionId=0;
    m_nextSpCode = (r_flag == 0) ? 2 : t_nextspcode;
}
void MDbGlobalInfo::updateDbTime()
{
    time(&m_dbTime);
}
void MDbGlobalInfo::updateSpTime()
{
    time(&m_spUpTime);
}
void MDbGlobalInfo::updateTbTime()
{
    time(&m_tbUpTime);
}
void MDbGlobalInfo::updateIdxTime()
{
    time(&m_idxUpTime);
}
unsigned int MDbGlobalInfo::getNextSpCode()
{
    m_nextSpCode++;
    return m_nextSpCode - 1;
}
void MDbGlobalInfo::setReserve(const size_t& r_offSet)
{
    m_reserve = r_offSet;
}

short int MDbGlobalInfo::getsemmark()
{
    int semmark = 0;
    unsigned char semstep[8] = { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};
    int i = 0;
    for (; i < LOCKWAITS_MASK_NUM; i++)
    {
        if (m_semmark[i] != 0xFF)
            break;
    }
    if (i == LOCKWAITS_MASK_NUM)
    {
        semmark = LOCKWAITS_SEM_MAX;
        return semmark;
    }
    for (int j = 0; j < 8; j++)
    {
        if ((m_semmark[i]&semstep[j]) == 0)
        {
            m_semmark[i] |= semstep[j];
            semmark = i * 8 + j;
            break;
        }
    }
    return semmark;
}

bool MDbGlobalInfo::ersemmark(short int semmark)
{
    unsigned char semstep[8] = { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};
    if ((semmark < 0) || (semmark >= LOCKWAITS_SEM_MAX))
        return false;
    int i = semmark / 8;
    int j = semmark % 8;
    m_semmark[i] &= ~semstep[j];
    return true;
}

void  MDbGlobalInfo::getSessionId(int& sessionid, short int& semmark)
{
    int t_sId = -1;
    for (t_sId = m_curSessionId; t_sId < MAX_SESSION_NUM; t_sId++)
    {
        if (m_sidState[t_sId] == 0)
        {
            m_sidState[t_sId] = getsemmark() + 1;
            m_curSessionId = t_sId;
            sessionid = t_sId;
            semmark = m_sidState[t_sId];
            return ;
        }
    }
    for (t_sId = 0; t_sId < m_curSessionId; t_sId++)
    {
        if (m_sidState[t_sId] == 0)
        {
            m_sidState[t_sId] = getsemmark() + 1;
            m_curSessionId = t_sId;
            sessionid = t_sId;
            semmark = m_sidState[t_sId];
            return ;
        }
    }
    sessionid = -1;
}
//释放SessionId
void MDbGlobalInfo::releadSid(const int& r_sid)
{
    if (r_sid >= 0 && r_sid < MAX_SESSION_NUM)
    {
        ersemmark(m_sidState[r_sid] - 1);
        m_sidState[r_sid] = 0;
    }
    return;
}

void SpaceInfo::dumpInfo(ostream& r_os)
{
    r_os << "m_dbName   =" << m_dbName << endl;
    r_os << "m_fileName =" << m_fileName << endl;
    r_os << "m_size     =" << m_size << endl;
    r_os << "m_pageSize =" << m_pageSize << endl;
    r_os << "m_type     =" << m_type << endl;
    r_os << "m_spaceCode=" << m_spaceCode << endl;
    r_os << "m_shmKey   =" << m_shmKey << endl;
}
void GlobalParam::dumpInfo(ostream& r_os)
{
    r_os << "m_paramName=" << m_paramName << endl;
    r_os << "m_value    =" << m_value << endl;
}
void SessionInfo::dumpInfo(ostream& r_os)
{
    r_os << "m_pId      =" << m_pId << endl;
    r_os << "m_sessionId=" << m_sessionId << endl;
    r_os << "m_time     =" << m_time << endl;
    r_os << "m_ipAddr   =" << m_ipAddr << endl;
}

//r_flag 为1 自动+1  0不加
///<获取SCN号，并自动+1 add by gaojf MDB2.0
void MDbGlobalInfo::getscn(T_SCN& r_scn, const int r_flag)
{
    if (_check_lock((atomic_p)&m_scnlacth, 0, 1) == true)
    {
        time_t t_stime0, t_nowtime;
        time(&t_stime0);
        while (_check_lock((atomic_p)&m_scnlacth, 0, 1) == true)
        {
            usleep(100); //休眠100us=0.1ms
            time(&t_nowtime);
            if (difftime(t_nowtime, t_stime0) >= 10)
            {
                //超过10s 判为 超时
                _clear_lock((atomic_p)&m_scnlacth, 0);
            }
        }
    }
    if (r_flag == 1) ++m_scn;
    r_scn.setOffSet(m_scn);
    _clear_lock((atomic_p)&m_scnlacth, 0);
}

///<获取Transid并自动+1
void MDbGlobalInfo::gettid(long& transid)
{
    if (_check_lock((atomic_p)&m_tidlatch, 0, 1) == true)
    {
        time_t t_stime0, t_nowtime;
        time(&t_stime0);
        while (_check_lock((atomic_p)&m_tidlatch, 0, 1) == true)
        {
            usleep(100); //休眠100us=0.1ms
            time(&t_nowtime);
            if (difftime(t_nowtime, t_stime0) >= 10)
            {
                //超过10s 判为 超时
                _clear_lock((atomic_p)&m_tidlatch, 0);
            }
        }
    }
    m_transid++;
    transid = m_transid;
    _clear_lock((atomic_p)&m_tidlatch, 0);
}

void MDbGlobalInfo::getlastckpscn(T_SCN& r_scn)
{
    //防止获取的时候，正在更新
    while (!(r_scn == m_lastckpscn))
    {
        r_scn = m_lastckpscn;
    }
}
void MDbGlobalInfo::setlastckpscn(const T_SCN& r_scn)
{
    m_lastckpscn = r_scn;
}

