#ifndef CONC_SPLITCH_SET_H
#define CONC_SPLITCH_SET_H

#include "data_structures/chained_hash_set.h"
#include "locks/mrqd_lock.h"
#include "locks/locks.h"
#include "misc/padded_types.h"

#define CONC_SPLIT_SET_NUMBER_OF_SUBTABLES 23

typedef struct {
    MRQDLock lock;
    ChainedHashSet set;
    char pad[CACHE_LINE_SIZE_PAD(sizeof(MRQDLock) + sizeof(ChainedHashSet))];
} LockAndSet;

typedef struct {
    unsigned int keyPosition;
    void * (*extract_key)(void * v, int keyPos);
    unsigned int (*hash_key)(void * k);
    bool (*are_equal)(void * v1, void * v2);
    char * (*to_string)(void * v1);
    char pad[CACHE_LINE_SIZE_PAD(sizeof(unsigned int) + 4 * sizeof(void *))];
    LockAndSet subsets[CONC_SPLIT_SET_NUMBER_OF_SUBTABLES];
}ConcSplitchSet;

extern
_Alignas(CACHE_LINE_SIZE)
OOSetMethodTable CONC_SPLITCH_SET_METHOD_TABLE;

static inline
void csh_set_initialize(ConcSplitchSet * set,
                        unsigned int keyPosition,
                        void * (*extract_key)(void * v, int keyPos),
                        unsigned int (*hash_key)(void * k),
                        bool (*are_equal)(void * v1, void * v2),
                        char * (*to_string)(void * v1)){
    set->keyPosition = keyPosition;
    set->extract_key = extract_key;
    set->hash_key = hash_key;
    set->are_equal = are_equal;
    set->to_string = to_string;
    for(int i = 0; i < CONC_SPLIT_SET_NUMBER_OF_SUBTABLES; i++){
        mrqd_initialize(&set->subsets[i].lock);
        ch_set_initialize(&set->subsets[i].set,
                          keyPosition,
                          extract_key,
                          hash_key,
                          are_equal,
                          to_string);
    }
}

static inline
ConcSplitchSet * plain_csh_set_create(unsigned int keyPosition,
                                      void * (*extract_key)(void * v, int keyPos),
                                      unsigned int (*hash_key)(void * k),
                                      bool (*are_equal)(void * v1, void * v2),
                                      char * (*to_string)(void * v1)){
    ConcSplitchSet * set = aligned_alloc(CACHE_LINE_SIZE, sizeof(ConcSplitchSet));
    csh_set_initialize(set,
                       keyPosition,
                       extract_key,
                       hash_key,
                       are_equal,
                       to_string);
    return set;
}

typedef struct {
    ChainedHashSet * set;
    unsigned int hashValue;
    char value[];
} CshSetInsertMessage;

static inline
void handle_csh_set_insert_message(unsigned int messageSize, void * message){
    unsigned int valueSize = messageSize - sizeof(CshSetInsertMessage);
    CshSetInsertMessage * insertMessage = (CshSetInsertMessage *) message;
    ch_set_insert_opt(insertMessage->set,
                      insertMessage->value, 
                      valueSize,
                      insertMessage->hashValue,
                      true);
}

static inline
void csh_set_insert(void * setParam, void * value, unsigned int valueSize){
    ConcSplitchSet * set = (ConcSplitchSet*)setParam;
    void * key = set->extract_key(value, set->keyPosition);
    unsigned int hashValue = set->hash_key(key);
    LockAndSet * lockAndSet = &set->subsets[hashValue % CONC_SPLIT_SET_NUMBER_OF_SUBTABLES];
    MRQDLock * lock = &lockAndSet->lock;
    ChainedHashSet * subset = &lockAndSet->set;
    void * delegateBuffer = LL_delegate_or_lock(lock, 
                                               valueSize + sizeof(CshSetInsertMessage));
    if(delegateBuffer == NULL){
        ch_set_insert_opt(subset,
                          value, 
                          valueSize,
                          hashValue,
                          true);
        LL_delegate_unlock(lock);
    }else{
        CshSetInsertMessage * message = (CshSetInsertMessage *)delegateBuffer;
        message->set = subset;
        message->hashValue = hashValue;
        unsigned char * valueChars = (unsigned char *)value;
        unsigned char * messageValueChars = (unsigned char *)message->value;
        for(unsigned int i = 0; i < valueSize; i++){
            messageValueChars[i] = valueChars[i];
        }
        LL_close_delegate_buffer(lock, delegateBuffer, handle_csh_set_insert_message);
    }
}


