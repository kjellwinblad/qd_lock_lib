#include "reader_groups_read_indicator.h"

volatile atomic_int rgri_get_thread_id_counter = ATOMIC_VAR_INIT(0);

_Alignas(CACHE_LINE_SIZE)
_Thread_local RGRIGetThreadIDVarWrapper rgri_get_thread_id_var = {.value = -1};
