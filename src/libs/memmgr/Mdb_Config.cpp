#include "Mdb_Config.h"
#if defined(_TYPE_CMCC)
#include "uconfig/SysParam.h"
#else
#include "base/SysParam.h"
#endif
#include <stdio.h>
#include <cstdlib>
#include "RedoLog.h"
#include "debuglog.h"

int RedoInfo::m_redosize;

ostream& operator<<(ostream& os, const MdbCtlInfo& r_obj)
{
    os << "m_dbName        =" << r_obj.m_dbName << endl;
    os << "m_ctlSpaceName  =" << r_obj.m_ctlSpaceName << endl;
    os << "m_size          =" << r_obj.m_size << endl;
    os << "m_pageSize      =" << r_obj.m_pageSize << endl;
    os << "m_gparamMaxNum  =" << r_obj.m_gparamMaxNum << endl;
    os << "m_gparamMaxSpNum=" << r_obj.m_gparamMaxSpNum << endl;
    os << "m_tableMaxNum   =" << r_obj.m_tableMaxNum << endl;
    os << "m_indexMaxNum   =" << r_obj.m_indexMaxNum << endl;
    os << "m_sessionMaxNum =" << r_obj.m_sessionMaxNum << endl;
    os << "m_idxHashSpName =" << r_obj.m_idxHashSpaceName << endl;
    os << "m_undoSpaceName =" << r_obj.m_undoSpaceName << endl;
    os << "m_logPath       =" << r_obj.m_logPath << endl;
    os << "m_lockfile      =" << r_obj.m_lockfile << endl;
    os << "m_datapath[0]   =" << r_obj.m_datapath[0] << endl;
    os << "m_datapath[1]   =" << r_obj.m_datapath[1] << endl;
    //os<<"m_undobuffsize  ="<<r_obj.m_undobuffsize<<endl;
    os << "m_undohashsize  =" << r_obj.m_undohashsize << endl;
    return os;
}
bool MdbCtlInfo::getCtlInfo(const char* r_dbName, SysParam* pSysParam)
{
    strcpy(m_dbName, r_dbName);
    char   t_tmp[128 + 1];
    string t_rootpath;
    map<string, string> t_keyValues;
    sprintf(t_tmp, "%c%s%cMdbCtlInfo", SECTDELIM, m_dbName, SECTDELIM);
    t_rootpath = t_tmp;
    if (pSysParam->getValue(t_rootpath, t_keyValues) == false)
    {
        log_error("getConfigValue false!{" << t_rootpath << "}");
        return false;
    }
    sprintf(m_ctlSpaceName, "%s", t_keyValues["CTLSPACENAME"].c_str());
    m_size = atol(t_keyValues["CTLSPACESIZE"].c_str()) * 1024 * 1024;
    m_pageSize = atol(t_keyValues["PAGESIZE"].c_str()) * 1024 * 1024;
    m_gparamMaxNum = atoi(t_keyValues["GPARAMMAXNUM"].c_str());
    m_gparamMaxSpNum = atoi(t_keyValues["SPACEMAXNUM"].c_str());
    m_tableMaxNum = atoi(t_keyValues["TABLEMAXNUM"].c_str());
    m_indexMaxNum = atoi(t_keyValues["INDEXMAXNUM"].c_str());
    m_sessionMaxNum = atoi(t_keyValues["SESSIONMAXNUM"].c_str());
    sprintf(m_lockpath, "%s", t_keyValues["DBLOCKPATH"].c_str());
    sprintf(m_logPath, "%s", t_keyValues["LOGPATH"].c_str());
    sprintf(m_lockfile, "%s/%s.lock", t_keyValues["DBLOCKPATH"].c_str(), r_dbName);
    sprintf(m_idxHashSpaceName, "%s", t_keyValues["INDEXSPACE_HASH"].c_str());
    sprintf(m_undoSpaceName, "%s", t_keyValues["UNDOSPACE_NAME"].c_str());
    sprintf(m_datapath[0], "%s", t_keyValues["DATAPATH0"].c_str());
    sprintf(m_datapath[1], "%s", t_keyValues["DATAPATH1"].c_str());
    //m_undobuffsize = atol(t_keyValues["UNDO_BUFFSIZE"].c_str())*1024*1024;
    m_undohashsize = atol(t_keyValues["UNDO_HASHSIZE"].c_str()) * 1024 * 1024;
    if (m_size <= 0           ||
            m_pageSize <= 0            || m_gparamMaxNum <= 0   ||
            m_gparamMaxSpNum <= 0      || m_tableMaxNum <= 0    ||
            m_indexMaxNum <= 0         || m_sessionMaxNum <= 0  ||
            strlen(m_logPath) <= 0)
    {
        log_error("配置项缺失或值不规范 false!" << t_rootpath);
        return false;
    }
    return true;
}

