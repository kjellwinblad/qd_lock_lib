#include <stdio.h>
#include <inttypes.h>
#include "misc/thread_includes.h"//Until c11 threads.h is available
#include "misc/bsd_stdatomic.h"//Until c11 stdatomic.h is available
#include <assert.h>

#include "locks/locks.h"


typedef struct {
    MRQDLock lock;
    long value;
} SharedInt;

void init(SharedInt * toInit, long initialValue){
    toInit->value = initialValue;
    LL_initialize(&toInit->lock);
}

void mult1(SharedInt * v1, long v2){
    LL_lock(&v1->lock);
    v1->value = v1->value * v2;
    LL_unlock(&v1->lock);
}

typedef struct{
    SharedInt * v1;
    long v2;
} MultMsg;

void mult_cs(unsigned int s, void * msgP){
    (void)s;
    MultMsg * msg = (MultMsg *)msgP;
    msg->v1->value = msg->v1->value * msg->v2;
}

void mult2(SharedInt * v1, long v2){
    MultMsg msg = {.v1 = v1, .v2 = v2};
    LL_delegate(&v1->lock, mult_cs, sizeof(msg), &msg);
}

typedef struct{
    SharedInt * v1;
    long v2;
    volatile atomic_int * w;
    long * r;
} MultResMsg;

void mult_res_cs(unsigned int s, void * msgP){
    (void)s;
    MultResMsg * msg = (MultResMsg *)msgP;
    msg->v1->value = msg->v1->value * msg->v2;
    *msg->r = msg->v1->value;
    atomic_store_explicit(msg->w, 0, memory_order_release); 
}

long mult_res(SharedInt * v1, long v2){
    volatile atomic_int wf = ATOMIC_VAR_INIT(1);
    long res;
    MultResMsg msg = {.v1 = v1, .v2 = v2, .w = &wf, .r = &res};
    LL_delegate(&v1->lock, mult_res_cs, sizeof(msg), &msg);
    while(atomic_load_explicit(&wf, memory_order_acquire)){
        thread_yield();
    }
    return res;
}

long read(SharedInt * v){
    long res;
    LL_rlock(&v->lock);
    res = v->value;
    LL_runlock(&v->lock);
    return res;
}

int main(){
    SharedInt mySharedInt;
    init(&mySharedInt, 1);
    mult1(&mySharedInt, 10);
    mult2(&mySharedInt, 10);
    long res = mult_res(&mySharedInt, 10);
    assert(res == 1000);
    res = read(&mySharedInt);
    assert(res == 1000);
    printf("SUCCESS!\n");
    return 0;
}
