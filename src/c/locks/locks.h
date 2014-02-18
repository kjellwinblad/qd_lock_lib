#ifndef LOCKS_H
#define LOCKS_H

// Lock API
// ========
// 
// This file provide an interface to all locks. The interface makes it
// possible to swap between lock implementations in your application
// with just a few line changes.

// **NOTE FOR GCC** GCC does not provided the C11 keyword `_Generic`
// yet. The full API is therefore not supported when compiling with
// `scons --use_gcc`. The functions `LL_initialize` and `LL_destroy`
// are not supported with GCC. The `LL_create` function does not
// support a parameter staring with `PLAIN_*` with GCC.

// To include this file:

//     #include "locks/locks.h"

#include "locks/mcs_lock.h"
#include "locks/drmcs_lock.h"
#include "locks/tatas_lock.h"
#include "locks/qd_lock.h"
#include "locks/mrqd_lock.h"
#include "locks/ccsynch_lock.h"
#include "misc/misc_utils.h"
#include "misc/error_help.h"

// ## LL_initialize
// 
// `LL_initialize(X)` initializes a value of one of the lock types:

// * `TATASLock*`
// * `QDLock*`
// * `MRQDLock*`
// * `CCSynch*`
// * `TATASLock*`
// * `MCSLock`
// * `DRMCSLock`

// The paramter `X` is a pointer to a value of one of the lock types.

// *Example:*

//     TATASLock lock;
//     LL_initialize(&lock)
#ifdef __clang__
#define LL_initialize(X) _Generic((X),      \
     TATASLock * : tatas_initialize((TATASLock *)X), \
     QDLock * : qd_initialize((QDLock *)X), \
     CCSynchLock * : ccsynch_initialize((CCSynchLock * )X), \
     MCSLock * : mcs_initialize((MCSLock * )X), \
     MRQDLock * : mrqd_initialize((MRQDLock *)X) \
                                )
#else
#define LL_initialize(X) LL_error_and_exit("Function not supported on GCC\n")
#endif
// ## LL_destroy
// 
// `LL_destroy(X)` destorys an initialized lock value. The parameter X
// is a pointer to a lock value. This call can free resources
// allocated by `LL_initialize(X)`.
#ifdef __clang__
#define LL_destroy(X) _Generic((X),      \
     default : UNUSED(X), \
                               )
#else
#define LL_destroy(X) LL_error_and_exit("Function not supported on GCC\n")
#endif

// ## LL_create

// The `LL_create(X)` function creates a lock with the specified type
// and returns a pointer to the lock. Use `LL_free` to free the memory
// for the lock when it is not used anymore. The following lock type
// names can be given as parameters:

// * `TATAS_LOCK` gives the return type `OOLock *`
// * `QD_LOCK` gives the return type `OOLock *`
// * `MRQD_LOCK` gives the return type `OOLock *`
// * `CCSYNCH_LOCK` gives the return type `OOLock *`
// * `MCS_LOCK` gives the return type `OOLock *`
// * `DRMCS_LOCK` gives the return type `OOLock *`
// * `PLAIN_TATAS_LOCK` gives the return type `TATASLock *`
// * `PLAIN_QD_LOCK` gives the return type `QDLock *`
// * `PLAIN_MRQD_LOCK` gives the return type `MRQDLock *`
// * `PLAIN_CCSYNCH_LOCK` gives the return type `CCSynchLock *`
// * `PLAIN_MCS_LOCK` gives the return type `MCSLock *`
// * `PLAIN_DRMCS_LOCK` gives the return type `DRMCSLock *`

typedef enum {
    DRMCS_LOCK,
    MCS_LOCK,
    TATAS_LOCK,
    QD_LOCK,
    CCSYNCH_LOCK,
    MRQD_LOCK,
    PLAIN_MCS_LOCK, 
    PLAIN_DRMCS_LOCK, 
    PLAIN_TATAS_LOCK, 
    PLAIN_QD_LOCK,
    PLAIN_CCSYNCH_LOCK,
    PLAIN_MRQD_LOCK
} LL_lock_type_name;

