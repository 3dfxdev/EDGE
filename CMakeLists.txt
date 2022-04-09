cmake_minimum_required(VERSION 3.2.2)
################################################################################
################################################################################
## Copyright (c) 1997-2020 The EDGE Team (see AUTHORS.md)
##
## This software is free software; you can redistribute it and/or
## modify it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2 of the License, or
## (at your option) any later version.
##
## This software is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 51 Franklin St, Fifth Floor,
## Boston, MA  02110-1301  USA
##
################################################################################
######################### CMake Configuration ##################################

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
	message(STATUS "VitaSDK found! Configuring to use VitaSDK")
  else()
    message(STATUS "Please define VITASDK to point to your SDK path if you want to build for VITA!")
  endif()
  if(DEFINED ENV{VCPKG_ROOT})
	 set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
     CACHE STRING "")
  else()
	message(STATUS "Please define VCPKG_ROOT to point to your cmake script if you want to use VCPKG!")
  endif()
endif()

project(EDGE
LANGUAGES CXX
VERSION 2.1.0
)


if(VITA)
	include("${VITASDK}/share/vita.cmake" REQUIRED)
	set(VITA_APP_NAME "vitaEDGE")
	set(VITA_TITLEID  "VSDK00017")
	set(VITA_VERSION  "01.00")
endif()

if( COMMAND cmake_policy )
	if( POLICY CMP0011 )
		cmake_policy( SET CMP0011 NEW )
	endif()
	if( POLICY CMP0054 )
		cmake_policy( SET CMP0054 NEW )
	endif()
endif()

IF (${CMAKE_BINARY_DIR} STREQUAL ${CMAKE_SOURCE_DIR})
        MESSAGE( FATAL_ERROR "In-tree Builds are NOT supported.")
ENDIF (${CMAKE_BINARY_DIR} STREQUAL ${CMAKE_SOURCE_DIR})

list( APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake )
#include( FindPackageHandleStandardArgs )
#set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH} cmake")


macro( make_release_only )
	set( CMAKE_C_FLAGS_MINSIZEREL ${CMAKE_C_FLAGS_RELEASE} )
	set( CMAKE_C_FLAGS_RELWITHDEBINFO ${CMAKE_C_FLAGS_RELEASE} )
	string( REPLACE "/MT " "/MTd " CMAKE_C_FLAGS_DEBUG ${CMAKE_C_FLAGS_RELEASE} )
	set( CMAKE_CXX_FLAGS_MINSIZEREL ${CMAKE_CXX_FLAGS_RELEASE} )
	set( CMAKE_CXX_FLAGS_RELWITHDEBINFO ${CMAKE_CXX_FLAGS_RELEASE} )
	string( REPLACE "/MT " "/MTd " CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_RELEASE} )
endmacro()

