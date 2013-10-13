#ifndef PADDED_TYPES_H
#define PADDED_TYPES_H

#include "misc/bsd_stdatomic.h"//Until c11 stdatoic.h is available

#define CACHE_LINE_SIZE 64

typedef union CacheLinePaddedFlagImpl {
    volatile atomic_flag value;
    char padding[CACHE_LINE_SIZE];
} CacheLinePaddedFlag;

typedef union CacheLinePaddedBoolImpl {
    volatile atomic_bool value;
    char padding[CACHE_LINE_SIZE];
} CacheLinePaddedBool;

typedef union CacheLinePaddedIntImpl {
    volatile atomic_int value;
    char padding[CACHE_LINE_SIZE];
} CacheLinePaddedInt;

typedef union CacheLinePaddedUIntImpl {
    volatile atomic_uint value;
    char padding[CACHE_LINE_SIZE];
} CacheLinePaddedUInt;

typedef union CacheLinePaddedULongImpl {
    volatile atomic_ulong value;
    char padding[CACHE_LINE_SIZE];
} CacheLinePaddedULong;

typedef union CacheLinePaddedPointerImpl {
    volatile atomic_intptr_t value;
    char padding[CACHE_LINE_SIZE];
} CacheLinePaddedPointer;

typedef union CacheLinePaddedDoubleImpl {
    volatile double value;
    char padding[CACHE_LINE_SIZE];
} CacheLinePaddedDouble;

#endif
