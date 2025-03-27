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

// ���ʽ������,��Ž�����ı��ʽ��root������ָ��

#include "OperatorBase.h"
#include "Mdb_Exception.h"

class Expression
{
    public:
        Expression();
        ~Expression();

        bool setOperator(const char* expression);                        // �������ʽ,�γ�operator��״�ṹ
        bool evaluate(void* pRecord
                      , RecordConvert* pRecordConvert
                      , const void** pParameters);		//  �����ѯ�ֶ�ֵ,����pRecordConvert����pRecord,�ж��Ƿ�����Ҫ��

    private:
        OperatorBase* m_rootOperator;

};

#endif /* EXPRESSION_H_INCLUDED */
