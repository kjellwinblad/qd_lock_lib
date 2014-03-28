#include "tests/test_framework.h"
#include "misc/random.h"

#include "misc/thread_includes.h"
#include "misc/bsd_stdatomic.h"//Until c11 stdatomic.h is available
#include <string.h>

#include "locks/locks.h"

typedef union {
    unsigned int value;
    char pad[CACHE_LINE_SIZE];
} LLPaddedSeed;

typedef union {
    unsigned long value;
    char pad[CACHE_LINE_SIZE];
} LLPaddedLocalCounter;

typedef union {
    LL_lock_type_name value;
    char pad[CACHE_LINE_SIZE];
} LLLockTypeNameWrapper;

typedef struct {
    unsigned long * localInCSCounter;
    unsigned int * localSeed;
} ThreadLocalData;

LLLockTypeNameWrapper lock_type;

int test_create(){
    LOCK_TYPE * lock = LL_create(lock_type.value);
    LL_free(lock);
    return 1;
}

int test_lock(){
    LOCK_TYPE * lock = LL_create(lock_type.value);
    for(int i = 0; i < 10; i++){
        LL_lock((lock));
        LL_unlock(lock);
    }
    LL_free(lock);
    return 1;
}

int test_is_locked(){
    LOCK_TYPE * lock = LL_create(lock_type.value);
    for(int i = 0; i < 10; i++){
        assert(LL_is_locked(lock) == false);
        LL_lock(lock);
        assert(LL_is_locked(lock) == true);
        LL_unlock(lock);
        assert(LL_is_locked(lock) == false);
    }
    LL_free(lock);
    return 1;
}

int test_try_locked(){
    LOCK_TYPE * lock = LL_create(lock_type.value);
    for(int i = 0; i < 10; i++){
        assert(LL_is_locked(lock) == false);
        LL_lock(lock);
        assert(LL_is_locked(lock) == true);
        LL_unlock(lock);
        assert(LL_is_locked(lock) == false);
    }
    LL_free(lock);
    return 1;
}

LOCK_TYPE * lock;
LLPaddedULong counter = {.value = ATOMIC_VAR_INIT(0)};
LLPaddedBool stop = {.value = ATOMIC_FLAG_INIT};
LLPaddedDouble delegatePercentage;
LLPaddedDouble readPercentage;
LLPaddedDouble delegateOrLockPercentage;
LLPaddedDouble delegateWaitPercentage;

void critical_section_code(unsigned long * localCounterPtr){
    unsigned long oldCounterValue = atomic_load(&counter.value);
    atomic_fetch_add(&counter.value, 1);
    *localCounterPtr = *localCounterPtr + 1;
    atomic_thread_fence(memory_order_seq_cst); 
    assert((oldCounterValue + 1) == atomic_load(&counter.value));
}

void delegate_function(unsigned int messageSize, void * messageAddress){
    assert(messageSize == sizeof(unsigned long *));
    unsigned long ** localCounterPtrPtr = (unsigned long **)messageAddress;
    critical_section_code(*localCounterPtrPtr);
}

void  * critical_section_thread(void * threadLocalDataVPtr){
    ThreadLocalData * threadLocalDataPtr = (ThreadLocalData*)threadLocalDataVPtr;
    unsigned long * localInCSCounter = threadLocalDataPtr->localInCSCounter;
    unsigned int * localSeed = threadLocalDataPtr->localSeed;
    unsigned long expectedLocalInCSCounterReadValue = 0;
    double delegatePercentageV = delegatePercentage.value;
    double delegatePlusReadPercentage = delegatePercentageV + readPercentage.value;
    double delegatePlusReadPlusDelegateOrLockPercentage = delegatePlusReadPercentage + delegateOrLockPercentage.value;
    double delegatePlusReadPlusDelegateOrLockPlusDelegateWaitPercentage = delegatePlusReadPercentage + delegateOrLockPercentage.value + delegateWaitPercentage.value;
    while(!atomic_load_explicit(&stop.value, memory_order_acquire)){
        double randomNumber = random_double(localSeed);
        if(randomNumber > delegatePlusReadPlusDelegateOrLockPlusDelegateWaitPercentage){
            LL_lock(lock);
            critical_section_code(localInCSCounter);
            LL_unlock(lock);
            expectedLocalInCSCounterReadValue++;
        }if(randomNumber > delegatePlusReadPlusDelegateOrLockPercentage){
            LL_delegate_wait(lock, delegate_function, sizeof(unsigned long *), &localInCSCounter);
            expectedLocalInCSCounterReadValue++;
        }else if(randomNumber > delegatePlusReadPercentage){
            void * buffer = LL_delegate_or_lock(lock, sizeof(sizeof(unsigned long *)));
            if(buffer == NULL){
                critical_section_code(localInCSCounter);
                LL_delegate_unlock(lock);
            }else{
                *((unsigned long **)buffer) = localInCSCounter;
                LL_close_delegate_buffer(lock, buffer, delegate_function);
            }
            expectedLocalInCSCounterReadValue++;
        } else if(randomNumber > delegatePercentageV){
            LL_rlock(lock);
            assert(expectedLocalInCSCounterReadValue == *localInCSCounter);
            unsigned long before = atomic_load(&counter.value);
            atomic_thread_fence(memory_order_seq_cst);
            assert(before == atomic_load(&counter.value));
            LL_runlock(lock);        
        }else {
            LL_delegate(lock, delegate_function, sizeof(unsigned long *), &localInCSCounter);
            expectedLocalInCSCounterReadValue++;
        } 
    }
    return NULL;
}
int test_mutual_exclusion(double delegatePercentageParm,
                          double readPercentageParm,
                          double delegateOrLockPercentageParm,
                          double delegateWaitPercentageParm){
    delegatePercentage.value = delegatePercentageParm;
    readPercentage.value = readPercentageParm;
    delegateOrLockPercentage.value = delegateOrLockPercentageParm;
    delegateWaitPercentage.value = delegateWaitPercentageParm;
    lock = LL_create(lock_type.value);
    struct timespec testTime= {.tv_sec = 1, .tv_nsec = 100000000}; 
    int threadCountsToTest[] = {1,2,4,8,16};
    int nrOfThreadCountsToTest = 5;
    for(int n = 0; n < nrOfThreadCountsToTest; n++){
        int i = threadCountsToTest[n];
        atomic_store(&counter.value, 0);
        atomic_store(&stop.value, false);
        pthread_t threads[i];
        LLPaddedLocalCounter localInCSCounters[i];
        LLPaddedSeed localSeeds[i];
        ThreadLocalData * threadLocalData = aligned_alloc(CACHE_LINE_SIZE, sizeof(ThreadLocalData) * i);
        for(int n = 0; n < i; n++){
            localInCSCounters[n].value = 0;
            localSeeds[n].value = n;
            threadLocalData[n].localInCSCounter = &localInCSCounters[n].value;
            threadLocalData[n].localSeed = &localSeeds[n].value;
            pthread_create(&threads[n], NULL,
                           critical_section_thread,
                           &threadLocalData[n]);
        }
        nanosleep(&testTime, NULL);
        atomic_store(&stop.value, true);
        unsigned long localInCSCountersSum = 0;
        for(int n = 0; n < i; n++){
            void * resp = NULL;
            pthread_join(threads[n], &resp);
        }
        for(int n = 0; n < i; n++){
            localInCSCountersSum = localInCSCountersSum + 
                localInCSCounters[n].value;
        }
        free(threadLocalData);
        assert(localInCSCountersSum == atomic_load(&counter.value));
    }
    LL_free(lock);
    return 1;
}

