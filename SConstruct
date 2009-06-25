#
# SConstruct file for EDGE
#
import os
import subprocess
import glob

EnsureSConsVersion(0, 96, 93)

#------------------------------------------------
# Utilities
#------------------------------------------------

class LibraryConfig():
	def __init__(self, name, header,
					dir=None,
					libs=None,
					defines=None,
					scons_script=None,
					required=1,
					no_include_dir=0,
					search_order=['system', 'internal', 'root'],
					subdirs={},
					syslib_script=None,
					deps=None):
		# Setup member vars
		self.name = name		# Name of library
		self.header = header	# Header to check
		# Directory name of library
		if dir == None:
			dir = name # Default to library name
		self.dir = dir
		# Libraries to include in the linker line
		if libs == None:
			libs = [name]
		self.libs = libs
		# Dictionary of defines to feed preprocessor if used.
		# Two entries:
		#   '<libtype>' => Defines to include if <libtype> library is used
		#   'all' => Defines to include in all cases
		if defines != None:
			defines = dict(defines) # Create a copy of the dict
		self.defines = defines
		# SCons script to call (used for libraries which are built)
		self.scons_script = scons_script
		# Required flag
		self.required = required
		# Do not include directory only preprocessor defines and linker line
		self.no_include_dir = no_include_dir
		# Order to search.
		# => 'external' - System wide libraries
		# => 'internal' - Platform specific lib directory within source directory
		# => 'root'     - Root of source directory
		if search_order != None:
			search_order = list(search_order) # Create copy
		self.search_order = search_order
		# Subdirectories to use. Root directory of lib is used if entry does not exist.
		# => 'lib' - library directory
		# => 'src' - Source directory
		if subdirs != None:
			subdirs = dict(subdirs) # Create copy
		self.subdirs = subdirs
		# Command to execute to get linker, compiler and preprocessor settings
		# if system library
		self.syslib_script = syslib_script
		# Setting dependencies. A list of other libraries to fetch settings on compile.
		# Special vale '__ALL__' indicates that all should be used
		if deps != None:
			deps = list(deps) # Create copy
		self.deps = deps
	# Force library choice to internal
	def forceInternal(self):
		if len(self.search_order) == 1 and self.search_order[0] == 'root':
			return
		for type in self.search_order:
			if type != 'internal':
				self.search_order.remove(type)
	# Get list of defines used for this library
	def __getDefines(self, libtype=None):
		if self.defines == None:
			return None
		def_list = [];
		if libtype != None and self.defines.has_key(libtype):
			def_list.extend(self.defines[libtype])
		if self.defines.has_key('all'):
			def_list.extend(self.defines['all'])
		if len(def_list) < 1:
			return None
		return def_list
	# Test this node for existence of library
	def __testNode(self, path, libtype):
		config = {}
		if self.subdirs.has_key('src'):
			test_path = os.path.join(path, self.subdirs['src'])
		else:
			test_path = path
		if os.path.isfile(os.path.join(test_path, self.header)):
			# Setup the include file path
			config['cpppath'] = test_path
			# Check if there is scons script file to run
			scons_script = None
			if self.scons_script != None:
				scons_script = os.path.join(config['cpppath'], self.scons_script)
				if os.path.isfile(scons_script):
					config['scons_script'] = scons_script
				else:
					scons_script = os.path.join(path, self.scons_script)
					if os.path.isfile(scons_script):
						config['scons_script'] = scons_script
			# Defines
			def_list = self.__getDefines(libtype)
			if def_list != None:
				config['defines'] = def_list
			# Setup the library path
			libpath = path
			if self.subdirs.has_key('lib'):
				libpath = os.path.join(path, self.subdirs['lib'])
			# Add library itself
			config['libs'] = []
			for lib_name in self.libs:
				(name,ext) = os.path.splitext(lib_name)
				if len(ext) != 0:
					name = name + ext
				else:
					name = "lib"+name+".a"
				config['libs'].append(os.path.join(libpath, name))
		return config
	# Get config information
	def get(self, env, conf, platform):
		config = {}
		if 'system' in self.search_order:
			if self.syslib_script != None:
				proc = subprocess.Popen(self.syslib_script,
										shell=True,
										stdout=subprocess.PIPE,
										stderr=subprocess.PIPE,
										close_fds=True)
				if proc.wait() == 0:
					config['raw'] = proc.stdout.read().strip()
					config['type'] = 'system'
			if len(config) == 0 and conf.CheckCHeader(self.header):
				libtype = 'system'
				config['libs'] = self.libs
				def_list = self.__getDefines(libtype)
				if def_list != None:
					config['defines'] = def_list
				config['type'] = libtype
		# Check interal
		if len(config) == 0 and 'internal' in self.search_order:
			libtype = 'system'
			test_pattern = os.path.join(platform+"_lib", self.dir+"*")
			dir_list = glob.glob(test_pattern)
			for dir_entry in dir_list:
				config = self.__testNode(os.path.abspath(dir_entry), libtype)
				if len(config) > 0:
					config['type'] = libtype
					break
		# Check root
		if len(config) == 0 and 'root' in self.search_order:
			libtype = 'root'
			dir_list = glob.glob(self.dir+"*")
			for dir_entry in dir_list:
				config = self.__testNode(os.path.abspath(dir_entry), libtype)
				if len(config) > 0:
					config['type'] = libtype
					break
		# Check for nothing found
		if len(config) == 0:
			return None
		# Build return string
		config_str = ""
		if config.has_key('raw'):
			config_str = config['raw']
		else:
			cwd = os.getcwd()
			if config.has_key('defines'):
				for define in config['defines']:
					config_str = config_str + " -D" + define
			if config.has_key('cpppath') and not self.no_include_dir:
				config_str = config_str + " -I"+config['cpppath']
			if config.has_key('libpath'):
				config_str = config_str + " -L"+config['libpath']
			if config.has_key('libs'):
				for lib in config['libs']:
					if len(os.path.dirname(lib)) == 0:
						config_str += " -l"+lib
					else:
						(name,ext) = os.path.splitext(lib)
						if len(ext) > 1:
							if os.path.isabs(name) and name.startswith(cwd):
								# Strip off absolute path if in current
								# dir and leading directory separator (visually pleasing)
								name = name[len(cwd)+1:]
						config_str = config_str + " " + (name+ext)
		scons_script = None
		if config.has_key('scons_script'):
			scons_script = config['scons_script']
		return Library(self.name, config['type'], config_str, scons_script, self.deps)

