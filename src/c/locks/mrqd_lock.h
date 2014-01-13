#ifndef MRQD_LOCK_H
#define MRQD_LOCK_H

#include "misc/bsd_stdatomic.h"//Until c11 stdatomic.h is available
#include "misc/thread_includes.h"//Until c11 thread.h is available
#include <stdbool.h>

#include "misc/padded_types.h"
#include "locks/tatas_lock.h"
#include "qd_queues/qd_queue.h"

/* Read Indicator */

#ifndef MRQD_LOCK_NUMBER_OF_READER_GROUPS
#    define MRQD_LOCK_NUMBER_OF_READER_GROUPS 64
#endif

#define MRQD_LOCK_READER_GROUP_PER_THREAD 1

typedef struct {
    LLPaddedUInt readerGroups[MRQD_LOCK_NUMBER_OF_READER_GROUPS];
} ReaderGroupsReadIndicator;

volatile atomic_int rgri_get_thread_id_counter = ATOMIC_VAR_INIT(0);
typedef union {
    int value;
    char pad[CACHE_LINE_SIZE];
} RGRIGetThreadIDVarWrapper;
_Alignas(CACHE_LINE_SIZE)
_Thread_local RGRIGetThreadIDVarWrapper rgri_get_thread_id_var = {.value = -1};

static inline
int rgri_get_thread_id(){
    //Warning this is not guranteed to work well on all platforms
    if(rgri_get_thread_id_var.value > -1) {
        return rgri_get_thread_id_var.value;
    } else {
        rgri_get_thread_id_var.value = atomic_fetch_add(&rgri_get_thread_id_counter, 1);
        return rgri_get_thread_id_var.value;
    }
}

void rgri_arrive(ReaderGroupsReadIndicator * indicator){
    int index = rgri_get_thread_id() % MRQD_LOCK_NUMBER_OF_READER_GROUPS;
#ifdef MRQD_LOCK_READER_GROUP_PER_THREAD
    atomic_store_explicit(&indicator->readerGroups[index].value, 1, memory_order_release);
#else
    atomic_fetch_add_explicit(&indicator->readerGroups[index].value, 1, memory_order_release);
#endif
}

void rgri_depart(ReaderGroupsReadIndicator * indicator){
    int index = rgri_get_thread_id() % MRQD_LOCK_NUMBER_OF_READER_GROUPS;
#ifdef MRQD_LOCK_READER_GROUP_PER_THREAD
    atomic_store_explicit(&indicator->readerGroups[index].value, 0, memory_order_release);
#else
    atomic_fetch_sub_explicit(&indicator->readerGroups[index].value, 1, memory_order_release);
#endif
}

void rgri_wait_all_readers_gone(ReaderGroupsReadIndicator * indicator){
    for(int i = 0; i < MRQD_LOCK_NUMBER_OF_READER_GROUPS; i++){
        atomic_thread_fence(memory_order_seq_cst);
        while(0 < atomic_load_explicit(&indicator->readerGroups[i].value, memory_order_relaxed)){
            thread_yield();
        }
    }
}

/* Multiple Reader Queue Delegation Lock */

#ifndef MRQD_READ_PATIENCE_LIMIT
#    define MRQD_READ_PATIENCE_LIMIT 1000
#endif

typedef struct {
    TATASLock mutexLock;
    QDQueue queue;
    ReaderGroupsReadIndicator readIndicator;
    LLPaddedUInt writeBarrier;
} MRQDLock;

void mrqd_initialize(MRQDLock * lock){
    tatas_initialize(&lock->mutexLock);
    qdq_initialize(&lock->queue);
    atomic_store(&lock->writeBarrier.value, 0);
    for(int i = 0; i < MRQD_LOCK_NUMBER_OF_READER_GROUPS; i++){
        atomic_store(&lock->readIndicator.readerGroups[i].value, 0);
    }
}

void mrqd_lock(void * lock) {
    MRQDLock *l = (MRQDLock*)lock;
    while(atomic_load_explicit(&l->writeBarrier.value, memory_order_seq_cst) > 0){
        thread_yield();
    }
    tatas_lock(&l->mutexLock);
    rgri_wait_all_readers_gone(&l->readIndicator);
}

void mrqd_unlock(void * lock) {
    MRQDLock *l = (MRQDLock*)lock;
    tatas_unlock(&l->mutexLock);
}

bool mrqd_is_locked(void * lock){
    MRQDLock *l = (MRQDLock*)lock;
    return tatas_is_locked(&l->mutexLock);
}