// When calling `LL_*` functions the parameter must be of the correct
// lock type.
void * LL_create(LL_lock_type_name llLockType){
    if(TATAS_LOCK == llLockType){
        return oo_tatas_create();
    } else if (QD_LOCK == llLockType){
        return oo_qd_create();
    } else if (CCSYNCH_LOCK == llLockType){
        return oo_ccsynch_create();
    } else if (MRQD_LOCK == llLockType){
        return oo_mrqd_create();
    }else if (MCS_LOCK == llLockType){
        return oo_mcs_create();
    }else if (DRMCS_LOCK == llLockType){
        return oo_drmcs_create();
    }
#ifdef __clang__
    else if(PLAIN_TATAS_LOCK == llLockType){
        return plain_tatas_create();
    } else if (PLAIN_QD_LOCK == llLockType){
        return plain_qd_create();
    } else if (PLAIN_CCSYNCH_LOCK == llLockType){
        return plain_ccsynch_create();
    } else if (PLAIN_MRQD_LOCK == llLockType){
        return plain_mrqd_create();
    }else if (PLAIN_MCS_LOCK == llLockType){
        return plain_mcs_create();
    }else if (PLAIN_DRMCS_LOCK == llLockType){
        return plain_drmcs_create();
    }
#endif
    LL_error_and_exit("Lock type not supprted with GCC\n");
    return NULL;/* Should not be reachable */
}

// ## LL_free

// `LL_free(X)` frees the memory of a lock created with `LL_create(X)`.

// *Example:*

//     OOLock * lock = LL_create(QD_LOCK);
//     LL_free(lock)
#ifdef __clang__
#define LL_free(X) _Generic((X),\
    OOLock * : oolock_free((OOLock *)X),        \
    default : free(X)           \
                            )
#else
#define LL_free(X) oolock_free((OOLock *)X)
#endif

// ## LL_lock

// `LL_lock(X)` acquires the lock X. Only one thread can hold the lock
// at a given moment. This call will block until X is acquired. X is a
// pointer to a value of a lock type.
#ifdef __clang__
#define LL_lock(X) _Generic((X),         \
    TATASLock *: tatas_lock((TATASLock *)X),                \
    QDLock * : tatas_lock(&((QDLock *)X)->mutexLock),       \
    CCSynchLock * : ccsynch_lock(X),       \
    MRQDLock * : mrqd_lock((MRQDLock *)X),       \
    MCSLock * : mcs_lock((MCSLock *)X),       \
    DRMCSLock * : drmcs_lock((DRMCSLock *)X),       \
    OOLock * : ((OOLock *)X)->m->lock(((OOLock *)X)->lock) \
                                )
#else
#define LL_lock(X) ((OOLock *)X)->m->lock(((OOLock *)X)->lock)
#endif                  

// ## LL_unlock

// `LL_unlock(X)` unlocks the acquired lock `X`. The behaviour is
// undefined if the lock X was not acquired by the current thread
// before the call to `LL_unlock(X)`.

// Example:

//     OOLock * lock = LL_create(QD_LOCK);
//     LL_lock(lock)
//     Critical section code...
//     LL_unlock(lock)
#ifdef __clang__
#define LL_unlock(X) _Generic((X),    \
    TATASLock *: tatas_unlock((TATASLock *)X), \
    QDLock * : tatas_unlock(&((QDLock *)X)->mutexLock), \
    CCSynchLock * : ccsynch_unlock(X), \
    MRQDLock * : tatas_unlock(&((MRQDLock *)X)->mutexLock), \
    MCSLock * : mcs_unlock(X), \
    DRMCSLock * : drmcs_unlock(X), \
    OOLock * : ((OOLock *)X)->m->unlock(((OOLock *)X)->lock)      \
    )
#else
#define LL_unlock(X) ((OOLock *)X)->m->unlock(((OOLock *)X)->lock)
#endif

// ## LL_is\_locked

// `LL_is_locked(X)` returns true if the lock `X` is locked and false
// if it is not locked. X is a pointer to a value of a lock type.
#ifdef __clang__
#define LL_is_locked(X) _Generic((X),    \
    TATASLock *: tatas_is_locked((TATASLock *)X), \
    QDLock * : tatas_is_locked(&((QDLock *)X)->mutexLock), \
    CCSynchLock * : ccsynch_is_locked(X), \
    MCSLock * : mcs_is_locked(X), \
    DRMCSLock * : drmcs_is_locked(X), \
    MRQDLock * : tatas_is_locked(&((MRQDLock *)X)->mutexLock), \
    OOLock * : ((OOLock *)X)->m->is_locked(((OOLock *)X)->lock)      \
    )
#else
#define LL_is_locked(X) ((OOLock *)X)->m->is_locked(((OOLock *)X)->lock)
#endif

// ## LL_try\_lock

