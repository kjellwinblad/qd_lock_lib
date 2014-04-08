#ifndef TATAS_LOCK_H
#define TATAS_LOCK_H

#include "locks/oo_lock_interface.h"
#include "misc/padded_types.h"

#include "misc/bsd_stdatomic.h"//Until c11 stdatomic.h is available
#include "misc/thread_includes.h"//Until c11 thread.h is available
#include <stdbool.h>


typedef struct TATASLockImpl {
    LLPaddedFlag lockFlag;
} TATASLock;


void tatas_initialize(TATASLock * lock);
void tatas_lock(void * lock);
static inline
void tatas_unlock(void * lock) {
    TATASLock *l = (TATASLock*)lock;
    atomic_flag_clear_explicit(&l->lockFlag.value, memory_order_release);
}
static inline
bool tatas_is_locked(void * lock){
    TATASLock *l = (TATASLock*)lock;
    return atomic_load(&l->lockFlag.value);
}
static inline
bool tatas_try_lock(void * lock) {
    TATASLock *l = (TATASLock*)lock;
    if(!atomic_load_explicit(&l->lockFlag.value, memory_order_acquire)){
        return !atomic_flag_test_and_set(&l->lockFlag.value);
    } else {
        return false;
    }
}
void tatas_delegate(void * lock,
                    void (*funPtr)(unsigned int, void *), 
                    unsigned int messageSize,
                    void * messageAddress);
void * tatas_delegate_or_lock(void * lock, unsigned int messageSize);
TATASLock * plain_tatas_create();
OOLock * oo_tatas_create();

#endif
