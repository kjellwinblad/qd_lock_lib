#ifndef QD_QUEUE_H
#define QD_QUEUE_H

#include <stdint.h>
#include <stdbool.h>
#include <limits.h>

#include "misc/padded_types.h"
#include "misc/bsd_stdatomic.h"//Until c11 stdatomic.h is available
#include "misc/thread_includes.h"//Until c11 thread.h is available
#include "locks/tatas_lock.h"

/* Queue Delegation Queue */

#ifndef QD_QUEUE_BUFFER_SIZE
#define QD_QUEUE_BUFFER_SIZE 4096
#endif
#define QD_QUEUE_EMPTY_POS 1
#define QD_QUEUE_EMPTY_POS_FULL 2
#define QDQ_CALCULATE_PAD(size) (sizeof(atomic_intptr_t) - 1) & (sizeof(atomic_intptr_t) - (size & (sizeof(atomic_intptr_t) - 1)))

typedef struct QDQueueRequestIDImpl {
    volatile atomic_uintptr_t requestIdentifier;
    uintptr_t messageSize;
} QDRequestRequestId;

typedef struct QDQueueImpl {
    LLPaddedULong counter;
    LLPaddedBool closed;
    unsigned char buffer[QD_QUEUE_BUFFER_SIZE];
} QDQueue;



static inline void qdq_initialize(QDQueue * q){
    for(int i = 0; i < QD_QUEUE_BUFFER_SIZE; i = i + sizeof(atomic_uintptr_t)){
        volatile atomic_uintptr_t * ptr = (void*)&q->buffer[i];
        atomic_store_explicit(ptr,
                              QD_QUEUE_EMPTY_POS,
                              memory_order_relaxed);
    }
    atomic_store_explicit( &q->counter.value,
                           QD_QUEUE_BUFFER_SIZE, 
                           memory_order_relaxed );
    atomic_store_explicit( &q->closed.value,
                           true,
                           memory_order_release );
}

static inline void qdq_open(QDQueue* q) {
    atomic_store_explicit( &q->counter.value,
                           0, 
                           memory_order_relaxed );
    atomic_store_explicit( &q->closed.value,
                           false, 
                           memory_order_release );
}

static inline void * qdq_enqueue_get_buffer(QDQueue* q,
                                     unsigned int messageSize) {
    if(atomic_load_explicit( &q->closed.value, memory_order_acquire )){
        return NULL;
    }
    unsigned int storeSize = sizeof(QDRequestRequestId) + messageSize;
    unsigned int pad = QDQ_CALCULATE_PAD(storeSize);
    unsigned long bufferOffset = atomic_fetch_add(&q->counter.value,
                                                  storeSize + pad);
    unsigned int writeToOffset = (bufferOffset + storeSize);
    unsigned int nextReqOffset = writeToOffset + pad;
    QDRequestRequestId * reqId = 
        (QDRequestRequestId*)&q->buffer[bufferOffset];
    if(nextReqOffset <= QD_QUEUE_BUFFER_SIZE) {
        reqId->messageSize = messageSize;
        unsigned long messageBodyStart = bufferOffset + sizeof(QDRequestRequestId);
        return (void*)&q->buffer[messageBodyStart];
    } else if(bufferOffset < QD_QUEUE_BUFFER_SIZE){
        atomic_store_explicit( &reqId->requestIdentifier,
                               QD_QUEUE_EMPTY_POS_FULL, 
                               memory_order_release );
        return NULL;
    } else {
        return NULL;
    }
}

static inline void qdq_enqueue_close_buffer(void * buffer,
                              void (*funPtr)(unsigned int, void *)) {
    QDRequestRequestId * reqId =
        (QDRequestRequestId*)(&((char *)buffer)[-sizeof(QDRequestRequestId)] );
    atomic_store_explicit( &reqId->requestIdentifier,
                           (uintptr_t)funPtr, 
                           memory_order_release );    
}