typedef struct {
    ChainedHashSet * set;
    unsigned int hashValue;
    volatile atomic_int * writeBackLocation;
    unsigned char value[];
}CshSetInsertNewMessage;

static inline
void handle_csh_set_insert_new_message(unsigned int messageSize, void * message){
    unsigned int valueSize = messageSize - sizeof(CshSetInsertMessage);
    CshSetInsertNewMessage * insertMessage = (CshSetInsertNewMessage *) message;
    bool writeBackValue = ch_set_insert_opt(insertMessage->set,
                                            insertMessage->value, 
                                            valueSize,
                                            insertMessage->hashValue,
                                            false);
    atomic_store_explicit(insertMessage->writeBackLocation, 
                          writeBackValue,
                          memory_order_release);
}

static inline
bool csh_set_insert_new(void * setParam, void * value, unsigned int valueSize){
    ConcSplitchSet * set = (ConcSplitchSet*)setParam;
    void * key = set->extract_key(value, set->keyPosition);
    unsigned int hashValue = set->hash_key(key);
    LockAndSet * lockAndSet = &set->subsets[hashValue % CONC_SPLIT_SET_NUMBER_OF_SUBTABLES];
    MRQDLock * lock = &lockAndSet->lock;
    ChainedHashSet * subset = &lockAndSet->set;
    void * delegateBuffer = LL_delegate_or_lock(lock, 
                                                valueSize + sizeof(CshSetInsertNewMessage));
    int returnValue;
    if(delegateBuffer == NULL){
        returnValue = ch_set_insert_opt(subset,
                                             value, 
                                             valueSize,
                                             hashValue,
                                             false);
        LL_delegate_unlock(lock);
        
    }else{
        CshSetInsertNewMessage * message = (CshSetInsertNewMessage *)delegateBuffer;
        message->set = subset;
        message->hashValue = hashValue;
        volatile atomic_int result = ATOMIC_VAR_INIT(99);
        message->writeBackLocation = &result;
        unsigned char * valueChars = (unsigned char *)value;
        unsigned char * messageValueChars = (unsigned char *)message->value;
        for(unsigned int i = 0; i < valueSize; i++){
            messageValueChars[i] = valueChars[i];
        }
        LL_close_delegate_buffer(lock, delegateBuffer, handle_csh_set_insert_new_message);
        while(99 == (returnValue = atomic_load_explicit(&result, memory_order_acquire))){
            thread_yield();
        };
    }
    return returnValue;
}

static inline
void * csh_set_lookup(void * setParam, void * key){
    ConcSplitchSet * set = (ConcSplitchSet*)setParam;
    unsigned int hashValue = set->hash_key(key);
    LockAndSet * lockAndSet = &set->subsets[hashValue % CONC_SPLIT_SET_NUMBER_OF_SUBTABLES];
    MRQDLock * lock = &lockAndSet->lock;
    ChainedHashSet * subset = &lockAndSet->set;
    LL_rlock(lock);
    unsigned int bucketIndex = hashValue & (subset->numberOfBuckets - 1);
    SortedListSetNode ** bucket = &subset->buckets[bucketIndex];
    void * result =  sl_set_lookup_opt(bucket,
                                       key,
                                       reverse_bits(hashValue),
                                       set->keyPosition,
                                       set->extract_key,
                                       set->are_equal);
    LL_runlock(lock);
    return result;
}

typedef struct {
    ChainedHashSet * set;
    unsigned int hashValue;
    unsigned int keySize;
    unsigned char key[];
}CshSetDeleteMessage;

static inline
void handle_csh_set_delete_message(unsigned int messageSize, void * message){
    (void)messageSize;
    CshSetDeleteMessage * deleteMessage = (CshSetDeleteMessage *) message;
    ch_set_delete(deleteMessage->set,
                  deleteMessage->key,
                  deleteMessage->keySize);
}

