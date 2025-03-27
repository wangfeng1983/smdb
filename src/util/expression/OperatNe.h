// ############################################
// Source file : OperatNe.h
// Version     : 0.1.1
// Language	   : ANSI C++
// OS Platform : UNIX
// Authors     : chen min
// E_mail      : chenm@lianchuang.com
// Create      : 2008-4-23
// Update      : 2008-4-23
// Copyright(C): chen min, Linkage.
// ############################################

#ifndef OPERATNE_H_INCLUDED
#define OPERATNE_H_INCLUDED

#include "OperatorBase.h"

// "!="Ô­×ÓÔËËã·û
class OperatNe : public OperatorBase
{
    public:
        virtual ~OperatNe() {};

        virtual bool evaluate(RecordConvert* pRecordConvert
                              , const void** pParameters);
};



#endif /* OPERATNE_H_INCLUDED */
