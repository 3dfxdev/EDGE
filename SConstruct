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

if build_info['platform'] == 'win32':
    base_env.Append(CPPPATH = ['#jpeg-6b'])
    base_env.Append(CPPPATH = ['#libpng-1.2.12'])
    base_env.Append(CPPPATH = ['#zlib-1.2.3'])

Export('base_env')


#----- LIBRARIES ----------------------------------

env = base_env.Copy()

# EDGE itself
###--- env.Append(CPPPATH = ['#src'])
env.Append(LIBPATH = ['#src'])
env.Append(LIBS = ['edge1'])

# epi
env.Append(LIBPATH = ['#epi'])
env.Append(LIBS = ['epi'])

# GLBSP
env.Append(LIBPATH = ['#glbsp'])
env.Append(LIBS = ['glbsp'])

# Deh_Edge
env.Append(LIBPATH = ['#deh_edge'])
env.Append(LIBS = ['dehedge'])

# LZO
env.Append(LIBPATH = ['#lzo'])
env.Append(LIBS = ['lzo'])

# JPEG, PNG and ZLIB
env.Append(LIBS = ['png', 'jpeg', 'z'])

# FLTK
if 0 and build_info['platform'] == 'linux':
    env.Append(CCFLAGS = ['-DUSE_FLTK'])
    # FIXME

# HawkNL
if 1:
    env.Append(CCFLAGS = ['-DUSE_HAWKNL'])
    env.Append(CPPPATH = ['#HawkNL1.70/include'])
    env.Append(LIBPATH = ['#HawkNL1.70/src'])
    env.Append(LIBS = ['NL2'])

# SDL
if build_info['platform'] == 'win32':
    env.Append(LIBS = ['libSDL'])
    # the following is a HACK (stupid fucking linker!)
    env.Append(LINKFLAGS = ['./SDL-1.2.11/build/SDL_win32_main.o'])
else: # linux
    env.ParseConfig('sdl-config --cflags --libs')

# OpenGL
if build_info['platform'] == 'win32':
    env.Append(LIBS = ['opengl32'])
else:
    env.Append(LIBS = ['GL'])

# Ogg/Vorbis
env.Append(LIBS = ['vorbisfile', 'vorbis', 'ogg'])

if build_info['platform'] == 'win32':
    env.Append(CPPPATH = ['#libogg-1.1.3/include'])
    env.Append(CPPPATH = ['#libvorbis-1.1.2/include'])
    env.Append(CPPPATH = ['#SDL-1.2.11/include'])
    #
    env.Append(LIBPATH = ['#jpeg-6b'])
    env.Append(LIBPATH = ['#libpng-1.2.12'])
    env.Append(LIBPATH = ['#zlib-1.2.3'])
    #
    env.Append(LIBPATH = ['#libogg-1.1.3/src'])
    env.Append(LIBPATH = ['#libvorbis-1.1.2/lib'])
    env.Append(LIBPATH = ['#SDL-1.2.11/build'])
    env.Append(LIBPATH = ['#SDL-1.2.11/build/.libs'])
    #
    env.Append(LIBS = ['wsock32', 'winmm', 'gdi32', 'dxguid', 'dinput'])
    env.Append(LINKFLAGS = ['-mwindows'])

main_env = env

Export('main_env')  # for src/SConscript

#------------------------------------------------

SConscript('src/SConscript')
SConscript('epi/SConscript')
SConscript('deh_edge/SConscript.edge')
SConscript('glbsp/SConscript.edge')
SConscript('lzo/SConscript')
# SConscript('humidity/SConscript.edge')

env.Program('gledge', ['main.cc'])

##--- editor settings ---
## vi:ts=4:sw=4:expandtab
