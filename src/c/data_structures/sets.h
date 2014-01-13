#ifndef SETS_H
#define SETS_H

// Set API
// ========

#include "data_structures/sorted_list_set.h"
#include "data_structures/conc_splitch_set.h"
#include "data_structures/chained_hash_set.h"
#include "data_structures/oo_set_interface.h"
#include "misc/error_help.h"

// ## SET_create

typedef enum {
    SORTED_LIST_SET,
    CHAINED_HASH_SET,
    CONC_SPLITCH_SET,
    PLAIN_SORTED_LIST_SET,
    PLAIN_CHAINED_HASH_SET,
    PLAIN_CONC_SPLITCH_SET
} SET_set_type_name;

void * SET_create(SET_set_type_name setType,
                  unsigned int keyPosition,
                  void * (*extract_key)(void * v, int keyPos),
                  unsigned int (*hash_key)(void * k),
                  bool (*are_equal)(void * v1, void * v2),
                  char * (*to_string)(void * v1)){
    if(SORTED_LIST_SET == setType){
        return oo_sl_set_create(keyPosition,
                                extract_key,
                                hash_key,
                                are_equal,
                                to_string);
    } else if (CHAINED_HASH_SET == setType){
        return oo_ch_set_create(keyPosition,
                                          extract_key,
                                          hash_key,
                                          are_equal,
                                          to_string);
    } else if (CONC_SPLITCH_SET == setType){
        return oo_csh_set_create(keyPosition,
                                     extract_key,
                                     hash_key,
                                     are_equal,
                                     to_string);
    }
#ifdef __clang__
    else if(PLAIN_SORTED_LIST_SET == setType){
        return plain_sl_set_create(keyPosition,
                                   extract_key,
                                   hash_key,
                                   are_equal,
                                   to_string);
    } else if (PLAIN_CHAINED_HASH_SET == setType){
        return plain_ch_set_create(keyPosition,
                                   extract_key,
                                   hash_key,
                                   are_equal,
                                   to_string);
    } else if (PLAIN_CONC_SPLITCH_SET == setType){
        return plain_csh_set_create(keyPosition,
                                 extract_key,
                                 hash_key,
                                 are_equal,
                                 to_string);
    }
#endif
    LL_error_and_exit("Set type not supprted with GCC\n");
    return NULL;/* Should not be reachable */
}

// ## SET_free

#ifdef __clang__
#define SET_free(X) _Generic((X),\
                             OOSet * : ooset_free((OOSet*)X), \
                             ChainedHashSet * : ch_set_free((ChainedHashSet *)X),       \
                             SortedListSet * : sl_set_free((SortedListSet *)X), \
                             ConcSplitchSet *: csh_set_free((ConcSplitchSet *)X)       \
                )
#else
#define SET_free(X) ooset_free(X)
#endif

// ## SET_insert

#ifdef __clang__
#define SET_insert(X, e, size) _Generic((X), \
    OOSet * : ((OOSet *)X)->m->insert(X, e, size), \
    ChainedHashSet * : ch_set_insert(X, e, size), \
    SortedListSet * : sl_set_insert(X, e, size), \
    ConcSplitchSet *: csh_set_insert((ConcSplitchSet *)X, e, size) \
                            )
#else
#define SET_insert(X) ((OOSet *)X)->m->insert(X, e, size)
#endif

// ## SET_insert_new

#ifdef __clang__
#define SET_insert_new(X, e, size) _Generic((X), \
    OOSet * : ((OOSet *)X)->m->insert_new(X, e, size), \
    ChainedHashSet * : ch_set_insert_new(X, e, size), \
    SortedListSet * : sl_set_insert_new(X, e, size), \
    ConcSplitchSet *: csh_set_insert_new((ConcSplitchSet *)X, e, size) \
                          )
#else
#define SET_insert_new(X) ((OOSet *)X)->m->insert_new(X, e, size)
#endif


// ## SET_delete

#ifdef __clang__
#define SET_delete(X, k, kSize) _Generic((X),      \
    OOSet * : ((OOSet *)X)->m->delete(X, k, kSize), \
    ChainedHashSet * : ch_set_delete(X, k, kSize), \
    SortedListSet * : sl_set_delete(X, k, kSize), \
    ConcSplitchSet *: csh_set_delete((ConcSplitchSet *)X, k, kSize) \
                            )
#else
#define SET_delete(X, k, kSize) ((OOSet *)X)->m->delete(X, k, kSize)
#endif

// ## SET_lookup

#ifdef __clang__
#define SET_lookup(X, k) _Generic((X), \
    OOSet * : ((OOSet *)X)->m->lookup(X, k), \
    ChainedHashSet * : ch_set_lookup(X, k), \
    SortedListSet * : sl_set_lookup(X, k), \
    ConcSplitchSet *: csh_set_lookup((ConcSplitchSet *)X, k) \
                            )
#else
#define SET_lookup(X) ((OOSet *)X)->m->lookup(X, k)
#endif

// ## SET_to_string

#ifdef __clang__
#define SET_to_string(X) _Generic((X), \
    OOSet * : ((OOSet *)X)->m->to_string(X), \
    ChainedHashSet * : ch_set_to_string(X), \
    SortedListSet * : sl_set_to_string(X), \
    ConcSplitchSet * : csh_set_to_string(X) \
                            )
#else
#define SET_to_string(X) ((OOSet *)X)->m->to_string(X)
#endif

// ## SET_print

#ifdef __clang__
#define SET_print(X) _Generic((X), \
    OOSet * : ((OOSet *)X)->m->print(X), \
    ChainedHashSet * : ch_set_print(X), \
    SortedListSet * : sl_set_print(X),        \
    ConcSplitchSet * : csh_set_print(X) \
                            )
#else
#define SET_print(X) ((OOSet *)X)->m->print(X)
#endif


#endif