set( LZMA_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/deps/lzma/C" )
option(FORCE_STATIC_MSVC_LIBS "Use pre-build via ./libs" OFF)
option(FORCE_INTERNAL_ZLIB "Use internal zlib")
option(FORCE_INTERNAL_BZIP2 "Use internal bzip2")
option(FORCE_INTERNAL_PHYSFS "Use internal physfs")
option(HYPERTENSION "Compile Hypertension Executable" OFF)
if(MINGW)
	option(MINGW_LINK_STATIC "Link statically on MinGW" ON)
endif()

if(MINGW AND MINGW_LINK_STATIC)
	SET(CMAKE_FIND_LIBRARY_PREFIXES "lib")
	SET(CMAKE_FIND_LIBRARY_SUFFIXES .lib .a)
endif()


#find_package(Ogg REQUIRED)
#find_package(Vorbis REQUIRED)
find_package( ZLIB )
find_package( BZip2 )
find_package ( PhysFS )
find_package(SDL2 REQUIRED)
#find_package ( libdumb )

#find_package ( FFMPEG )
#find_package ( SDLKitchensink )

if(APPLE)
find_library(COCOA_LIBRARY Cocoa)
FIND_PATH(COCOA_INCLUDE_DIR Cocoa/Cocoa.h)
endif()

if(MSVC)
add_compile_options(/Qspectre-)
endif()

#set (SDL_kitchensink_static "Statically link SDLKitchensink" ON)
#set (SDL_kitchensink "Do not Statically link SDLKitchensink" OFF)

if(NOT ZLIB_FOUND OR INTERNAL_ZLIB)
	message(STATUS "Using internal zlib")
	set(ZLIB_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/zlib)
	add_subdirectory(deps/zlib)
	set(ZLIB_LIBRARIES ZLIB::ZLIB)
	set(ZLIB_LIBRARY ZLIB::ZLIB)
else()
	message(STATUS "Using system zlib, includes found at ${ZLIB_INCLUDE_DIR}")
endif()

if( BZIP2_FOUND AND NOT FORCE_INTERNAL_BZIP2 )
	message( STATUS "Using system bzip2 library, includes found at ${BZIP2_INCLUDE_DIR}" )
else()
	message( STATUS "Using internal bzip2 library" )
	add_subdirectory( deps/bzip2 )
	set( BZIP2_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/deps/bzip2" )
	set( BZIP2_LIBRARIES bz2 )
	set( BZIP2_LIBRARY bz2 )
endif()

if( PHYSFS_FOUND AND NOT FORCE_INTERNAL_PHYSFS )
	message( STATUS "Using system physfs library, includes found at ${PHYSFS_INCLUDE_DIR}" )
else()
	message( STATUS "Using internal physfs library" )
	add_subdirectory( deps/physfs )
	set (PHYSFS_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/deps/physfs/src")
	set( PHYSFS_LIBRARIES physfs-static )
	set( PHYSFS_LIBRARY physfs-static )
endif()

if( LIBGME_FOUND AND NOT FORCE_INTERNAL_LIBGME )
	message( STATUS "Using system libgme library, includes found at ${LIBGME_INCLUDE_DIR}" )
else()
	message( STATUS "Using internal libgme library" )
	add_subdirectory( deps/libgme )
	set (LIBGME_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/deps/libgme/gme")
	set( LIBGME_LIBRARIES gme)
	set( LIBGME libgme )
endif()

#find_package(FLTK)
#if(NOT FLTK_FOUND)
    # if(WIN32)
        # message("FLTK not found in standard location; looking in ${lookout_dir}/lib/x86")
        # set(FLTK_LIBRARIES
            # ${lookout_dir}/lib/x86/fltk.lib
            # ${lookout_dir}/lib/x86/fltkforms.lib
            # ${lookout_dir}/lib/x86/fltkgl.lib
            # ${lookout_dir}/lib/x86/fltkimages.lib
            # ${lookout_dir}/lib/x86/fltkjpeg.lib
            # ${lookout_dir}/lib/x86/fltkpng.lib
            # ${lookout_dir}/lib/x86/fltkzlib.lib
        # )
        # set(FLTK_INCLUDE_DIR ${lookout_dir}/include)
    # endif()
# endif()

#if( LIBDUMB_FOUND AND NOT FORCE_INTERNAL_LIBDUMB )
#	message( STATUS "Using system libdumb library, includes found at ${LIBDUMB_INCLUDE_DIR}" )
#else()
#	message( STATUS "Using internal libdumb library" )
#	add_subdirectory( deps/dumb )
#	set (LIBDUMB_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/deps/dumb/include")
#	set( LIBDUMB_LIBRARIES dumb )
#	set( LIBDUMB_LIBRARY dumb )
#endif()
#add_subdirectory( deps/miniz )
#add_subdirectory( deps/libgme/gme )
add_subdirectory( deps/libxmp-lite )
add_subdirectory( deps/libwebsid )

add_subdirectory( deps/lzma )
add_subdirectory( zipdir )
add_subdirectory( ajbsp ) 

if(MSVC)
	add_definitions(-D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE)
endif()

# Recursive function to place PK3 archive source files into a hierarchy of source file in the IDE
function( assort_pk3_source_folder FOLDER_NAME PK3_DIR )
	# Assort source files into folders in the IDE
	file(GLOB PK3_SRCS ${PK3_DIR}/*) # Create list of all files in this folder
	foreach(PK3_SRC ${PK3_SRCS})
		# If there are subfolders, recurse into them
		if(IS_DIRECTORY ${PK3_SRC})
			get_filename_component(DIRNAME ${PK3_SRC} NAME)
			# Exclude folder from list of source files
			list(REMOVE_ITEM PK3_SRCS ${PK3_SRC})
			# Recurse deeper into the filesystem folder tree
			assort_pk3_source_folder( ${FOLDER_NAME}\\${DIRNAME} ${PK3_SRC} )
		endif()
		# Assign IDE group for current top-level source files
		source_group(${FOLDER_NAME} FILES ${PK3_SRCS})
	endforeach()
endfunction()

# Simplify pk3 building, add_pk3(filename srcdirectory)
function( add_pk3 PK3_NAME PK3_DIR )
	# message(STATUS "Creating build rule for PK3 ${PK3_NAME} ${PK3_DIR}")
	# Generate target name. Just use "pk3" for main pk3 target.
	string( REPLACE "." "_" PK3_TARGET ${PK3_NAME} )
	if( ${PK3_TARGET} STREQUAL "zdoom_pk3" )
		set( PK3_TARGET "pk3" )
	endif()

	add_custom_command( OUTPUT ${CMAKE_BINARY_DIR}/${PK3_NAME}
		COMMAND zipdir -udf ${CMAKE_BINARY_DIR}/${PK3_NAME} ${PK3_DIR}
		COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_BINARY_DIR}/${PK3_NAME} $<TARGET_FILE_DIR:EDGE>/${PK3_NAME}
		DEPENDS zipdir )
	# Create a list of source files for this PK3, for use in the IDE
	# Phase 1: Create a list of all source files for this PK3 archive, except
	#  for a couple of strife image file names that confuse CMake.
	file(GLOB_RECURSE PK3_SRCS ${PK3_DIR}/*)
	# Exclude from the source list some gzdoom .png files with brackets in the
	# file names here, because they confuse CMake.
	# This only affects the list of source files shown in the IDE.
	# It does not actually remove the files from the PK3 archive.
	# First replace that toxic bracket character with something we can handle
	string(REPLACE "[" confusing_bracket PK3_SRCS "${PK3_SRCS}")
	string(REPLACE "]" confusing_bracket PK3_SRCS "${PK3_SRCS}")
	foreach(PK3_SRC ${PK3_SRCS}) # All source files at all levels
		# Exclude those quarantined source file source file names that once had a bracket
		if(${PK3_SRC} MATCHES confusing_bracket)
			# message(STATUS "Ignoring PK3 file name containing brackets "${PK3_SRC})
			list(REMOVE_ITEM PK3_SRCS ${PK3_SRC})
		endif()
	endforeach()
	# Phase 2: Create the PK3 build rule, including the source file list for the IDE
	# Touch the zipdir executable here so that the pk3s are forced to
	# rebuild each time since their dependency has "changed."
	add_custom_target( ${PK3_TARGET} ALL
		COMMAND ${CMAKE_COMMAND} -E touch $<TARGET_FILE:zipdir>
		DEPENDS ${CMAKE_BINARY_DIR}/${PK3_NAME}
		SOURCES ${PK3_SRCS})
	# Phase 3: Assign source files to a nice folder structure in the IDE
	assort_pk3_source_folder("Source Files" ${PK3_DIR})
	# Phase 4: Add the resulting PK3 to the install target.
	if( WIN32 )
		set( INSTALL_PK3_PATH . CACHE STRING "Directory where zdoom.pk3 will be placed during install." )
	else()
		set( INSTALL_PK3_PATH share/games/doom CACHE STRING "Directory where zdoom.pk3 will be placed during install." )
	endif()
	install(FILES "${PROJECT_BINARY_DIR}/${PK3_NAME}"
			DESTINATION ${INSTALL_PK3_PATH}
			COMPONENT "Game resources")
endfunction()

add_subdirectory(wadsrc)
add_subdirectory(data)

add_definitions(-D_CRT_SECURE_NO_WARNINGS=1)

if (MSVC)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	set( CMAKE_CXX_FLAGS "/MP /GF /Ox /Ob2 /Oi /Ot /GT /W1 ${CMAKE_CXX_FLAGS} /std:c++17" )
	add_definitions(-DSIMD_X64)
	add_definitions(-DWIN64)
	add_definitions(/W1)
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
	set( CMAKE_CXX_FLAGS "/MP /GF /Ox /Ob2 /Oi /Ot /GT /W1 ${CMAKE_CXX_FLAGS} /std:c++17 /arch:SSE2" )
	add_definitions(/W1)
endif()
endif(MSVC)

if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm" AND NOT APPLE)
		set ( CMAKE_CXX_FLAGS "-mfpu=neon-vfpv4 -mtune=cortex-a7 -mhard-float ${CMAKE_CXX_FLAGS}" )
	elseif(${CMAKE_SYSTEM_PROCESSOR} MATCHES "aarch64" AND NOT APPLE)
		set( CMAKE_CXX_FLAGS "-mcpu=cortex-a72 -mtune=cortex-a72 -march=armv8-a+fp+simd+crc ${CMAKE_CXX_FLAGS}" )
	elseif(${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm64" AND APPLE)
		set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" )
	elseif(CMAKE_SIZEOF_VOID_P EQUAL 4 AND NOT MSVC)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse2")
	endif()
if(NOT MSVC)
	if( CMAKE_CXX_COMPILER_ID STREQUAL "Clang" )
		set ( CMAKE_CXX_FLAGS "-std=c++17 ${CMAKE_CXX_FLAGS}" )
	else ()
		set ( CMAKE_CXX_FLAGS "-std=gnu++17 ${CMAKE_CXX_FLAGS}" )
endif()
endif(NOT MSVC)

if(CMAKE_BUILD_TOOL MATCHES "(msdev|devenv|nmake)")
    add_definitions(/W1)
endif()

add_definitions(
	-DDEH_EDGE_PLUGIN
	-DHAVE_PHYSFS
	-DHAVE_EDGE_AJBSP_H
	#-DUSE_FFMPEG
)

# For the Hypertension team, we must continue using SSE2 and _not_ AVX, since not everyone has an AVX-compatible processor.
if(HYPERTENSION)
	add_definitions(
		-DHYPERTENSION
		-DSSE2
	)
endif(HYPERTENSION)

# For the Daikatana team, we must continue using SSE2 and _not_ AVX, since not everyone has an AVX-compatible processor.
if(DAIKATANA)
	add_definitions(
		-DDAIKATANA
		-DSSE2
	)
endif(DAIKATANA)

# Removed HAVE_STDINT_H on Visual Studio/Win32 machines (Caused problems, so I Shoved it into UNIX define)
if(APPLE)
	add_definitions(
		-DMACOSX
		-DHAVE_STDINT_H
	)
elseif(UNIX)
	if(${CMAKE_SYSTEM_NAME} MATCHES "(Free|Open)BSD")
		add_definitions(
			-DBSD
			-DHAVE_STDINT_H
		)
		else()
			add_definitions(
			-DLINUX
			-DHAVE_STDINT_H
		)
	endif()
endif()


# Gitinfo Puller thingy
add_custom_target(revision_check ALL
   COMMAND updaterevision src/gitinfo.h
   WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
   DEPENDS updaterevision
)

SET (BIN_DIR bin)
SET (SHARE_DIR share/edge)
IF (WIN32)
        SET (LIB_DIR ${BIN_DIR})
        SET (SHARE_DIR ${BIN_DIR})
ELSE (WIN32)
        SET (LIB_DIR lib)
ENDIF (WIN32)

################################################################################
######################### Find Needed Libs #####################################

# See /VSLibs/lib_versions.txt [libs have been compiled with MSVC 2017!]
# CA: 11/30/18 - For MSVC, libs are no longer required to be downloaded seperately, they now exist in /libs/msvc/
if (FORCE_STATIC_MSVC_LIBS)
	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		set( SDL_NET_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/libs/msvc/SDL_net-2.0.1/lib/x64/SDL2_net.lib" )
	elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
		set( SDL_NET_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/libs/msvc/SDL_net-2.0.1/lib/x86/SDL2_net.lib" )
		#set (kitchensink_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs/msvc/kitchensink/include")
		#set (kitchensink_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/libs/msvc/kitchensink/lib/SDL_kitchensink_static.lib")
	endif()
		include("${CMAKE_CURRENT_SOURCE_DIR}/libs/msvc/SDL2/sdl-config.cmake")
		set( SDL2_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs/msvc/SDL2")
		set( SDLNET_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs/msvc/SDL_net-2.0.1/include" )
endif(FORCE_STATIC_MSVC_LIBS)

if(UNIX)
	if(APPLE)
		set(ogg_RELEASE_LIBRARY "libvorbisfile.dylib" )
	else(APPLE)
		set(ogg_RELEASE_LIBRARY "libvorbisfile.so" )
	endif(APPLE)
		#set( PHYSFS_LIBRARY "libphysfs.so" )
	#set(SDL2_INCLUDE_DIR /usr/include/SDL2)
	#set(SDL2_LIBRARIES /usr/lib/libSDL2.so)
	find_package(SDL2 REQUIRED)
endif(UNIX)

#INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/src ${SDL2_INCLUDE_DIRS})

## SDL2
#set(SDL2_PATH "${CMAKE_CURRENT_SOURCE_DIR/SDL2-2.0.9}" CACHE PATH “The location to search for SDL2”)
INCLUDE_DIRECTORIES (${SDL2_INCLUDE_DIR})

## FIND_PACKAGE (SDL_net REQUIRED)
INCLUDE_DIRECTORIES (${SDLNET_INCLUDE_DIR})

INCLUDE_DIRECTORIES (${PHYSFS_INCLUDE_DIR})

INCLUDE_DIRECTORIES (${FLTK_INCLUDE_DIR})

## FIND FFMPEG AND KITCHENSINK INCLUDES
##INCLUDE_DIRECTORIES (${FFMPEG_INCLUDE_DIR})
##INCLUDE_DIRECTORIES (${SDLKitchensink_INCLUDE_DIRS})

# OpenGL
if (NOT VITA)
    if(POLICY CMP0072)
        cmake_policy(SET CMP0072 NEW)
    endif()
    FIND_PACKAGE (OpenGL)
elseif(VITA)
    FIND_PACKAGE (VitaGL)
endif()


# zlib
find_package(ZLIB REQUIRED)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/deps/miniz)

# dumb
#include_directories(${LIBDUMB_INCLUDE_DIR})

# EDGE
include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/sse2neon)
set(EDGE_SOURCES
	coal/c_compile.cc
	coal/c_execute.cc
	coal/c_memory.cc

	ddf/anim.cc
	ddf/attack.cc
	ddf/boom.cc
	ddf/colormap.cc
	ddf/flat.cc
	ddf/font.cc
	ddf/game.cc
	ddf/image.cc
	ddf/language.cc
	ddf/level.cc
	ddf/line.cc
	ddf/main.cc
	ddf/playlist.cc
	ddf/sector.cc
	ddf/sfx.cc
	ddf/states.cc
	ddf/style.cc
	ddf/switch.cc
	ddf/thing.cc
	ddf/weapon.cc

	deh_edge/ammo.cc
	deh_edge/attacks.cc
	deh_edge/buffer.cc
	deh_edge/convert.cc
	deh_edge/frames.cc
	deh_edge/info.cc
	deh_edge/main.cc
	deh_edge/system.cc
	deh_edge/util.cc
	deh_edge/wad.cc
	deh_edge/mobj.cc
	deh_edge/sounds.cc
	deh_edge/things.cc
	deh_edge/weapons.cc
	deh_edge/misc.cc
	deh_edge/text.cc
	deh_edge/storage.cc
	deh_edge/patch.cc
	deh_edge/rscript.cc

	src/system/i_main.cc
	src/system/i_ctrl.cc
	src/system/i_video.cc
	src/system/i_sound.cc
	src/system/i_net.cc
	src/system/i_timer.cc
	src/system/i_x86.cc
	src/system/i_cinematic.cc
	src/system/i_ffmpeg.cc
	src/system/GL/gl_load.c

	src/am_map.cc
	src/con_con.cc
	src/con_main.cc
	src/con_var.cc
	src/dem_chunk.cc
	src/dem_glob.cc
	src/e_demo.cc
	src/e_input.cc
	src/e_main.cc
	src/e_player.cc
	src/f_finale.cc
	src/f_interm.cc
	src/e_splash.cc
	src/gitinfo.cc
	src/g_game.cc
	src/hu_draw.cc
	src/hu_font.cc
	src/hu_stuff.cc
	src/hu_style.cc
	src/l_ajbsp.cc   
	#src/l_glbsp.cc
	src/l_deh.cc
	src/m_argv.cc
	src/m_bbox.cc
	src/m_cheatcodes.cc
	src/m_math.cc
	src/m_menu.cc
	src/m_misc.cc
	src/m_option.cc
	src/m_netgame.cc
	src/m_random.cc
	src/m_shift.cc
	src/n_bcast.cc
	src/n_reliable.cc
	src/n_network.cc
	src/p_action.cc
	src/p_blockmap.cc
	src/p_bot.cc
	src/p_cheats.cc
	src/p_enemy.cc
	src/p_inter.cc
	src/p_lights.cc
	src/p_map.cc
	src/p_maputl.cc
	src/p_mobj.cc
	src/p_plane.cc
	src/p_pobj.cc
	src/p_setup.cc
	src/p_sight.cc
	src/p_spec.cc
	src/p_switch.cc
	src/p_tick.cc
	src/p_user.cc
	src/p_forces.cc
	src/p_telept.cc
	src/p_weapon.cc
	src/rad_act.cc
	src/rad_pars.cc
	src/rad_trig.cc
	src/r_draw.cc
	src/r_shader.cc
	src/r_render.cc
	src/r_effects.cc
	src/r_main.cc
	src/r_occlude.cc
	src/r_occlude2.cc
	src/m_logo.cc
	src/r_things.cc
	src/r_units.cc
	src/r_wipe.cc
	src/r_misc.cc
	src/r_sky.cc
	src/r_colormap.cc
	src/r_modes.cc
	src/r_md2.cc
	src/r_md5.cc
	src/r_graph.cc
	src/r_image.cc
	src/r_doomtex.cc
	src/r_texgl.cc
	src/r_rottgfx.cc
	src/r_bumpmap.cc
	src/r_playpal.cc
	src/r_renderbuffers.cc
	src/r_postprocessstate.cc
	src/r_shaderprogram.cc
	src/r_lensdistortion.cc
	src/r_qbb.cc
	src/r_qvf.cc
	src/r_fxaa.cc
	src/r_bloom.cc
	src/s_blit.cc
	#src/s_dumb.cc
	src/s_cache.cc
	src/s_gme.cc
	src/s_sound.cc
	src/s_mp3.cc
	src/s_music.cc
	src/s_ogg.cc
	src/s_opl.cc
	src/s_sid.cc
	src/s_tsf.cc
	src/s_xmp.cc
	src/sv_chunk.cc
	src/sv_glob.cc
	src/sv_level.cc
	src/sv_load.cc
	src/sv_main.cc
	src/sv_misc.cc
	src/sv_mobj.cc
	src/sv_play.cc
	src/sv_save.cc
	src/tinybsp.cc
	src/w_flat.cc
	src/w_model.cc
	src/w_sprite.cc
	src/w_texture.cc
	src/w_wad.cc
	src/w_rottwad.cc
	src/z_zone.cc
	src/vm_coal.cc
	src/vm_hud.cc
	src/vm_player.cc

	src/games/rott/rt_byteordr.cc
	src/games/wolf3d/wlf_util.cc
	src/games/wolf3d/wlf_vswap.cc
	src/games/wolf3d/wlf_maps.cc
	src/games/wolf3d/wlf_setup.cc
	src/games/wolf3d/wlf_bsp.cc

	src/md5_conv/md5_load.cc
	src/md5_conv/md5_parse.cc
	src/md5_conv/md5_anim.cc
	src/md5_conv/md5_draw.cc
	epi/kmq2/epi_quake2stuff.cc

	#src/utility/lib_file.cc
	#src/utility/lib_util.cc
	src/utility/m_strings.cc

	epi/arrays.cc
	epi/bytearray.cc
	epi/exe_path.cc
	epi/file.cc
	epi/filesystem.cc
	epi/file_memory.cc
	epi/file_sub.cc
	epi/image_data.cc
	epi/image_hq2x.cc
	epi/image_jpeg.cc
	epi/image_png.cc
	epi/image_tga.cc
	epi/stb_image.cc
	epi/stb_image_write.cc
	epi/math_angle.cc
	epi/math_bbox.cc
	epi/math_crc.cc
	epi/math_md5.cc
	epi/math_oddity.cc
	epi/math_matrix.cc
	epi/math_vector.cc
	epi/math_quaternion.cc
	epi/math_random.cc
	epi/mersenne_twist.cc
	epi/mus_2_midi.cc
	epi/path.cc
	epi/str_format.cc
	epi/sound_data.cc
	epi/sound_gather.cc
	epi/sound_wav.cc
	epi/timestamp.cc
	epi/utility.cc
	
	#glbsp/src/analyze.cc
	#glbsp/src/blockmap.cc
	#glbsp/src/glbsp.cc
	#glbsp/src/level.cc
	#glbsp/src/node.cc
	#glbsp/src/reject.cc
	#glbsp/src/seg.cc
	#glbsp/src/system.cc
	#glbsp/src/util.cc
	#glbsp/src/wad.cc

	src/system/sound/opllib/opl3.cc
	src/system/sound/opllib/oplapi.cc
	
)


set(EDGE_WIN32_SOURCES
	src/system/win32/w32_music.cc
	src/system/win32/w32_mus.cc
	src/system/win32/w32_net.cc
	src/system/win32/w32_system.cc
	src/system/win32/w32_res.rc
	src/system/win32/i_exception.cc
	epi/epi_win32.cc
	epi/filesystem_win32.cc
	#epi/physfs/physfs_platform_windows.c
)

set(EDGE_UNIX_SOURCES
    #	src/system/unx/unx_main.cc
	src/system/unx/unx_music.cc
	src/system/unx/unx_net.cc
	src/system/unx/unx_system.cc
	epi/filesystem_linux.cc
	epi/epi_linux.cc
	#epi/physfs/physfs_platform_unix.c
)

set(EDGE_DREAMCAST_SOURCES
	src/dc_music.cc
	src/dc_net.cc
	src/dc_system.cc
	src/i_videodc.cc
	src/i_main.cc
)

set(EDGE_VITA_SOURCES
	#src/system/vita/neon_math.cc
	#src/system/unx/unx_music.cc
	#src/system/unx/unx_net.cc
	#src/system/unx/unx_system.cc
	#epi/filesystem_linux.cc
	#epi/epi_linux.cc
	#epi/physfs/physfs_platform_unix.c
)

if (APPLE)
	set(EDGE_UNIX_SOURCES
		${EDGE_UNIX_SOURCES}
		src/system/unx/mac/mac_ui.mm
		#epi/physfs/physfs_platform_apple.m
	)
endif()

add_subdirectory( tools )

if(WIN32)
	set(EDGE_SOURCES ${EDGE_SOURCES}
		${EDGE_WIN32_SOURCES}
	)
	if(MSVC)
		set(EDGE_SOURCES ${EDGE_SOURCES}
			src/system/win32/w32_manifest.manifest
		)
	else()
		set(EDGE_SOURCES ${EDGE_SOURCES}
			src/system/win32/w32_manifest_mgw.rc
		)	
	endif()
elseif(UNIX)
	set(EDGE_SOURCES ${EDGE_SOURCES}
		${EDGE_UNIX_SOURCES}
	)
elseif(DREAMCAST)
	set(EDGE_SOURCES ${EDGE_SOURCES}
		${EDGE_DREAMCAST_SOURCES}
	)
elseif(VITA)
	set(EDGE_SOURCES ${EDGE_SOURCES}
		${EDGE_VITA_SOURCES}
	)
endif()

add_executable(EDGE
	${EDGE_SOURCES}
)

if(MINGW AND MINGW_LINK_STATiIC)
	set_target_properties(EDGE PROPERTIES LINK_SEARCH_START_STATIC 1)
	set_target_properties(EDGE PROPERTIES LINK_SEARCH_END_STATIC 1)
endif()

if (NOT VITA)
add_dependencies( EDGE revision_check )
endif()
if(MSVC)
	add_dependencies( EDGE eecrashreport )
endif()

#copy base folder to build root
add_custom_command(TARGET EDGE POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                       ${CMAKE_SOURCE_DIR}/data/base $<TARGET_FILE_DIR:${PROJECT_NAME}>/base)

#copy soundfont folder to build root (Not sure where you want this in the final iteration - Dasho)
add_custom_command(TARGET EDGE POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                       ${CMAKE_SOURCE_DIR}/data/soundfont $<TARGET_FILE_DIR:${PROJECT_NAME}>/soundfont)

target_include_directories(EDGE 
	PRIVATE
	deps/libgme/gme
    deps/libxmp-lite
    deps/libwebsid
	deps/minivorbis
	deps/miniz
	src/edge_ajbsp
)

target_link_libraries(EDGE
	edge_ajbsp
	${LIBGME_LIBRARIES}
	libxmp-lite
	libwebsid
	${SDL2_LIBRARY}
	${SDL_NET_LIBRARY}
	ZLIB::ZLIB
	${PHYSFS_LIBRARY}
#	Ogg::ogg
#	Vorbis::vorbis Vorbis::vorbisenc Vorbis::vorbisfile
#	${VORBIS_LIBRARIES} ${VORBISFILE_LIBRARIES}
	#	${LIBDUMB_LIBRARY}
	#	${SDLKitchensink_STATIC_LIBRARY}
)


if(MINGW)
	set(MINGW_BUILD_EDGE_TARGET_PROPERTIES "-Wl,-subsystem,windows")
	if(MINGW_LINK_STATIC)
		set(MINGW_BUILD_EDGE_TARGET_PROPERTIES "${MINGW_BUILD_EDGE_TARGET_PROPERTIES} -static-libgcc -static-libstdc++ -static")
	endif()
	set_target_properties(EDGE PROPERTIES LINK_FLAGS "${MINGW_BUILD_EDGE_TARGET_PROPERTIES}")
endif()

if (WIN32)
	target_link_libraries(EDGE
		wsock32
		winmm
		gdi32
		opengl32
		Imm32
		Setupapi
		Version
	)
elseif(APPLE)
	set_target_properties(EDGE PROPERTIES LINK_FLAGS "-framework OpenGL -framework Cocoa -framework IOKit")
	target_link_libraries(EDGE stdc++ "-framework Foundation" "-framework Cocoa" "-framework IOKIT" objc)
elseif(VITA)
	target_link_libraries(EDGE SDL2 vitaGL SceDisplay_stub SceCtrl_stub SceAudio_stub SceSysmodule_stub SceGxm_stub SceCommonDialog_stub SceTouch_stub SceHid_stub m)
elseif(NOT ${CMAKE_SYSTEM_NAME} MATCHES "(Free|Open)BSD")
	target_link_libraries(EDGE SDL2 OpenGL::GL dl)
endif()

#if(MSVC)
#   target_link_libraries(EDGE rpcrt4)
#endif()

if (NOT VITA)
INSTALL (TARGETS EDGE
        RUNTIME DESTINATION ${BIN_DIR}
        LIBRARY DESTINATION ${LIB_DIR}
        ARCHIVE DESTINATION ${LIB_DIR})

INSTALL (DIRECTORY ${CMAKE_SOURCE_DIR}/data/base
        DESTINATION ${PROJECT_BINARY_DIR}
        PATTERN "delete.me" EXCLUDE
        PATTERN ".svn" EXCLUDE)
elseif(VITA)
vita_create_self(${PROJECT_NAME}.self ${PROJECT_NAME})
vita_create_vpk(${PROJECT_NAME}.vpk ${VITA_TITLEID} ${PROJECT_NAME}.self
  VERSION ${VITA_VERSION}
  NAME ${VITA_APP_NAME}
  FILE sce_sys/icon0.png sce_sys/icon0.png
  FILE sce_sys/livearea/contents/bg.png sce_sys/livearea/contents/bg.png
  FILE sce_sys/livearea/contents/startup.png sce_sys/livearea/contents/startup.png
  FILE sce_sys/livearea/contents/template.xml sce_sys/livearea/contents/template.xml
)
endif()
## EOF
