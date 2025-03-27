// ############################################
// Source file : OperatNe.cpp
// Version     : 0.1.1
// Language	   : ANSI C++
// OS Platform : UNIX
// Authors     : chen min
// E_mail      : chenm@lianchuang.com
// Create      : 2008-4-23
// Update      : 2008-4-23
// Copyright(C): chen min, Linkage.
// ############################################

#include "OperatNe.h"

bool OperatNe::evaluate(RecordConvert* pRecordConvert
                        , const void** pParameters)
{
    char cColumnValue[MAX_COLUMN_LEN];
    pRecordConvert->getColumnValue(m_leftParamName.c_str(), cColumnValue, m_iColumnLenth);
    return  memcmp(cColumnValue, pParameters[m_rightParamNum], m_iColumnLenth) != 0;
}


