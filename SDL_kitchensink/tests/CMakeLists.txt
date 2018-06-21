find_package(CUnit)

add_executable(test_lib
    test_lib.c
    test_source.c
)

include_directories(${CUNIT_INCLUDE_DIR} . ../include/)
if(MINGW)
    target_link_libraries(test_lib mingw32)
endif()
target_link_libraries(test_lib
    SDL_kitchensink_static
    ${CUNIT_LIBRARIES}
    ${SDL2_LIBRARIES}
    ${FFMPEG_LIBRARIES}
)
add_custom_target(unittest test_lib)
