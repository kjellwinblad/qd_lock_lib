#ifndef SORTED_LIST_SET_H
#define SORTED_LIST_SET_H

#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "misc/padded_types.h"
#include "data_structures/oo_set_interface.h"

typedef struct SortedListSetNodeImpl {
    struct SortedListSetNodeImpl * next;    
    unsigned int key_hash_value;
    char value[]; /* Flexible size array member */
} SortedListSetNode;

typedef struct {
    SortedListSetNode * head;
    unsigned int keyPosition;
    void * (*extract_key)(void * v, int keyPos);
    unsigned int (*hash_key)(void * k);
    bool (*are_equal)(void * v1, void * v2);
    char * (*to_string)(void * v1);
}SortedListSet;

extern
_Alignas(CACHE_LINE_SIZE)
OOSetMethodTable SORTED_LIST_SET_METHOD_TABLE;

static inline
SortedListSet * plain_sl_set_create(unsigned int keyPosition,
                                    void * (*extract_key)(void * v, int keyPos),
                                    unsigned int (*hash_key)(void * k),
                                    bool (*are_equal)(void * v1, void * v2),
                                    char * (*to_string)(void * v1)){
    SortedListSet * set = aligned_alloc(CACHE_LINE_SIZE, sizeof(SortedListSet));
    set->head = NULL;
    set->keyPosition = keyPosition;
    set->extract_key = extract_key;
    set->hash_key = hash_key;
    set->are_equal = are_equal;
    set->to_string = to_string;
    return set;
}

static inline
int compare_hash_codes(unsigned int code1, unsigned int code2){
    return code1 - code2;
}

static inline
bool sl_set_insert_opt(SortedListSetNode ** root,
                       void * valuePtr,
                       unsigned int valueSize,
                       unsigned int keyHashValue,
                       int keyPosition,
                       bool overwrite,
                       void * (*extract_key)(void * v, int keyPos),
                       bool (*are_equal)(void * v1, void * v2)){
    void * key = extract_key(valuePtr, keyPosition);
    SortedListSetNode * previous = (SortedListSetNode *)root;
    SortedListSetNode * current = previous->next;
    bool oneMore = true;
    int compareResult;
    while(current != NULL){
        compareResult = compare_hash_codes(current->key_hash_value,
                                           keyHashValue);
        if(compareResult < 0){
            previous = current;
            current = previous->next;
        } else if (compareResult > 0){
            break;
        } else {
            if( are_equal(extract_key(current->value, keyPosition), key)){
                if (overwrite) {
                    SortedListSetNode * oldCurrent = current; 
                    current = current->next;
                    previous->next = current;
                    free(oldCurrent);
                    oneMore = false;
                    break;
                } else{
                    return false;
                }
            } else {
                previous = current;
                current = previous->next;
            }
        }
    }
    SortedListSetNode * newNode = malloc(sizeof(SortedListSetNode) + valueSize);
    previous->next = newNode;
    newNode->next = current;
    newNode->key_hash_value = keyHashValue;
    memcpy(newNode->value, valuePtr, valueSize);
    return oneMore;
}

static inline
void sl_set_insert(void * setParam, void * valuePtr, unsigned int valueSize){
    SortedListSet * set = (SortedListSet *)setParam;
    sl_set_insert_opt(&set->head,
                      valuePtr,
                      valueSize,
                      set->hash_key(set->extract_key(valuePtr, set->keyPosition)),
                      set->keyPosition,
                      true,
                      set->extract_key,
                      set->are_equal);
}

static inline
bool sl_set_insert_new(void * setParam, void * valuePtr, unsigned int valueSize){
    SortedListSet * set = (SortedListSet *)setParam;
    return sl_set_insert_opt(&set->head,
                             valuePtr, valueSize,
                             set->hash_key(set->extract_key(valuePtr, set->keyPosition)),
                             set->keyPosition,
                             false,
                             set->extract_key,
                             set->are_equal);
}

