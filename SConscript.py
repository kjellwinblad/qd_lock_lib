import os

############
#Definitions
############

Import('mode')

use_gcc = GetOption('use_gcc')

std_cc_flags = ['-std=gnu11',
                '-Wall',
                '-Wextra',
                '-pedantic',
                '-fPIC',
                '-pthread']
std_cxx_flags = ['-std=c++11',
		 '-Wall',
		 '-Wextra',
		 '-pedantic',
		 '-fPIC']

std_link_flags = ['-pthread']

debug_flags = ['-O0',
               '-g']

optimization_flags = ['-O3']

profile_flags = ['-fno-omit-frame-pointer -O2']

if(mode=='debug'):
    std_cc_flags = std_cc_flags + debug_flags
    std_cxx_flags = std_cxx_flags + debug_flags
    std_link_flags = std_link_flags + debug_flags
elif(mode=='profile'):
    std_cc_flags = std_cc_flags + profile_flags
    std_cxx_flags = std_cxx_flags + profile_flags
    std_link_flags = std_link_flags + profile_flags
else:
    std_cc_flags = std_cc_flags + optimization_flags
    std_cxx_flags = std_cxx_flags + optimization_flags
    std_link_flags = std_link_flags + optimization_flags

cc = 'clang'
cxx = 'clang++'

if use_gcc:
    cc = 'gcc-4.8'
    cxx = 'g++'

env = Environment(
    ENV = os.environ,
    CFLAGS = ' '.join(std_cc_flags),
    CXXFLAGS = ' '.join(std_cxx_flags),
    CC = cc,
    CXX = cxx,
    LINKFLAGS = ' '.join(std_link_flags),
    CPPPATH = ['src/c/'])


######
#Build
######

#Tests
######

# env.Program(source='src/c/tests/test_set.c',
#             target='test_set')

env.Program(source='src/c/tests/test_lock.c',
            target='test_lock',
            CPPDEFINES=[('LOCK_TYPE', 'OOLock')])

env.Program(source='src/c/tests/test_qd_queue.c',
            target='test_qd_queue')

if not use_gcc:
    #Plain locks
    #type, type name
    all_locks = [('TATASLock', 'PLAIN_TATAS_LOCK'),
                 ('QDLock', 'PLAIN_QD_LOCK'),
                 ('MRQDLock', 'PLAIN_MRQD_LOCK')]
    
    for (lock_type, lock_type_name) in all_locks:
        object = env.Object(source='src/c/tests/test_lock.c',
                            target='objects/test_' + lock_type_name + '.o',
                            CPPDEFINES=[('LOCK_TYPE', lock_type),
                                        ('LOCK_TYPE_NAME', lock_type_name)])
        env.Program(source=object,
                    target='test_' + lock_type_name)
    #Plain Sets
    #type, type name
    all_sets = [('ChainedHashSet', 'PLAIN_CHAINED_HASH_SET'),
                ('SortedListSet', 'PLAIN_SORTED_LIST_SET'),
                ('ConcSplitchSet', 'PLAIN_CONC_SPLITCH_SET')
                ]
    
    for (set_type, set_type_name) in all_sets:
        object = env.Object(source='src/c/tests/test_set.c',
                            target='objects/test_' + set_type_name + '.o',
                            CPPDEFINES=[('SET_TYPE', set_type),
                                        ('SET_TYPE_NAME', set_type_name)])
        env.Program(source=object,
                    target='test_' + set_type_name)


#Examples
#########

env.Program(source='src/c/examples/qd_lock_delegate_example.c',
            target='qd_lock_delegate_example')

env.Program(source='src/c/examples/shared_int_example.c',
            target='shared_int_example')
