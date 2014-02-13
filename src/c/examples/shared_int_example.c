#include <stdio.h>
#include <inttypes.h>
#include "misc/thread_includes.h"//Until c11 threads.h is available
#include "misc/bsd_stdatomic.h"//Until c11 stdatomic.h is available
#include <assert.h>
#include <string.h>

#include "locks/locks.h"

/*
 * Data structure
 */
typedef struct {
    OOLock * lock;
    long value;
} SharedInt;

void init(SharedInt * toInit, long initialValue){
    toInit->value = initialValue;
    toInit->lock = LL_create(MRQD_LOCK);
}

/*
 * Standard locking
 */
void mult1(SharedInt * v1, long v2){
    LL_lock(v1->lock);
    v1->value = v1->value * v2;
    LL_unlock(v1->lock);
}


/*
 * Delegate
 */
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
    LL_delegate(v1->lock, mult_cs, sizeof(msg), &msg);
}

/*
 * Delegate and wait
 */
typedef struct{
    SharedInt * v1;
    long v2;
    long * r;
} MultResMsg;

void mult_res_cs(unsigned int s, void * msgP){
    (void)s;
    MultResMsg * msg = (MultResMsg *)msgP;
    msg->v1->value = msg->v1->value * msg->v2;
    *msg->r = msg->v1->value;
}

long mult_res(SharedInt * v1, long v2){
    long res;
    MultResMsg msg = {.v1 = v1, .v2 = v2, .r = &res};
    LL_delegate_wait(v1->lock, mult_res_cs, sizeof(msg), &msg);
    return res;
}

/*
 * Read critical section
 */

long read(SharedInt * v){
    long res;
    LL_rlock(v->lock);
    res = v->value;
    LL_runlock(v->lock);
    return res;
}

/*
 * Test
 */

int main(){
    SharedInt mySharedInt;
    init(&mySharedInt, 1);
    mult1(&mySharedInt, 10);
    mult2(&mySharedInt, 10);
    long res = mult_res(&mySharedInt, 10);
    assert(res == 1000);
    res = read(&mySharedInt);
    assert(res == 1000);
    LL_free(mySharedInt.lock);
    printf("SUCCESS!\n");
    return 0;
}
