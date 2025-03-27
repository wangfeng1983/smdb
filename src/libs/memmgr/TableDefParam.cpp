#include <stdio.h>
#include "TableDefParam.h"
#include "Bucket.h"

//前提：表空间已经创建
//功能：将表空间和表描述符绑定
bool TableDef::addSpace(const char* r_spaceName)
{
    int t_len = strlen(r_spaceName);
    if (t_len >= sizeof(T_NAMEDEF))
    {
#ifdef _DEBUG_
        cout << "表空间名称过长:r_spaceName=" << r_spaceName
             << " " << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    for (int i = 0; i < m_spaceNum; i++)
    {
        //校验，该表空间是否已经绑定
        if (strcasecmp(m_spaceList[i], r_spaceName) == 0)
        {
            //return false; //modified by gaojf 2011/12/9 10:39:44
            return true;
        }
    }
    if (m_spaceNum >= MAX_SPACE_NUM)
    {
#ifdef _DEBUG_
        cout << "绑定的表空间个数已达到限制值！" << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    memcpy(m_spaceList[m_spaceNum], r_spaceName, t_len + 1);
    m_spaceNum++;
    return true;
}


int TableDef::getSlotSize()
{
    int  t_slotSize = 0;
    //DEF只管本身的大小
    //#ifdef _USEDSLOT_LIST_
    //  t_slotSize += 2*sizeof(ShmPosition); //指向前后两个Slot
    //#endif

    for (int i = 0; i < m_columnNum; i++)
    {
        m_columnList[i].m_offSet = t_slotSize;
        switch (m_columnList[i].m_type)
        {
            case VAR_TYPE_INT2:
                t_slotSize += 2;
                break;
            case VAR_TYPE_INT:
                t_slotSize += sizeof(int);
                break;
            case VAR_TYPE_DATE:
                t_slotSize += 2 * sizeof(int);
                break;
            case VAR_TYPE_LONG:
                t_slotSize += sizeof(long);
                break;
            case VAR_TYPE_REAL:
                t_slotSize += sizeof(float);
                break;
            case VAR_TYPE_NUMSTR: //数字字符串，采用BCD码压缩
                t_slotSize += (m_columnList[i].m_len + 1) / 2;
                break;
            case VAR_TYPE_VSTR: //字符串方式
                t_slotSize += m_columnList[i].m_len;
                break;
            case VAR_TYPE_UNKNOW:
            default:
                return -1;
        }
    }
    return t_slotSize;
}

bool IndexDef::addSpace(const char* r_spaceName)
{
    int t_len = strlen(r_spaceName);
    if (t_len >= sizeof(T_NAMEDEF))
    {
#ifdef _DEBUG_
        cout << "表空间名称过长:r_spaceName=" << r_spaceName
             << " " << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    for (int i = 0; i < m_spaceNum; i++)
    {
        //校验，该表空间是否已经绑定
        if (strcasecmp(m_spaceList[i], r_spaceName) == 0)
        {
            //return false; //modified by gaojf 2011/12/9 10:37:19
            return true;
        }
    }
    if (m_spaceNum >= MAX_SPACE_NUM)
    {
#ifdef _DEBUG_
        cout << "绑定的表空间个数已达到限制值！" << __FILE__ << __LINE__ << endl;
#endif
        return false;
    }
    memcpy(m_spaceList[m_spaceNum], r_spaceName, t_len + 1);
    m_spaceNum++;
    return true;
}

int IndexDef::getSlotSize()
{
    if (sizeof(Bucket) < sizeof(ShmPosition))
    {
        return sizeof(ShmPosition);
    }
    else
    {
        return sizeof(Bucket);
    }
}
void IndexDef::getSpaceList(vector<string> &r_spaceList)
{
    r_spaceList.clear();
    for (int i = 0; i < m_spaceNum; i++)
    {
        r_spaceList.push_back(m_spaceList[i]);
    }
}
void TableDef::getSpaceList(vector<string> &r_spaceList)
{
    r_spaceList.clear();
    for (int i = 0; i < m_spaceNum; i++)
    {
        r_spaceList.push_back(m_spaceList[i]);
    }
}

ostream& operator<<(ostream& os, const ColumnDef& r_obj)
{
    os << r_obj.m_name << "," << r_obj.m_type << ","
       << r_obj.m_len << "," << r_obj.m_offSet << endl;
    return os;
}
ostream& operator<<(ostream& os, const IndexDef& r_obj)
{
    string out;
    char cLine[1024];
    sprintf(cLine, "索引名称  (m_indexName):%s\n", r_obj.m_indexName);
    out += cLine;
    sprintf(cLine, "归属表名  (m_tableName):%s\n", r_obj.m_tableName);
    out += cLine;
    sprintf(cLine, "索引类型  (m_indexType):%d(0:HASH_INDEX,1:TREE_INDEX,2:HASH_TREE,3:BITMAP_INDEX,4:HASH_INDEX_NP)\n", r_obj.m_indexType);
    out += cLine;
    sprintf(cLine, "Hash桶大小(m_hashSize ):%ld\n", r_obj.m_hashSize);
    out += cLine;
    sprintf(cLine, "唯一索引  (m_isUnique ):%s\n", r_obj.m_isUnique ? "true" : "false");
    out += cLine;
    sprintf(cLine, "索引字段数(m_columnNum):%d\n", r_obj.m_columnNum);
    out += cLine;
    out += "索引字段列表:";
    for (int i = 0; i < r_obj.m_columnNum; i++)
    {
        out += r_obj.m_columnList[i];
        out += " ";
    }
    out += "\n";
    sprintf(cLine, "表空间数  (m_spaceNum ):%d\n", r_obj.m_spaceNum);
    out += cLine;
    out += "表空间列表:";
    for (int j = 0; j < r_obj.m_spaceNum; j++)
    {
        out += r_obj.m_spaceList[j];
        out += " ";
    }
    out += "\n";
    os << out;
    return os;
    /*
    os<<"----------Index:"<<r_obj.m_indexName<<"--------"<<endl;
    os<<"r_obj.m_indexType="<<r_obj.m_indexType<<endl;
    os<<"r_obj.m_hashSize ="<<r_obj.m_hashSize<<endl;
    os<<"r_obj.m_columnNum="<<r_obj.m_columnNum<<endl;
    for(int i=0;i<r_obj.m_columnNum;i++)
    {
      os<<r_obj.m_columnList[i];
    }
    os << endl;
    os<<"r_obj.m_spaceNum = "<<r_obj.m_spaceNum<<endl;
    for(int j=0;j<r_obj.m_spaceNum;j++)
    {
      if(j>0) os<<",";
      os<<r_obj.m_spaceList[j];
    }
    if(r_obj.m_spaceNum>0) os<<endl;
    os<<"r_obj.m_isUnique="<<r_obj.m_isUnique<<endl;
    os<<"------------------------------------------------"<<endl;
    return os;
    */
}
ostream& operator<<(ostream& os, const TableDef& r_obj)
{
    string out;//="****************** 表结构定义 ******************\n";
    char cLine[1024];
    sprintf(cLine, "表  名(m_tableName):%s\n", r_obj.m_tableName);
    out += cLine;
    sprintf(cLine, "表类型(m_tableType):%d(1-普通表,2-索引表,3-锁表,11-不持久化的表)\n\n", r_obj.m_tableType);
    out += cLine;
    sprintf(cLine, "字段数(m_columnNum):%d\n", r_obj.m_columnNum);
    out += cLine;
    sprintf(cLine, "%-35s %-17s %11s %16s\n", "字段名(m_name)", "字段类型(m_type)", "长度(m_len)", "偏移量(m_offSet)");
    out += cLine;
    char cColType[20];
    for (int i = 0; i < r_obj.m_columnNum; i++)
    {
        memset(cColType, 0, sizeof(cColType));
        switch (r_obj.m_columnList[i].m_type)
        {
            case VAR_TYPE_INT2  :
                strcpy(cColType, "0:VAR_TYPE_INT2");
                break;
            case VAR_TYPE_INT   :
                strcpy(cColType, "1:VAR_TYPE_INT");
                break;
            case VAR_TYPE_DATE  :
                strcpy(cColType, "2:VAR_TYPE_DATE");
                break;
            case VAR_TYPE_LONG  :
                strcpy(cColType, "3:VAR_TYPE_LONG");
                break;
            case VAR_TYPE_REAL  :
                strcpy(cColType, "4:VAR_TYPE_REAL");
                break;
            case VAR_TYPE_NUMSTR:
                strcpy(cColType, "5:VAR_TYPE_NUMSTR");
                break;
            case VAR_TYPE_VSTR  :
                strcpy(cColType, "6:VAR_TYPE_VSTR");
                break;
            default:
                strcpy(cColType, "7:VAR_TYPE_UNKNOW");
                break;
        }
        sprintf(cLine, "%-35s %-17s %11d %16ld\n", r_obj.m_columnList[i].m_name
                , cColType
                , r_obj.m_columnList[i].m_len
                , r_obj.m_columnList[i].m_offSet);
        out += cLine;
    }
    out += "\n";
    sprintf(cLine, "表空间总数(m_spaceNum):%d\n", r_obj.m_spaceNum);
    out += cLine;
    if (r_obj.m_spaceNum > 0)
    {
        out += "表空间列表:";
        for (int j = 0; j < r_obj.m_spaceNum; j++)
        {
            out += r_obj.m_spaceList[j];
            out += " ";
        }
        out += "\n";
    }
    sprintf(cLine, "有无主键  (m_keyFlag    ):%d(0-无,1-有)\n", r_obj.m_keyFlag);
    out += cLine;
    sprintf(cLine, "主键字段数(m_keyClumnNum):%d\n", r_obj.m_keyClumnNum);
    out += cLine;
    if (r_obj.m_keyClumnNum > 0)
    {
        out += "主键字段:";
        for (int k = 0; k < r_obj.m_keyClumnNum; k++)
        {
            out += r_obj.m_keyColumnList[k];
            out += " ";
        }
        out += "\n";
    }
    sprintf(cLine, "支持锁类型(m_lockType):%d(0-表锁,1-记录锁)\n", r_obj.m_lockType);
    out += cLine;
    sprintf(cLine, "是否写日志(m_logType ):%d(0-不写,1-写)\n", r_obj.m_logType);
    out += cLine;
    os << out << endl;
    return os;
}

