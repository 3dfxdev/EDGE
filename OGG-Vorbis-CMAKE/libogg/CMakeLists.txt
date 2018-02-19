cmake_minimum_required(VERSION 3.0)

project (ogg)

if(NOT (${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC))
set(INCLUDE_INTTYPES_H 0)
set(INCLUDE_STDINT_H 1)
set(INCLUDE_SYS_TYPES_H 0)
set(SIZE16 int16_t)
set(USIZE16 uint16_t)
set(SIZE32 int32_t)
set(USIZE32 uint32_t)
set(SIZE64 int64_t)
set(USIZE64 uint64_t)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/ogg/config_types.h.in ${CMAKE_CURRENT_SOURCE_DIR}/include/ogg/config_types.h @ONLY)
endif()

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src SOURCE_LIB)

add_library(${PROJECT_NAME} STATIC ${SOURCE_LIB})

target_include_directories(${PROJECT_NAME} 
PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_property(TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)