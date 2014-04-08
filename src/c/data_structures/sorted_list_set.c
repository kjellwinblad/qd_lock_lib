#include "sorted_list_set.h"

_Alignas(CACHE_LINE_SIZE)
OOSetMethodTable SORTED_LIST_SET_METHOD_TABLE = 
{
    .free = &sl_set_free,
    .insert = &sl_set_insert,
    .insert_new = &sl_set_insert_new,
    .lookup = &sl_set_lookup,
    .delete = &sl_set_delete,
    .to_string = &sl_set_to_string,
    .print = &sl_set_print
};
