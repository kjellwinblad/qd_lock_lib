#ifndef DRMCS_LOCK_H
#define DRMCS_LOCK_H

#include "locks/oo_lock_interface.h"
#include "locks/mcs_lock.h"
#include "misc/padded_types.h"
#include "read_indicators/reader_groups_read_indicator.h"

#include "misc/bsd_stdatomic.h"//Until c11 stdatomic.h is available
#include "misc/thread_includes.h"//Until c11 thread.h is available
#include <stdbool.h>

#define DRMCS_READ_PATIENCE_LIMIT 130000

typedef struct {
    MCSLock lock;
    LLPaddedInt writeBarrier;
    ReaderGroupsReadIndicator readIndicator;
} DRMCSLock;

extern
_Alignas(CACHE_LINE_SIZE)
OOLockMethodTable DRMCS_LOCK_METHOD_TABLE;


void drmcs_initialize(DRMCSLock * lock);
void drmcs_lock(void * lock);
void drmcs_unlock(void * lock);
bool drmcs_is_locked(void * lock);
bool drmcs_try_lock(void * lock);
void drmcs_rlock(void * lock);
void drmcs_runlock(void * lock);
void drmcs_delegate(void * lock,
                    void (*funPtr)(unsigned int, void *), 
                    unsigned int messageSize,
                    void * messageAddress);
void * drmcs_delegate_or_lock(void * lock, unsigned int messageSize);
DRMCSLock * plain_drmcs_create();
OOLock * oo_drmcs_create();


#endif
