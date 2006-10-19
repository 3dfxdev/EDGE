#
# SConstruct file for EDGE
#
import os

base_env = Environment()

# warnings
base_env.Append(CCFLAGS = ['-Wall'])

# optimisation
base_env.Append(CCFLAGS = ['-O', '-g3'])

Export('base_env')

SConscript('src/SConscript')
SConscript('epi/SConscript')
SConscript('deh_edge/SConscript.plugin')
SConscript('glbsp/SConscript.plugin')
SConscript('lzo/SConscript')
# SConscript('humidity/SConscript.plugin')

env = base_env.Copy()

#----- LIBRARIES ----------------------------------

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

