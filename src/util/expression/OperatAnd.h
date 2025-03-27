// ############################################
// Source file : OperatAnd.h
// Version     : 0.1.1
// Language	   : ANSI C++
// OS Platform : UNIX
// Authors     : chen min
// E_mail      : chenm@lianchuang.com
// Create      : 2008-4-23
// Update      : 2008-4-23
// Copyright(C): chen min, Linkage.
// ############################################

#ifndef OPERATAND_H_INCLUDED
#define OPERATAND_H_INCLUDED

#include "OperatorBase.h"

//## 并且原子运算符
class OperatAnd : public OperatorBase
{
    public:
        virtual ~OperatAnd() {};

        virtual bool evaluate(RecordConvert* pRecordConvert
                              , const void** pParameters);

};

#endif /* OPERATAND_H_INCLUDED */
