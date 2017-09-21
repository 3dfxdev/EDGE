add_library(libglvis STATIC
	cmdlib.cpp
	cmdlib.h
	fmapdefs.h
	fwaddefs.h
	wadlib.cpp
	wadlib.h
	flow.cpp
	glvis.h
	glvisint.h
	level.cpp
	vector.h
)
set_target_properties(libglvis PROPERTIES OUTPUT_NAME glvis)

add_executable(glvis
	glvis.cpp
)
add_dependencies(glvis libglvis)
target_link_libraries(glvis libglvis)

#EXTRA_DIST = glvis.txt