class Library():
	def __init__(self, name, type, config_str=None, scons_script=None, deps=None):
		self.name = name
		self.type = type
		self.config_str = config_str
		self.scons_script = scons_script
		self.deps = deps
	#
	def configEnv(self, env):
		output_files = []
		cwd = os.getcwd()
		for cfg_str in self.config_str.split(' '):
			if len(cfg_str) > 0:
				if not cfg_str.startswith('-'):
					output_files.append(cfg_str)
				elif cfg_str.startswith('-I'):
					env.Append(CCFLAGS = cfg_str) # Needed as scons under win32 breaks directory names
				else:
					env.MergeFlags(env.ParseFlags(cfg_str))
		return output_files

def getPlatform(win32_cross_compile="no"):
	platform = None
	# check platform
	if (os.name == "nt") or win32_cross_compile:
		platform = 'win32'
	elif os.name == "posix":
		# Determine the type of POSIX variant -ACB- 2009/06/08
		uname_info = os.uname()
		if (uname_info[0] == "Darwin"):
			platform = 'macosx'
		else:
			platform = 'linux'
	return platform

#------------------------------------------------
# Init base environment
#------------------------------------------------

base_env = Environment()

#------------------------------------------------
# Determine Build Setup
#------------------------------------------------

build_info = {}

build_info['cross'] = ('cross' in ARGUMENTS) and ARGUMENTS['cross']
build_info['release'] = ('release' in ARGUMENTS) and ARGUMENTS['release']
build_info['platform'] = getPlatform(build_info['cross'])

if build_info['platform'] == None:
	print "Unable to detect platform type."
	Exit(1)

