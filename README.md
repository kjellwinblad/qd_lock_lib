qd\_lock\_lib
===========

qd\_lock\_lib is a locking library for multi-threaded programs that
targets queue delegation (QD) locking. QD locking is an efficient
delegation locking technique that can perform much better than
traditional locks but it requires a different interface. QD locking
makes it possible to delegate critical sections to the current lock
holder with the possibility to continue without waiting for its actual
execution. The lock holding thread can execute many critical sections
in sequence while it has the data protected by the critical section in
its fast private cache. More information about QD locking can be found
[here](http://www.it.uu.se/research/group/languages/software/qd_lock_lib). qd\_lock\_lib
contains a C implementation (C11) of QD locking as well as a few other
locks that can be used with the same generic API. The corresponding
library for C++ can be found
[here](http://github.com/davidklaftenegger/qd_library).

## Features

* easy to use
* efficient
* works on many platforms 

## Documentation

The paper [Queue Delegation
Locking](http://www.it.uu.se/research/group/languages/software/qd_lock_lib/paper.pdf)
is a good place to start if you have not heard about delegation
locking before or if you want to learn more.  [This
tutorial](https://github.com/kjellwinblad/qd_lock_lib/wiki/Tutorial)
is a good place to start if you want to use qd_lock_lib.  The full
locking API together with documentation can be found in [this
file](https://github.com/kjellwinblad/qd_lock_lib/blob/master/src/c/locks/locks.h).
Compilation instructions can be found in this file.

## Compile and test

qd\_lock\_lib uses the [Scons](http://www.scons.org/) build system so
you have to install that first. On Debian based systems, you can run
`sudo apt-get install scons`.

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
    ./bin_test MRQD_LOCK
    ./bin_test TATAS_LOCK
    ./bin_test MCS_LOCK
    ./bin_test DRMCS_LOCK
    ./bin_test CCSYNCH_LOCK

If this fails it is likely that you will have to upgrade your
compiler. clang has a bug in its atomics API so it is not safe to use
an old version of clang even if it compiles.

## How to use

[This tutorial](http://github.com/kjellwinblad/qd_lock_lib/wiki/Tutorial)
is a good place to start.  The API is documented in the file
`src/c/locks/locks.h`. You might also want to have a look at the
examples:

`src/c/examples/shared_int_example.c` - shows how to use the functions
`LL_delegate`, `LL_delegate_wait`, `LL_lock`, `LL_unlock`, `LL_rlock`
and `LL_runlock`.

`src/c/examples/concurrent_queue_example.c` - shows how to use the
functions `LL_delegate_or_lock`, `LL_delegate_unlock` and
`LL_close_delegate_buffer`. It also shows how to implement a
concurrent queue with QD locking.

`src/c/examples/qd_lock_delegate_example.c` - starts up multiple
threads that issues delegated critical sections.

## How to compile with your program

Include the `qd_lock_lib/src/c` directory in your library path. You
also need to set the flag `-std=gnu11` for both clang and gcc.

This is how you compile one of the example programs with clang:

    clang -std=gnu11 -pthread  -Isrc/c src/c/examples/shared_int_example.c
    
Here is how you compile another example program with GCC:

    gcc -std=gnu11 -pthread  -Isrc/c src/c/examples/concurrent_queue_example.c


