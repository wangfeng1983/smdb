#include "MdbStatusMgr.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>

MdbStatusMgr::MdbStatusMgr()
{
    m_openflag = false;
}
MdbStatusMgr::~MdbStatusMgr()
{
    close_inner();
}
//初始化
bool MdbStatusMgr::initialize(const char* r_dbname, const char* r_path)
{
    sprintf(m_fileName, "%s/%s.status", r_path, r_dbname);
    return true;
}
//从文件中读取状态
bool MdbStatusMgr::readStatus()
{
    if (open_inner() == false)
    {
#ifdef _DEBUG_
        cout << "open file {" << m_fileName << "} error!" << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    if (fseek(m_file, 0, SEEK_SET) < 0)
    {
#ifdef _DEBUG_
        cout << "lseek file {" << m_fileName << ",0} error!" << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    fread(&m_status, sizeof(MdbStatus), 1, m_file);
    close_inner();
    return true;
}
//将状态更新到文件中
bool MdbStatusMgr::writeStatus()
{
    if (open_inner() == false)
    {
#ifdef _DEBUG_
        cout << "open file {" << m_fileName << "} error!" << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    if (fseek(m_file, 0, SEEK_SET) < 0)
    {
#ifdef _DEBUG_
        cout << "lseek file {" << m_fileName << ",0} error!" << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    fwrite(&m_status, sizeof(MdbStatus), 1, m_file);
    close_inner();
    return true;
}
MdbStatus* MdbStatusMgr::getStatus()
{
    return &m_status;
}

bool MdbStatusMgr::close_inner()
{
    if (m_openflag == true)
    {
        fclose(m_file);
        m_openflag = false;
    }
    return true;
}

bool MdbStatusMgr::open_inner()
{
    if (m_openflag == true) close_inner();
    //1.检查文件是否存在，存在则报错
    int t_exist = access(m_fileName, F_OK) ;
    string t_mode;
    if (t_exist == 0)
    {
        t_mode = "r+b";
    }
    else
    {
        t_mode = "a+b";
    }
    if ((m_file = fopen(m_fileName, t_mode.c_str())) == NULL)
    {
#ifdef _DEBUG_
        cout << "open(" << m_fileName << ",a+b) false!"
             << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    if (t_exist != 0)
    {
        //2.创建文件，大小为:sizeof(MdbStatusMgr)
        MdbStatus t_status; //初始化状态为-1
        t_status.m_filetype = -1;
        fwrite(&m_status, sizeof(MdbStatus), 1, m_file);
    }
    m_openflag = true;
    return true;
}


