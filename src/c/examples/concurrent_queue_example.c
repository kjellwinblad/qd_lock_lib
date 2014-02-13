#include <stdio.h>
#include <inttypes.h>
#include "misc/thread_includes.h"//Until c11 threads.h is available
#include "misc/bsd_stdatomic.h"//Until c11 stdatomic.h is available
#include <assert.h>
#include <string.h>

#include "locks/locks.h"

struct QueueNodeImpl;
typedef struct QueueNodeImpl {
    struct QueueNodeImpl * prev;
    unsigned int valueSize;
    char value[];
} QueueNode;

/* 
 * Helper functions
 */

void enq(QueueNode ** head, QueueNode ** tail, void* value, unsigned int valueSize){
    QueueNode * newNode = malloc(sizeof(QueueNode) + valueSize);
    newNode->prev = NULL;
    newNode->valueSize = valueSize;
    memcpy(newNode->value, value, valueSize);
    if(*tail != NULL){
        (*tail)->prev = newNode;
    }else{
        *head = newNode;
    }
    *tail = newNode;
}

void* deq(QueueNode ** head, QueueNode ** tail){
    void * toReturn = NULL;
    QueueNode * oldHead = *head;
    if(oldHead != NULL){
        toReturn = malloc(oldHead->valueSize);
        memcpy(toReturn, oldHead->value, oldHead->valueSize);
        *head = oldHead->prev;
        free(oldHead);
        if(*head == NULL){
            *tail = NULL;
        }
    }
    return toReturn;
}

/* 
 * Concurrent queue data structure
 */
typedef struct {
    OOLock * lock;
    QueueNode * head;
    QueueNode * tail;
} ConcurrentQueue;

/* 
 * Constructor
 */
ConcurrentQueue * create_concurrent_queue(){
    ConcurrentQueue * q = malloc(sizeof(ConcurrentQueue));
    q->lock = LL_create(MRQD_LOCK);
    q->head = NULL;
    q->tail = NULL;
    return q;
}

/* 
 * Free function
 */
void free_concurrent_queue(ConcurrentQueue * q){
    LL_free(q->lock);
    free(q);
}

void enqueue_cs(unsigned int messageSize, void* m) {
    ConcurrentQueue* queue = *((ConcurrentQueue**)m);
    enq(&queue->head,
        &queue->tail, 
        &((char*)m)[sizeof(queue)],
        messageSize - sizeof(queue));
}
/* 
 * Enqueue function
 */
void enqueue(ConcurrentQueue* q, void* value, int valueSize) {
    void* buff = LL_delegate_or_lock(q->lock, valueSize + sizeof(q));
    if (buff == NULL) {
        enq(&q->head, &q->tail, value, valueSize);
        LL_delegate_unlock(q->lock);
    } else {
        memcpy(buff, &q, sizeof(q));
        memcpy(&((char*)buff)[sizeof(q)], value, valueSize);
        LL_close_delegate_buffer(q->lock, buff, enqueue_cs);
    }
}

typedef struct {
    ConcurrentQueue* queue;
    void** writeBack;
} DeqMsg;
void dequeue_cs(unsigned int messageSize, void* m) {
    (void)messageSize;
    DeqMsg* msg = m;
    *(msg->writeBack) = deq(&msg->queue->head,
                            &msg->queue->tail);
}

/* 
 * Dequeue function
 */
void * dequeue(ConcurrentQueue* q) {
    void * res;
    DeqMsg msg = {.queue = q, .writeBack = &res};
    LL_delegate_wait(q->lock, dequeue_cs, sizeof(msg), &msg);
    return res;
}

/* 
 * Test
 */
int main(){
    ConcurrentQueue * queue = create_concurrent_queue();

    for(int i = 0; i < 10; i++){
        enqueue(queue, &i, sizeof(int));
    }

    for(int i = 0; i < 10; i++){
        int * v = dequeue(queue);
        assert((*v) == i);
        free(v);
    }

    int * v = dequeue(queue);
    assert(v == NULL);

    free_concurrent_queue(queue);

    printf("SUCCESS!\n");
    return 0;
}
