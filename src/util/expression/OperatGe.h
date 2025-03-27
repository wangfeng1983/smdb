// ############################################
// Source file : OperatGe.h
// Version     : 0.1.1
// Language	   : ANSI C++
// OS Platform : UNIX
// Authors     : chen min
// E_mail      : chenm@lianchuang.com
// Create      : 2008-4-23
// Update      : 2008-4-23
// Copyright(C): chen min, Linkage.
// ############################################

#ifndef OPERATGE_H_INCLUDED
#define OPERATGE_H_INCLUDED

#include "OperatorBase.h"

//## ">="т╜встккЦ╥Ш
class OperatGe : public OperatorBase
{
    public:
        virtual ~OperatGe() {};

        virtual bool evaluate(RecordConvert* pRecordConvert
                              , const void** pParameters);

};

#endif /* OPERATAND_H_INCLUDED */
