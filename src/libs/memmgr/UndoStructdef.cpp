#include "UndoStructdef.h"

void Undo_spaceinfo::updatetime()
{
    time(&m_ddluptime);
}


void Undo_spaceinfo::dumpInfo(ostream& r_os)
{
    r_os << "-----------Undo_spaceinfo-----------" << endl;
    r_os << "m_ddluptime  =" << m_ddluptime << endl;
    r_os << "m_tablenums  =" << m_tablenums << endl;
    r_os << "m_pageoffset =" << m_pageoffset << endl;
    r_os << "m_pagetsize  =" << m_pagetsize << endl;
    r_os << "m_redooffset =" << m_redooffset << endl;
    r_os << "m_redosize   =" << m_redosize << endl;
    r_os << "m_indexoffset=" << m_indexoffset << endl;
    r_os << "m_indexsize  =" << m_indexsize << endl;
}
