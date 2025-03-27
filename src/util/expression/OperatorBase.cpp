// ############################################
// Source file : OperatorBase.cpp
// Version     : 0.1.1
// Language	   : ANSI C++
// OS Platform : UNIX
// Authors     : chen min
// E_mail      : chenm@lianchuang.com
// Create      : 2008-4-23
// Update      : 2008-4-23
// Copyright(C): chen min, Linkage.
// ############################################

#include "OperatorBase.h"
#include "base/StringUtil.h"
#include "OperatAnd.h"
#include "OperatEq.h"
#include "OperatGe.h"
#include "OperatGt.h"
#include "OperatLe.h"
#include "OperatLt.h"
#include "OperatNe.h"
#include "OperatOr.h"
#include "OperatStrEq.h"
#include "OperatStrnEq.h"
#include "OperatorBase.h"
#include "Mdb_Exception.h"

OperatorBase::OperatorBase()
{
    m_leftOperator      = NULL;
    m_rightOperator     = NULL;
    m_rightParamNum     = -1;
    m_leftParamName     = "";
}

OperatorBase::~OperatorBase()
{
    if (m_leftOperator != NULL)
        delete m_leftOperator;
    if (m_rightOperator != NULL)
        delete m_rightOperator;
}

//expression举例:(msisdn=:1 and startTime=:2)
OperatorBase* OperatorBase::getOperator(const char* expression)
{
    int flag = 0;
    OperatorBase* pOperatorBase = 0;
    string operatType, leftSubExpr, rightSubExpr;
    // 得到该表达式中最低级别的运算符
    getOperatType(expression, operatType, leftSubExpr, rightSubExpr);
    if (operatType.compare("=") == 0)
    {
        pOperatorBase = new OperatEq;
    }
    else if (operatType.compare(">=")  == 0)
    {
        pOperatorBase = new OperatGe;
    }
    else if (operatType.compare(">")   == 0)
    {
        pOperatorBase = new OperatGt;
    }
    else if (operatType.compare("<")   == 0)
    {
        pOperatorBase = new OperatLt;
    }
    else if (operatType.compare("<=")  == 0)
    {
        pOperatorBase = new OperatLe;
    }
    else if (operatType.compare("!=")  == 0)
    {
        pOperatorBase = new OperatNe;
    }
    else if (operatType.compare("neq")  == 0)
    {
        pOperatorBase = new OperatStrnEq;
    }
    else if (operatType.compare("eq")  == 0)
    {
        pOperatorBase = new OperatStrEq;
    }
    else if (operatType.compare("and") == 0)
    {
        pOperatorBase     = new OperatAnd;
        flag     = 1;
    }
    else if (operatType.compare("or")  == 0)
    {
        pOperatorBase     = new OperatOr;
        flag    = 1;
    }
    else
    {
        throw Mdb_Exception(__FILE__, __LINE__, ("未定义的表达式操作符：%s"), operatType.c_str());
    }
    if (flag == 0)
    {
        pOperatorBase->m_leftParamName = leftSubExpr;
        pOperatorBase->m_rightParamNum = atoi(rightSubExpr.c_str() + 1);
    }
    else // and or
    {
        // 此处递归,生成树状结构
        pOperatorBase->m_leftOperator = getOperator(leftSubExpr.c_str());
        pOperatorBase->m_rightOperator = getOperator(rightSubExpr.c_str());
    }
    return pOperatorBase;
}//getExp

void OperatorBase::getOperatType(const char* expression, string& operatType, string& leftSubExpr, string& rightSubExpr)
{
    int i, size, r_size = 0, l_size = 0;
    string      w = expression;
    w = StringUtil::trim(w);
    const char* t = w.c_str();
    size       = w.size();
    // 举例：expression     = "( (l_1>=:1) and (l_1<=:2) ) and (l_0==:0)";
    // 查找第一对括号的结束位置
    for (i = 0; i < size; i++)
    {
        if (t[i] == '(')
            r_size++;
        if (t[i] == ')')
            l_size++;
        if (r_size == l_size)
            break;
    }
    i++;
    if (l_size == 0)
    {
        if (operatType.size() == 0)
            operatType = getOperatMark(w);
        leftSubExpr = w.substr(0, w.find(operatType.c_str()));
        leftSubExpr = StringUtil::trim(leftSubExpr);
        rightSubExpr = w.substr(w.find(operatType.c_str()) + operatType.length());
        rightSubExpr = StringUtil::trim(rightSubExpr);
        return;
    }
    leftSubExpr = w.substr(0, i);    //leftSubExpr="( (l_1>=:1) and (l_1<=:2) )"
    leftSubExpr = StringUtil::trim(leftSubExpr);
    rightSubExpr = w.substr(i);	    // rightSubExpr="and (l_0==:0)"
    rightSubExpr = StringUtil::trim(rightSubExpr);
    t = rightSubExpr.c_str();
    size = rightSubExpr.size();
    for (i = 0; i < size; i++)
    {
        if (t[i] == '(')
            break;
    }
    operatType = rightSubExpr.substr(0, i) ;
    rightSubExpr = rightSubExpr.substr(i);
    rightSubExpr = StringUtil::trim(rightSubExpr);
    //去括号
    if (leftSubExpr.size() > 2)
    {
        leftSubExpr  = leftSubExpr.substr(1, leftSubExpr.size() - 2);
        leftSubExpr  = StringUtil::trim(leftSubExpr);
    }
    if (rightSubExpr.size() > 2)
    {
        rightSubExpr = rightSubExpr.substr(1, rightSubExpr.size() - 2);
        rightSubExpr = StringUtil::trim(rightSubExpr);
    }
    if (operatType.size() == 0)
        operatType = getOperatMark(leftSubExpr);
    operatType = StringUtil::trim(operatType);
}

string OperatorBase::getOperatMark(const string& expression)
{
    if (expression.find(">=") != string::npos)
    {
        return  ">=";
    }
    else if (expression.find(">") != string::npos)
    {
        return  ">";
    }
    else if (expression.find("!=") != string::npos)
    {
        return  "!=";
    }
    else if (expression.find("<=") != string::npos)
    {
        return  "<=";
    }
    else if (expression.find("<") != string::npos)
    {
        return  "<";
    }
    else if (expression.find("=") != string::npos)
    {
        return  "=";
    }
    else if (expression.find("neq") != string::npos)
    {
        return  "neq";
    }
    else if (expression.find("eq") != string::npos)
    {
        return  "eq";
    }
    return "";
}
