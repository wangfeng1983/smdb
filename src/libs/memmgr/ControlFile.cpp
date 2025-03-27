#include "ControlFile.h"
#include <unistd.h>
#include <stdio.h>
#include <cstdlib>

//创建数据库控制文件
bool ControlFile::createFile(const Mdb_Config& r_mdbConfig)
{
    //1. 根据数据库名求取对应的控制文件名
    memcpy(&m_ctlInfo, &(r_mdbConfig.m_ctlInfo), sizeof(MdbCtlInfo));
    sprintf(m_fileName, "%s/ctl/%s.ctl", getenv(MDB_HOMEPATH.c_str()), m_ctlInfo.m_dbName);
    //2. 生成对应的文件，将控制信息写入文件中
    if (dumpCtlInfo() == false)
    {
#ifdef _DEBUG_
        cout << "dumpCtlInfo() false!" << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    return true;
}
//导出控制信息
bool ControlFile::dumpCtlInfo()
{
    //如果文件已经存在则报错 add by gaojf 2010-5-13 10:40
    if (access(m_fileName, F_OK) == 0)
    {
#ifdef _DEBUG_
        cout << "File {" << m_fileName << "} already exists!" << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    //over 2010-5-13 10:41
    FILE*   t_fp;
    if ((t_fp = fopen(m_fileName, "wb")) == NULL)
    {
#ifdef _DEBUG_
        cout << "open file {" << m_fileName << "} error!" << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    if (fwrite(&m_ctlInfo, sizeof(MdbCtlInfo), 1, t_fp) != 1)
    {
        fclose(t_fp);
#ifdef _DEBUG_
        cout << "fwrite file {" << m_fileName << "} error!" << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    fclose(t_fp);
    return true;
}
//初始化,从控制文件中获取控制信息
bool ControlFile::initialize(const char* r_dbName)
{
    FILE*   t_fp;
    //1. 根据数据库名求取对应的控制文件名
    sprintf(m_fileName, "%s/ctl/%s.ctl", getenv(MDB_HOMEPATH.c_str()), r_dbName);
    if ((t_fp = fopen(m_fileName, "rb")) == NULL)
    {
#ifdef _DEBUG_
        cout << "open file {" << m_fileName << "} error!" << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    if (fread(&m_ctlInfo, sizeof(MdbCtlInfo), 1, t_fp) != 1)
    {
        fclose(t_fp);
#ifdef _DEBUG_
        cout << "fread file {" << m_fileName << "} error!" << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    fclose(t_fp);
    return true;
}
MdbCtlInfo* ControlFile::getCtlInfoPt()
{
    return &m_ctlInfo;
}
