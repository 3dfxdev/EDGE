# 3DGE - Makefile Building Instructions
# (C) 2011-2016 Isotope SoftWorks & Contributors
##### Original DOOM engine (C) id Software, LLC
#### Licensed under the GPLv2
http://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html
# See our 3DGE Wiki (http://3dfxdev.net/edgewiki/index.php/Main_Page)
---
There are custom configurations for each platform. We generally build these under MinGW/Cygwin for Windows, all Linux versions/builds, and MacOSX uses Clang. 

*Note: The Makefiles have moved into their own folder, Makefiles, in /root. Easiest way to use them is to just copy the makefile to the root and execute the build that way.*

# Building with Make:
## Due to recent Visual Studio support, Win32 users should never build with Make anymore, unless they want to test the effects of their changes across other platforms, or the GCC-version of 3DGE.
---
---
#### Build Linux debugging + shared-lib binary:

    > make -f Makefile.linux

#### Build Win32 statically-linked binary:
 (note: this should *always* be built with VS, not Make)

    > make -f Makefile.xming

#### Build MacOSX binary:
 (note: should build this with Clang)
 
    > make -f Makefile.osx

#### Build Dreamcast:
(note: this requires KallistiOS)

    > make -f Makefile.dc

MAKE was the original system that 3DGE and its predecessors used to build. This works in good faith accross all platforms, though building with GCC might produce some very strange results in comparison to either Clang or Visual Studio. If these are encountered, please report them on the Github page.

Static libraries tailored for MinGW/G++ can be found [here](http://tdgmods.net/MinGWLibs.7z)

(C) Isotope SoftWorks, 2016


