#ifndef UNDOSTRUCTDEF_H_INCLUDE_20100507
#define UNDOSTRUCTDEF_H_INCLUDE_20100507
#include "MdbConstDef.h"
#include "MdbConstDef2.h"
#include "PageInfo.h"
#include "TableDescript.h"

//UNDO空间头部信息
class Undo_spaceinfo
{
    public:
        time_t        m_ddluptime;    //UNDO表空间DDL更新时间
        int           m_tablenums;    //描述符最大个数
        size_t        m_pageoffset;   //page起始偏移
        size_t        m_pagetsize;    //PAGE部分总大小
        size_t        m_redooffset;   //REDO日志部分其实偏移
        size_t        m_redosize;     //REDO日志部分总大小
        size_t        m_indexoffset;  //INDEX段偏移量
        size_t        m_indexsize;    //INDEX段大小
    public:
        void updatetime();
        void dumpInfo(ostream& r_os);
};


//定义datainfo值在SLOT中的偏移量
const size_t UNDO_VAL_OFFSET = sizeof(Undo_Slot);

#endif //UNDOSTRUCTDEF_H_INCLUDE_20100507

