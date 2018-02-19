CMake files for development needs , because the official repo supports only makefiles.

Basically, can be used as CMake subproject with static linking.

#How to build 
```bash
mkdir cmake
cd cmake
cmake ..
make (or refer to cmake dir, if you are using Visual Studio)
```

or just add that library to your CMake project using [add_subdirectory()](https://cmake.org/cmake/help/latest/command/add_subdirectory.html) and [target_link_libraries()](https://cmake.org/cmake/help/latest/command/target_link_libraries.html) commands
