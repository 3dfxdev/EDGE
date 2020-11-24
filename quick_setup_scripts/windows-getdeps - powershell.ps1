@echo -off
setlocal
set path=%path%;%~dp0\vcpkg
./vcpkg install sdl2:x86-windows sdl2:x64-windows libogg:x64-windows-static libogg:x86-windows-static libvorbis:x64-windows-static libvorbis:x86-windows-static physfs:x86-windows-static physfs:x64-windows-static zlib:x64-windows-static zlib:x86-windows-static bzip2:x64-windows-static bzip2:x86-windows-static liblzma:x64-windows-static liblzma:x86-windows-static
./vcpkg integrate install
 