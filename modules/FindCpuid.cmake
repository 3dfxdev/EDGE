
# This module defines
# CPUID_LIBRARY, the name of the library to link against
# CPUID_FOUND, if false, do not try to link to libcpuid
# CPUID_INCLUDE_DIR, where to find libcpuid.h
#

message("<FindCpuid.cmake>")

SET(CPUID_SEARCH_PATHS
	~/Library/Frameworks
	/Library/Frameworks
	/usr/local
	/usr
	/sw # Fink
	/opt/local # DarwinPorts
	/opt/csw # Blastwave
	/opt
	${CPUID_PATH}
)

FIND_PATH(CPUID_INCLUDE_DIR libcpuid.h
	HINTS
	$ENV{CPUIDDIR}
	PATH_SUFFIXES libcpuid
	PATHS ${CPUID_SEARCH_PATHS}
)

FIND_LIBRARY(CPUID_LIBRARY
	NAMES libcpuid
	HINTS
	$ENV{CPUIDDIR}
	PATH_SUFFIXES lib
	PATHS ${CPUID_SEARCH_PATHS}
)

message("</FindCpuid.cmake>")

INCLUDE(FindPackageHandleStandardArgs)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(CPUID REQUIRED_VARS CPUID_LIBRARY CPUID_INCLUDE_DIR)