static inline
void * sl_set_lookup_opt(SortedListSetNode ** root, 
                         void * key,
                         unsigned int keyHashValue,
                         int keyPosition,
                         void * (*extract_key)(void * v, int keyPos),
                         bool (*are_equal)(void * v1, void * v2)){
    SortedListSetNode * previous = (SortedListSetNode *)root;
    SortedListSetNode * current = previous->next;
    int compareResult;
    while(current != NULL){
        compareResult = compare_hash_codes(current->key_hash_value,
                                           keyHashValue);
        if(compareResult < 0){
            previous = current;
            current = previous->next;
        } else if (compareResult > 0){
            return NULL;
        } else if (are_equal(extract_key(current->value, keyPosition), key)){
            return current->value;
        } else {
            previous = current;
            current = previous->next;
        }
    }
    return NULL;
}

static inline
void * sl_set_lookup(void * setParam, 
                     void * key){
    SortedListSet * set = (SortedListSet *)setParam;
    return sl_set_lookup_opt(&set->head,
                             key,
                             set->hash_key(key),
                             set->keyPosition,
                             set->extract_key,
                             set->are_equal);
}

static inline
bool sl_set_delete_opt(SortedListSetNode ** root, 
                       void * key,
                       unsigned int keyHashValue,
                       int keyPosition,
                       void * (*extract_key)(void * v, int keyPos),
                       bool (*are_equal)(void * v1, void * v2)){
    SortedListSetNode * previous = (SortedListSetNode *)root;
    SortedListSetNode * current = previous->next;
    int compareResult;
    while(current != NULL){
        compareResult = compare_hash_codes(current->key_hash_value,
                                           keyHashValue);
        if(compareResult < 0){
            previous = current;
            current = previous->next;
        } else if (compareResult > 0){
            return false;
        } else if (are_equal(extract_key(current->value, keyPosition), key)){
            previous->next = current->next;
            free(current);
            return true;
        } else {
            previous = current;
            current = previous->next;
        }
    }
    return false;
}

static inline
void sl_set_delete(void * setParam, 
                   void * key,
                   unsigned int keySize){
    (void)keySize;
    SortedListSet * set = (SortedListSet *)setParam;
    sl_set_delete_opt(&set->head,
                      key,
                      set->hash_key(key),
                      set->keyPosition,
                      set->extract_key,
                      set->are_equal);
}

static inline
void sl_set_free_opt(SortedListSetNode ** root){
    SortedListSetNode * previous = (SortedListSetNode *)root;
    SortedListSetNode * current = previous->next;
    while(current != NULL){
        previous = current;
        current = current->next;
        free(previous);
    }
}

static inline
void sl_set_free(void * setParam){
    SortedListSet * set = (SortedListSet *)setParam;
    sl_set_free_opt(&set->head);
    free(set);
}

static inline
SortedListSetNode * sl_set_split_opt(SortedListSetNode ** root,
                                     unsigned int splitPattern){
    SortedListSetNode * previous = (SortedListSetNode *)root;
    SortedListSetNode * current = previous->next;
    while(current != NULL){
        if(current->key_hash_value & splitPattern){
            previous->next = NULL;
            return current;
        }
        previous = current;
        current = previous->next;
    }
    return NULL;
}

static inline
void sl_set_concat_opt(SortedListSetNode ** root,
                       SortedListSetNode * list){
    SortedListSetNode * previous = (SortedListSetNode *)root;
    SortedListSetNode * current = previous->next;
    while(current != NULL){
        previous = current;
        current = previous->next;
    }
    previous->next = list;
}

static inline
void sl_set_append_opt(SortedListSetNode ** root,
                       SortedListSetNode * list){
    if(list == NULL){
        return;
    }
    SortedListSetNode * previous = (SortedListSetNode *)root;
    SortedListSetNode * current = previous->next;
    while(current != NULL){
        previous = current;
        current = previous->next;
    }
    previous->next = list;
}

