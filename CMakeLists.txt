cmake_minimum_required(VERSION 3.2.2)
project(3DGE)

if( COMMAND cmake_policy )
	if( POLICY CMP0011 )
		cmake_policy( SET CMP0011 NEW )
	endif()
	if( POLICY CMP0054 )
		cmake_policy( SET CMP0054 NEW )
	endif()
endif()

list( APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake )
include( FindPackageHandleStandardArgs )

macro( make_release_only )
	set( CMAKE_C_FLAGS_MINSIZEREL ${CMAKE_C_FLAGS_RELEASE} )
	set( CMAKE_C_FLAGS_RELWITHDEBINFO ${CMAKE_C_FLAGS_RELEASE} )
	string( REPLACE "/MT " "/MTd " CMAKE_C_FLAGS_DEBUG ${CMAKE_C_FLAGS_RELEASE} )
	set( CMAKE_CXX_FLAGS_MINSIZEREL ${CMAKE_CXX_FLAGS_RELEASE} )
	set( CMAKE_CXX_FLAGS_RELWITHDEBINFO ${CMAKE_CXX_FLAGS_RELEASE} )
	string( REPLACE "/MT " "/MTd " CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_RELEASE} )
endmacro()

set( LZMA_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lzma/C" )
option(FORCE_INTERNAL_ZLIB "Use internal zlib")
option(FORCE_INTERNAL_BZIP2 "Use internal bzip2")

find_package( ZLIB )
find_package( BZip2 )

if( ZLIB_FOUND AND NOT FORCE_INTERNAL_ZLIB )
	message( STATUS "Using system zlib, includes found at ${ZLIB_INCLUDE_DIR}" )
else()
	message( STATUS "Using internal zlib" )
	set( SKIP_INSTALL_ALL TRUE ) # Avoid installing zlib alongside 3dge
	add_subdirectory( zlib )
	set( ZLIB_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/zlib )
	set( ZLIB_LIBRARIES z )
	set( ZLIB_LIBRARY z )
endif()

if( BZIP2_FOUND AND NOT FORCE_INTERNAL_BZIP2 )
	message( STATUS "Using system bzip2 library, includes found at ${BZIP2_INCLUDE_DIR}" )
else()
	message( STATUS "Using internal bzip2 library" )
	add_subdirectory( bzip2 )
	set( BZIP2_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/bzip2" )
	set( BZIP2_LIBRARIES bz2 )
	set( BZIP2_LIBRARY bz2 )
endif()

add_subdirectory( lzma )

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
		COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_BINARY_DIR}/${PK3_NAME} $<TARGET_FILE_DIR:3DGE>/${PK3_NAME}
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

# MSVC
if(MSVC)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS=1)
	set( CMAKE_CXX_FLAGS "/MP /GF /Ox /Ob2 /Oi /Ot /GT ${CMAKE_CXX_FLAGS} /arch:SSE2" )
else()
	if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm")
		set ( CMAKE_CXX_FLAGS "-mfpu=neon-vfpv4 -mtune=cortex-a7 -mhard-float ${CMAKE_CXX_FLAGS}" )
	else ()
		set ( CMAKE_CXX_FLAGS "-msse2 ${CMAKE_CXX_FLAGS}" )
	endif()
	if( CMAKE_CXX_COMPILER_ID STREQUAL "Clang" )
		set ( CMAKE_CXX_FLAGS "-std=c++11 ${CMAKE_CXX_FLAGS}" )
	else ()
		set ( CMAKE_CXX_FLAGS "-std=gnu++11 ${CMAKE_CXX_FLAGS}" )
	endif()
endif()

add_definitions(
	-DDEH_EDGE_PLUGIN
	-DHAVE_PHYSFS
)
# Removed HAVE_STDINT_H on Visual Studio/Win32 machines (Caused problems, so I Shoved it into UNIX define)
if(APPLE)
	add_definitions(
		-DMACOSX
		-DHAVE_STDINT_H
	)
