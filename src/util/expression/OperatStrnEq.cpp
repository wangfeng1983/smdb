// ############################################
// Source file : OperatStrnEq.cpp
// Version     : 0.1.1
// Language	   : ANSI C++
// OS Platform : UNIX
// Authors     : chen min
// E_mail      : chenm@lianchuang.com
// Create      : 2008-4-23
// Update      : 2008-4-23
// Copyright(C): chen min, Linkage.
// ############################################

#include "OperatStrnEq.h"

bool OperatStrnEq::evaluate(RecordConvert* pRecordConvert
                            , const void** pParameters)
{
    char cColumnValue[MAX_COLUMN_LEN];
    int  iLenthLeft, iLenthRight;
    pRecordConvert->getColumnValue(m_leftParamName.c_str(), cColumnValue);
    iLenthLeft = strlen(cColumnValue);
    iLenthRight = strlen((const char*)pParameters[m_rightParamNum]);
    if (iLenthLeft > iLenthRight)
        return  strncmp(cColumnValue, (const char*)pParameters[m_rightParamNum], iLenthRight) == 0;
    else
        return  strncmp(cColumnValue, (const char*)pParameters[m_rightParamNum], iLenthLeft) == 0;
}
