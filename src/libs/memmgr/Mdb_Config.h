#ifndef _MDB_CONFIG_H_INCLUDE_20080414_
#define _MDB_CONFIG_H_INCLUDE_20080414_
#include "TableDefParam.h"
#include "SpaceInfo.h"

class SysParam;
class MdbCtlInfo
{
    public:
        T_NAMEDEF       m_dbName;          ///<���ݿ���
        T_NAMEDEF       m_ctlSpaceName;    ///<��������ռ���
        size_t          m_size;            ///<��������ռ��С
        size_t          m_pageSize;        ///<ҳ���С(1M)
        int             m_gparamMaxSpNum;  ///<֧�ֵ�����ı�ռ�����
        int             m_gparamMaxNum;    ///<֧�ֵ�ȫ�ֲ�������(������)
        int             m_tableMaxNum;     ///<֧�ֵı�������
        int             m_indexMaxNum;     ///<֧�ֵ�����������
        int             m_sessionMaxNum;   ///<֧�ֵ�Session������
        T_NAMEDEF       m_idxHashSpaceName;///<������ռ䣨���hash���֣�
        T_NAMEDEF       m_undoSpaceName;   ///<UNDO��ռ䣨���UNDO���֣�
        //    size_t          m_undobuffsize;    ///<UNDO��ռ���ͨ��BUFF��С
        size_t          m_undohashsize;    ///<UNDOHASHͰ�ռ��С
        T_FILENAMEDEF   m_lockpath;        ///<LOCKPATH
        T_FILENAMEDEF   m_logPath;         ///<��־·��
        T_FILENAMEDEF   m_lockfile;        ///<�ź��������ļ�
        T_FILENAMEDEF   m_datapath[2];     ///<����Ŀ¼0/1
    public:
        bool getCtlInfo(const char* r_dbName, SysParam* pSysParam);
        friend ostream& operator<<(ostream& os, const MdbCtlInfo& r_obj);
};

class RedoInfo
{
    public:
        vector<int> m_chnlst;

        T_NAMEDEF   m_path;
        int         m_logfilesize;
        int         m_groupcount;
        int         m_buffercount;
        int         m_buffersize;
        T_NAMEDEF   m_writemode;
        T_NAMEDEF   m_waitmode;
        int         m_syncmode;
        static int  m_redosize;
    public:
        //int getUndoBufferSize();
        friend ostream& operator<<(ostream& os, const RedoInfo& r_obj);
};


class Mdb_Config
{
    public:
        MdbCtlInfo            m_ctlInfo;
        //��ռ䶨����Ϣ������������ռ䶨����Ϣ�����ݱ�ռ䶨����Ϣ��UNDO��ռ䣩
        vector<SpaceInfo>     m_spaceInfoList;
        RedoInfo              m_redoInfo;
    public:
        //�������ݿ����������ļ���ȡ������Ϣ
        bool getConfigInfo(const char* r_dbName);
    private:
        bool getConfigInfoLocal(const char* r_dbName, SysParam* r_pSysParam);
        bool check();
};

#endif //_MDB_CONFIG_H_INCLUDE_20080414_