// `LL_try_lock(X)` tries to lock `X`. The function returns true if
// `X` was successfully locked and false otherwise. X is a pointer to a
// value of a lock type.
#ifdef __clang__
#define LL_try_lock(X) _Generic((X),    \
    TATASLock *: tatas_try_lock(X), \
    MRQDLock * : tatas_try_lock(&((MRQDLock *)X)->mutexLock), \
    QDLock * : tatas_try_lock(&((QDLock *)X)->mutexLock), \
    CCSynchLock * : ccsynch_try_lock(X), \
    MCSLock * : mcs_try_lock(X), \
    DRMCSLock * : drmcs_try_lock(X), \
    OOLock * : ((OOLock *)X)->m->try_lock(((OOLock *)X)->lock)      \
    )
#else
#define LL_try_lock(X) ((OOLock *)X)->m->try_lock(((OOLock *)X)->lock)
#endif

// ## LL_rlock

#ifdef __clang__
#define LL_rlock(X) _Generic((X),         \
    TATASLock *: tatas_lock((TATASLock *)X),                \
    QDLock * : tatas_lock(&((QDLock *)X)->mutexLock),       \
    CCSynchLock * : ccsynch_lock(X),       \
    MCSLock * : mcs_lock(X),       \
    DRMCSLock * : drmcs_lock(X),       \
    MRQDLock * : mrqd_rlock((MRQDLock *)X),       \
    OOLock * : ((OOLock *)X)->m->rlock(((OOLock *)X)->lock) \
                                )
#else
#define LL_rlock(X) ((OOLock *)X)->m->rlock(((OOLock *)X)->lock)
#endif                  

// ## LL_runlock

#ifdef __clang__
#define LL_runlock(X) _Generic((X),    \
    TATASLock *: tatas_unlock((TATASLock *)X), \
    QDLock * : tatas_unlock(&((QDLock *)X)->mutexLock), \
    CCSynchLock * : ccsynch_unlock(X), \
    MCSLock * : mcs_unlock(X), \
    DRMCSLock * : drmcs_unlock(X), \
    MRQDLock * : mrqd_runlock((MRQDLock *)X), \
    OOLock * : ((OOLock *)X)->m->runlock(((OOLock *)X)->lock)      \
    )
#else
#define LL_runlock(X) ((OOLock *)X)->m->runlock(((OOLock *)X)->lock)
#endif


// ## LL_delegate

// A call to `LL_delegate(X, funPtr, messageSize, messageAddress)`
// guaratees:

// * that the function pointed to by `funPtr` will be executed with
//   the message specifed with `messageSize` and `messageAddress` as
//   paramter,

// * that the execution of the function pointed to by `funPtr` is
//   serialized with other critical sections for the lock X (including
//   other delegated functions),

// * and finally that no critical section for the lock `X` (including
//   other delegated functions) that is issued after the a delegation
//   of a function will execute before that delegated function.

// The function pointed to by `funPtr` can be executed by another
// thread that is currently holding the lock so special care has to be
// taken if thread local variables are accessed inside the function
// `funPtr`.

// *Parameters:*

// * `X` is a pointer to the lock data structure.

// * `funPtr` has type `void (*)(unsigned int, void *)`. The first
//   parameter to funPtr will have the same value as `messageSize` and
//   the second parameter will be a pointer to a copy of the message
//   data pointed to by `messageAddress`.

// * `messageSize` has the type `unsigned int` and should contain the
//   size of the message in bytes stored at `messageAddress`.

// * `messageAddress` has the type `void *` and should contain a
//   pointer to the message of size `messageSize`

// A return value from a function that is delegated with `LL_delegate`
// can be written back if one specify a write back address in the
// message that given as parameter to the function. This
// [example](../examples/qd_lock_delegate_example.html) shows how one
// can use the `LL_delegate` function and also how values can be
// written back.

#ifdef __clang__
#define LL_delegate(X, funPtr, messageSize, messageAddress) _Generic((X),      \
    TATASLock *: tatas_delegate((TATASLock *)X, funPtr, messageSize, messageAddress), \
    QDLock * : qd_delegate((QDLock *)X, funPtr, messageSize, messageAddress), \
    CCSynchLock * : ccsynch_delegate(X, funPtr, messageSize, messageAddress), \
    MCSLock * : mcs_delegate(X, funPtr, messageSize, messageAddress), \
    DRMCSLock * : drmcs_delegate(X, funPtr, messageSize, messageAddress), \
    MRQDLock * : mrqd_delegate((MRQDLock *)X, funPtr, messageSize, messageAddress), \
    OOLock * : ((OOLock *)X)->m->delegate(((OOLock *)X)->lock, funPtr, messageSize, messageAddress) \
    )
