#ifndef MCS_LOCK_H
#define MCS_LOCK_H

#include "locks/oo_lock_interface.h"
#include "misc/padded_types.h"

#include "misc/bsd_stdatomic.h"//Until c11 stdatomic.h is available
#include "misc/thread_includes.h"//Until c11 thread.h is available
#include <stdbool.h>

// WARNING: DOES NOT WORK FOR NESTED LOCKS

typedef struct {
    LLPaddedPointer next;
    LLPaddedInt locked;
} MCSNode;

typedef struct {
    LLPaddedPointer endOfQueue;
} MCSLock;


void mcs_initialize(MCSLock * lock);
bool mcs_lock_status(void * lock); //Not part of public API but is used by DRMCS
void mcs_lock(void * lock);
void mcs_unlock(void * lock);
static inline
bool mcs_is_locked(void * lock){
    MCSLock * l = lock;
    return atomic_load(&l->endOfQueue.value) != (intptr_t)NULL;
}
bool mcs_try_lock(void * lock);
void mcs_delegate(void * lock,
                  void (*funPtr)(unsigned int, void *), 
                  unsigned int messageSize,
                  void * messageAddress);
void * mcs_delegate_or_lock(void * lock, unsigned int messageSize);
MCSLock * plain_mcs_create();
OOLock * oo_mcs_create();

#endif
