EDGE Library Versions
===================
This document outlines the libraries EDGE needs for compiling. 

Windows x86/x64: Please note that all libraries **must be statically linked** to the EXE (except for SDL2).

----------

    /libogg-1.3.3
    /libvorbis-1.3.6
    /physfs (3.1), https://icculus.org/physfs/
    /SDL2 (2.0.8)
    /SDL2_net-2.0.1
    /Zlib-1.2.11

Visual Studio users can download VSLibs.7z, which contain pre-compiled libraries
for Visual Studio, here: [tdgmods.net/VSLibs.7z](tdgmods.net/VSLibs.7z). The archive is periodically updated
when new versions of libraries are built. x64 libraries pre-built for Visual Studio 2017 can be
downloaded from here: [tdgmods.net/vsLibsx64.7z].


Cross-Compile users (Unix-based) must build these libraries from scratch. Just look
at the version table above and you should be fine. You can also look into /quick_setup_scripts to quickly download,
build, and link the required libraries automatically. 
