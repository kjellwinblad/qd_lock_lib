#ifndef CCSYNCH_LOCK_H
#define CCSYNCH_LOCK_H

// Implementation of the CC-Synch algorithm as described in the paper:
// Revisiting the combining synchronization technique
// by Panagiota Fatourou and Nikolaos D. Kallimanis
// PPoPP '12 Proceedings of the 17th ACM SIGPLAN symposium on 
// Principles and Practice of Parallel Programming

#include "misc/bsd_stdatomic.h"//Until c11 stdatomic.h is available
#include "misc/thread_includes.h"//Until c11 thread.h is available
#include <stdbool.h>

#include "misc/padded_types.h"

#define CCSYNCH_BUFFER_SIZE 512
#define CCSYNCH_HAND_OFF_LIMIT 512

typedef struct {
    void (*requestFunction)(unsigned int, void *);
    unsigned int messageSize;
    unsigned char buffer[CCSYNCH_BUFFER_SIZE];
    volatile atomic_int wait;
    bool completed;
    volatile atomic_uintptr_t next;
} CCSynchLockNode;

typedef struct {
    volatile atomic_uintptr_t tailPtr;
} CCSynchLock;

__thread CCSynchLockNode * ccsynchNextLocalNode = NULL;

static inline void ccsynchlock_initNode(CCSynchLockNode * node){
    node->requestFunction = ATOMIC_VAR_INIT(NULL);
    node->messageSize = CCSYNCH_BUFFER_SIZE + 1;
    atomic_store_explicit(&node->wait, 0, memory_order_relaxed);
    node->completed = false;
    node->next = ATOMIC_VAR_INIT((uintptr_t)NULL);
}

static inline void ccsynchlock_initLocalIfNeeded(){
    if(ccsynchNextLocalNode == NULL){
        ccsynchNextLocalNode = aligned_alloc(CACHE_LINE_SIZE, sizeof(CCSynchLockNode));
        ccsynchlock_initNode(ccsynchNextLocalNode);
    }
}

void ccsynch_initialize(CCSynchLock * l){
    CCSynchLockNode * dummyNode = aligned_alloc(CACHE_LINE_SIZE, sizeof(CCSynchLockNode));
    ccsynchlock_initNode(dummyNode);
    atomic_store_explicit(&l->tailPtr, (uintptr_t)dummyNode, memory_order_release);
}


void ccsynch_lock(void * lock) {
    CCSynchLock *l = (CCSynchLock*)lock;
    CCSynchLockNode *nextNode;
    CCSynchLockNode *curNode;
    ccsynchlock_initLocalIfNeeded();
    nextNode = ccsynchNextLocalNode;
    atomic_store_explicit(&nextNode->next, (uintptr_t)NULL, memory_order_relaxed);
    nextNode->wait = true;
    nextNode->completed = false;
    curNode = (CCSynchLockNode *)atomic_exchange_explicit( &l->tailPtr, (uintptr_t)nextNode, memory_order_release);
    curNode->requestFunction = NULL;
    atomic_store_explicit(&curNode->next, (uintptr_t)nextNode, memory_order_release);
    ccsynchNextLocalNode = curNode;
    while (atomic_load_explicit(&curNode->wait, memory_order_acquire) == 1){
        thread_yield();
    }
}

void ccsynch_unlock(void * lock) {
    (void)lock;
    CCSynchLockNode *tmpNode;
    void (*tmpFunPtr)(unsigned int, void *);
    CCSynchLockNode *tmpNodeNext;
    int counter = 0;
    tmpNode = ccsynchNextLocalNode;
    while ((tmpNodeNext=(CCSynchLockNode *)atomic_load_explicit(&tmpNode->next, memory_order_acquire)) != NULL && counter < CCSYNCH_HAND_OFF_LIMIT) {
        counter = counter + 1;
        tmpFunPtr = tmpNode->requestFunction;
        if(tmpFunPtr==NULL){
            tmpNode = tmpNodeNext;
            break;
        }
        tmpFunPtr(tmpNode->messageSize, tmpNode->buffer);
        tmpNode->completed = true;
        atomic_store_explicit(&tmpNode->wait, 0, memory_order_release);
        tmpNode = tmpNodeNext;
    }
    atomic_store_explicit(&tmpNode->wait, 0, memory_order_release);
}

bool ccsynch_is_locked(void * lock){
    CCSynchLock *l = (CCSynchLock*)lock;
    CCSynchLockNode * tail =  (CCSynchLockNode *)atomic_load_explicit(&l->tailPtr, memory_order_acquire);
    return atomic_load_explicit(&tail->wait, memory_order_acquire) == 1;
}


