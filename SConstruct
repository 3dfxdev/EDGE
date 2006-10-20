#
# SConstruct file for EDGE
#
import os

build_info = {}

build_info['cross'] = ('cross' in ARGUMENTS) and ARGUMENTS['cross']
build_info['debug'] = ('debug' in ARGUMENTS) and ARGUMENTS['debug']

# check platform
if (os.name == "nt") or build_info['cross']:
    build_info['platform'] = 'win32'
elif os.name == "posix":
    build_info['platform'] = 'linux'
else:
    print 'Unknown OS type: ' + os.name
    Exit(1)

Export('build_info')


#------------------------------------------------

base_env = Environment()

if build_info['cross']:
  base_env.Tool('crossmingw', toolpath=['build'])

# warnings
base_env.Append(CCFLAGS = ['-Wall'])

# optimisation
if build_info['debug']:
  base_env.Append(CCFLAGS = ['-O', '-g3'])
else:
  base_env.Append(CCFLAGS = ['-O2'])

# platform
base_env.Append(CCFLAGS = ['-D' + build_info['platform'].upper()])

Export('base_env')


#------------------------------------------------

SConscript('src/SConscript')
SConscript('epi/SConscript')
SConscript('deh_edge/SConscript.plugin')
SConscript('glbsp/SConscript.plugin')
SConscript('lzo/SConscript')
# SConscript('humidity/SConscript.plugin')


#----- LIBRARIES ----------------------------------

env = base_env.Copy()

# EDGE itself
env.Append(CPPPATH = ['./src'])
env.Append(LIBPATH = ['./src'])
env.Append(LIBS = ['edge'])

# epi
env.Append(LIBPATH = ['./epi'])
env.Append(LIBS = ['epi'])

# GLBSP
env.Append(LIBPATH = ['./glbsp'])
env.Append(LIBS = ['glbsp'])

# Deh_Edge
env.Append(LIBPATH = ['./deh_edge'])
env.Append(LIBS = ['dehedge'])

# LZO
env.Append(LIBPATH = ['./lzo'])
env.Append(LIBS = ['lzo'])

# JPEG, PNG and ZLIB
env.Append(LIBS = ['png', 'jpeg', 'z'])

# SDL
env.ParseConfig('sdl-config --cflags --libs')

# OpenGL
env.Append(LIBS = ['GL'])

# OpenAL
env.Append(LIBS = ['openal'])

# Ogg/Vorbis
env.Append(LIBS = ['ogg', 'vorbis', 'vorbisfile'])

env.Program('gledge', [])

##--- editor settings ---
## vi:ts=4:sw=4:expandtab
