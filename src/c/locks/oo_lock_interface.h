#ifndef OO_LOCK_INTERFACE_H
#define OO_LOCK_INTERFACE_H

#include <stdbool.h>
#include "misc/padded_types.h"

typedef struct {
    void (*free)(void*);
    void (*lock)(void*);
    void (*unlock)(void*);
    bool (*is_locked)(void*);
    bool (*try_lock)(void*);
    void (*rlock)(void*);
    void (*runlock)(void*);
    void (*delegate)(void*,
                     void (*funPtr)(unsigned int, void *),
                     unsigned int messageSize,
                     void * messageAddress);
    void (*delegate_wait)(void*,
                          void (*funPtr)(unsigned int, void *),
                          unsigned int messageSize,
                          void * messageAddress);
    void * (*delegate_or_lock)(void* lock,
                               unsigned int messageSize);
    void (*close_delegate_buffer)(void * buffer,
                                  void (*funPtr)(unsigned int, void *));
    void (*delegate_unlock)(void* lock);
    char pad[CACHE_LINE_SIZE -  (8 * sizeof(void*)) % CACHE_LINE_SIZE];
} OOLockMethodTable;

typedef struct {
    OOLockMethodTable * m;
    void * lock;
    char pad[CACHE_LINE_SIZE - (2 * sizeof(void*)) % CACHE_LINE_SIZE];
} OOLock;

static inline void oolock_free(OOLock * lock){
    lock->m->free(lock->lock);
    free(lock);
}

#endif
