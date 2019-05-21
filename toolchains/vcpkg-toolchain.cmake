set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)
if(PORT MATCHES "sdl2")
    set(VCPKG_LIBRARY_LINKAGE dynamic)
endif()

if(PORT STREQUAL "SDL2")
    set(VCPKG_LIBRARY_LINKAGE dynamic)
endif()