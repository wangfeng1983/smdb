#ifndef _LINKAGE_MDB_REDO_REDOLOCK_H_
#define _LINKAGE_MDB_REDO_REDOLOCK_H_

#include <string>

#include "RedoConst.h"
#include "Lock.h"

class MDBLatchMgr;
class SrcLatchInfo;

/// 使用 MDBLatchMgr 实现的 Redo 资源锁
class RedoLock : public Lock
{
    public:
        RedoLock(redo_const::lock::LockType type, int id, const char* dbname);
        virtual ~RedoLock(void);

        virtual bool initialize();

        virtual bool lock();
        virtual bool unlock();
        virtual bool trylock();

    private:
        redo_const::lock::LockType m_type;
        int m_id;
        std::string m_dbname;

        MDBLatchMgr* m_latchmgr;
        SrcLatchInfo* m_latchinfo;
};

#endif