#else
#define LL_delegate(X, funPtr, messageSize, messageAddress) ((OOLock *)X)->m->delegate(((OOLock *)X)->lock, funPtr, messageSize, messageAddress)
#endif

// ## LL_delegate_wait

// Works in the same way as LL_delegate but the function will not
// return until the delegated critical section has exexuted

#ifdef __clang__
#define LL_delegate_wait(X, funPtr, messageSize, messageAddress) _Generic((X),      \
    TATASLock *: tatas_delegate((TATASLock *)X, funPtr, messageSize, messageAddress), \
    QDLock * : qd_delegate_wait((QDLock *)X, funPtr, messageSize, messageAddress), \
    CCSynchLock * : ccsynch_delegate(X, funPtr, messageSize, messageAddress), \
    MCSLock * : mcs_delegate(X, funPtr, messageSize, messageAddress), \
    DRMCSLock * : drmcs_delegate(X, funPtr, messageSize, messageAddress), \
    MRQDLock * : mrqd_delegate_wait((MRQDLock *)X, funPtr, messageSize, messageAddress), \
    OOLock * : ((OOLock *)X)->m->delegate_wait(((OOLock *)X)->lock, funPtr, messageSize, messageAddress) \
    )
#else
#define LL_delegate_wait(X, funPtr, messageSize, messageAddress) ((OOLock *)X)->m->delegate_wait(((OOLock *)X)->lock, funPtr, messageSize, messageAddress)
#endif


// ## LL_delegate_or_lock

// See the tutorial located at
// https://github.com/kjellwinblad/qd_lock_lib/wiki/Tutorial for
// information about how to use the LL_delegate_or_lock family of
// functions.

#ifdef __clang__
#define LL_delegate_or_lock(X, messageSize) _Generic((X),             \
    TATASLock *: tatas_delegate_or_lock((TATASLock *)X, messageSize), \
    QDLock * : qd_delegate_or_lock((QDLock *)X, messageSize), \
    CCSynchLock * : ccsynch_delegate_or_lock(X, messageSize), \
    MCSLock * : mcs_delegate_or_lock(X, messageSize), \
    DRMCSLock * : drmcs_delegate_or_lock(X, messageSize), \
    MRQDLock * : mrqd_delegate_or_lock((MRQDLock *)X, messageSize), \
    OOLock * : ((OOLock *)X)->m->delegate_or_lock(((OOLock *)X)->lock, messageSize) \
    )
#else
#define LL_delegate_or_lock(X, messageSize) ((OOLock *)X)->m->delegate_or_lock(((OOLock *)X)->lock, messageSize)
#endif

#ifdef __clang__
#define LL_close_delegate_buffer(X, buffer, funPtr) _Generic((X),        \
    TATASLock *: printf("Can not be called\n"), \
    QDLock * : qd_close_delegate_buffer(buffer, funPtr), \
    CCSynchLock * : ccsynch_close_delegate_buffer(buffer, funPtr), \
    MCSLock * : printf("Can not be called\n"), \
    DRMCSLock * : printf("Can not be called\n"), \
    MRQDLock * : mrqd_close_delegate_buffer(buffer, funPtr), \
    OOLock * : ((OOLock *)X)->m->close_delegate_buffer(buffer, funPtr) \
    )
#else
#define LL_close_delegate_buffer(X, buffer, funPtr) ((OOLock *)X)->m->close_delegate_buffer(buffer, funPtr)
#endif


#ifdef __clang__
#define LL_delegate_unlock(X) _Generic((X),         \
    TATASLock *: tatas_unlock((TATASLock *)X),                \
    QDLock * : qd_delegate_unlock(((QDLock *)X)), \
    CCSynchLock * : ccsynch_delegate_unlock(((QDLock *)X)), \
    MCSLock * : mcs_unlock(((QDLock *)X)), \
    DRMCSLock * : drmcs_unlock(((QDLock *)X)), \
    MRQDLock * : mrqd_delegate_unlock((MRQDLock *)X),       \
    OOLock * : ((OOLock *)X)->m->delegate_unlock(((OOLock *)X)->lock) \
                                )
#else
#define LL_delegate_unlock(X) ((OOLock *)X)->m->delegate_unlock(((OOLock *)X)->lock)
#endif

#endif
