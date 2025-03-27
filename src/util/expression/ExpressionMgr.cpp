// ############################################
// Source file : ExpressionMgr.cpp
// Version     : 0.1.1
// Language	   : ANSI C++
// OS Platform : UNIX
// Authors     : chen min
// E_mail      : chenm@lianchuang.com
// Create      : 2008-5-2
// Update      : 2008-5-2
// Copyright(C): chen min, Linkage.
// ############################################

#include "ExpressionMgr.h"

ExpressionMgr::ExpressionMgr()
{
    m_expressionMap.clear();
}

ExpressionMgr::~ExpressionMgr()
{
    EXPRESSION_POOL_ITR itr;
    for (itr = m_expressionMap.begin(); itr != m_expressionMap.end(); ++itr)
    {
        if (itr->second != NULL)
        {
            delete 	itr->second;
        }
    }
    m_expressionMap.clear();
}

Expression* ExpressionMgr::getExpre(const char* sInputExpre)
{
    EXPRESSION_POOL_ITR itr;
    string s(sInputExpre);
    itr = m_expressionMap.find(s)	;
    if (itr != m_expressionMap.end())
    {
        return itr->second;
    }
    else
    {
        Expression* pExpression;
        try
        {
            pExpression = new Expression();
            pExpression->setOperator(sInputExpre);
            m_expressionMap.insert(EXPRESSION_POOL::value_type(s, pExpression));
            return 	pExpression;
        }
        catch (Mdb_Exception e)
        {
            delete pExpression;
            throw e;
        }
    }
}

