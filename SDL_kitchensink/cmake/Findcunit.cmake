# A Simple CUnit Finder.
# (c) Tuomas Virtanen 2016 (Licensed under MIT license)
# Usage:
# find_package(cunit)
#
# Declares:
#  * CUNIT_FOUND
#  * CUNIT_INCLUDE_DIRS
#  * CUNIT_LIBRARIES
#

set(CUNIT_SEARCH_PATHS
    /usr/local/
    /usr
    /opt
)

find_path(CUNIT_INCLUDE_DIR CUnit/CUnit.h 
    HINTS 
    PATH_SUFFIXES include
    PATHS ${CUNIT_SEARCH_PATHS}
)
find_library(CUNIT_LIBRARY cunit 
    HINTS
    PATH_SUFFIXES lib
    PATHS ${CUNIT_SEARCH_PATHS}
)

if(CUNIT_INCLUDE_DIR AND CUNIT_LIBRARY)
   set(CUNIT_FOUND TRUE)
endif()

if(CUNIT_FOUND)
    set(CUNIT_LIBRARIES ${CUNIT_LIBRARY})
    set(CUNIT_INCLUDE_DIRS ${CUNIT_INCLUDE_DIR})
    message(STATUS "Found CUnit: ${CUNIT_LIBRARY}")
else()
    message(WARNING "Could not find CUnit.")
endif()

mark_as_advanced(CUNIT_LIBRARY CUNIT_INCLUDE_DIR)
