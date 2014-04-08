#include "conc_splitch_set.h"


_Alignas(CACHE_LINE_SIZE)
OOSetMethodTable CONC_SPLITCH_SET_METHOD_TABLE =
{
    .free = &csh_set_free,
    .insert = &csh_set_insert,
    .insert_new = &csh_set_insert_new,
    .lookup = &csh_set_lookup,
    .delete = &csh_set_delete,
    .to_string = &csh_set_to_string,
    .print = &csh_set_print
};