bool ccsynch_try_lock(void * lock) {
    if(ccsynch_is_locked(lock)){
        return false;
    }else{
        ccsynch_lock(lock);
        return true;
    }
}

void ccsynch_delegate(void* lock,
                      void (*funPtr)(unsigned int, void *), 
                      unsigned int messageSize,
                      void * messageAddress) {
    CCSynchLock *l = (CCSynchLock*)lock;
    unsigned char * messageBuffer =  (unsigned char *) messageAddress;
    CCSynchLockNode *nextNode;
    CCSynchLockNode *curNode;
    CCSynchLockNode *tmpNode;
    void (*tmpFunPtr)(unsigned int, void *);
    CCSynchLockNode *tmpNodeNext;
    int counter = 0;
    ccsynchlock_initLocalIfNeeded();
    nextNode = ccsynchNextLocalNode;
    atomic_store_explicit(&nextNode->next, (uintptr_t)NULL, memory_order_relaxed);
    nextNode->wait = true;
    nextNode->completed = false;
    curNode = (CCSynchLockNode *)atomic_exchange_explicit(&l->tailPtr, (uintptr_t)nextNode, memory_order_release);
    if(messageSize < CCSYNCH_BUFFER_SIZE){
        for(unsigned int i = 0; i < messageSize; i++){
            curNode->buffer[i] = messageBuffer[i];
        }
        curNode->messageSize = messageSize;
        curNode->requestFunction = funPtr;
    }else{
        curNode->requestFunction = NULL;
    }
    atomic_store_explicit(&curNode->next, (uintptr_t)nextNode, memory_order_release);
    ccsynchNextLocalNode = curNode;
    while (atomic_load_explicit(&curNode->wait, memory_order_acquire) == 1){
        thread_yield();
    }
    if(curNode->completed==true){
        return;
    }else{
        funPtr(messageSize, messageBuffer);
    }
    tmpNode = (CCSynchLockNode *)atomic_load_explicit(&curNode->next, memory_order_acquire);
    while ((tmpNodeNext=(CCSynchLockNode *)atomic_load_explicit(&tmpNode->next, memory_order_acquire)) != NULL && counter < CCSYNCH_HAND_OFF_LIMIT) {
        counter = counter + 1;
        tmpFunPtr = tmpNode->requestFunction;
        if(tmpFunPtr==NULL){
            break;
        }
        tmpFunPtr(tmpNode->messageSize, tmpNode->buffer);
        tmpNode->completed = true;
        atomic_store_explicit(&tmpNode->wait, 0, memory_order_release);
        tmpNode = tmpNodeNext;
    }
    atomic_store_explicit(&tmpNode->wait, 0, memory_order_release);
}


void * ccsynch_delegate_or_lock(void* lock,
                                unsigned int messageSize) {
    (void)messageSize;
    ccsynch_lock(lock);//TODO do proper work here
    return NULL;
}

void ccsynch_close_delegate_buffer(void * buffer,
                                   void (*funPtr)(unsigned int, void *)){
    (void)buffer;
    (void)funPtr;
}

void ccsynch_delegate_unlock(void* lock) {
    ccsynch_unlock(lock);
}

_Alignas(CACHE_LINE_SIZE)
OOLockMethodTable CCSYNCH_LOCK_METHOD_TABLE = 
{
     .free = &free,
     .lock = &ccsynch_lock,
     .unlock = &ccsynch_unlock,
     .is_locked = &ccsynch_is_locked,
     .try_lock = &ccsynch_try_lock,
     .rlock = &ccsynch_lock,
     .runlock = &ccsynch_unlock,
     .delegate = &ccsynch_delegate,
     .delegate_or_lock = &ccsynch_delegate_or_lock,
     .close_delegate_buffer = &ccsynch_close_delegate_buffer,
     .delegate_unlock = &ccsynch_delegate_unlock
};

CCSynchLock * plain_ccsynch_create(){
    CCSynchLock * l = aligned_alloc(CACHE_LINE_SIZE, sizeof(CCSynchLock));
    ccsynch_initialize(l);
    return l;
}

OOLock * oo_ccsynch_create(){
    CCSynchLock * l = plain_ccsynch_create();
    OOLock * ool = aligned_alloc(CACHE_LINE_SIZE, sizeof(OOLock));
    ool->lock = l;
    ool->m = &CCSYNCH_LOCK_METHOD_TABLE;
    return ool;
}

#endif
