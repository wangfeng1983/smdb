#ifndef _LINKAGE_MDB_REDO_LOCK_H_
#define _LINKAGE_MDB_REDO_LOCK_H_

/// Ëø½Ó¿Ú
class Lock
{
    public:
        virtual ~Lock(void) {};

        virtual bool initialize(void) = 0;

        virtual bool lock(void) = 0;
        virtual bool unlock(void) = 0;
        virtual bool trylock(void) = 0;
};

#endif
