![hyper3DGE](http://3dfxdev.net/edgewiki/images/f/f6/EDGElogo.jpg)
# CMake/Visual Studio Building Instructions
# (C) 2010-2018 [Isotope SoftWorks](https://www.facebook.com/IsotopeSoftWorks/) & [Contributors (aka The EDGE Team)](https://github.com/3dfxdev/hyper3DGE/blob/master/AUTHORS.md)
> #### id Tech/id Tech 2/ id Tech 3/ id Tech 4 and DOOM (C) 1997-2011 id Software/Zenimax Media.
> #### [EDGE (et al.) licensed under the GPLv2 or greater license](https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html)
# Check out our [EDGEWiki](http://3dfxdev.net/edgewiki/) for more information!
### Build Preface:
This build uses CMakeLists.txt, CMake-GUI, and Visual Studio to compile for Win32/Win64. *This is now the preferred method for anyone building a Windows version of EDGE. Also note that CMake can also be used to cross-compile EDGE for Win32 (or others, like Linux, Raspberry Pi, MacOSX, etc), with or without Visual Studio (called Cross-Compiling). We will not cover that here.*

# Configuring/Building with Cmake-GUI/Visual Studio:

> ##### Due to recent Visual Studio support for EDGE, Windows users should *never* build with the *Make/Makefile* system anymore (use CMakeLists in combination with MinGW/Cygwin/MSYS2/etc if you want to cross-compile, or use GCC/g++ to compile 3DGE). This is acceptable, since GCC and MSVC compilers produce different behavior/warnings via compile time _and_ as a binary - however, as a rule of thumb by the EDGE team: public releases/binaries of Windows versions of 3DGE should always be built with Visual Studio using the MSVC compiler!

---
#### Pre-Requisites
1) [Cmake](https://www.cmake.org)
2) [Visual Studio Community 2017](https://www.visualstudio.com/en-us/products/visual-studio-community-vs.aspx)
3) *Pre-Built Libraries for [WIN32/X86](http://tdgmods.net/VSLibs.7z) or [WIN64/x64](http://tdgmods.net/vsLibsx64.7z), *unless* you have [vcpkg](https://github.com/Microsoft/vcpkg) or desire to build the libraries yourself. *For the former, see Step 4a in 'Configuring EDGE'.*

> * The libs provided by us for Visual Studio are built using MSVC 17. If
> you are using MSVC 15, the pre-built libs must be rebuilt with your
> version of Visual Studio! 

 > Note that these pre-built libs should only be used with Visual Studio/CMake. While we have a different set of standard Makefiles (located in /Makefiles), they are deprecated and not kept up to date -- you will have to manually update them. It is therefore highly recommended to use CMakeLists to generate your Visual Studio (or MinGW/GCC cross-compile) project for Windows!


---

### Configuring EDGE (CMakeLists.txt)
1) Open CMake-GUI.
2) Set the source code directory to your 3DGE source folder and the build directory to a subdirectory called build (or however you choose to name these). After those are set, click 'Configure'.
 ##### *This next step is version-dependant and coincides with the version of Visual Studio you have installed!*
3)  Click *Generate* and CMake-GUI will open a new window for configuration options.

    *3a)* If you have Visual Studio Community 15, the generator would be *Visual Studio 14 2015*.
    
    *3b)* If you have Visual Studio Community 17, the generator would be *Visual Studio 15 2017*.
    
    *3c)* Use the [-T] parameter *v140_xp* if you want to compile with Windows XP support. If you are using Visual Studio 2017, the [-T] parameter would instead be *v141_xp* for Visual Studio Community 2017. If you do not care about running the project with Windows XP, you should skip this parameter altogether. 
  

 4) For most people, the default 'Use default native compilers' is sufficient. If you are a  [vcpkg](https://github.com/Microsoft/vcpkg) user, please refer to step *4a*, if not, skip to step *5*. If not, click 'Finish'.

    *4a)* If you use [Microsoft's vcpkg](https://github.com/Microsoft/vcpkg) service under Windows for library building and linking, you will instead select *Specify toolchain file for cross-compiling*. Click *Next*, and you will be prompted to specify the vcpkg buildscript. This file usually resides in *c:/vcpkg/scripts/buildsystems/*, unless your hard drive or vcpkg location is different. Select *vcpkg.cmake* and continue. This will ignore the need for Visual Studio to look in our VSLibs for libraries and refer to the libraries installed globally by vcpkg. If you haven't already, you should run the command *.\vcpkg integrate install*, as this will make those libraries universally included by Visual Studio (this option also works even if a vcpkg toolchain is not explicitly specified).

5) Once Configure completes successfully, click Generate and then you'll get a file called 3DGE.sln in the build folder, which you can use to compile 3DGE with Visual Studio Community 2015 (other VS versions are unttested).

### Building EDGE (Visual Studio Community)
1) Go to the directory where Cmake-Gui set your Build directory. Open up 3DGE.sln (solution project).
2) Right click the entire solution, and then 'Build Solution'.
3) Depending on what you are building with (Debug, Release, RelWithDebInfo), you will find it in the appropriate folder in the VS Build directory you specified (named 3DGE.exe).
4) For compiler options under RelWithDebInfo, it is recommended to set the /W (Warning) level to /W1. It is also recommended to change the Optimization flags to Full (/Ox), Inline Function Expansion to (/Ob2), Intrinsic Functions Enabled (/Oi), Favor Speed (/Ot), and Enable Fiber-Safe Optimizations (/GT).

If you have an issue with setting up or building EDGE, please [open an issue](https://github.com/3dfxdev/hyper3DGE/issues/new) and we will do our best to help you.
---
(C) Isotope SoftWorks, 2018