bool mrqd_try_lock(void * lock) {
    MRQDLock *l = (MRQDLock*)lock;
    while(atomic_load_explicit(&l->writeBarrier.value, memory_order_seq_cst) > 0){
        thread_yield();
    }
    if(tatas_try_lock(&l->mutexLock)){
        rgri_wait_all_readers_gone(&l->readIndicator);
        return true;
    }else{
        return false;
    } 
}

void mrqd_rlock(void * lock) {
    MRQDLock *l = (MRQDLock*)lock;
    bool bRaised = false;
    int readPatience = 0;
 start:
    rgri_arrive(&l->readIndicator);
    if(tatas_is_locked(&l->mutexLock)) {
        rgri_depart(&l->readIndicator);
        while(tatas_is_locked(&l->mutexLock)) {
            thread_yield();
            if((readPatience == MRQD_READ_PATIENCE_LIMIT) && !bRaised) {
                atomic_fetch_add_explicit(&l->writeBarrier.value, 1, memory_order_seq_cst);
                bRaised = true;
            }
            readPatience = readPatience + 1;
        }
        goto start;
    }
    if(bRaised) {
        atomic_fetch_sub_explicit(&l->writeBarrier.value, 1, memory_order_seq_cst);
    }
}

void mrqd_runlock(void * lock) {
    MRQDLock *l = (MRQDLock*)lock;
    rgri_depart(&l->readIndicator);
}

void mrqd_delegate(void* lock,
                   void (*funPtr)(unsigned int, void *), 
                   unsigned int messageSize,
                   void * messageAddress) {
    MRQDLock *l = (MRQDLock*)lock;
    while(atomic_load_explicit(&l->writeBarrier.value, memory_order_seq_cst) > 0){
        thread_yield();
    }
    while(true) {
        if(tatas_try_lock(&l->mutexLock)) {
            qdq_open(&l->queue);
            rgri_wait_all_readers_gone(&l->readIndicator);
            funPtr(messageSize, messageAddress);
            qdq_flush(&l->queue);
            tatas_unlock(&l->mutexLock);
            return;
        } else if(qdq_enqueue(&l->queue,
                              funPtr,
                              messageSize,
                              messageAddress)){
            return;
        }
        thread_yield();
    }
}

void * mrqd_delegate_or_lock(void* lock,
                             unsigned int messageSize) {
    MRQDLock *l = (MRQDLock*)lock;
    void * buffer;
    while(atomic_load_explicit(&l->writeBarrier.value, memory_order_seq_cst) > 0){
        thread_yield();
    }
    while(true) {
        if(tatas_try_lock(&l->mutexLock)) {
            qdq_open(&l->queue);
            rgri_wait_all_readers_gone(&l->readIndicator);
            return NULL;
        } else if(NULL != (buffer = qdq_enqueue_get_buffer(&l->queue,
                                                           messageSize))){
            return buffer;
        }
        thread_yield();
    }
}

void mrqd_close_delegate_buffer(void * buffer,
                                void (*funPtr)(unsigned int, void *)){
    qdq_enqueue_close_buffer(buffer, funPtr);
}

void mrqd_delegate_unlock(void* lock) {
    MRQDLock *l = (MRQDLock*)lock;
    qdq_flush(&l->queue);
    tatas_unlock(&l->mutexLock);
}

_Alignas(CACHE_LINE_SIZE)
OOLockMethodTable MRQD_LOCK_METHOD_TABLE = 
{
    .free = &free,
    .lock = &mrqd_lock,
    .unlock = &mrqd_unlock,
    .is_locked = &mrqd_is_locked,
    .try_lock = &mrqd_try_lock,
    .rlock = &mrqd_rlock,
    .runlock = &mrqd_runlock,
    .delegate = &mrqd_delegate,
    .delegate_or_lock = &mrqd_delegate_or_lock,
    .close_delegate_buffer = &mrqd_close_delegate_buffer,
    .delegate_unlock = &mrqd_delegate_unlock
};

MRQDLock * plain_mrqd_create(){
    MRQDLock * l = aligned_alloc(CACHE_LINE_SIZE, sizeof(MRQDLock));
    mrqd_initialize(l);
    return l;
}

OOLock * oo_mrqd_create(){
    MRQDLock * l = plain_mrqd_create();
    OOLock * ool = aligned_alloc(CACHE_LINE_SIZE, sizeof(OOLock));
    ool->lock = l;
    ool->m = &MRQD_LOCK_METHOD_TABLE;
    return ool;
}

#endif
