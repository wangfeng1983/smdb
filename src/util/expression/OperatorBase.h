// ############################################
// Source file : OperatorBase.h
// Version     : 0.1.1
// Language	   : ANSI C++
// OS Platform : UNIX
// Authors     : chen min
// E_mail      : chenm@lianchuang.com
// Create      : 2008-4-23
// Update      : 2008-4-23
// Copyright(C): chen min, Linkage.
// ############################################

#ifndef OPERATORBASE_H_INCLUDED
#define OPERATORBASE_H_INCLUDED

#include "base/config-all.h"

#include <string>
#include <vector>

#include "MdbConstDef.h"
#include "RecordConvert.h"
//## ԭ�ӱ��ʽ����
class OperatorBase
{
    public:
        OperatorBase();
        virtual ~OperatorBase();

        OperatorBase* getOperator(const char* expression);
        virtual bool evaluate(RecordConvert* pRecordConvert
                              , const void** pParameters)
        {
            return true;
        }

    private:
        void getOperatType(const char* expression, string& expType, string& leftSubExpr, string& rightSubExpr);
        string getOperatMark(const string& expression);

    protected:
        string m_leftParamName;   // ���ʽ����ֶ��� ��:msisdn='13512519742'�е�'msisdn'
        int    m_rightParamNum;   // ���ʽ�Ҳ�ֵ�ڴ�����������е��±�ֵ,������,��Ϊ'0'

        OperatorBase* m_leftOperator;  //���ṹ����ڵ�
        OperatorBase* m_rightOperator; //���ṹ���ҽڵ�

        int m_iColumnLenth;            //�ڴ��¼�ֶ�ֵ�ĳ���
};

#endif /* OPERATORBASE_H_INCLUDED */