void test_lock_type(LL_lock_type_name name){
    lock_type.value = name;

    printf("\n\n\n\033[32m ### STARTING LOCK TESTS! -- \033[m\n\n\n");

    T(test_create(), "test_create()");
    T(test_lock(), "test_lock()");
    T(test_is_locked(), "test_is_locked()");
    T(test_is_locked(), "test_is_locked()");
    T(test_mutual_exclusion(0.0, 0.0, 0.0, 0.0), "test_mutual_exclusion LL_lock = 100%");
    T(test_mutual_exclusion(0.5, 0.0, 0.0, 0.0), "test_mutual_exclusion LL_delegate = 50% LL_lock = 50%");
    T(test_mutual_exclusion(1.0, 0.0, 0.0, 0.0), "test_mutual_exclusion LL_delegate = 100%");
    T(test_mutual_exclusion(0.0, 1.0, 0.0, 0.0), "test_mutual_exclusion LL_rlock = 100%");
    T(test_mutual_exclusion(0.5, 0.5, 0.0, 0.0), "test_mutual_exclusion LL_delegate = 50% LL_rlock = 50%");
    T(test_mutual_exclusion(0.33, 0.34, 0.0, 0.0), "test_mutual_exclusion LL_delegate = 33% LL_lock = 33% LL_rlock = 34%");
    T(test_mutual_exclusion(0.0, 0.0, 1.0, 0.0), "LL_lock_or_delegate = 100%");
    T(test_mutual_exclusion(0.0, 0.5, 0.5, 0.0), "LL_rlock = 50% LL_lock_or_delegate = 50%");
    T(test_mutual_exclusion(0.0, 0.0, 1.0, 0.0), "LL_delegate_wait = 100%");
    T(test_mutual_exclusion(0.2, 0.2, 0.2, 0.2), "20% All ops");

    printf("\n\n\n\033[32m ### LOCK TESTS COMPLETED! -- \033[m\n\n\n");    

}


int main(int argc, char **argv){
#ifndef LOCK_TYPE_NAME
    if(argc > 1){
        if(strcmp("TATAS_LOCK", argv[1]) == 0){
            test_lock_type(TATAS_LOCK);
        }else if(strcmp("QD_LOCK", argv[1]) == 0){
            test_lock_type(QD_LOCK);
        }else if(strcmp("CCSYNCH_LOCK", argv[1]) == 0){
            test_lock_type(CCSYNCH_LOCK);
        }else if(strcmp("MRQD_LOCK", argv[1]) == 0){
            test_lock_type(MRQD_LOCK);
        }else if(strcmp("MCS_LOCK", argv[1]) == 0){
            test_lock_type(MCS_LOCK);
        }else if(strcmp("DRMCS_LOCK", argv[1]) == 0){
            test_lock_type(DRMCS_LOCK);
        }else{
            printf("No lock with the name %s.\n", argv[1]);
        }
    }else{
        printf("Give a lock type as parameter:\n");
        printf("\tTATAS_LOCK\n");
        printf("\tQD_LOCK\n");
        printf("\tMRQD_LOCK\n");
        printf("\tCCSYNCH_LOCK\n");
        printf("\tMCS_LOCK\n");
        printf("\tDRMCS_LOCK\n");
    }
#else
    UNUSED(argc);
    UNUSED(argv);
    test_lock_type(LOCK_TYPE_NAME);
#endif
    return 0;
}

