#ifndef CHAINED_HASH_SET_H
#define CHAINED_HASH_SET_H

#include "data_structures/sorted_list_set.h"
#include "data_structures/oo_set_interface.h"
#include "misc/bitreversal.h"


#define CHAIN_LENGHT_EXPAND_THRESHOLD 2
#define CHAIN_LENGHT_SHRINK_THRESHOLD 0.5
/*Must be power of two*/
#define INITIAL_NUMBER_OF_BUCKETS 4

typedef struct {
    unsigned int keyPosition;
    void * (*extract_key)(void * v, int keyPos);
    unsigned int (*hash_key)(void * key);
    bool (*are_equal)(void * v1, void * v2);
    char * (*to_string)(void * v1);
    unsigned int numberOfBuckets;
    unsigned int expandTreshold;
    unsigned int shrinkTreshold;
    unsigned int size;
    SortedListSetNode ** buckets;
}ChainedHashSet;

void ch_set_increase_size(ChainedHashSet * set){
    set->size = set->size + 1;
    if(set->size > set->expandTreshold){
        unsigned int oldNumberOfBuckets = set->numberOfBuckets;
        unsigned int newNumberOfBuckets = oldNumberOfBuckets*2;
        unsigned int splitUpMask = reverse_bits(newNumberOfBuckets - 1) ^ reverse_bits(oldNumberOfBuckets - 1);
        SortedListSetNode ** newBuckets = malloc(sizeof(SortedListSetNode *)*newNumberOfBuckets);
        SortedListSetNode ** oldBuckets = set->buckets;
        SortedListSetNode * moveTemp;
        for(unsigned int i = 0; i < oldNumberOfBuckets; i++){
            moveTemp = sl_set_split_opt(&oldBuckets[i],
                                        splitUpMask);
            newBuckets[i] = oldBuckets[i];
            newBuckets[i + oldNumberOfBuckets] = moveTemp;
        }
        free(oldBuckets);
        set->buckets = newBuckets;
        set->numberOfBuckets = newNumberOfBuckets;
        set->expandTreshold = newNumberOfBuckets * CHAIN_LENGHT_EXPAND_THRESHOLD;
        set->shrinkTreshold = newNumberOfBuckets * CHAIN_LENGHT_SHRINK_THRESHOLD;
    }
}

void ch_set_decrease_size(ChainedHashSet * set){
    set->size = set->size - 1;
    if(set->size < set->shrinkTreshold){
        unsigned int oldNumberOfBuckets = set->numberOfBuckets;
        unsigned int newNumberOfBuckets = oldNumberOfBuckets/2;
        SortedListSetNode ** newBuckets = malloc(sizeof(SortedListSetNode *)*newNumberOfBuckets);
        SortedListSetNode ** oldBuckets = set->buckets;
        for(unsigned int i = 0; i < newNumberOfBuckets; i++){
            newBuckets[i] = oldBuckets[i];
            sl_set_concat_opt(&newBuckets[i],
                              oldBuckets[i + newNumberOfBuckets]);
        }
        free(oldBuckets);
        set->buckets = newBuckets;
        set->numberOfBuckets = newNumberOfBuckets;
        set->expandTreshold = newNumberOfBuckets * CHAIN_LENGHT_EXPAND_THRESHOLD;
        if(set->numberOfBuckets == INITIAL_NUMBER_OF_BUCKETS){
            set->shrinkTreshold = 0;
        } else {
            set->shrinkTreshold = newNumberOfBuckets * CHAIN_LENGHT_SHRINK_THRESHOLD;
        }
    }
}

void ch_set_initialize(ChainedHashSet * set,
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
    set->numberOfBuckets = INITIAL_NUMBER_OF_BUCKETS;
    set->expandTreshold = set->numberOfBuckets * CHAIN_LENGHT_EXPAND_THRESHOLD;
    set->shrinkTreshold = set->numberOfBuckets * CHAIN_LENGHT_EXPAND_THRESHOLD;
    set->size = 0;
    set->buckets = aligned_alloc(CACHE_LINE_SIZE, sizeof(SortedListSetNode *)*INITIAL_NUMBER_OF_BUCKETS);
    for(int i = 0; i < INITIAL_NUMBER_OF_BUCKETS; i++){
        set->buckets[i] = NULL;
    }
}

ChainedHashSet * plain_ch_set_create(unsigned int keyPosition,
                                     void * (*extract_key)(void * v, int keyPos),
                                     unsigned int (*hash_key)(void * k),
                                     bool (*are_equal)(void * v1, void * v2),
                                     char * (*to_string)(void * v1)){
    ChainedHashSet * set = aligned_alloc(CACHE_LINE_SIZE, sizeof(ChainedHashSet));
    ch_set_initialize(set,
                      keyPosition,
                      extract_key,
                      hash_key,
                      are_equal,
                      to_string);
    return set;
}

bool ch_set_insert_opt(ChainedHashSet * set,
                       void * value, 
                       unsigned int valueSize,
                       unsigned int hashValue,
                       bool overwrite){
    unsigned int bucketIndex = hashValue & (set->numberOfBuckets - 1);
    SortedListSetNode ** bucket = &set->buckets[bucketIndex];
    bool oneAdded = sl_set_insert_opt(bucket,
                                      value,
                                      valueSize,
                                      reverse_bits(hashValue),
                                      set->keyPosition,
                                      overwrite,
                                      set->extract_key,
                                      set->are_equal);
    if(oneAdded){
        ch_set_increase_size(set);
    }
    return oneAdded;
}

