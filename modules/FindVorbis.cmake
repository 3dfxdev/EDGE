#	CMake module to find libvorbis
#	Alan Witkowski
#
#	Input:
# 		VORBIS_ROOT          - Environment variable that points to the vorbis root directory
#
#	Output:
#		VORBIS_FOUND         - Set to true if vorbis was found
#		VORBIS_INCLUDE_DIR   - Path to vorbis.h
#		VORBIS_LIBRARIES     - Contains the list of vorbis libraries
#

set(VORBIS_FOUND false)

# find include path
find_path(
		VORBIS_INCLUDE_DIR
	NAMES
		vorbis/vorbisfile.h
	HINTS
		ENV VORBIS_ROOT
		ENV VORBISDIR
	PATHS
		/usr
		/usr/local
	PATH_SUFFIXES
		include
		include/vorbis
		vorbis
)

# find debug library
find_library(
		VORBIS_DEBUG_LIBRARY vorbis_d libvorbis_d vorbis libvorbis
	HINTS
		ENV VORBIS_ROOT
		ENV VORBISDIR
	PATHS
		/usr/lib
		/usr/local/lib
	PATH_SUFFIXES
		lib
		win32/VS2010/x64/Debug
)

# find release library
find_library(
		VORBIS_RELEASE_LIBRARY vorbis libvorbis
	HINTS
		ENV VORBIS_ROOT
		ENV VORBISDIR
	PATHS
		/usr/lib
		/usr/local/lib
	PATH_SUFFIXES
		lib
		win32/VS2010/x64/Release
)

# find debug vorbisfile library
find_library(
		VORBISFILE_DEBUG_LIBRARY vorbisfile_d libvorbisfile_d vorbisfile libvorbisfile
	HINTS
		ENV VORBIS_ROOT
		ENV VORBISDIR
	PATHS
		/usr/lib
		/usr/local/lib
	PATH_SUFFIXES
		lib
		win32/VS2010/x64/Debug
)

# find release vorbisfile library
find_library(
		VORBISFILE_RELEASE_LIBRARY vorbisfile libvorbisfile
	HINTS
		ENV VORBIS_ROOT
		ENV VORBISDIR
	PATHS
		/usr/lib
		/usr/local/lib
	PATH_SUFFIXES
		lib/
		win32/VS2010/x64/Release
)

# combine debug and release
set(VORBIS_LIBRARIES
	debug ${VORBIS_DEBUG_LIBRARY}
	optimized ${VORBIS_RELEASE_LIBRARY}
)

# combine debug and release
set(VORBISFILE_LIBRARIES
	debug ${VORBISFILE_DEBUG_LIBRARY}
	optimized ${VORBISFILE_RELEASE_LIBRARY}
)

# set libraries var
set(VORBIS_LIBRARIES ${VORBIS_LIBRARIES} ${VORBISFILE_LIBRARIES})

# handle QUIET and REQUIRED
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Vorbis DEFAULT_MSG VORBIS_LIBRARIES VORBIS_INCLUDE_DIR)

# advanced variables only show up in gui if show advanced is turned on
mark_as_advanced(VORBIS_INCLUDE_DIR VORBIS_LIBRARIES)