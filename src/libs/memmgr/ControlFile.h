#ifndef _CONTROLFILE_H_INCLUDE_20080422_
#define _CONTROLFILE_H_INCLUDE_20080422_

#include "Mdb_Config.h"

class ControlFile
{
    public:
        //�������ݿ�����ļ�
        bool createFile(const Mdb_Config& r_mdbConfig);
        //��ʼ��,�ӿ����ļ��л�ȡ������Ϣ
        bool initialize(const char* r_dbName);
        MdbCtlInfo* getCtlInfoPt();
        const char* getFileName()
        {
            return m_fileName;
        }
    private:
        //����������Ϣ
        bool dumpCtlInfo();
    private:
        T_FILENAMEDEF  m_fileName;
        MdbCtlInfo     m_ctlInfo;
};

#endif //_CONTROLFILE_H_INCLUDE_20080422_
