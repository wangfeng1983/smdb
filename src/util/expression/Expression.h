// ############################################
// Source file : Expresion.h
// Version     : 0.1.1
// Language	   : ANSI C++
// OS Platform : UNIX
// Authors     : chen min
// E_mail      : chenm@lianchuang.com
// Create      : 2008-4-23
// Update      : 2008-4-23
// Copyright(C): chen min, Linkage.
// ############################################

#ifndef EXPRESSION_H_INCLUDED
#define EXPRESSION_H_INCLUDED

// 表达式解析类,存放解析后的表达式的root操作符指针

#include "OperatorBase.h"
#include "Mdb_Exception.h"

class Expression
{
    public:
        Expression();
        ~Expression();

        bool setOperator(const char* expression);                        // 解析表达式,形成operator树状结构
        bool evaluate(void* pRecord
                      , RecordConvert* pRecordConvert
                      , const void** pParameters);		//  传入查询字段值,根据pRecordConvert解析pRecord,判断是否满足要求

    private:
        OperatorBase* m_rootOperator;

};

#endif /* EXPRESSION_H_INCLUDED */
