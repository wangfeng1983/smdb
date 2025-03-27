#ifndef MDBSTATUSMGR_H_INCLUDE_20100604
#define MDBSTATUSMGR_H_INCLUDE_20100604

#include "MdbConstDef2.h"

//说明：状态文件以2进制方式读写，固定大小sizeof(MdbStatus)
class MdbStatus
{
    public:
        int           m_filetype;       ///< 数据文件的类型, -1 初始(一次未做); 0：checkpoint备份; 1：stop database 备份
        int           m_filegroup;      ///< 可用的文件组号：第0组/第1组对应A/B两个镜像
        T_SCN         m_chkptscn[2];    ///< CHKPT的SCN号
        T_FILENAMEDEF m_datapath[2];    ///< 数据目录
        time_t        m_chkpt_time[2];  ///< CHKPT时间信息

    public:
        MdbStatus()
        {
            m_filetype = -1;
            m_filegroup = 0;
        }
        void setpath(const T_FILENAMEDEF r_datapath[2])
        {
            strcpy(m_datapath[0], r_datapath[0]);
            strcpy(m_datapath[1], r_datapath[1]);
        }
        void setvalue(const int& r_filetype, const int& r_filegrp,
                      const T_SCN& r_scn)
        {
            m_filetype = r_filetype;
            m_filegroup = r_filegrp ;
            m_chkptscn[m_filegroup] = r_scn;
            time(&(m_chkpt_time[m_filegroup]));
        }
        const char* getpath()
        {
            //获取当前使用的目录
            return m_datapath[m_filegroup];
        }
        time_t getchkpttime()
        {
            return m_chkpt_time[m_filegroup];
        }
        void dump()
        {
            cout << "m_filetype             =" << m_filetype << endl;
            cout << "m_filegroup            =" << m_filegroup << endl;
            cout << "m_datapath[m_filegroup]=" << m_datapath[m_filegroup] << endl;
        }
};
class MdbStatusMgr
{
    public:
        MdbStatusMgr();
        virtual ~MdbStatusMgr();
    public:
        //初始化
        bool initialize(const char* r_dbname, const char* r_path);
        //从文件中读取状态
        bool readStatus();
        //将状态更新到文件中
        bool writeStatus();
        MdbStatus* getStatus();
    protected:
        bool open_inner();
        bool close_inner();
    protected:
        MdbStatus       m_status;   ///<状态信息
        T_FILENAMEDEF   m_fileName; ///<文件名
        bool            m_openflag; ///<文件状态
        FILE*           m_file;     ///<文件句柄
};
#endif //MDBSTATUSMGR_H_INCLUDE_20100604