Export('build_info')

#------------------------------------------------
# Detect Libraries
#------------------------------------------------

lib_configs = []

# The Engine itself
lib_configs.append(LibraryConfig(name='edge1',
								header='dm_defs.h',
								search_order=['root'],
								dir='src',
								no_include_dir=1,
								scons_script='SConscript',
								deps=['__ALL__']))
# OpenGL
if build_info['platform'] != 'macosx':
	lib_name = 'GL'
	if build_info['platform']=='win32':
		lib_name = 'opengl32'
	lib_configs.append(LibraryConfig(name='gl', header='GL/gl.h', libs=[lib_name]))
# GLEW
glew_libname = "GLEW"
if build_info['platform'] == 'win32':
	glew_libname = "glew32"
lib_configs.append(LibraryConfig(name='glew',
								header='GL/glew.h',
								subdirs={ 'src':'include', 'lib':'lib' },
								libs=[glew_libname]))
del glew_libname # No more use at global level for this
# glBSP
lib_configs.append(LibraryConfig(name='glbsp',
				  header='glbsp.h',
				  defines={ 'system' : ['HAVE_GLBSP_H'],
				            'internal' : ['HAVE_GLBSP_H']}, # Include HAVE_GLBSP_H if system wide lib used
				  subdirs={ 'src':'src' },
				  scons_script='SConscript.edge',
				  deps=['z']))
# SDL
if build_info['platform'] == 'win32':
	lib_configs.append(LibraryConfig(name='SDL',
									header='SDL.h',
									defines={'system': ['HAVE_SDL_H']},
									subdirs={ 'src' : 'include', 'lib':'build' },
									libs=['SDLmain', '.libs/SDL.dll']))
elif build_info['platform'] == 'macosx':
	lib_configs.append(LibraryConfig(name='SDLmain',
									header='SDLmain.h',
									search_order=['internal'],
									defines={'all': ['HAVE_SDL_H']},
									dir='SDL',
									no_include_dir=1,
									scons_script='SConscript'))
elif build_info['platform'] == 'linux':
	lib_configs.append(LibraryConfig(name='SDLmain',
									header='SDLmain.h',
									defines={'system': ['HAVE_SDL_H']},
									dir='SDL',
									syslib_script='sdl-config --cflags --libs'))

# LUA Scripting
lib_configs.append(LibraryConfig(name='lua',
								header='lua.h',
								search_order=['internal'], # Do not use system LUA
								subdirs={ 'src':'src', 'lib':'src'}))
# OGG Vorbis Support
lib_configs.append(LibraryConfig(name='vorbis',
								dir='libvorbis',
								header='vorbis/vorbisfile.h',
								subdirs={'src':'include', 'lib':'lib/.libs'},
								libs=['vorbisfile', 'vorbis']))
lib_configs.append(LibraryConfig(name='ogg',
								dir='libogg',
								header='ogg/ogg.h',
								subdirs={'src':'include', 'lib':'src/.libs'}))
# Zlib
lib_configs.append(LibraryConfig(name='z', dir='zlib', header='zlib.h'))
# FLTK
lib_configs.append(LibraryConfig(name='fltk',
								header='FL/Fl_Window.H',
								defines={'all': ['NO_CONSOLE_ECHO', 'USE_FLTK']},
								required=0,
								libs=['fltk', 'fltk_images'],
								subdirs={ 'lib':'lib' }))
# DDF Support for EDGE
lib_configs.append(LibraryConfig(name='ddf',
								header='main.h',
								search_order=['root'],
								dir='ddf',
								no_include_dir=1,
								scons_script='SConscript'))
# Dehacked support for EDGE
lib_configs.append(LibraryConfig(name='dehedge',
								header='dh_plugin.h',
								search_order=['root'],
								dir='deh_edge',
								no_include_dir=1,
								scons_script='SConscript.edge'))
# EDGE Programmer Interface
lib_configs.append(LibraryConfig(name='epi',
								header='epi.h',
								search_order=['root'],
								no_include_dir=1,
								scons_script='SConscript',
								deps=['png','jpeg','z']))
