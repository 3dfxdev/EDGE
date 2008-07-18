#
# SConstruct file for EDGE
#
import os

build_info = {}

build_info['cross'] = ('cross' in ARGUMENTS) and ARGUMENTS['cross']
build_info['release'] = ('release' in ARGUMENTS) and ARGUMENTS['release']

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
if build_info['release']:
    base_env.Append(CCFLAGS = ['-O2'])
else:
    base_env.Append(CCFLAGS = ['-O', '-g3'])

# platform
base_env.Append(CCFLAGS = ['-D' + build_info['platform'].upper()])

if build_info['platform'] == 'win32':
    base_env.Append(CPPPATH = ['#win32_lib/jpeg-6b'])
    base_env.Append(CPPPATH = ['#win32_lib/libpng-1.2.12'])
    base_env.Append(CPPPATH = ['#win32_lib/zlib-1.2.3'])

if build_info['platform'] == 'linux' and build_info['release']:
    base_env.Append(CPPPATH = ['#linux_lib/jpeg-6b'])
    base_env.Append(CPPPATH = ['#linux_lib/libpng-1.2.12'])
    base_env.Append(CPPPATH = ['#linux_lib/zlib-1.2.3'])

Export('base_env')


#----- LIBRARIES ----------------------------------

env = base_env.Copy()

# check for globally installed glBSP headers
have_glbsp_h = 0
have_lua_h = 0

if 1 and build_info['platform'] == 'linux':
    conf = Configure(env)

    if conf.CheckCXXHeader('glbsp.h'):
        have_glbsp_h = 1
        env.Append(CCFLAGS = ['-DHAVE_GLBSP_H'])

### -AJA- System-wide lua is disabled since we require the Lua library
###       to be compiled with a C++ compiler
#
#    if conf.CheckCHeader('lua.h'):
#        have_lua_h = 1
#        env.Append(CCFLAGS = ['-DHAVE_LUA_H'])
#
#    # Debian crap
#    if conf.CheckCHeader('lua5.1/lua.h'):
#        have_lua_h = 2
#        env.Append(CCFLAGS = ['-DHAVE_LUA_51_H'])

    env = conf.Finish()


env.Append(LINKFLAGS = ['-Wl,--warn-common'])

## if build_info['platform'] == 'linux' and build_info['release']:
##    env.Append(LINKFLAGS = ['-Wl,--as-needed'])

# EDGE itself
env.Append(LIBPATH = ['#src'])
env.Append(LIBS = ['edge1'])

# DDF parser
env.Append(LIBPATH = ['#ddf'])
env.Append(LIBS = ['ddf'])

# Timidity
env.Append(LIBPATH = ['#timidity'])
env.Append(LIBS = ['timidity'])

# epi
env.Append(LIBPATH = ['#epi'])
env.Append(LIBS = ['epi'])

# GLBSP
if not have_glbsp_h:
    env.Append(LIBPATH = ['#glbsp'])

env.Append(LIBS = ['glbsp'])

# Deh_Edge
env.Append(LIBPATH = ['#deh_edge'])
env.Append(LIBS = ['dehedge'])

# LUA
if have_lua_h == 2:
    env.Append(LIBS = ['lua5.1'])
elif have_lua_h:
    env.Append(LIBS = ['lua'])
else:
    if build_info['platform'] == 'win32':
        env.Append(CPPPATH = ['#win32_lib/lua-5.1.3/src'])
        env.Append(LIBPATH = ['#win32_lib/lua-5.1.3/src'])
    else: #linux
        env.Append(CPPPATH = ['#linux_lib/lua-5.1.3/src'])
        env.Append(LIBPATH = ['#linux_lib/lua-5.1.3/src'])
    env.Append(LIBS = ['lua'])

# JPEG, PNG and ZLIB
if build_info['platform'] == 'win32':
    env.Append(LIBPATH = ['#win32_lib/jpeg-6b'])
    env.Append(LIBPATH = ['#win32_lib/libpng-1.2.12'])
    env.Append(LIBPATH = ['#win32_lib/zlib-1.2.3'])

