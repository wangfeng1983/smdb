#ifndef _TABLEDEFPARAM_H_INCLUDE_20080411_
#define _TABLEDEFPARAM_H_INCLUDE_20080411_
#include "MdbConstDef.h"
#include <cstring>

class ColumnDef ///<字段结构定义
{
    public:
        T_NAMEDEF         m_name; ///<字段名
        COLUMN_VAR_TYPE   m_type;  ///<字段定义
        int               m_len;        ///<长度，在类型为VAR_TYPE_VSTR时有效
        int               m_offSet;     ///<该字段在表中的位置：偏移量
    public:
        ColumnDef() {}
        ~ColumnDef() {}
        friend ostream& operator<<(ostream& os, const ColumnDef& r_obj);
};
class IndexDef ///<索引参数定义（创建时使用）
{
    public:
        T_NAMEDEF      m_indexName; ///<索引名称
        T_INDEXTYPE    m_indexType; ///<索引类型：1 Hash索引、2 树索引
        size_t         m_hashSize ; ///<Hash索引申请的hash结构大小
        T_NAMEDEF      m_tableName; ///<归属表名
        int            m_columnNum; ///<索引字段数
        T_NAMEDEF      m_columnList[MAX_IDXCLMN_NUM]; ///<索引字段
        int            m_spaceNum;  ///<表空间数
        T_NAMEDEF      m_spaceList[MAX_SPACE_NUM]; ///<表空间列表
        bool           m_isUnique;  ///<是否唯一索引：0 普通索引，1 唯一索引
        int            m_parlnum;   ///<并发度 add by MDB2.0
    public:
        IndexDef()
        {
            m_parlnum = 1;
        }
        ~IndexDef() {}

    public:
        friend int operator<(const IndexDef& left, const IndexDef& right)
        {
            return (strcasecmp(left.m_indexName, right.m_indexName) < 0);
        }

        friend int operator==(const IndexDef& left, const IndexDef& right)
        {
            return (strcasecmp(left.m_indexName, right.m_indexName) == 0);
        }
        friend ostream& operator<<(ostream& os, const IndexDef& r_obj);
    public:
        bool addSpace(const char* r_spaceName);
        void getSpaceList(vector<string> &r_spaceList);
        int  getSlotSize();
};

class TableDef ///<表结构参数定义（创建时使用）
{
    public:
        T_NAMEDEF      m_tableName; ///<表名
        T_TABLETYPE    m_tableType; ///<表类型：1 普通表,2 索引表,3 锁表
        int            m_columnNum; ///<字段数
        ColumnDef      m_columnList[MAX_COLUMN_NUM];///<字段列表
        int            m_spaceNum;///<表空间数
        T_NAMEDEF      m_spaceList[MAX_SPACE_NUM];///<表空间列表
        int            m_keyFlag; ///<是否有主键:0 无、1 有
        int            m_keyClumnNum;//主键字段个数
        T_NAMEDEF      m_keyColumnList[MAX_IDXCLMN_NUM];
        int            m_lockType;///<表支持的锁类型（0:表锁、1:记录锁）
        int            m_logType; ///<表是否写日志：0 不写、1 写
        int            m_parlnum;  ///<并发度 add by MDB2.0
    public:
        TableDef()
        {
            m_parlnum = 1;
        }
        ~TableDef() {}
    public:
        friend int operator<(const TableDef& left, const TableDef& right)
        {
            return (strcasecmp(left.m_tableName, right.m_tableName) < 0);
        }

        friend int operator==(const TableDef& left, const TableDef& right)
        {
            return (strcasecmp(left.m_tableName, right.m_tableName) == 0);
        }
        friend ostream& operator<<(ostream& os, const TableDef& r_obj);
    public:
        //功能：将表空间和表描述符绑定
        bool addSpace(const char* r_spaceName);
        void getSpaceList(vector<string> &r_spaceList);
        int  getSlotSize();
        void dumpInfo(ostream& r_os)
        {
            r_os << *this;
        }
};

#endif //_TABLEDEFPARAM_H_INCLUDE_20080411_