static inline
void * sl_set_fold_opt(SortedListSetNode ** root,
                       void * initialValue,
                       void *(*f)(void * soFar, void * currentValue)){
    SortedListSetNode * previous = (SortedListSetNode *)root;
    SortedListSetNode * current = previous->next;
    void * soFar = initialValue;
    while(current != NULL){
        soFar = f(soFar, current->value);
        current = current->next;
    }
    return soFar;
}

static inline
void * sl_set_fold(SortedListSet * set,
                   void * initialValue,
                   void *(*f)(void * soFar, void * currentValue)){
    return sl_set_fold_opt(&set->head, initialValue, f);
}

static inline
void *_______size_helper(void * soFarParam, void * currentValue){
    unsigned int * soFar = (unsigned int*)soFarParam;
    (void)currentValue;
    unsigned int prev = *soFar;
    *soFar = prev + 1;
    return soFar;
}

static inline
unsigned int sl_set_size_opt(SortedListSetNode ** root){
    unsigned int size = 0;
    sl_set_fold_opt(root, &size, _______size_helper);
    return size;
}

static inline
unsigned int sl_set_size(SortedListSet * set){
    return sl_set_size_opt(&set->head);
}

static inline
void sl_set_print(void * setParam){
    SortedListSet * set = (SortedListSet *)setParam;
    SortedListSetNode * previous = (SortedListSetNode *)set;
    SortedListSetNode * current = previous->next;
    printf("[");
    while(current != NULL){
        char * string = set->to_string(current->value);
        printf("%s", string);
        free(string);
        previous = current;
        current = previous->next;
        if(current != NULL){
            printf(",");
        }        
    }
    printf("]\n");
}

static inline
char * sl_set_to_string(void * setParam){
    SortedListSet * set = (SortedListSet *)setParam;
    unsigned int numberOfElements = sl_set_size(set);
    char * stringArray[numberOfElements];
    unsigned int stringLengths[numberOfElements];
    SortedListSetNode * previous = (SortedListSetNode *)set;
    SortedListSetNode * current = previous->next;
    unsigned int elementNumber = 0;
    unsigned int totalCharCount = 2;
    if(numberOfElements == 0){
        char * buffer = (char*)malloc(3);
        sprintf(buffer, "[]");
        return buffer;
    }
    while(current != NULL){
        stringArray[elementNumber] = set->to_string(current->value);
        stringLengths[elementNumber] = strlen(stringArray[elementNumber]);
        totalCharCount = totalCharCount + stringLengths[elementNumber];
        current = current->next;
        elementNumber++;
    }
    totalCharCount = totalCharCount + numberOfElements;
    char* stringBuffer = malloc(totalCharCount);
    stringBuffer[0] = '[';
    unsigned int currentPosition = 1;
    for(unsigned int i = 0; i < numberOfElements; i++){
        sprintf(&stringBuffer[currentPosition], "%s", stringArray[i]);
        free(stringArray[i]);
        currentPosition = currentPosition + stringLengths[i];
        if(i != (numberOfElements - 1)){
            sprintf(&stringBuffer[currentPosition], ",");
            currentPosition = currentPosition + 1;
        }
    }
    sprintf(&stringBuffer[currentPosition], "]");
    return stringBuffer;
}

static inline
OOSet * oo_sl_set_create(unsigned int keyPosition,
                         void * (*extract_key)(void * v, int keyPos),
                         unsigned int (*hash_key)(void * k),
                         bool (*are_equal)(void * v1, void * v2),
                         char * (*to_string)(void * v1)){
    SortedListSet * set = plain_sl_set_create(keyPosition,
                                              extract_key,
                                              hash_key,
                                              are_equal,
                                              to_string);
    OOSet * ooset = aligned_alloc(CACHE_LINE_SIZE, sizeof(OOSet));
    ooset->set = set;
    ooset->m = &SORTED_LIST_SET_METHOD_TABLE;
    return ooset;
}


#endif
