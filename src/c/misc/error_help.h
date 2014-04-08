#ifndef ERROR_HELP_H
#define ERROR_HELP_H

#include <stdio.h>
#include <stdlib.h>

static inline void LL_error_and_exit_verbose(const char * file, const char * function, int line, char * message){
    printf("ERROR IN FILE: %s, FUNCTION: %s, LINE: %d\n", file, function, line);
    printf("%s\n", message);
    printf("EXITING\n");
    exit(1);
}

#define LL_error_and_exit(message) LL_error_and_exit_verbose(__FILE__, __func__, __LINE__, message)

#endif
