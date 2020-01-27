cmake_minimum_required( VERSION 2.8.12 )

set( CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -D_DEBUG" )

# Disable warnings for << operator precedence (4554) and
# unreferenced labels (4102) from VC
if( MSVC )
	set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /wd4554 /wd4102" )
endif()

if( ZD_CMAKE_COMPILER_IS_GNUC_COMPATIBLE )
	set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra" )
endif()

include_directories( ${CMAKE_CURRENT_BINARY_DIR} )
add_definitions( -DINFNAN_CHECK -DMULTIPLE_THREADS )

# These can't be run effectively in a cross compiling environment so we'll just
# not generate the header and let it error if we haven't collected the data
# statically.
if( NOT MSVC AND NOT APPLE AND NOT CMAKE_CROSSCOMPILING )
	add_executable( arithchk arithchk.c )
	add_custom_command( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/arith.h 
		COMMAND arithchk >${CMAKE_CURRENT_BINARY_DIR}/arith.h
		DEPENDS arithchk )

	add_executable( qnan qnan.c arith.h )
	add_custom_command( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/gd_qnan.h
		COMMAND qnan >${CMAKE_CURRENT_BINARY_DIR}/gd_qnan.h
		DEPENDS qnan )
		
	set( GEN_FP_FILES arith.h gd_qnan.h )
	set( GEN_FP_DEPS ${CMAKE_CURRENT_BINARY_DIR}/arith.h ${CMAKE_CURRENTY_BINARY_DIR}/gd_qnan.h )
endif()

add_library( gdtoa STATIC
	${GEN_FP_FILES}
	dmisc.c
	dtoa.c
	misc.c
	)
target_link_libraries( gdtoa )

