// LL_delegate example
// ========
// 
// This file provide an example of how the lock API can be used and in
// particular how the `LL_delegate` function can be used. More
// examples can be found in the [test code](../tests/test_lock.html).


#include <stdio.h>
#include <inttypes.h>
#include "misc/thread_includes.h"//Until c11 threads.h is available
#include "misc/bsd_stdatomic.h"//Until c11 stdatomic.h is available

#include "locks/locks.h"

// ## Declare lock

OOLock * my_lock;

// ## Message

typedef struct {
    int number;
    uintptr_t from_thread;
    volatile atomic_int * wait_on_me;
} MyMessage;

// ## Critical section examples

void critical_section(unsigned int ignored, void * message){
    (void)ignored;
    MyMessage * my_message = (MyMessage*)message;
    printf("Got number: %d, from thread: %" PRIxPTR "\n",
           my_message->number,
           my_message->from_thread);
}

void critical_section_write_back(unsigned int ignored, 
                                 void * message){
    (void)ignored;
    MyMessage * my_message = (MyMessage*)message;
    printf("Got number: %d, from thread: %" PRIxPTR "\n",
           my_message->number,
           my_message->from_thread);
    atomic_store(my_message->wait_on_me, 0);
}

// ## Issue critical sections

// This is an example of how you can issue critical sections with the
// `LL_delegate` function.
void * issue_critical_sections(void * thread_id_wrapper){
    uintptr_t thread_id = (uintptr_t)thread_id_wrapper;
    // Define message
    MyMessage message;
    for(int i = 0; i < 10; i++){
        // Write message paramters
        message.number = i;
        message.from_thread = thread_id;
        // Send the critical section to the lock
        LL_delegate(my_lock, 
                    critical_section, 
                    sizeof(MyMessage), 
                    &message);
        printf("Critical section has been sent\n");
    }
    return NULL;
}

// ## Wait for response

// This is an example of how you can send critical sections to a lock
// with the `LL_delegate` function and then directly wait until the
// critical section has executed.
void * issue_critical_sections_and_wait(void * thread_id_wrapper){
    uintptr_t thread_id = (uintptr_t)thread_id_wrapper;
    // Define message
    MyMessage message;
    for(int i = 0; i < 10; i++){
        volatile atomic_int wait_on_me = ATOMIC_VAR_INIT(1);
        // Write message paramters
        message.number = i;
        message.from_thread = thread_id;
        message.wait_on_me = &wait_on_me;
        // Send the critical section to the lock
        LL_delegate(my_lock,
                    critical_section_write_back,
                    sizeof(MyMessage), 
                    &message);
        // Wait for event that is indicating that the critical section
        // has been executed.
        while(atomic_load(&wait_on_me)){
            thread_yield();
        }
        printf("Critical section sent by thread %" PRIxPTR " is executed\n", 
               thread_id);
    }
    return NULL;
}

// ## Start up the example
void start_and_wait_for_threads_with(void *(*start_routine) (void *)){
    int mumber_of_threads = 2;
    pthread_t threads[mumber_of_threads];
    for(int i = 0; i < mumber_of_threads; i++){
        uintptr_t thread_id = (uintptr_t)i;
        pthread_create(&threads[i],
                       NULL,
                       start_routine,
                       (void*)thread_id);
    }
    for(int i = 0; i < mumber_of_threads; i++){
        void * resp = NULL;
        pthread_join(threads[i], &resp);
    }
}

int main(){
    // ## Create lock
    my_lock = LL_create(QD_LOCK);
    printf("\n\nStarting example 1...\n");
    printf("=========================\n\n");
    start_and_wait_for_threads_with(&issue_critical_sections);
    printf("\n\nStarting example 2...\n");
    printf("=========================\n\n");
    start_and_wait_for_threads_with(&issue_critical_sections_and_wait);
    // ## Free lock
    LL_free(my_lock);
    return 0;
}