void ch_set_insert(void * setParam, void * value, unsigned int valueSize){
    ChainedHashSet * set = (ChainedHashSet*)setParam;
    void * key = set->extract_key(value, set->keyPosition);
    unsigned int hashValue = set->hash_key(key);
    ch_set_insert_opt(set,
                      value, 
                      valueSize,
                      hashValue,
                      true);
}


bool ch_set_insert_new(void * setParam, void * value, unsigned int valueSize){
    ChainedHashSet * set = (ChainedHashSet*)setParam;
    void * key = set->extract_key(value, set->keyPosition);
    unsigned int hashValue = set->hash_key(key);
    return ch_set_insert_opt(set,
                             value, 
                             valueSize,
                             hashValue,
                             false);
}

void * ch_set_lookup(void * setParam, void * key){
    ChainedHashSet * set = (ChainedHashSet*)setParam;
    unsigned int hashValue = set->hash_key(key);
    unsigned int bucketIndex = hashValue & (set->numberOfBuckets - 1);
    SortedListSetNode ** bucket = &set->buckets[bucketIndex];
    return sl_set_lookup_opt(bucket,
                             key,
                             reverse_bits(hashValue),
                             set->keyPosition,
                             set->extract_key,
                             set->are_equal);
}

void ch_set_delete(void * setParam, void * key, unsigned int keySize){
    (void)keySize;
    ChainedHashSet * set = (ChainedHashSet*)setParam;
    unsigned int hashValue = set->hash_key(key);
    unsigned int bucketIndex = hashValue & (set->numberOfBuckets - 1);
    SortedListSetNode ** bucket = &set->buckets[bucketIndex];
    bool oneRemoved =  sl_set_delete_opt(bucket,
                                         key,
                                         reverse_bits(hashValue),
                                         set->keyPosition,
                                         set->extract_key,
                                         set->are_equal);
    if(oneRemoved){
        ch_set_decrease_size(set);
    }
}

void ch_set_free(void * setParam){
    ChainedHashSet * set = (ChainedHashSet*)setParam;
    unsigned int numberOfBuckets = set->numberOfBuckets;
    for(unsigned int i = 0; i < numberOfBuckets; i++){
        sl_set_free_opt(&set->buckets[i]);
    }
    free(set->buckets);
    free(set);
}

char * ch_set_to_string(void * setParam){
    ChainedHashSet * set = (ChainedHashSet*)setParam;
    unsigned int numberOfBuckets = set->numberOfBuckets;
    char * bucketStrings[numberOfBuckets];
    unsigned int bucketStringSizes[numberOfBuckets];
    unsigned int totalBucketsStringSize = 0;
    SortedListSet * tempListSet = plain_sl_set_create(set->keyPosition,
                                                      set->extract_key,
                                                      set->hash_key,
                                                      set->are_equal,
                                                      set->to_string);
    for(unsigned int i = 0; i < numberOfBuckets; i++){
        tempListSet->head = set->buckets[i];
        bucketStrings[i] = sl_set_to_string(tempListSet);
        bucketStringSizes[i] = strlen(bucketStrings[i]);
        totalBucketsStringSize = totalBucketsStringSize + bucketStringSizes[i];
    }
    tempListSet->head = NULL;
    sl_set_free(tempListSet);
    char * resultString = malloc(totalBucketsStringSize + numberOfBuckets*3 - 3 + 3);
    resultString[0] = '[';
    unsigned int currentPosition = 1;
    for(unsigned int i = 0; i < numberOfBuckets; i++){
        sprintf(&resultString[currentPosition], "%s", bucketStrings[i]);
        free(bucketStrings[i]);
        currentPosition = currentPosition + bucketStringSizes[i];
        if(i != (numberOfBuckets - 1)){
            sprintf(&resultString[currentPosition], ",\n ");
            currentPosition = currentPosition + 3;
        }
    }
    sprintf(&resultString[currentPosition], "]");
    return resultString;
}

void ch_set_print(void * setParam){
    ChainedHashSet * set = (ChainedHashSet*)setParam;
    char * str =  ch_set_to_string(set);
    printf("%s\n", str);
    free(str);
}

_Alignas(CACHE_LINE_SIZE)
OOSetMethodTable CHAINED_HASH_SET_METHOD_TABLE = 
{
    .free = &ch_set_free,
    .insert = &ch_set_insert,
    .insert_new = &ch_set_insert_new,
    .lookup = &ch_set_lookup,
    .delete = &ch_set_delete,
    .to_string = &ch_set_to_string,
    .print = &ch_set_print
};

OOSet * oo_ch_set_create(unsigned int keyPosition,
                          void * (*extract_key)(void * v, int keyPos),
                          unsigned int (*hash_key)(void * k),
                          bool (*are_equal)(void * v1, void * v2),
                          char * (*to_string)(void * v1)){
    ChainedHashSet * set = plain_ch_set_create(keyPosition,
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
