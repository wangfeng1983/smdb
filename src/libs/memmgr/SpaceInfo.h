#ifndef _SPACEINFO_H_INCLUDE_20080416_
#define _SPACEINFO_H_INCLUDE_20080416_
#include "MdbConstDef.h"
#include "MdbConstDef2.h"
#include <sys/ipc.h>
//��ռ�ṹ
class SpaceInfo
{
    public:
        T_NAMEDEF     m_spaceName;
        T_NAMEDEF     m_dbName;
        T_FILENAMEDEF m_fileName;
        size_t        m_size;
        size_t        m_pageSize;
        T_SPACETYPE   m_type;
        unsigned int  m_spaceCode;//��ֻ֧��1-255
    public:
        key_t         m_shmKey;
        char*         m_shmAddr;
    public:
        SpaceInfo()
        {
            m_shmAddr = NULL;
        }
        void startmdb_init() {};
        friend int operator<(const SpaceInfo& left, const SpaceInfo& right)
        {
            return (strcasecmp(left.m_spaceName, right.m_spaceName) < 0);
        }

        friend int operator==(const SpaceInfo& left, const SpaceInfo& right)
        {
            return (strcasecmp(left.m_spaceName, right.m_spaceName) == 0);
        }
        friend ostream& operator<<(ostream& os, const SpaceInfo& r_obj)
        {
            os << "m_spaceName = " << r_obj.m_spaceName << endl;
            os << "m_dbName    = " << r_obj.m_dbName << endl;
            os << "m_fileName  = " << r_obj.m_fileName << endl;
            os << "m_size      = " << r_obj.m_size << endl;
            os << "m_pageSize  = " << r_obj.m_pageSize << endl;
            os << "m_type      = " << r_obj.m_type << endl;
            os << "m_spaceCode = " << r_obj.m_spaceCode << endl;
            os << "m_shmKey    = " << r_obj.m_shmKey << endl;
            os << "m_shmAddr   = " << (void*)(r_obj.m_shmAddr) << endl;
            return os;
        }
    public:
        void dumpInfo(ostream& r_os);
};
class MDbGlobalInfo
{
    public:
        time_t  m_dbTime;      ///<���ݿ�ʱ����Ϣ
        time_t  m_spUpTime;    ///<��ռ���Ϣ����ʱ��
        time_t  m_tbUpTime;    ///<������������ʱ��
        time_t  m_idxUpTime;   ///<��������������ʱ��
        unsigned int  m_nextSpCode;  ///<��һ��������ڲ�����
        int     m_curSessionId;///<��ǰSessionId
        ///<SessionId״̬:true ��ʹ�ã�falseδʹ��
        unsigned short int m_sidState[MAX_SESSION_NUM];
        unsigned char   m_semmark[LOCKWAITS_MASK_NUM];
        size_t  m_scn;         ///<SCN�Ĵ��� add by gaojf MDB2.0
        volatile int    m_scnlacth;    ///<SCN�� add by gaojf MDB2.0
        T_SCN   m_lastckpscn;  ///<���һ���ɹ���CKP SCN
        size_t  m_reserve;     ///<�����ռ���ʼλ��
        long    m_transid;     ///<transid�Ĵ���   added by wangfp MDB2.0
        volatile int m_tidlatch;    ///<tid�� added by wangfp MDB2.0
    public:
        //r_flag 0 �״δ�����ʼ�������� ����ʱ��ʼ��
        void init(const int r_flag = 0); ///<�״δ���ʱ��ʼ��
        void updateDbTime();
        void updateSpTime();
        void updateTbTime();
        void updateIdxTime();
        void setReserve(const size_t& r_offSet);
        unsigned int getNextSpCode();
        //��ȡSessionId��<0 ��ʾ����
        void  getSessionId(int& sessionid, short int& semmark);
        //�ͷ�SessionId
        void releadSid(const int& r_sid);
        //added by wangfp for sem-session relation
        short int getsemmark();
        bool ersemmark(short int semmark);
        //added end
        //r_flag Ϊ1 �Զ�+1  0����
        void getscn(T_SCN& r_scn, const int r_flag = 1); ///<��ȡSCN�ţ����Զ�+1 add by gaojf MDB2.0
        void gettid(long& transid);///<��ȡTransid?���Զ�+1 add by wangfp
        void getlastckpscn(T_SCN& r_scn);
        void setlastckpscn(const T_SCN& r_scn);
};

class GlobalParam
{
    public:
        T_NAMEDEF       m_paramName;///<��������
        T_GPARAMVALUE   m_value;    ///<ֵ
    public:
        void startmdb_init() {};
        friend int operator<(const GlobalParam& left, const GlobalParam& right)
        {
            return (strcasecmp(left.m_paramName, right.m_paramName) < 0);
        }
        friend int operator==(const GlobalParam& left, const GlobalParam& right)
        {
            return (strcasecmp(left.m_paramName, right.m_paramName) == 0);
        }
        friend ostream& operator<<(ostream& os, const GlobalParam& r_obj)
        {
            os << r_obj.m_paramName << " " << r_obj.m_value << endl;
            return os;
        }
    public:
        void dumpInfo(ostream& r_os);
};

class SessionInfo ///<Session����
{
    public:
        pid_t          m_pId;         ///<���̺�
        int            m_sessionId;   ///<SessionId
        time_t         m_time;        ///<ʱ��
        T_IPADDR       m_ipAddr;      ///<IP��ַ��Ϣ
        short int      m_semmark;     ///<�ź�����־
    public:
        void startmdb_init() {};
        friend int operator<(const SessionInfo& left, const SessionInfo& right)
        {
            if (left.m_sessionId < right.m_sessionId)
                return true;
            if (left.m_sessionId > right.m_sessionId)
                return false;
            return false;
        }

        friend int operator==(const SessionInfo& left, const SessionInfo& right)
        {
            return (left.m_sessionId == right.m_sessionId);
        }
        friend ostream& operator<<(ostream& os, const SessionInfo& r_obj)
        {
            os << r_obj.m_sessionId << " " << r_obj.m_pId << " "
               << r_obj.m_ipAddr << " " << ctime(&(r_obj.m_time)) << " "
               << r_obj.m_semmark;
            return os;
        }
    public:
        void dumpInfo(ostream& r_os);
};

#endif //_SPACEINFO_H_INCLUDE_20080416_
