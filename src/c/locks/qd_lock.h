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

void qd_initialize(QDLock * lock);
void qd_lock(void * lock);
void qd_unlock(void * lock);
static inline
bool qd_is_locked(void * lock){
    QDLock *l = (QDLock*)lock;
    return tatas_is_locked(&l->mutexLock);
}
bool qd_try_lock(void * lock);
void qd_delegate(void* lock,
                 void (*funPtr)(unsigned int, void *), 
                 unsigned int messageSize,
                 void * messageAddress);
void * qd_delegate_or_lock(void* lock,
                           unsigned int messageSize);
void qd_close_delegate_buffer(void * buffer,
                              void (*funPtr)(unsigned int, void *));
void qd_delegate_unlock(void* lock);
void qd_delegate_wait(void* lock,
                      void (*funPtr)(unsigned int, void *), 
                      unsigned int messageSize,
                      void * messageAddress);
QDLock * plain_qd_create();
OOLock * oo_qd_create();

#endif
