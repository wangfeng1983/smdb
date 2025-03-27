#ifndef UNDOSTRUCTDEF_H_INCLUDE_20100507
#define UNDOSTRUCTDEF_H_INCLUDE_20100507
#include "MdbConstDef.h"
#include "MdbConstDef2.h"
#include "PageInfo.h"
#include "TableDescript.h"

//UNDO�ռ�ͷ����Ϣ
class Undo_spaceinfo
{
    public:
        time_t        m_ddluptime;    //UNDO��ռ�DDL����ʱ��
        int           m_tablenums;    //������������
        size_t        m_pageoffset;   //page��ʼƫ��
        size_t        m_pagetsize;    //PAGE�����ܴ�С
        size_t        m_redooffset;   //REDO��־������ʵƫ��
        size_t        m_redosize;     //REDO��־�����ܴ�С
        size_t        m_indexoffset;  //INDEX��ƫ����
        size_t        m_indexsize;    //INDEX�δ�С
    public:
        void updatetime();
        void dumpInfo(ostream& r_os);
};


//����datainfoֵ��SLOT�е�ƫ����
const size_t UNDO_VAL_OFFSET = sizeof(Undo_Slot);

#endif //UNDOSTRUCTDEF_H_INCLUDE_20100507

