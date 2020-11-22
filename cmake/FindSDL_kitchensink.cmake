# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#.rst:
# FindSDLKitchensink
# --------
#
# Find the libsdlkitchensink headers and libraries.
#
# This module reports information about the libsdlkitchensink
# installation in several variables.  General variables::
#
#   SDLKitchensink_FOUND - true if the libsdlkitchensink headers and libraries were found
#   SDLKitchensink_INCLUDE_DIRS - the directory containing the libsdlkitchensink headers
#   SDLKitchensink_LIBRARIES - libsdlkitchensink libraries to be linked
#
# The following cache variables may also be set::
#
#   SDLKitchensink_INCLUDE_DIR - the directory containing the libsdlkitchensink headers
#   SDLKitchensink_LIBRARY - the libsdlkitchensink library (if any)


# Based on FindIconv written by Roger Leigh <rleigh@codelibre.net>

# Find include directory
find_path(SDL_kitchensink_INCLUDE_DIR
          NAMES "kitchensink/kitchensink.h"
          DOC "libsdlkitchensink include directory")
mark_as_advanced(SDL_kitchensink_INCLUDE_DIR)

# Find all SDLKitchensink libraries
find_library(SDL_kitchensink_LIBRARY
  NAMES SDL_kitchensink
  DOC "libsdlkitchensink libraries)")
find_library(SDL_kitchensink_STATIC_LIBRARY
  NAMES SDL_kitchensink_static
  DOC "libsdlkitchensink static libraries)")
mark_as_advanced(SDL_kitchensink_STATIC_LIBRARY)

include(${CMAKE_ROOT}/Modules/FindPackageHandleStandardArgs.cmake)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(SDL_kitchensink
                                  FOUND_VAR SDL_kitchensink_FOUND
                                  REQUIRED_VARS SDL_kitchensink_INCLUDE_DIR
                                  FAIL_MESSAGE "Failed to find libsdlkitchensink")

if(SDL_kitchensink_FOUND)
  set(SDL_kitchensink_INCLUDE_DIRS "${SDL_kitchensink_INCLUDE_DIR}")
  if(SDL_kitchensink_LIBRARY)
    set(SDL_kitchensink_LIBRARIES "${SDL_kitchensink_LIBRARY}")
  else()
    unset(SDL_kitchensink_LIBRARIES)
  endif()
  if(SDL_kitchensink_STATIC_LIBRARY)
    set(SDL_kitchensink_STATIC_LIBRARIES "${SDL_kitchensink_STATIC_LIBRARY}")
  else()
    unset(SDL_kitchensink_STATIC_LIBRARIES)
  endif()
endif()
