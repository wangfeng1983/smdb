// ############################################
// Source file : OperatStrnEq.h
// Version     : 0.1.1
// Language	   : ANSI C++
// OS Platform : UNIX
// Authors     : chen min
// E_mail      : chenm@lianchuang.com
// Create      : 2008-4-23
// Update      : 2008-4-23
// Copyright(C): chen min, Linkage.
// ############################################

#ifndef OPERATSTRNEQ_H_INCLUDED
#define OPERATSTRNEQ_H_INCLUDED

#include "OperatorBase.h"

// ���̳��Ƚ����ַ�����ȱȽϵ������
class OperatStrnEq : public OperatorBase
{
    public:
        virtual ~OperatStrnEq() {};

        virtual bool evaluate(RecordConvert* pRecordConvert
                              , const void** pParameters);
};



#endif /* OPERATSTRNEQ_H_INCLUDED */
