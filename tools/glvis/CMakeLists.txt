include_directories(../common)

add_library(libglvis STATIC
	../common/cmdlib.cpp
	../common/cmdlib.h
	../common/fmapdefs.h
	../common/fwaddefs.h
	../common/wadlib.cpp
	../common/wadlib.h
	flow.cpp
	glvis.h
	glvisint.h
	level.cpp
	vector.h
)
set_target_properties(libglvis PROPERTIES OUTPUT_NAME glvis)
add_dependencies(libglvis core)

add_executable(glvis
	glvis.cpp
)
add_dependencies(glvis libglvis)
target_link_libraries(glvis libglvis core)
set_target_properties(glvis PROPERTIES OUTPUT_NAME ../bin/glvis)
add_dependencies(glvis core)

#EXTRA_DIST = glvis.txt
