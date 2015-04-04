Welcome to hyper3DGE!
---------------------

* http://edge2.sourceforge.net
* (c) Isotope SoftWorks & Contributors

* This is based upon EDGE, by the EDGE Team.
* http://edge.sourceforge.net
* EDGE is (c) The EDGE Team

* Visit the 3DGE Wiki: http://3dfxdev.net/edgewiki


Build System for 3DGE
---------------------

There are custom configurations for each platform. This project uses Makefiles for compiling.
(There is no autoconf/automake/libtool support).

Build Linux debugging + shared-lib binary:

    > make -f Makefile.linux

Build Win32 statically-linked binary:

     > make -f Makefile.xming

Build MacOSX binary:
 
     > make -f Makefile.osx

Build Dreamcast:

     > make -f Makefile.dc


Libraries
---------

For the list of libraries required by 3DGE, please see the
following document: docs/tech/libraries.txt

The Makefiles not only build the main engine code
(i.e. all the stuff in the src/ directory) but also build
the EPI, DEH_EDGE, COAL, TIMIDITY, MD5, and GLBSP libraries.

The following libraries are linked statically in the
release builds and must be built manually beforehand:
zlib, libpng, jpeglib, libogg, libvorbis and libvorbisfile.
Also FLTK for Linux release binaries.
