@echo -off
setlocal
set path=%path%;%~dp0\vcpkg
./vcpkg install bzip2:x64-windows-static bzip2:x86-windows-static zlib:x64-windows-static zlib:x86-windows-static libogg:x64-windows-static libogg:x86-windows-static libvorbis:x64-windows-static libvorbis:x86-windows-static SDL2:x86-windows SDL2:x64-windows sdl2-net:x64-windows sdl2-net:x86-windows liblzma:x64-windows-static liblzma:x86-windows-static
./vcpkg integrate install
 