cmake_minimum_required(VERSION 3.1)
project(SDL_kitchensink C)
include(GNUInstallDirs)
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

set(KIT_VERSION_MAJOR "0")
set(KIT_VERSION_MINOR "0")
set(KIT_VERSION_PATCH "7")
set(KIT_VERSION ${KIT_VERSION_MAJOR}.${KIT_VERSION_MINOR}.${KIT_VERSION_PATCH})
add_definitions(
    -DKIT_VERSION_MAJOR=${KIT_VERSION_MAJOR}
    -DKIT_VERSION_MINOR=${KIT_VERSION_MINOR}
    -DKIT_VERSION_PATCH=${KIT_VERSION_PATCH}
)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -ggdb -Werror -fno-omit-frame-pointer -Wno-deprecated-declarations")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} -ggdb -O2 -fno-omit-frame-pointer -DNDEBUG")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O2 -DNDEBUG")
set(CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL} -Os -DNDEBUG")


option(USE_DYNAMIC_LIBASS "Use dynamically loaded libass" ON)
option(USE_ASAN "Use AddressSanitizer" OFF)

find_package(SDL2)
find_package(ffmpeg COMPONENTS avcodec avformat avutil swscale swresample)

set(LIBRARIES
    ${SDL2_LIBRARIES}
    ${FFMPEG_LIBRARIES}
)
set(INCLUDES
    include/
    ${SDL2_INCLUDE_DIRS}
    ${FFMPEG_INCLUDE_DIRS}
)

if(BUILD_TESTS)
    add_subdirectory(tests)
endif()

if(USE_DYNAMIC_LIBASS)
    if(WIN32 OR MINGW OR MSYS)
        set(DYNAMIC_LIBASS_NAME "\"libass-9.dll\"")
    else()
        set(DYNAMIC_LIBASS_NAME "\"libass.so\"")
    endif()
    add_definitions(-DUSE_DYNAMIC_LIBASS)
    add_definitions(-DDYNAMIC_LIBASS_NAME=${DYNAMIC_LIBASS_NAME})
else()
    find_package(ass)
    set(LIBRARIES ${LIBRARIES} ${ASS_LIBRARIES})
    set(INCLUDES ${INCLUDES} ${ASS_INCLUDE_DIRS})
endif()

FILE(GLOB_RECURSE SOURCES "src/*.c")
FILE(GLOB INSTALL_HEADERS "include/kitchensink/*.h")

add_library(SDL_kitchensink SHARED ${SOURCES})
add_library(SDL_kitchensink_static STATIC ${SOURCES})

set_target_properties(SDL_kitchensink PROPERTIES VERSION ${KIT_VERSION})
set_target_properties(SDL_kitchensink PROPERTIES SOVERSION ${KIT_VERSION_MAJOR})

set_target_properties(SDL_kitchensink PROPERTIES DEBUG_POSTFIX "d")
set_target_properties(SDL_kitchensink_static PROPERTIES DEBUG_POSTFIX "d")

target_compile_definitions(SDL_kitchensink PRIVATE "KIT_DLL;KIT_DLL_EXPORTS")
target_compile_options(SDL_kitchensink PRIVATE "-fvisibility=hidden")

set_property(TARGET SDL_kitchensink PROPERTY C_STANDARD 99)
set_property(TARGET SDL_kitchensink_static PROPERTY C_STANDARD 99)

if(USE_ASAN)
    set(LIBRARIES asan ${LIBRARIES})
    target_compile_options(SDL_kitchensink PRIVATE "-fsanitize=address")
    message(STATUS "DEVELOPMENT: AddressSanitizer enabled!")
endif()

include_directories(${INCLUDES})
target_link_libraries(SDL_kitchensink ${LIBRARIES})

set(PKG_CONFIG_FILE "${CMAKE_CURRENT_BINARY_DIR}/SDL_kitchensink.pc")

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/pkg-config.pc.in"
    ${PKG_CONFIG_FILE}
    @ONLY
)

# Installation
install(FILES ${PKG_CONFIG_FILE} DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
install(FILES ${INSTALL_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/kitchensink)
INSTALL(TARGETS SDL_kitchensink SDL_kitchensink_static
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
