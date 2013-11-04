qd\_lock\_lib
===========

qd\_lock\_lib is mutual exclusion lock library that targets queue
delegation locking in particular. Currently, qd\_lock\_lib contains a
C implementation (C11) of queue delegation locking but more lock
implementations and programming languages are planned.

## Goals

* easy to use
* efficiency
* work on many platforms 

## Compile and test

### clang

qd\_lock\_lib requires a recent version of clang. It has been tested
with a version from the clang svn repository based on clang 3.4. Type
the following in the root directory of the repository to compile with
clang:

    scons

### gcc

qd\_lock\_lib has been tested with gcc 4.7.3. Type
the following in the root directory of the repository to compile with
gcc:

    scons --use_gcc

### Run tests

    ./bin_test QD_LOCK
    ./bin_test TATAS_LOCK

## How to use

Include the `qd_lock_lib/src/c` directory in your library path. You
also need to set the flag `-std=gnu11` for both clang and gcc. The
documentation in `qd_lock_lib/src/c/locks/locks.h` describes how to
use the locks.

    clang -std=gnu11 -pthread -Ipath_to_qd_lock_lib/src/c my_program.c