static inline bool qdq_enqueue(QDQueue* q,
                 void (*funPtr)(unsigned int, void *), 
                 unsigned int messageSize,
                 void * messageAddress) {
    if(atomic_load_explicit( &q->closed.value, memory_order_acquire )){
        return false;
    }
    char * messageBuffer = (char *) messageAddress;
    unsigned int storeSize = sizeof(QDRequestRequestId) + messageSize;
    unsigned int pad = QDQ_CALCULATE_PAD(storeSize);
    unsigned long bufferOffset = atomic_fetch_add(&q->counter.value,
                                                  storeSize + pad);
    unsigned int writeToOffset = (bufferOffset + storeSize);
    unsigned int nextReqOffset = writeToOffset + pad;
    QDRequestRequestId * reqId = 
        (QDRequestRequestId*)&q->buffer[bufferOffset];
    if(nextReqOffset <= QD_QUEUE_BUFFER_SIZE) {
        reqId->messageSize = messageSize;
        unsigned long messageBodyStart = bufferOffset + sizeof(QDRequestRequestId);
        for(unsigned long i = messageBodyStart; 
            i < writeToOffset;
            i++){
            q->buffer[i] = messageBuffer[i - messageBodyStart];
        }
        atomic_store_explicit( &reqId->requestIdentifier,
                               (uintptr_t)funPtr, 
                               memory_order_release );
        return true;
    } else if(bufferOffset < QD_QUEUE_BUFFER_SIZE){
        atomic_store_explicit( &reqId->requestIdentifier,
                               QD_QUEUE_EMPTY_POS_FULL, 
                               memory_order_release );
        return false;
    } else {
        return false;
    }
}

#include <stdlib.h>
#include <stdio.h>
static inline void qdq_flush(QDQueue* q) {
    unsigned long todo = 0;
    bool open = true;
    while(open) {
        unsigned long done = todo;
        todo = atomic_load_explicit( &q->counter.value, memory_order_relaxed );
        if((todo == done) &&
           atomic_compare_exchange_strong( &q->counter.value,
                                           &todo,
                                           QD_QUEUE_BUFFER_SIZE)) { 
            open = false;
            atomic_store_explicit( &q->closed.value,
                                   true, 
                                   memory_order_relaxed );
        }
        if(todo >= QD_QUEUE_BUFFER_SIZE) { /* queue closed */
            todo = QD_QUEUE_BUFFER_SIZE;
            open = false;
            atomic_store_explicit( &q->closed.value,
                                   true, 
                                   memory_order_relaxed );
        }
        unsigned long index = done;
        while( index < todo ) {
            QDRequestRequestId * reqId = 
                (QDRequestRequestId*)&q->buffer[index];
            void (*funPtr)(unsigned int, void *);
            uintptr_t funPtrValue;
            while(QD_QUEUE_EMPTY_POS == 
                  (funPtrValue = atomic_load_explicit( &reqId->requestIdentifier,
                                                       memory_order_acquire ))){
                /* spin wait */
                atomic_thread_fence(memory_order_seq_cst);/*hw threads*/
            }
            if(funPtrValue == QD_QUEUE_EMPTY_POS_FULL){
                atomic_store_explicit( &reqId->requestIdentifier,
                                       QD_QUEUE_EMPTY_POS,
                                       memory_order_relaxed );
                return; /* Too big, we can return */
            }
            funPtr = (void (*)(unsigned int, void *))funPtrValue;
            unsigned int messageSize = reqId->messageSize;
            unsigned int storeSize = sizeof(QDRequestRequestId) + messageSize;
            unsigned int messageEndOffset = index + storeSize;
            unsigned int pad = QDQ_CALCULATE_PAD(storeSize);
            unsigned int nextReqOffset = messageEndOffset + pad;
            void * messageAddress = q->buffer + sizeof(QDRequestRequestId) + index;
            funPtr(messageSize, messageAddress);           
            for(unsigned int i = index; i < messageEndOffset; i = i + sizeof(uintptr_t)){
                volatile atomic_uintptr_t * ptr = (void*)&q->buffer[i];
                atomic_store_explicit(ptr,
                                      QD_QUEUE_EMPTY_POS,
                                      memory_order_relaxed);
            }
            index = nextReqOffset;
        }
    }
}

#endif
