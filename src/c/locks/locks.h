#ifndef LOCKS_H
#define LOCKS_H

// Generic Lock API
// ================
// 
// This file provide a generic interface to all locks. The generic
// interface makes it possible to swap between lock implementations in
// your application with just a few line changes.
//
// To include this file:

//     #include "locks/tatas_lock.h"

#include "locks/tatas_lock.h"
#include "locks/qd_lock.h"
#include "misc/misc_utils.h"

// ## LL_initialize
// 
// `LL_initialize(X)` initializes a value of one of the lock types:

// * `TATASLock`
// * `QDLock`

// The paramter `X` is a pointer to a value of one of the lock types.

// *Example:*

//     TATASLock lock;
//     LL_initialize(&lock)

#define LL_initialize(X) _Generic((X),      \
     TATASLock * : tatas_initialize((TATASLock *)X), \
     QDLock * : qd_initialize((QDLock *)X)   \
                                )

// ## LL_destroy
// 
// `LL_destroy(X)` destorys an initialized lock value. The parameter X
// is a pointer to a lock value. The program might leak memory if
// `LL_destroy(X)` is not called after an initialized lock value is
// not used anymore.
#define LL_destroy(X) _Generic((X),      \
     default : UNUSED(X), \
                               )

// ## LL_create

// The `LL_create(X)` function creates a lock with the specified type
// and returns a pointer to the lock. Use LL_free to free the memory
// for the lock when it is not used anymore. The following lock type
// names can be given as parameters:

// * `TATAS_LOCK` gives the return type `*OOLock`
// * `QD_LOCK` gives the return type `*OOLock`
// * `PLAIN_TATAS_LOCK` gives the return type `*TATASLock`
// * `PLAIN_TATAS_LOCK` gives the return type `*QDLock`
typedef enum {
    TATAS_LOCK,
    QD_LOCK,
    PLAIN_TATAS_LOCK, 
    PLAIN_QD_LOCK,
} LL_lock_type_name;

// When calling `LL_*` functions the parameter must be of the correct
// lock type.
void * LL_create(LL_lock_type_name llLockType){
    if(TATAS_LOCK == llLockType){
        TATASLock * l = aligned_alloc(CACHE_LINE_SIZE, sizeof(TATASLock));
        LL_initialize(l);
        OOLock * ool = aligned_alloc(CACHE_LINE_SIZE, sizeof(OOLock));
        ool->lock = l;
        ool->m = &TATAS_LOCK_METHOD_TABLE;
        return ool;
    } else if (QD_LOCK == llLockType){
        QDLock * l = aligned_alloc(CACHE_LINE_SIZE, sizeof(QDLock));
        LL_initialize(l);
        OOLock * ool = aligned_alloc(CACHE_LINE_SIZE, sizeof(OOLock));
        ool->lock = l;
        ool->m = &QD_LOCK_METHOD_TABLE;
        return ool;
    }
    else if(PLAIN_TATAS_LOCK == llLockType){
        TATASLock * l = aligned_alloc(CACHE_LINE_SIZE, sizeof(TATASLock));
        LL_initialize(l);
        return l;
    } else if (PLAIN_QD_LOCK == llLockType){
        QDLock * l = aligned_alloc(CACHE_LINE_SIZE, sizeof(QDLock));
        LL_initialize(l);
        return l;
    }
    return NULL;/* Should not be reachable */
}

// ## LL_free

// `LL_free(X)` frees the memory of a lock created with `LL_create(X)`.

// *Example:*

//     OOLock * lock = LL_create(QD_LOCK);
//     LL_free(&lock)

#define LL_free(X) _Generic((X),\
    OOLock * : oolock_free((OOLock *)X),        \
    default : free(X)           \
                            )

#define LL_lock(X) _Generic((X),         \
    TATASLock *: tatas_lock((TATASLock *)X),                \
    QDLock * : tatas_lock(&((QDLock *)X)->mutexLock),       \
    OOLock * : ((OOLock *)X)->m->lock(((OOLock *)X)->lock) \
                            )

#define LL_unlock(X) _Generic((X),    \
    TATASLock *: tatas_unlock((TATASLock *)X), \
    QDLock * : tatas_unlock(&((QDLock *)X)->mutexLock), \
    OOLock * : ((OOLock *)X)->m->unlock(((OOLock *)X)->lock)      \
    )

#define LL_is_locked(X) _Generic((X),    \
    TATASLock *: tatas_is_locked((TATASLock *)X), \
    QDLock * : tatas_is_locked(&((QDLock *)X)->mutexLock), \
    OOLock * : ((OOLock *)X)->m->is_locked(((OOLock *)X)->lock)      \
    )

#define LL_try_lock(X) _Generic((X),    \
    TATASLock *: tatas_try_lock(X), \
    QDLock * : tatas_try_lock(&((QDLock *)X)->mutexLock), \
    OOLock * : ((OOLock *)X)->m->try_lock(((OOLock *)X)->lock)      \
    )

#define LL_delegate(X, funPtr, messageSize, messageAddress) _Generic((X),      \
    TATASLock *: tatas_delegate((TATASLock *)X, funPtr, messageSize, messageAddress), \
    QDLock * : qd_delegate((QDLock *)X, funPtr, messageSize, messageAddress), \
    OOLock * : ((OOLock *)X)->m->delegate(((OOLock *)X)->lock, funPtr, messageSize, messageAddress) \
    )

#endif
