// ############################################
// Source file : OperatorBase.h
// Version     : 0.1.1
// Language	   : ANSI C++
// OS Platform : UNIX
// Authors     : chen min
// E_mail      : chenm@lianchuang.com
// Create      : 2008-4-23
// Update      : 2008-4-23
// Copyright(C): chen min, Linkage.
// ############################################

#ifndef OPERATORBASE_H_INCLUDED
#define OPERATORBASE_H_INCLUDED

#include "base/config-all.h"

#include <string>
#include <vector>

#include "MdbConstDef.h"
#include "RecordConvert.h"
//## 原子表达式基类
class OperatorBase
{
    public:
        OperatorBase();
        virtual ~OperatorBase();

        OperatorBase* getOperator(const char* expression);
        virtual bool evaluate(RecordConvert* pRecordConvert
                              , const void** pParameters)
        {
            return true;
        }

    private:
        void getOperatType(const char* expression, string& expType, string& leftSubExpr, string& rightSubExpr);
        string getOperatMark(const string& expression);

    protected:
        string m_leftParamName;   // 表达式左侧字段名 如:msisdn='13512519742'中的'msisdn'
        int    m_rightParamNum;   // 表达式右侧值在传入参数数组中的下标值,如上例,则为'0'

        OperatorBase* m_leftOperator;  //树结构的左节点
        OperatorBase* m_rightOperator; //树结构的右节点

        int m_iColumnLenth;            //内存记录字段值的长度
};

#endif /* OPERATORBASE_H_INCLUDED */
