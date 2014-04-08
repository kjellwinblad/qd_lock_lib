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

read_indicator_object = env.Object(source='src/c/read_indicators/reader_groups_read_indicator.c')
ccsynch_lock_object = env.Object(source='src/c/locks/ccsynch_lock.c')
drmcs_lock_object = env.Object(source='src/c/locks/drmcs_lock.c')
mcs_lock_object = env.Object(source='src/c/locks/mcs_lock.c')
mrqd_lock_object = env.Object(source='src/c/locks/mrqd_lock.c')
qd_lock_object = env.Object(source='src/c/locks/qd_lock.c')
tatas_lock_object = env.Object(source='src/c/locks/tatas_lock.c')

lock_dependencies = [read_indicator_object,ccsynch_lock_object,drmcs_lock_object,mcs_lock_object,mrqd_lock_object,qd_lock_object,tatas_lock_object]

chained_hash_set_object = env.Object(source='src/c/data_structures/chained_hash_set.c')
conc_splitch_set_object = env.Object(source='src/c/data_structures/conc_splitch_set.c')
sorted_list_set_object = env.Object(source='src/c/data_structures/sorted_list_set.c')

data_structures_dependencies = [chained_hash_set_object,conc_splitch_set_object,sorted_list_set_object]

dependencies = lock_dependencies + data_structures_dependencies


#Static Library
################

static_lib = env.StaticLibrary(target = 'qd_lock_lib', source = dependencies)


#Dynamic Library
################

dynamic_lib = env.SharedLibrary(target = 'qd_lock_lib',
                                source = 
                                Glob('src/c/data_structures/*.c') + 
                                Glob('src/c/locks/*.c') +
                                Glob('src/c/read_indicators/*.c'))

#Tests
######

env.Program(source=['src/c/tests/test_lock.c'] + dependencies,
            target='test_lock',
            CPPDEFINES=[('LOCK_TYPE', 'OOLock')])

env.Program(source='src/c/tests/test_qd_queue.c',
            target='test_qd_queue')

if not use_gcc:
    #Plain locks
    #type, type name
    all_locks = [('TATASLock', 'PLAIN_TATAS_LOCK'),
                 ('QDLock', 'PLAIN_QD_LOCK'),
                 ('MRQDLock', 'PLAIN_MRQD_LOCK'),
                 ('CCSynchLock', 'PLAIN_CCSYNCH_LOCK'),
                 ('MCSLock', 'PLAIN_MCS_LOCK'),
                 ('DRMCSLock', 'PLAIN_DRMCS_LOCK')]
    
    for (lock_type, lock_type_name) in all_locks:
        object = env.Object(source='src/c/tests/test_lock.c',
                            target='objects/test_' + lock_type_name + '.o',
                            CPPDEFINES=[('LOCK_TYPE', lock_type),
                                        ('LOCK_TYPE_NAME', lock_type_name)])
        env.Program(source=[object] + dependencies,
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
        env.Program(source=[object] + dependencies,
                    target='test_' + set_type_name)



#Static Library
################

static_lib = env.StaticLibrary(target = 'qd_lock_lib', source = dependencies)

#Examples
#########

env.Program(source=['src/c/examples/qd_lock_delegate_example.c'] + static_lib,
            target='qd_lock_delegate_example')

env.Program(source=['src/c/examples/shared_int_example.c'] + static_lib,
            target='shared_int_example')

env.Program(source=['src/c/examples/concurrent_queue_example.c'] + static_lib,
            target='concurrent_queue_example')
