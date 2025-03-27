#include "RedoLock.h"

#include "MdbConstDef.h"
#include "MDBLatchMgr.h"

#include "debuglog.h"

RedoLock::RedoLock(redo_const::lock::LockType type, int id, const char* dbname)
    : m_type(type), m_id(id), m_dbname(dbname)
{
    m_latchmgr = 0;
    m_latchinfo = 0;
}

RedoLock::~RedoLock(void)
{
}

bool RedoLock::initialize(void)
{
    if (m_dbname.empty())
    {
        log_error("m_dbname.empty()");
        return false;
    }
    if (m_latchinfo != 0)
    {
        log_warn("class initialized");
        return true;
    }
    char lockname[64];
    if (m_type == redo_const::lock::BUFFER)
        ::sprintf(lockname, "Lock_RedoLogBuffer_%02d", m_id);
    else if (m_type == redo_const::lock::STATUS)
        ::sprintf(lockname, "Lock_LGWRRunStatus_%02d", m_id);
    else
    {
        log_error("invalid LockType " << m_type);
        return false;
    }
    if (m_latchmgr == 0)
    {
        try
        {
            m_latchmgr = MDBLatchMgr::getInstance(m_dbname.c_str());
        }
        catch (Mdb_Exception& e)
        {
            m_latchmgr = 0;
            log_error("MDBLatchMgr::getInstance(" << m_dbname
                      << ")  throws Mdb_Exception " << e.GetString());
            return false;
        }
    }
    try
    {
        m_latchinfo = m_latchmgr->getSrcLatchInfo(T_DBLK_TABLE, lockname);
    }
    catch (Mdb_Exception& e)
    {
        m_latchinfo = 0;
        log_error("MDBLatchMgr::getSrcLatchInfo(T_DBLK_TABLE, " << lockname
                  << ") throws Mdb_Exception " << e.GetString());
        return false;
    }
    return m_latchinfo != 0;
}

bool RedoLock::lock()
{
    try
    {
        m_latchmgr->mlatch_lock(m_latchinfo);
        return true;
    }
    catch (Mdb_Exception& e)
    {
        log_warn("MDBLatchMgr::mlatch_lock(m_latchinfo) throws Mdb_Exception "
                 << e.GetString());
        return false;
    }
}

bool RedoLock::unlock()
{
    try
    {
        m_latchmgr->mlatch_unlock(m_latchinfo);
        return true;
    }
    catch (Mdb_Exception& e)
    {
        log_warn("MDBLatchMgr::mlatch_unlock(m_latchinfo) throws Mdb_Exception " << e.GetString());
        return false;
    }
}

bool RedoLock::trylock()
{
    try
    {
        m_latchmgr->mlatch_trylock(m_latchinfo);
        return true;
    }
    catch (Mdb_Exception& e)
    {
        log_warn("MDBLatchMgr::mlatch_trylock(m_latchinfo) throws Mdb_Exception " << e.GetString());
        return false;
    }
}

