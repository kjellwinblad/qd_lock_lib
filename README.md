qd\_lock\_lib
===========

qd\_lock\_lib is a locking library for multi-threaded programs that
targets queue delegation (QD) locking. QD locking is an efficient
delegation locking technique that can perform much better than
traditional locks but it requires a different interface.

QD locking makes it possible to delegate critical sections to the
current lock holder with the possibility to continue without waiting
for its actual execution. The lock holding thread can also execute
many critical sections in sequence while it has the data protected by
the critical section in its fast private cache which makes it very
efficent. More information about QD locking can be found
[here](http://www.it.uu.se/research/group/languages/software/qd_lock_lib).

qd\_lock\_lib contains a C (C11) implementation of QD locking as well
as a few other locks (TATAS, MCS, DR-MCS, CC-Synch) that can be used
with the same generic API. The corresponding library for C++ can be
found [here](http://github.com/davidklaftenegger/qd_library).

## Supported Platforms

The library is written with the intention of being as platform
independent as possible. The library tries to follow the C11 standard
so it should work on platforms that have compiler support for this
standard. Unfortunately, C11 is not fully supported by all major
compilers yet so some tweaking might be required to make it work on
some platforms. So far, only clang and gcc has been tested under Linux
and x86.

## Documentation

The paper [Queue Delegation
Locking](http://www.it.uu.se/research/group/languages/software/qd_lock_lib/paper.pdf)
is a good place to start if you have not heard about delegation
locking before or if you want to learn more.  [This
tutorial](https://github.com/kjellwinblad/qd_lock_lib/wiki/Tutorial)
is a good place to start if you want to use qd_lock_lib.  The full
locking API together with documentation can be found in [this
file](https://github.com/kjellwinblad/qd_lock_lib/blob/master/src/c/locks/locks.h).
Compilation instructions can be found below.

## Compile and test

qd\_lock\_lib uses the [Scons](http://www.scons.org/) build system so
you have to install that first. On Debian based systems, you can run
`sudo apt-get install scons`.

### clang or GCC

qd\_lock\_lib requires a recent version of clang or GCC. Type the
following in the root directory of the repository to compile with
clang:

    scons

Type the following in the root directory of the repository to compile
with gcc:

    scons --use_gcc

### Run tests

You first need to compile with Scons as explained above. When
everything has compiled, you should be able to run the tests with the
following commands:

    ./bin/test_lock QD_LOCK
    ./bin/test_lock MRQD_LOCK
    ./bin/test_lock TATAS_LOCK
    ./bin/test_lock MCS_LOCK
    ./bin/test_lock DRMCS_LOCK
    ./bin/test_lock CCSYNCH_LOCK

If this fails it might be because you are using an old version of
clang. clang had a bug in its atomics API so it is not safe to use an
old version of clang even if it compiles. If that is not the case,
report the bug in the
[issue tracker](https://github.com/kjellwinblad/qd_lock_lib/issues).

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


1. Include the `qd_lock_lib/src/c` directory in your source path.
2. Set the flag `-std=gnu11` for both clang and gcc.
3. Run `scons` or `scons --use_gcc` to produce the static and shared
   library files.
4. Add the static or shared library when compiling the program as
   explained below.

The following command shows how you compile the shared int example
program with clang and link with the static library. Replace clang
with gcc to compile with gcc instead. Also note that the name of
the static library differ between platforms.

    clang -o int_example -std=gnu11 -pthread  -Isrc/c src/c/examples/shared_int_example.c bin/libqd_lock_lib.a

Test the compiled program with:

    ./int_example

Use the following command to let clang compile the concurrent queue
example with the shared library:

    clang -o q_example -std=gnu11 -pthread -Isrc/c src/c/examples/concurrent_queue_example.c -L bin -l qd_lock_lib

To run the program with the shared library, you first have to add the
shared library to your system's library path:

    export LD_LIBRARY_PATH=bin/:$LD_LIBRARY_PATH

    ./q_example

## License

Open Source BSD License. See the
[LICENSE file](https://github.com/kjellwinblad/qd_lock_lib/blob/master/LICENSE)
for more information.
