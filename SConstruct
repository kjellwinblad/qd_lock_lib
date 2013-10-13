

AddOption('--use_gcc',
          action='store_true',
          dest='use_gcc',
          default=False)

mode = 'release'

SConscript('SConscript.py', variant_dir='bin', duplicate=0, exports='mode')

mode = 'debug'

SConscript('SConscript.py', variant_dir='bin_debug', duplicate=0, exports='mode')

mode = 'profile'

SConscript('SConscript.py', variant_dir='bin_profile', duplicate=0, exports='mode')
