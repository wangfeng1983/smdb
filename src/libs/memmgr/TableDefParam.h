#ifndef _TABLEDEFPARAM_H_INCLUDE_20080411_
#define _TABLEDEFPARAM_H_INCLUDE_20080411_
#include "MdbConstDef.h"
#include <cstring>

class ColumnDef ///<�ֶνṹ����
{
    public:
        T_NAMEDEF         m_name; ///<�ֶ���
        COLUMN_VAR_TYPE   m_type;  ///<�ֶζ���
        int               m_len;        ///<���ȣ�������ΪVAR_TYPE_VSTRʱ��Ч
        int               m_offSet;     ///<���ֶ��ڱ��е�λ�ã�ƫ����
    public:
        ColumnDef() {}
        ~ColumnDef() {}
        friend ostream& operator<<(ostream& os, const ColumnDef& r_obj);
};
class IndexDef ///<�����������壨����ʱʹ�ã�
{
    public:
        T_NAMEDEF      m_indexName; ///<��������
        T_INDEXTYPE    m_indexType; ///<�������ͣ�1 Hash������2 ������
        size_t         m_hashSize ; ///<Hash���������hash�ṹ��С
        T_NAMEDEF      m_tableName; ///<��������
        int            m_columnNum; ///<�����ֶ���
        T_NAMEDEF      m_columnList[MAX_IDXCLMN_NUM]; ///<�����ֶ�
        int            m_spaceNum;  ///<��ռ���
        T_NAMEDEF      m_spaceList[MAX_SPACE_NUM]; ///<��ռ��б�
        bool           m_isUnique;  ///<�Ƿ�Ψһ������0 ��ͨ������1 Ψһ����
        int            m_parlnum;   ///<������ add by MDB2.0
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

class TableDef ///<��ṹ�������壨����ʱʹ�ã�
{
    public:
        T_NAMEDEF      m_tableName; ///<����
        T_TABLETYPE    m_tableType; ///<�����ͣ�1 ��ͨ��,2 ������,3 ����
        int            m_columnNum; ///<�ֶ���
        ColumnDef      m_columnList[MAX_COLUMN_NUM];///<�ֶ��б�
        int            m_spaceNum;///<��ռ���
        T_NAMEDEF      m_spaceList[MAX_SPACE_NUM];///<��ռ��б�
        int            m_keyFlag; ///<�Ƿ�������:0 �ޡ�1 ��
        int            m_keyClumnNum;//�����ֶθ���
        T_NAMEDEF      m_keyColumnList[MAX_IDXCLMN_NUM];
        int            m_lockType;///<��֧�ֵ������ͣ�0:������1:��¼����
        int            m_logType; ///<���Ƿ�д��־��0 ��д��1 д
        int            m_parlnum;  ///<������ add by MDB2.0
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
        //���ܣ�����ռ�ͱ���������
        bool addSpace(const char* r_spaceName);
        void getSpaceList(vector<string> &r_spaceList);
        int  getSlotSize();
        void dumpInfo(ostream& r_os)
        {
            r_os << *this;
        }
};

#endif //_TABLEDEFPARAM_H_INCLUDE_20080411_


