// ############################################
// Source file : ExpressionMgr.h
// Version     : 0.1.1
// Language	   : ANSI C++
// OS Platform : UNIX
// Authors     : chen min
// E_mail      : chenm@lianchuang.com
// Create      : 2008-5-2
// Update      : 2008-5-2
// Copyright(C): chen min, Linkage.
// ############################################

#ifndef EXPRESSIONMGR_H_INCLUDED
#define EXPRESSIONMGR_H_INCLUDED

#include <map>
#include <string>

#include "Expression.h"
#include "PublicInfo.h"

typedef map<string, Expression*> EXPRESSION_POOL;
typedef EXPRESSION_POOL::iterator EXPRESSION_POOL_ITR;

// session�ı��ʽ������
// ��¼��session��������,ʹ�ù��ı��ʽ,������ظ���ѯ��Ч��
class ExpressionMgr
{
    public:
        ExpressionMgr();
        ~ExpressionMgr();

        Expression* getExpre(const char* sInputExpre);   // ����Ϊ�û����ʽ,����Ϊ������ı��ʽ��

    private:

    private:
        EXPRESSION_POOL m_expressionMap;
};

#endif //EXPRESSIONMGR_H_INCLUDED

