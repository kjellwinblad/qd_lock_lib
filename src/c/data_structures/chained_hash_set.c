#include "chained_hash_set.h"

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