static inline
void csh_set_delete(void * setParam, void * key, unsigned int keySize){
    ConcSplitchSet * set = (ConcSplitchSet*)setParam;
    unsigned int hashValue = set->hash_key(key);
    LockAndSet * lockAndSet = &set->subsets[hashValue % CONC_SPLIT_SET_NUMBER_OF_SUBTABLES];
    MRQDLock * lock = &lockAndSet->lock;
    ChainedHashSet * subset = &lockAndSet->set;
    void * delegateBuffer = LL_delegate_or_lock(lock, 
                                               keySize + sizeof(CshSetDeleteMessage));
    if(delegateBuffer == NULL){
        ch_set_delete(subset, key, keySize);
        LL_delegate_unlock(lock);
    }else{
        CshSetDeleteMessage * message = (CshSetDeleteMessage *)delegateBuffer;
        message->set = subset;
        message->hashValue = hashValue;
        unsigned char * keyChars = (unsigned char *)key;
        unsigned char * messageKeyChars = (unsigned char *)message->key;
        for(unsigned int i = 0; i < keySize; i++){
            messageKeyChars[i] = keyChars[i];
        }
        LL_close_delegate_buffer(lock, delegateBuffer, handle_csh_set_delete_message);
    }
}

static inline
void csh_set_free(void * setParam){
    ConcSplitchSet * set = (ConcSplitchSet*)setParam;
    for(int i = 0; i < CONC_SPLIT_SET_NUMBER_OF_SUBTABLES; i++){
        LockAndSet * lockAndSet = &set->subsets[i];
        MRQDLock * lock = &lockAndSet->lock;
        LL_lock(lock);
        ChainedHashSet * subset = &lockAndSet->set;
        unsigned int numberOfBuckets = subset->numberOfBuckets;
        for(unsigned int i = 0; i < numberOfBuckets; i++){
            sl_set_free_opt(&subset->buckets[i]);
        }
        free(subset->buckets);
        LL_unlock(lock);
    }
    free(set);
}

static inline
char * csh_set_to_string(void * setParam){
    ConcSplitchSet * set = (ConcSplitchSet*)setParam;
    char * subtableStrings[CONC_SPLIT_SET_NUMBER_OF_SUBTABLES];
    unsigned int subtableStrSizes[CONC_SPLIT_SET_NUMBER_OF_SUBTABLES];
    unsigned int totalStringLength = 0;
    for(int i = 0; i < CONC_SPLIT_SET_NUMBER_OF_SUBTABLES; i++){
        LL_lock(&set->subsets[i].lock);
    }
    for(int i = 0; i < CONC_SPLIT_SET_NUMBER_OF_SUBTABLES; i++){
        subtableStrings[i] = ch_set_to_string(&set->subsets[i].set);
        subtableStrSizes[i] =strlen(subtableStrings[i]);
        totalStringLength = totalStringLength + subtableStrSizes[i];
    }
    for(int i = 0; i < CONC_SPLIT_SET_NUMBER_OF_SUBTABLES; i++){
        LL_unlock(&set->subsets[i].lock);
    }
    char * resultString = malloc(totalStringLength + CONC_SPLIT_SET_NUMBER_OF_SUBTABLES + 2);
    unsigned int currentPosition = 0;
    for(int i = 0; i < CONC_SPLIT_SET_NUMBER_OF_SUBTABLES; i++){
        sprintf(&resultString[currentPosition], "%s\n", subtableStrings[i]);
        currentPosition = currentPosition + subtableStrSizes[i] + 1;
        free(subtableStrings[currentPosition]);
    }
    return resultString;
}

static inline
void csh_set_print(void * setParam){
    ConcSplitchSet * set = (ConcSplitchSet*)setParam;
    char * str =  csh_set_to_string(set);
    printf("%s\n", str);
    free(str);
}

static inline
OOSet * oo_csh_set_create(unsigned int keyPosition,
                          void * (*extract_key)(void * v, int keyPos),
                          unsigned int (*hash_key)(void * k),
                          bool (*are_equal)(void * v1, void * v2),
                          char * (*to_string)(void * v1)){
    ConcSplitchSet * set = plain_csh_set_create(keyPosition,
                                                extract_key,
                                                hash_key,
                                                are_equal,
                                                to_string);
    OOSet * ooset = aligned_alloc(CACHE_LINE_SIZE, sizeof(OOSet));
    ooset->set = set;
    ooset->m = &CHAINED_HASH_SET_METHOD_TABLE;
    return ooset;
}

#endif
