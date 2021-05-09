
![hyper3DGE](http://3dfxdev.net/edgewiki/images/f/f6/EDGElogo.jpg)
# vcpkg/CMake/MSVC Instructions
# (C) 1999-2018 [Isotope SoftWorks](https://www.facebook.com/IsotopeSoftWorks/) & [Contributors (aka The EDGE Team)](https://github.com/3dfxdev/hyper3DGE/blob/master/AUTHORS.md)
> #### id Tech/id Tech 2/ id Tech 3/ id Tech 4 and DOOM (C) 1997-2011 id Software/Zenimax Media.
> #### [EDGE (et al.) licensed under the GPLv2 or greater license](https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html)
# Check out our [EDGEWiki](http://3dfxdev.net/edgewiki/) for more information!
### Build Preface:
This build uses CMakeLists.txt, CMake-GUI, Microsoft's vcpkg, and Visual Studio to compile for Win32 (32-bit). *This is now the preferred method for anyone building a Windows version of EDGE. Also note that CMake can also be used to cross-compile EDGE for Win32 (or others, like Linux, Raspberry Pi, MacOSX, etc), with or without Visual Studio (called Cross-Compiling). We will not cover that here.*

# Configuring/Building with CMake/Visual Studio (2015+):
---
#### Pre-Requisites
1) [Cmake](https://www.cmake.org)
2) [Visual Studio Community 2015/2017/2019](https://www.visualstudio.com/en-us/products/visual-studio-community-vs.aspx)
3) [vcpkg](https://github.com/Microsoft/vcpkg) 

*Note that the required libraries should have been built with Microsoft's vcpkg tool under Windows. Preferably, all required libraries (except SDL2/SDL2_Net) should be compiled and linked with the windows-static triplet.

You should have vcpkg installed and ready to use at this point. If you haven't, visit the vcpkg link listed in Pre-Requisites first!!!
---

### Configuring EDGE (CMakeLists.txt) with CMake-GUI
1) Open CMake-GUI.
2) Set the source code directory to your 3DGE source folder and the build directory to a subdirectory called build (or however you choose to name these). After those are set, click 'Configure'.
 ##### *This next step is version-dependant and coincides with the version of Visual Studio you have installed!*
3)  Click *Generate* and CMake-GUI will open a new window for configuration options.

    *3a)* If you have Visual Studio Community 15, the generator would be *Visual Studio 14 2015*.
    
    *3b)* If you have Visual Studio Community 17, the generator would be *Visual Studio 15 2017*. Ditto for Visual Studio 19.
    
    *3c)* Use the [-T] parameter *v140_xp* if you want to compile with Windows XP support. If you are using Visual Studio 2017, the [-T] parameter would instead be *v141_xp* for Visual Studio Community 2017. If you do not care about running the project with Windows XP, you should skip this parameter altogether. 
  

4) Select *Specify toolchain file for cross-compiling*. Click *Next*, and you will be prompted to specify the vcpkg buildscript. This file usually resides in *c:/vcpkg/scripts/buildsystems/*, unless your hard drive or vcpkg location is different. Select *vcpkg.cmake* and continue. This will refer to the libraries installed globally by vcpkg. If you haven't already, you should run the command *.\vcpkg integrate install*, as this will make those libraries universally included by Visual Studio (this option also works even if a vcpkg toolchain is not explicitly specified).

	4a. You might need to rebuild all libraries required by EDGE to use the windows-static target. If they are already built, change VCPKG_TARGET_TRIPLET to "(xarch-windows-static", where arch is 86 or 64; or refer to the following article to have MSVC handle this:
	https://blogs.msdn.microsoft.com/vcblog/2016/11/01/vcpkg-updates-static-linking-is-now-available/

	4b. There might be a few entries that need manual definitions, in the case of SDL2_Main. Just point SDLMAIN2_LIBRARY to "xARCH-windows/lib/manual-link/SDL2main.lib". Keep in mind that SDL2 and SDL2_Net should not be built or linked statically (as we want SDL2.dll in the EDGE folder).

5) Once Configure completes successfully, click Generate and then you'll get a file called 3DGE.sln in the build folder, which you can use to compile 3DGE with Visual Studio Community 2015 (other VS versions are unttested).

### Configuring EDGE (CMakeLists.txt) with CMake (command line)
1) Just build the project and make sure to add the following to your command-line when building EDGE:
	*-DCMAKE_TOOLCHAIN_FILE=.../vcpkg.cmake*

Again, you might need to specify some options manually, like the location of SDL2_Main.
        (where xARCH is either "x86" or "x64")
	If you want to set it to automatically use the STATIC target_triplet (all libraries except SDL2/SDL2_NET _should_ be statically linked), add this to the end of the above CMAKE command:
	-DVCPKG_TARGET_TRIPLET=xARCH-windows-static

### Potential Issues
1) In Visual Studio using vcpkg/CMake, you might need to change the Runtime Library from /MD to /MT
2) If you encounter issues, please see the following article:
https://blogs.msdn.microsoft.com/vcblog/2016/11/01/vcpkg-updates-static-linking-is-now-available/

### Building EDGE (Visual Studio Community)
1) Go to the directory where Cmake-Gui set your Build directory. Open up 3DGE.sln (solution project).
2) Right click the entire solution, and then 'Build Solution'.
3) Depending on what you are building with (Debug, Release, RelWithDebInfo), you will find it in the appropriate folder in the VS Build directory you specified (named EDGE.exe).
4) For compiler options under RelWithDebInfo, it is recommended to set the /W (Warning) level to /W1. It is also recommended to change the Optimization flags to Full (/Ox), Inline Function Expansion to (/Ob2), Intrinsic Functions Enabled (/Oi), Favor Speed (/Ot), and Enable Fiber-Safe Optimizations (/GT).

If you have an issue with setting up or building EDGE, please [open an issue](https://github.com/3dfxdev/hyper3DGE/issues/new) and we will do our best to help you.
---
(C) Isotope SoftWorks, 2018-2021


