#ifndef OO_SET_INTERFACE_H
#define OO_SET_INTERFACE_H

#include <stdbool.h>
#include "misc/padded_types.h"

typedef struct {
    void (*free)(void*);
    void (*insert)(void*,void*,unsigned int);
    bool (*insert_new)(void*, void*, unsigned int);
    void * (*lookup)(void*, void*);
    void (*delete)(void*, void*, unsigned int);
    char * (*to_string)(void*);
    void (*print)(void*);
    char pad[CACHE_LINE_SIZE -  (7 * sizeof(void*)) % CACHE_LINE_SIZE];
} OOSetMethodTable;

typedef struct {
    OOSetMethodTable * m;
    void * set;
    char pad[CACHE_LINE_SIZE - (2 * sizeof(void*)) % CACHE_LINE_SIZE];
} OOSet;

static inline
void ooset_free(OOSet * set){
    set->m->free(set->set);
    free(set);
}

#endif
