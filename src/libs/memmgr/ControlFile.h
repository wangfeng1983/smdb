#ifndef _CONTROLFILE_H_INCLUDE_20080422_
#define _CONTROLFILE_H_INCLUDE_20080422_

#include "Mdb_Config.h"

class ControlFile
{
    public:
        //创建数据库控制文件
        bool createFile(const Mdb_Config& r_mdbConfig);
        //初始化,从控制文件中获取控制信息
        bool initialize(const char* r_dbName);
        MdbCtlInfo* getCtlInfoPt();
        const char* getFileName()
        {
            return m_fileName;
        }
    private:
        //导出控制信息
        bool dumpCtlInfo();
    private:
        T_FILENAMEDEF  m_fileName;
        MdbCtlInfo     m_ctlInfo;
};

#endif //_CONTROLFILE_H_INCLUDE_20080422_
