// ############################################
// Source file : OperatLt.h
// Version     : 0.1.1
// Language	   : ANSI C++
// OS Platform : UNIX
// Authors     : chen min
// E_mail      : chenm@lianchuang.com
// Create      : 2008-4-23
// Update      : 2008-4-23
// Copyright(C): chen min, Linkage.
// ############################################

#ifndef OPERATLT_H_INCLUDED
#define OPERATLT_H_INCLUDED

#include "OperatorBase.h"

//## "<"ԭ�������
class OperatLt : public OperatorBase
{
    public:
        virtual ~OperatLt() {};

        virtual bool evaluate(RecordConvert* pRecordConvert
                              , const void** pParameters);

};

#endif /* OPERATLT_H_INCLUDED */
