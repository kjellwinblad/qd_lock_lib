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

void tatas_initialize(TATASLock * lock){
    atomic_init( &lock->lockFlag.value, false );
}

void tatas_lock(void * lock) {
    TATASLock *l = (TATASLock*)lock;
    while(true){
        while(atomic_load_explicit(&l->lockFlag.value, 
                                   memory_order_acquire)){
            thread_yield();
        }
        if( ! atomic_flag_test_and_set_explicit(&l->lockFlag.value,
                                                memory_order_acquire)){
            return;
        }
   }
}

void tatas_unlock(void * lock) {
    TATASLock *l = (TATASLock*)lock;
    atomic_flag_clear_explicit(&l->lockFlag.value, memory_order_release);
}

bool tatas_is_locked(void * lock){
    TATASLock *l = (TATASLock*)lock;
    return atomic_load(&l->lockFlag.value);
}

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
                    void * messageAddress){
    TATASLock *l = (TATASLock*)lock;
    tatas_lock(l);
    funPtr(messageSize, messageAddress);
    tatas_unlock(l);
}

void * tatas_delegate_or_lock(void * lock, unsigned int messageSize){
    (void)messageSize;
    TATASLock *l = (TATASLock*)lock;
    tatas_lock(l);
    return NULL;
}

_Alignas(CACHE_LINE_SIZE)
OOLockMethodTable TATAS_LOCK_METHOD_TABLE = 
{
     .free = &free,
     .lock = &tatas_lock,
     .unlock = &tatas_unlock,
     .is_locked = &tatas_is_locked,
     .try_lock = &tatas_try_lock,
     .rlock = &tatas_lock,
     .runlock = &tatas_unlock,
     .delegate = &tatas_delegate,
     .delegate_wait = &tatas_delegate,
     .delegate_or_lock = &tatas_delegate_or_lock,
     .close_delegate_buffer = NULL, /* Should never be called */
     .delegate_unlock = &tatas_unlock
};

TATASLock * plain_tatas_create(){
    TATASLock * l = aligned_alloc(CACHE_LINE_SIZE, sizeof(TATASLock));
    tatas_initialize(l);
    return l;
}

OOLock * oo_tatas_create(){
    TATASLock * l = plain_tatas_create();
    OOLock * ool = aligned_alloc(CACHE_LINE_SIZE, sizeof(OOLock));
    ool->lock = l;
    ool->m = &TATAS_LOCK_METHOD_TABLE;
    return ool;
}

#endif
