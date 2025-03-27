#ifndef MDBSTATUSMGR_H_INCLUDE_20100604
#define MDBSTATUSMGR_H_INCLUDE_20100604

#include "MdbConstDef2.h"

//˵����״̬�ļ���2���Ʒ�ʽ��д���̶���Сsizeof(MdbStatus)
class MdbStatus
{
    public:
        int           m_filetype;       ///< �����ļ�������, -1 ��ʼ(һ��δ��); 0��checkpoint����; 1��stop database ����
        int           m_filegroup;      ///< ���õ��ļ���ţ���0��/��1���ӦA/B��������
        T_SCN         m_chkptscn[2];    ///< CHKPT��SCN��
        T_FILENAMEDEF m_datapath[2];    ///< ����Ŀ¼
        time_t        m_chkpt_time[2];  ///< CHKPTʱ����Ϣ

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
            //��ȡ��ǰʹ�õ�Ŀ¼
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
        //��ʼ��
        bool initialize(const char* r_dbname, const char* r_path);
        //���ļ��ж�ȡ״̬
        bool readStatus();
        //��״̬���µ��ļ���
        bool writeStatus();
        MdbStatus* getStatus();
    protected:
        bool open_inner();
        bool close_inner();
    protected:
        MdbStatus       m_status;   ///<״̬��Ϣ
        T_FILENAMEDEF   m_fileName; ///<�ļ���
        bool            m_openflag; ///<�ļ�״̬
        FILE*           m_file;     ///<�ļ����
};
#endif //MDBSTATUSMGR_H_INCLUDE_20100604




