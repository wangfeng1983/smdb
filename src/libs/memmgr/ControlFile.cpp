#include "ControlFile.h"
#include <unistd.h>
#include <stdio.h>
#include <cstdlib>

//�������ݿ�����ļ�
bool ControlFile::createFile(const Mdb_Config& r_mdbConfig)
{
    //1. �������ݿ�����ȡ��Ӧ�Ŀ����ļ���
    memcpy(&m_ctlInfo, &(r_mdbConfig.m_ctlInfo), sizeof(MdbCtlInfo));
    sprintf(m_fileName, "%s/ctl/%s.ctl", getenv(MDB_HOMEPATH.c_str()), m_ctlInfo.m_dbName);
    //2. ���ɶ�Ӧ���ļ�����������Ϣд���ļ���
    if (dumpCtlInfo() == false)
    {
#ifdef _DEBUG_
        cout << "dumpCtlInfo() false!" << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    return true;
}
//����������Ϣ
bool ControlFile::dumpCtlInfo()
{
    //����ļ��Ѿ������򱨴� add by gaojf 2010-5-13 10:40
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
//��ʼ��,�ӿ����ļ��л�ȡ������Ϣ
bool ControlFile::initialize(const char* r_dbName)
{
    FILE*   t_fp;
    //1. �������ݿ�����ȡ��Ӧ�Ŀ����ļ���
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
