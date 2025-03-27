// ############################################
// Source file : OperatLe.h
// Version     : 0.1.1
// Language	   : ANSI C++
// OS Platform : UNIX
// Authors     : chen min
// E_mail      : chenm@lianchuang.com
// Create      : 2008-4-23
// Update      : 2008-4-23
// Copyright(C): chen min, Linkage.
// ############################################

#ifndef OPERATLE_H_INCLUDED
#define OPERATLE_H_INCLUDED

#include "OperatorBase.h"

//## "<="ԭ�������
class OperatLe : public OperatorBase
{
    public:
        virtual ~OperatLe() {};

        virtual bool evaluate(RecordConvert* pRecordConvert
                              , const void** pParameters);

};

#endif /* OPERATLE_H_INCLUDED */
