# 3DGE - CMake/Visual Studio Building Instructions
# (C) 2011-2017 Isotope SoftWorks & Contributors
##### id Tech/id Tech 2/ id Tech 3/ id Tech 4 (C) id Software, LLC
#### All id techs (and 3DGE itself) Licensed under the GPLv2
http://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html
# See our 3DGE Wiki (http://3dfxdev.net/edgewiki/index.php/Main_Page)
---
This build uses CMakeLists.txt, CMake-GUI, and Visual Studio to compile for Win32. *This is now the preferred method for anyone building a Windows version of 3DGE.* 

*Also note that plain CMake can be used to build Linux and MacOSX version, without Visual Studio. We will not cover that here.*

# Configuring/Building with Cmake-GUI/Visual Studio:
##### Due to recent Visual Studio support for 3DGE, Win32 users should *never* build with Make anymore, unless they want to test the effects of their changes across other platforms, or the GCC/W32-version of 3DGE. This is acceptable, since GCC and VS produce different behavior in-game - however, public releases/binaries of Windows versions of 3DGE should be built with Visual Studio!
---
---
#### Pre-Requisites
1) [Cmake](https://www.cmake.org)
2) [Visual Studio Community 2015](https://www.visualstudio.com/en-us/products/visual-studio-community-vs.aspx)
3) [Pre-Built Libraries](http://tdgmods.net/VSLibs.7z)

*Note that these pre-built libs should only be used with Visual Studio/CMake. We have a different library set for the [MAKE Projects, see](../blob/master/build_guide/MAKEBuild.md)*

### Configuring 3DGE (CMakeLists.txt)
1) Open CMake-GUI
2) Set the source code directory to your 3DGE source folder and the build directory to a subdirectory called build (or however you choose to do this)
3) When asked for a Generator, *choose 'Visual Studio 14 2015'*. Use the paremeter v140_xp if you want to compile with WinXP support.
4) You'll likely get some errors, that's because it can't find the libraries on its own without extra information; for now just go through all the red entries and point them to the appropriate .lib files and include folders.
5) Once Configure completes successfully, click Generate and then you'll get a file called 3DGE.sln in the build folder, which you can use to compile 3DGE with Visual Studio Community 2015 (other VS versions are unttested).

### Building 3DGE (Visual Studio 2K15)
1) Go to the directory where Cmake-Gui outputted your Build folder. Open up 3DGE.sln (solution project).
2) Right click the entire solution, and then 'Build Solution'.
3) Depending on what you are building with (Debug, Release), you will find it in the appropriate folder in the VS Build directory you specificied (named 3DGE.exe).

#### NOTE: You might run into compile issues with VS if you have MinGW installed on your system (usually c:\MinGW). If that happens, as a quick fix, rename your MinGW directory (e.g. c:\MinGW_) and build again.

---
(C) Isotope SoftWorks, 2017


