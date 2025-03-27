// ############################################
// Source file : OperatOr.h
// Version     : 0.1.1
// Language	   : ANSI C++
// OS Platform : UNIX
// Authors     : chen min
// E_mail      : chenm@lianchuang.com
// Create      : 2008-4-23
// Update      : 2008-4-23
// Copyright(C): chen min, Linkage.
// ############################################

#ifndef OPERATOR_H_INCLUDED_C03E3607
#define OPERATOR_H_INCLUDED_C03E3607

#include "OperatorBase.h"

// ����ԭ�������
class OperatOr : public OperatorBase
{
    public:
        virtual ~OperatOr() {};

        virtual bool evaluate(RecordConvert* pRecordConvert
                              , const void** pParameters);
};



#endif /* OPERATOR_H_INCLUDED */
