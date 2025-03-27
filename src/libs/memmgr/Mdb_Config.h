#ifndef _MDB_CONFIG_H_INCLUDE_20080414_
#define _MDB_CONFIG_H_INCLUDE_20080414_
#include "TableDefParam.h"
#include "SpaceInfo.h"

class SysParam;
class MdbCtlInfo
{
    public:
        T_NAMEDEF       m_dbName;          ///<数据库名
        T_NAMEDEF       m_ctlSpaceName;    ///<控制区表空间名
        size_t          m_size;            ///<控制区表空间大小
        size_t          m_pageSize;        ///<页面大小(1M)
        int             m_gparamMaxSpNum;  ///<支持的允许的表空间数量
        int             m_gparamMaxNum;    ///<支持的全局参数个数(最大个数)
        int             m_tableMaxNum;     ///<支持的表最大个数
        int             m_indexMaxNum;     ///<支持的索引最大个数
        int             m_sessionMaxNum;   ///<支持的Session最大个数
        T_NAMEDEF       m_idxHashSpaceName;///<索引表空间（存放hash部分）
        T_NAMEDEF       m_undoSpaceName;   ///<UNDO表空间（存放UNDO部分）
        //    size_t          m_undobuffsize;    ///<UNDO表空间中通用BUFF大小
        size_t          m_undohashsize;    ///<UNDOHASH桶空间大小
        T_FILENAMEDEF   m_lockpath;        ///<LOCKPATH
        T_FILENAMEDEF   m_logPath;         ///<日志路径
        T_FILENAMEDEF   m_lockfile;        ///<信号量控制文件
        T_FILENAMEDEF   m_datapath[2];     ///<数据目录0/1
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
        //表空间定义信息（包括索引表空间定义信息和数据表空间定义信息、UNDO表空间）
        vector<SpaceInfo>     m_spaceInfoList;
        RedoInfo              m_redoInfo;
    public:
        //根据数据库名从配置文件中取配置信息
        bool getConfigInfo(const char* r_dbName);
    private:
        bool getConfigInfoLocal(const char* r_dbName, SysParam* r_pSysParam);
        bool check();
};

#endif //_MDB_CONFIG_H_INCLUDE_20080414_

