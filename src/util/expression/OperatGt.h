// ############################################
// Source file : OperatGt.h
// Version     : 0.1.1
// Language	   : ANSI C++
// OS Platform : UNIX
// Authors     : chen min
// E_mail      : chenm@lianchuang.com
// Create      : 2008-4-23
// Update      : 2008-4-23
// Copyright(C): chen min, Linkage.
// ############################################

#ifndef OPERATGT_H_INCLUDED
#define OPERATGT_H_INCLUDED

#include "OperatorBase.h"

//## ">"т╜встккЦ╥Ш
class OperatGt : public OperatorBase
{
    public:
        virtual ~OperatGt() {};

        virtual bool evaluate(RecordConvert* pRecordConvert
                              , const void** pParameters);

};

#endif /* OPERATGT_H_INCLUDED */
