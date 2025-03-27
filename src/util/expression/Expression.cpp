// ############################################
// Source file : Expression.cpp
// Version     : 0.1.1
// Language	   : ANSI C++
// OS Platform : UNIX
// Authors     : chen min
// E_mail      : chenm@lianchuang.com
// Create      : 2008-4-23
// Update      : 2008-4-23
// Copyright(C): chen min, Linkage.
// ############################################

#include "Expression.h"

Expression::Expression()
{
    m_rootOperator = 0;
}

Expression::~Expression()
{
    if (m_rootOperator != 0)
        delete m_rootOperator;
}

bool Expression::setOperator(const char* expression)
{
    OperatorBase tmpOperatorBase;
    try
    {
        m_rootOperator = tmpOperatorBase.getOperator(expression);
    }
    catch (Mdb_Exception e)
    {
        if (m_rootOperator != 0)
            delete 	m_rootOperator;
        throw e;
    }
    return true;
}

bool Expression::evaluate(void* pRecord
                          , RecordConvert* pRecordConvert
                          , const void** pParameters)
{
    if (m_rootOperator != NULL)
    {
        pRecordConvert->setRecordValue(pRecord);
        return m_rootOperator->evaluate(pRecordConvert, pParameters);
    }
    else
        throw Mdb_Exception(__FILE__, __LINE__, "查询条件表达式未初始化!");
}
