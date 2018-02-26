3DGE Library Versions
===================
This document outlines the libraries 3DGE needs for compile. 

    Please note that all libraries **must be statically linked** to the EXE (except for SDL2 on Win32).

----------

    /GLEW-2.0.0
    /libcpuid (0.4.0, upgraded from 0.2.2)
    /libjpeg-turbo (1.5.2, developmental version)
    /libogg-1.3.2
    /libpng16 (lpng1632)
    /libvorbis-1.3.5
    /physfs (pre3.0, upgraded from 2.0.3)
    /SDL2 (2.0.5, upgraded from 2.0.3)
    /SDL2_net-2.0.1 (deprecated. . .?)
    /Zlib-1.2.11 (upgraded from 1.2.8)

Visual Studio users can download VSLibs.7z, which contain pre-compiled libraries
for Visual Studio, here: [tdgmods.net/VSLibs.7z](tdgmods.net/VSLibs.7z). The archive is periodically updated
when new versions of libraries are built. 

Cross-Compile users (Unix-based) must build these libraries from scratch. Just look
at the version table above and you should be fine.
