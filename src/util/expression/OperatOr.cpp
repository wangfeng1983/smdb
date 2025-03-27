// ############################################
// Source file : OperatOr.cpp
// Version     : 0.1.1
// Language	   : ANSI C++
// OS Platform : UNIX
// Authors     : chen min
// E_mail      : chenm@lianchuang.com
// Create      : 2008-4-23
// Update      : 2008-4-23
// Copyright(C): chen min, Linkage.
// ############################################

#include "OperatOr.h"

bool OperatOr::evaluate(RecordConvert* pRecordConvert
                        , const void** pParameters)
{
    return m_leftOperator->evaluate(pRecordConvert, pParameters)
           || m_rightOperator->evaluate(pRecordConvert, pParameters);
}