elseif(UNIX)
	if(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
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

# Module Paths
#list(APPEND CMAKE_MODULE_PATH
	#${CMAKE_CURRENT_SOURCE_DIR}
	#${CMAKE_CURRENT_SOURCE_DIR}/modules
#)
SET(CMAKE_MODULE_PATH ${CMAKE_ROOT}/Modules  "${CMAKE_CURRENT_SOURCE_DIR}/modules")

#message(${CMAKE_MODULE_PATH})

add_subdirectory( tools/zipdir )

# Gitinfo Puller thingy
add_custom_target(revision_check ALL
   COMMAND updaterevision src/gitinfo.h
   WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
   DEPENDS updaterevision 
)

# See /VSLibs/lib_versions.txt
# Download VSLibs for MSVC - http://tdgmods.net/VSLibs.7z (up to date as of listing below)

if(MSVC)
	#set( GLEW_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/VSLibs/glew-2.1.0/include" )
	#set( GLEW_LIBRARY_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/VSLibs/glew-2.1.0/lib/Release/Win32/glew32s.lib" )
	#set( GLEW_LIBRARY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/VSLibs/glew-2.1.0/lib/Release/Win32/glew32s.lib" )
	set( JPEG_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/VSLibs/libjpeg-turbo/include" )
	set( JPEG_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/VSLibs/libjpeg-turbo/lib/turbojpeg-static.lib" )
	set( OGG_DEBUG_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/VSLibs/libogg-1.3.2/lib/libogg_static.lib" )
	set( OGG_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/VSLibs/libogg-1.3.2/include" )
	set( OGG_RELEASE_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/VSLibs/libogg-1.3.2/lib/libogg_static.lib" )
	set( PHYSFS_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/VSLibs/physfs/include" )
	set( PHYSFS_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/VSLibs/physfs/lib/physfs.lib" )
	set( PNG_LIBRARY_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/VSLibs/libpng16/lib/libpng16_staticd.lib" )
	set( PNG_LIBRARY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/VSLibs/libpng16/lib/libpng16_static.lib" )
	set( PNG_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/VSLibs/libpng16/include" )
	set( SDL2MAIN_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/VSLibs/SDL2/lib/x86/SDL2main.lib" )
	set( SDL2_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/VSLibs/SDL2/include" )
	set( SDL2_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/VSLibs/SDL2/lib/x86/SDL2.lib" )
	set( SDL_NET_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/VSLibs/SDL2_net-2.0.1/include" )
	set( SDL_NET_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/VSLibs/SDL2_net-2.0.1/lib/x86/SDL2_net.lib" )
	set( VORBISFILE_DEBUG_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/VSLibs/libvorbis-1.3.5/lib/libvorbisfile_static.lib" )
	set( VORBISFILE_RELEASE_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/VSLibs/libvorbis-1.3.5/lib/libvorbisfile_static.lib" )
	set( VORBIS_DEBUG_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/VSLibs/libvorbis-1.3.5/lib/libvorbis_static.lib" )
	set( VORBIS_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/VSLibs/libvorbis-1.3.5/include" )
	set( VORBIS_RELEASE_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/VSLibs/libvorbis-1.3.5/lib/libvorbis_static.lib" )
	set( ZLIB_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/VSLibs/zlib-1.2.11/include" )
	set( ZLIB_LIBRARY_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/VSLibs/zlib-1.2.11/lib/zlibstatic.lib" )
	set( ZLIB_LIBRARY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/VSLibs/zlib-1.2.11/lib/zlibstatic.lib" )
else(MSVC)
	find_package(PhysFS REQUIRED)
	find_package(SDL2 REQUIRED)
	find_package(SDL2_net REQUIRED)
	#find_package(GLEW REQUIRED)
	find_package(OGG REQUIRED)
	find_package(Vorbis REQUIRED)
	find_package(ZLIB REQUIRED)
	find_package(PNG REQUIRED)
	find_package(JPEG REQUIRED)
endif(MSVC)

# Phys
#find_package(PHYSFS REQUIRED)
include_directories(${PHYSFS_INCLUDE_DIR})

# SDL
#find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIR})

# SDL_net
#find_package(SDL2_net REQUIRED)
include_directories(${SDL_INCLUDE_DIR})

# GLEW
#find_package(GLEW REQUIRED)
#include_directories(${GLEW_INCLUDE_DIR})

# libvorbis/libogg
#find_package(OGG REQUIRED)
#find_package(Vorbis REQUIRED)
include_directories(
	${OGG_INCLUDE_DIR}
	${VORBIS_INCLUDE_DIR}
)

# zlib
#find_package(ZLIB REQUIRED)
include_directories(${ZLIB_INCLUDE_DIR})

# png
#find_package(PNG REQUIRED)
include_directories(${PNG_INCLUDE_DIR})

# jpeg
#find_package(JPEG REQUIRED)
include_directories(${JPEG_INCLUDE_DIR})

# 3DGE
include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(3DGE_SOURCES
	coal/c_compile.cc
	coal/c_execute.cc
	coal/c_memory.cc
	
	ddf/anim.cc
	ddf/attack.cc
	ddf/boom.cc
	ddf/colormap.cc
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
	src/system/i_x86.cc
	src/system/i_cinematic.cc
	src/system/GL/gl_load.c

	src/am_map.cc
	src/con_con.cc
	src/con_main.cc
	src/con_link.cc
	src/con_var.cc
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
	src/l_glbsp.cc
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
	src/r_fxaa.cc
	src/r_bloom.cc
	src/s_blit.cc
	src/s_cache.cc
	src/s_sound.cc
	src/s_music.cc
	src/s_ogg.cc
	src/s_opl.cc
	src/s_timid.cc
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

	src/games/wolf3d/wlf_util.cc
	src/games/wolf3d/wlf_vswap.cc
	src/games/wolf3d/wlf_maps.cc
	src/games/wolf3d/wlf_setup.cc
	
	src/md5_conv/md5_load.cc
	src/md5_conv/md5_parse.cc
	src/md5_conv/md5_anim.cc
	src/md5_conv/md5_draw.cc
	epi/kmq2/epi_quake2stuff.cc
	
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
	
	
	glbsp/src/analyze.cc
	glbsp/src/blockmap.cc
	glbsp/src/glbsp.cc
	glbsp/src/level.cc
	glbsp/src/node.cc
	glbsp/src/reject.cc
	glbsp/src/seg.cc
	glbsp/src/system.cc
	glbsp/src/util.cc
	glbsp/src/wad.cc
	
	timidity/common.cc
	timidity/instrum.cc
	timidity/mix.cc
	timidity/playmidi.cc
	timidity/loadmidi.cc
	timidity/resample.cc
	timidity/tables.cc
	timidity/timidity.cc

	src/opllib/opl3.cc
	src/opllib/oplapi.cc

)

set(3DGE_WIN32_SOURCES
	src/system/win32/w32_music.cc
	src/system/win32/w32_mus.cc
	src/system/win32/w32_net.cc
	src/system/win32/w32_system.cc
	src/system/win32/w32_res.rc
	src/system/win32/i_exception.cc
	epi/epi_win32.cc
	epi/filesystem_win32.cc
)

set(3DGE_UNIX_SOURCES
    #	src/system/unx/unx_main.cc
	src/system/unx/unx_music.cc
	src/system/unx/unx_net.cc
	src/system/unx/unx_system.cc
	epi/filesystem_linux.cc
	epi/epi_linux.cc
)

set(3DGE_DREAMCAST_SOURCES
	src/dc_music.cc
	src/dc_net.cc
	src/dc_system.cc
	src/i_videodc.cc
	src/i_main.cc
)

if (APPLE)
	set(3DGE_UNIX_SOURCES
		${3DGE_UNIX_SOURCES}
		src/system/unx/mac/mac_ui.mm
	)
endif()

add_subdirectory( tools )

if(WIN32)
	set(3DGE_SOURCES ${3DGE_SOURCES}
		${3DGE_WIN32_SOURCES}
	)
elseif(UNIX)
	set(3DGE_SOURCES ${3DGE_SOURCES}
		${3DGE_UNIX_SOURCES}
	)
elseif(DREAMCAST)
	set(3DGE_SOURCES ${3DGE_SOURCES}
		${3DGE_DREAMCAST_SOURCES}
	)
endif()

add_executable(3DGE
	${3DGE_SOURCES}
)

add_dependencies( 3DGE revision_check )
if(MSVC)
	add_dependencies( 3DGE eecrashreport )
endif()

target_link_libraries(3DGE
	${SDL2MAIN_LIBRARY}
	${SDL2_LIBRARY}
	${SDL2_NET_LIBRARY}
	${OGG_RELEASE_LIBRARY}
	${VORBIS_RELEASE_LIBRARY}
	${VORBISFILE_RELEASE_LIBRARY}
	${ZLIB_LIBRARY_RELEASE}
	#${GLEW_LIBRARY_RELEASE}
	${PNG_LIBRARY_RELEASE}
	${JPEG_LIBRARY}
	${PHYSFS_LIBRARY}
)

if (WIN32)
	target_link_libraries(3DGE
		wsock32
		winmm
		gdi32
		opengl32
	)
elseif(APPLE)
	set_target_properties(3DGE PROPERTIES LINK_FLAGS "-framework OpenGL -framework Cocoa")
else()
	target_link_libraries(3DGE GL)
endif()
