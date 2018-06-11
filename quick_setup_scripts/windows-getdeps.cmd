@echo off
setlocal
set path=%path%;%~dp0\vcpkg
vcpkg install bzip2:x64-windows-static bzip2:x86-windows-static zlib:x64-windows-static zlib:x86-windows-static libjpeg-turbo:x64-windows-static libjpeg-turbo:x86-windows-static

