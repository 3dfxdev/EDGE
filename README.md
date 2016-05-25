Welcome to hyper3DGE!
---------------------

![](http://a.fsdn.com/con/app/proj/edge2/screenshots/327069.jpg)

#### 3DGE is an advanced OpenGL source port based on the DOOM engine

* http://edge2.sourceforge.net
* (c) 2011-2015 Isotope SoftWorks & Contributors
* [Visit the hyper3DGE Wiki for support and documentation](http://3dfxdev.net/edgewiki)


Build System for 3DGE
---------------------

There are custom configurations for each platform. This project uses Makefiles for compiling.
(There is no autoconf/automake/libtool support).

### Visual Studio and CMake support was recently added along with our standard Make system -- proper building instructions are coming soon!

### Building with Make:

#### Build Linux debugging + shared-lib binary:

    > make -f Makefile.linux

#### Build Win32 statically-linked binary:

    > make -f Makefile.xming

#### Build MacOSX binary:
 
    > make -f Makefile.osx

#### Build Dreamcast:
(note: this requires KallistiOS)

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

To find a list of the requirements to build the Dreamcast version,
please visit the [KallistiOS homepage](gamedev.allusion.net/softprj/kos/).

#### Support
* Visit the [3DGE forums](http://tdgmods.net/smf) and get involved with the
community and the various projects for the engine.
* The [3DGEWiki](http://3dfxdev.net/edgewiki) is also a great resource for
editing documentation and other information related to the engine.