# PNG Support
lib_configs.append(LibraryConfig(name='png', header='png.h', dir='libpng'))
# JPEG Support
lib_configs.append(LibraryConfig(name='jpeg', header='jconfig.h'))
# Timidity
lib_configs.append(LibraryConfig(name='timidity',
								header='timidity.h',
								search_order=['root'],
								no_include_dir=1,
								scons_script='SConscript'))

# Check the list of libraries to use internally
if 'force_internal' in ARGUMENTS:
	force_internal_libs = ARGUMENTS['force_internal'].split(',')
	for lib_config in lib_configs:
		if lib_config.name in force_internal_libs:
			lib_config.forceInternal()

if build_info['cross']:
	force_internal_list=['jpeg','png','z','SDL','glew','ogg','vorbis']
	for lib_config in lib_configs:
		if lib_config.name in force_internal_list:
			lib_config.forceInternal()

# Get the configuration details for each library
conf = Configure(base_env.Clone())
libs = []
for lib_config in lib_configs:
	lib = lib_config.get(base_env, conf, build_info['platform'])
	if lib == None and lib_config.required:
		print "Unable to find library: "+lib_config.name
		Exit(1)
	if lib != None:
		libs.append(lib)
base_env = conf.Finish()

# Output libraries and their setup
tbl_format = " %-12s %-12s %s"
print
print tbl_format % ("Library", "Type", "Config")
print tbl_format % ("=======", "====", "======")
for lib in libs:
	print tbl_format % (lib.name, lib.type, lib.config_str.strip())

# Output build scripts being used by libraries
tbl_format = " %-12s %s"
print
print tbl_format % ("Library", "Build Script")
print tbl_format % ("=======", "============")
for lib in libs:
	if lib.scons_script != None:
		print tbl_format % (lib.name, lib.scons_script)

#------------------------------------------------
# Load any special setings from cross-compiling
#------------------------------------------------

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

#------------------------------------------------
# Run the SConscripts for each library
#------------------------------------------------

for lib in libs:
	# If library has a script for scons, run it.
	if lib.scons_script != None:
		env = base_env.Clone()
		# Handle library dependencies
		if lib.deps != None:
			lib_deps = []
			if lib.deps[0] == '__ALL__':
				for other_lib in libs:
					if other_lib != lib:
						lib_deps.append(other_lib)
			else:
				for other_lib in libs:
					if other_lib != lib and other_lib.name in lib.deps:
						lib_deps.append(other_lib)
			for other_lib in lib_deps:
				other_lib.configEnv(env)
		# Setup to build the library
		SConscript(lib.scons_script, exports='env')

#------------------------------------------------
# Overall final settings compile
#------------------------------------------------

engine_env = base_env.Clone()

EXECUTABLE_NAME = 'gledge32'

# ---> Merge the flags into the configuration
dep_files = ['main.cc']
for lib in libs:
	if len(lib.config_str) > 0:
		dep_files.extend(lib.configEnv(engine_env))

# platform specifics for the linker
if build_info['platform'] == 'win32':
	engine_env.Append(LIBS = ['wsock32', 'winmm', 'gdi32', 'dxguid', 'dinput'])
	engine_env.Append(LINKFLAGS = ['-mwindows','-lmingw32'])
	engine_env.Append(LINKFLAGS = ['edge32_res.o'])

if build_info['platform'] == 'macosx':
	engine_env.Append(LINKFLAGS = ['-Wl,-framework,Cocoa'])
	engine_env.Append(LINKFLAGS = ['-Wl,-framework,OpenGL'])
	engine_env.Append(LINKFLAGS = ['-Wl,-framework,QTKit'])
	engine_env.Append(LINKFLAGS = ['-Wl,-framework,SDL'])

if build_info['platform'] == 'linux':
	engine_env.Append(LINKFLAGS = ['-Wl,--warn-common'])

# Overall executable
engine_env.Program(EXECUTABLE_NAME, dep_files)

##--- editor settings ---
## vi:ts=4:sw=4:noexpandtab:filetype=python
