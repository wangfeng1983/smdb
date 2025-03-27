// ############################################
// Source file : OperatEq.h
// Version     : 0.1.1
// Language	   : ANSI C++
// OS Platform : UNIX
// Authors     : chen min
// E_mail      : chenm@lianchuang.com
// Create      : 2008-4-23
// Update      : 2008-4-23
// Copyright(C): chen min, Linkage.
// ############################################

#ifndef OPERATEQ_H_INCLUDED
#define OPERATEQ_H_INCLUDED

#include "OperatorBase.h"

//## "=="Ô­×ÓÔËËã·û
class OperatEq : public OperatorBase
{
    public:
        virtual ~OperatEq() {};

        virtual bool evaluate(RecordConvert* pRecordConvert
                              , const void** pParameters);

};

#endif /* OPERATEQ_H_INCLUDED */