if build_info['platform'] == 'linux' and build_info['release']:
    env.Append(LIBPATH = ['#linux_lib/zlib-1.2.3'])
    env.Append(LIBPATH = ['#linux_lib/libpng-1.2.12'])
    env.Append(LIBPATH = ['#linux_lib/jpeg-6b'])

env.Append(LIBS = ['png', 'jpeg', 'z'])

# FLTK
if build_info['platform'] == 'linux' and build_info['release']:
    env.Append(CCFLAGS = ['-DUSE_FLTK'])
    env.ParseConfig('#linux_lib/fltk-1.1.7/fltk-config --cflags')
    env.Append(LIBPATH = ['#linux_lib/fltk-1.1.7/lib'])
    env.Append(LIBS = ['fltk', 'fltk_images'])

# SDL
if build_info['platform'] == 'win32':
    env.Append(CPPPATH = ['#win32_lib/SDL-1.2.13/include'])
    env.Append(LIBPATH = ['#win32_lib/SDL-1.2.13/lib'])
    # fucking stupid linker needs the next line
    env.Append(LIBS = ['-lmingw32'])
    env.Append(LIBS = ['-lSDLmain', '-lSDL.dll'])
else: # linux
    env.ParseConfig('sdl-config --cflags --libs')

# GLEW (GL Extension Wrangler) library
if build_info['platform'] == 'win32':
    env.Append(CPPPATH = ['#win32_lib/glew-1.4/include'])
    env.Append(LIBPATH = ['#win32_lib/glew-1.4/lib'])
    env.Append(LIBS = ['GLEW'])
else:
    env.Append(CPPPATH = ['#linux_lib/glew-1.4/include'])
    env.Append(LIBPATH = ['#linux_lib/glew-1.4/lib'])
    env.Append(LIBS = ['GLEW'])

# OpenGL
if build_info['platform'] == 'win32':
    env.Append(LIBS = ['opengl32'])
else:
    env.Append(LIBS = ['GL'])

# Ogg/Vorbis
env.Append(CCFLAGS = ['-DUSE_OGG'])
env.Append(LIBS = ['vorbisfile', 'vorbis', 'ogg'])

if build_info['platform'] == 'win32':
    env.Append(CPPPATH = ['#win32_lib/libogg-1.1.3/include'])
    env.Append(LIBPATH = ['#win32_lib/libogg-1.1.3/src'])
    env.Append(CPPPATH = ['#win32_lib/libvorbis-1.1.2/include'])
    env.Append(LIBPATH = ['#win32_lib/libvorbis-1.1.2/lib'])

if build_info['platform'] == 'linux' and build_info['release']:
    env.Append(CPPPATH = ['#linux_lib/libogg-1.1.3/include'])
    env.Append(LIBPATH = ['#linux_lib/libogg-1.1.3/'])
    env.Append(CPPPATH = ['#linux_lib/libvorbis-1.1.2/include'])
    env.Append(LIBPATH = ['#linux_lib/libvorbis-1.1.2/'])
    #
    # env.Append(LINKFLAGS = ['linux_lib/libvorbis-1.1.2/libvorbisfile.a'])
    # env.Append(LINKFLAGS = ['linux_lib/libvorbis-1.1.2/libvorbis.a'])
    # env.Append(LINKFLAGS = ['linux_lib/libogg-1.1.3/libogg.a'])


# platform specifics

if build_info['platform'] == 'win32':
    env.Append(LIBS = ['wsock32', 'winmm', 'gdi32', 'dxguid', 'dinput'])
    env.Append(LINKFLAGS = ['-mwindows'])
    env.Append(LINKFLAGS = ['edge32_res.o'])

main_env = env

Export('main_env')  # for src/SConscript

#------------------------------------------------

SConscript('src/SConscript')
SConscript('ddf/SConscript')
SConscript('epi/SConscript')
SConscript('deh_edge/SConscript.edge')

if not have_glbsp_h:
    SConscript('glbsp/SConscript.edge')

SConscript('timidity/SConscript')

env.Program('gledge32', ['main.cc'])

##--- editor settings ---
## vi:ts=4:sw=4:expandtab:filetype=python
