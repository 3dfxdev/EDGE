cmake_minimum_required (VERSION 2.6)
project (eBSP)

add_subdirectory(src)


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

option(FORCE_INTERNAL_ZLIB "Use internal zlib")

find_package( ZLIB )

if( ZLIB_FOUND AND NOT FORCE_INTERNAL_ZLIB )
	message( STATUS "Using system zlib, includes found at ${ZLIB_INCLUDE_DIR}" )
else()
	message( STATUS "Using internal zlib" )
	set( SKIP_INSTALL_ALL TRUE ) # Avoid installing zlib alongside eBSP
	add_subdirectory( zlib )
	set( ZLIB_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/zlib )
	set( ZLIB_LIBRARIES z )
	set( ZLIB_LIBRARY z )
endif()


if(MSVC)
	find_package(ZLIB REQUIRED)
	find_package(FLTK REQUIRED)
endif(MSVC)

include_directories(${ZLIB_INCLUDE_DIR})
include_directories(${FLTK_INCLUDE_DIR})


# eBSP
include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(eBSP_SOURCES
	src/analyze.cc
	src/blockmap.cc
	src/glbsp.cc
	src/level.cc
	src/node.cc
	src/reject.cc
	src/seg.cc
	src/system.cc
	src/util.cc
	src/wad.cc
	
	src/cmdline/display.cc
	src/cmdline/main.cc
	
	src/gui/main.cc
	src/gui/about.cc
	src/gui/book.cc
	src/gui/booktext.cc
	src/gui/cookie.cc  
	src/gui/dialog.cc  
	src/gui/files.cc
	src/gui/helper.cc  
	src/gui/images.cc  
	src/gui/license.cc 
	src/gui/menu.cc
	src/gui/options.cc 
	src/gui/prefs.cc
	src/gui/progress.cc
	src/gui/textbox.cc 
	src/gui/window.cc  
	src/gui/glbsp_res.cc
)