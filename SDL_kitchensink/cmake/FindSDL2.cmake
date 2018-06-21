# A Simple SDL2 Finder.
# (c) Tuomas Virtanen 2016 (Licensed under MIT license)
# Usage:
# find_package(SDL2)
#
# Declares:
#  * SDL2_FOUND
#  * SDL2_INCLUDE_DIRS
#  * SDL2_LIBRARIES
#

set(SDL2_SEARCH_PATHS
    /usr/local/
    /usr/
    /opt
)

find_path(SDL2_INCLUDE_DIR SDL2/SDL.h
    HINTS
    PATH_SUFFIXES include
    PATHS ${SDL2_SEARCH_PATHS}
)

find_library(SDL2_LIBRARY
    NAMES SDL2
    HINTS
    PATH_SUFFIXES lib
    PATHS ${SDL2_SEARCH_PATHS}
)

if(MINGW)
    find_library(SDL2MAIN_LIBRARY
        NAMES SDL2main
        HINTS
        PATH_SUFFIXES lib
        PATHS ${SDL2_SEARCH_PATHS}
    )
else()
    set(SDL2MAIN_LIBRARY "")
endif()

if(SDL2_INCLUDE_DIR AND SDL2_LIBRARY)
   set(SDL2_FOUND TRUE)
endif()

if(SDL2_FOUND)
    set(SDL2_LIBRARIES ${SDL2MAIN_LIBRARY} ${SDL2_LIBRARY})
    set(SDL2_INCLUDE_DIRS ${SDL2_INCLUDE_DIR})
    message(STATUS "Found SDL2: ${SDL2_LIBRARIES}")
else()
    message(WARNING "Could not find SDL2")
endif()

mark_as_advanced(SDL2MAIN_LIBRARY SDL2_LIBRARY SDL2_INCLUDE_DIR SDL2_SEARCH_PATHS)