bool Mdb_Config::getConfigInfo(const char* r_dbName)
{
    //获取配置文件名称
    T_FILENAMEDEF t_configFile;
    sprintf(t_configFile, "%s/config/%s.conf", getenv(MDB_HOMEPATH.c_str()), r_dbName);
    //读取配置文件
    SysParam* t_pSysParam = NULL;
#if defined(_TYPE_CMCC)
    t_pSysParam = SysParam::getNewInstance();
#else
    t_pSysParam = new SysParam();
#endif
    t_pSysParam->initialize(t_configFile);
    bool   t_bRet = getConfigInfoLocal(r_dbName, t_pSysParam);
    t_pSysParam->closeSection();
    delete t_pSysParam;
    t_pSysParam = NULL;
    return t_bRet;
}

//从配置文件中读取配置信息
//配置文件$MDB_HOMEPATH/config/dbName.conf
bool Mdb_Config::getConfigInfoLocal(const char* r_dbName, SysParam* r_pSysParam)
{
    bool   t_bRet = true;
    string t_path, t_name, t_value;
    map<string, string> t_keyValues;
    if (m_ctlInfo.getCtlInfo(r_dbName, r_pSysParam) == false)
    {
        log_error("m_ctlInfo.getCtlInfo(r_pSysParam) false!");
        return false;
    }
    /*
    cout<<"-----m_ctlInfo---------------"<<endl;
    cout<<m_ctlInfo;
    cout<<"-----------------------------"<<endl;
    */
    char  t_tmp[128 + 1];
    //表空间定义信息（包括索引表空间定义信息和数据表空间定义信息）
    sprintf(t_tmp, "%c%s%cSpaceDefInfos", SECTDELIM, r_dbName, SECTDELIM);
    string t_spaceRoot = t_tmp;
    string t_spaceName;
    vector<string> t_spaceList;
    SpaceInfo t_spaceInfo;
    int       t_spaceType;
    m_spaceInfoList.clear();
    strcpy(t_spaceInfo.m_dbName, r_dbName);
    t_spaceInfo.m_pageSize = m_ctlInfo.m_pageSize;
    if (r_pSysParam->setSectionPath(t_spaceRoot) == false)
    {
        log_error("r_pSysParam->setSectionPath(" << t_spaceRoot << ") false!");
        return false;
    }
    while (r_pSysParam->getSubSection(t_spaceName) == true)
    {
        t_spaceList.push_back(t_spaceName);
    }
    for (vector<string>::iterator t_spSItr = t_spaceList.begin();
            t_spSItr != t_spaceList.end(); t_spSItr++)
    {
        t_path = t_spaceRoot + SECTDELIM + *t_spSItr;
        t_keyValues.clear();
        if (r_pSysParam->getValue(t_path, t_keyValues) == false)
        {
            log_error("getConfigValue false!{" << t_path << "}");
            return false;
        }
        sprintf(t_spaceInfo.m_spaceName, "%s", t_spSItr->c_str());
        t_spaceInfo.m_size = atol(t_keyValues["SPACESIZE"].c_str()) * 1024 * 1024;
        t_spaceType        = atoi(t_keyValues["SPACETYPE"].c_str());
        t_spaceInfo.m_type = (T_SPACETYPE)t_spaceType;
        size_t t_pageszie = atol(t_keyValues["PAGESIZE"].c_str()) * 1024;
        if (t_pageszie > 0)
        {
            t_spaceInfo.m_pageSize = t_pageszie;
        }
        sprintf(t_spaceInfo.m_fileName, "%s.data", t_spaceInfo.m_spaceName);
        m_spaceInfoList.push_back(t_spaceInfo);
    }
    //redo log
    sprintf(t_tmp, "%c%s%cREDO", SECTDELIM, r_dbName, SECTDELIM);
    t_path = t_tmp;
    if (r_pSysParam->getValue(t_path, t_keyValues) == false)
    {
        log_error("getConfigValue false!{" << t_path << "}");
        return false;
    }
    //LOGWRITER_CHANNEL_LIST
    char tChnLst[200];
    memset(tChnLst, 0x0, sizeof(tChnLst));
    strcpy(tChnLst, t_keyValues["LOGWRITER_CHANNEL_LIST"].c_str());
    int chn = 0;
    m_redoInfo.m_chnlst.clear();
    tChnLst[strlen(tChnLst)] = ',';
    for (int i = 0; i < sizeof(tChnLst); i++)
    {
        if (tChnLst[i] != ',')
            chn = chn * 10 + (tChnLst[i] - 48) ;
        else
        {
            m_redoInfo.m_chnlst.push_back(chn);
            chn = 0;
        }
        if (tChnLst[i] == 0x0)
            break;
    }
    strcpy(m_redoInfo.m_path, t_keyValues["PATH"].c_str());
    m_redoInfo.m_logfilesize = atoi(t_keyValues["LOGFILE_SIZE"].c_str());
    m_redoInfo.m_groupcount = atoi(t_keyValues["GROUP_COUNT"].c_str());
    m_redoInfo.m_buffercount = atoi(t_keyValues["BUFFER_COUNT"].c_str());
    m_redoInfo.m_buffersize = atoi(t_keyValues["BUFFER_SIZE"].c_str());
    strcpy(m_redoInfo.m_writemode, t_keyValues["WRITE_MODE"].c_str());
    m_redoInfo.m_syncmode = atoi(t_keyValues["SYNC_MODE"].c_str());
    //计算redobuffer大小
    m_redoInfo.m_redosize = (sizeof(RedoLogBuffer::Header) + m_redoInfo.m_buffersize * 1024
                             + sizeof(LGWRRunStatus::RunStatus)) * m_redoInfo.m_buffercount;
#ifdef _DEBUG_
    for (vector<SpaceInfo>::iterator t_spItr = m_spaceInfoList.begin();
            t_spItr != m_spaceInfoList.end(); t_spItr++)
    {
        cout << "--------------------------------------" << endl;
        cout << "spaceName = " << t_spItr->m_spaceName << endl;
        cout << "spaceSize = " << t_spItr->m_size << endl;
        cout << "spaceType = " << t_spItr->m_type << endl;
        cout << "pageSize  = " << t_spItr->m_pageSize << endl;
        cout << "--------------------------------------" << endl;
    }
#endif
    if (check() == false)
    {
        log_error("check() failed");
        return false;
    }
    return true;
}
bool Mdb_Config::check()
{
    bool t_bRet1 = false, t_bRet2 = false;
    for (vector<SpaceInfo>::iterator t_spItr = m_spaceInfoList.begin();
            t_spItr != m_spaceInfoList.end(); t_spItr++)
    {
        if (strcasecmp(m_ctlInfo.m_idxHashSpaceName, t_spItr->m_spaceName) == 0)
        {
            t_bRet1 = true;
        }
        else if (strcasecmp(m_ctlInfo.m_undoSpaceName, t_spItr->m_spaceName) == 0)
        {
            //2. 检查Undo表空间是否定义
            t_bRet2 = true;
        }
        else continue;
        if (t_bRet1 && t_bRet2) break;
    }

    if (t_bRet1 && t_bRet2)
    {
        return true;
    } else {
        log_error("t_bRet1=" << t_bRet1 << " t_bRet2=" << t_bRet2);
        return false;
    }
}

