#ifndef QD_LOCK_H
#define QD_LOCK_H

#include "misc/bsd_stdatomic.h"//Until c11 stdatomic.h is available
#include "misc/thread_includes.h"//Until c11 thread.h is available
#include <stdbool.h>

#include "misc/padded_types.h"
#include "locks/tatas_lock.h"
#include "qd_queues/qd_queue.h"

/* Queue Delegation Lock */

typedef struct {
    TATASLock mutexLock;
    QDQueue queue;
} QDLock;

void qd_initialize(QDLock * lock){
    tatas_initialize(&lock->mutexLock);
    qdq_initialize(&lock->queue);
}

void qd_lock(void * lock) {
    QDLock *l = (QDLock*)lock;
    tatas_lock(&l->mutexLock);
}

void qd_unlock(void * lock) {
    QDLock *l = (QDLock*)lock;
    tatas_unlock(&l->mutexLock);
}

bool qd_is_locked(void * lock){
    QDLock *l = (QDLock*)lock;
    return tatas_is_locked(&l->mutexLock);
}

bool qd_try_lock(void * lock) {
    QDLock *l = (QDLock*)lock;
    return tatas_try_lock(&l->mutexLock);
}

void qd_delegate(void* lock,
                 void (*funPtr)(unsigned int, void *), 
                 unsigned int messageSize,
                 void * messageAddress) {
    QDLock *l = (QDLock*)lock;
    while(true) {
        if(tatas_try_lock(&l->mutexLock)) {
            qdq_open(&l->queue);
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

_Alignas(CACHE_LINE_SIZE)
OOLockMethodTable QD_LOCK_METHOD_TABLE = 
{
     .free = &free,
     .lock = &qd_lock,
     .unlock = &qd_unlock,
     .is_locked = &qd_is_locked,
     .try_lock = &qd_try_lock,
     .rlock = &qd_lock,
     .runlock = &qd_unlock,
     .delegate = &qd_delegate
};

QDLock * plain_qd_create(){
    QDLock * l = aligned_alloc(CACHE_LINE_SIZE, sizeof(QDLock));
    qd_initialize(l);
    return l;
}

OOLock * oo_qd_create(){
    QDLock * l = plain_qd_create();
    OOLock * ool = aligned_alloc(CACHE_LINE_SIZE, sizeof(OOLock));
    ool->lock = l;
    ool->m = &QD_LOCK_METHOD_TABLE;
    return ool;
}

#endif
