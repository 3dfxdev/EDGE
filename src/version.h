//----------------------------------------------------------------------------
//  EDGE2 Version Header
//----------------------------------------------------------------------------
//
//  Copyright (c) 2016 Isotope SoftWorks
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//----------------------------------------------------------------------------

#include "gitinfo.h"

#define TITLE  "EDGE ENGINE"

const char *GetGitDescription();
const char *GetGitHash();
const char *GetGitTime();
const char *GetVersionString();

#define EDGEVER     210
#define EDGEVERHEX  0x210

#ifdef GIT_DESCRIPTION
#define EDGEVERSTR GIT_DESCRIPTION
#else
#define EDGEVERSTR "2.1.0 - TEST3"
#endif

#ifdef __clang__
#define EDGECOMPILER "Clang"
#elif defined(__GNUC__)
#define EDGECOMPILER "GCC"
#elif defined(_MSC_VER)
#define EDGECOMPILER "MSVC"
#endif

// Build Platform Strings (Visual Studio/Windows)
#ifdef _MSC_VER

#ifdef _M_X64
#define EDGEPLATFORM "x64 " EDGECOMPILER
#define EDGEPRINTBIT "64-bit"
#define E_TITLE "EDGE (64-bit)" EDGEVERSTR
#elif _M_IX86
#define EDGEPLATFORM "x86 " EDGECOMPILER
#define EDGEPRINTBIT "32-bit"
#define E_TITLE "EDGE (32-bit)" EDGEVERSTR
#endif

#else
// Build Platform Strings (GCC/Linux, GCC/Windows, et al)
#ifdef __i386__
#define EDGEPLATFORM "x86 " EDGECOMPILER
#define EDGEPRINTBIT "32-bit"
#define E_TITLE "EDGE (32-bit)"  EDGEVERSTR
#elif __x86_64__
#define EDGEPLATFORM "x64 " EDGECOMPILER
#define EDGEPRINTBIT "64-bit"
#define E_TITLE "EDGE (64-bit)"  EDGEVERSTR
#elif __aarch64__
#define EDGEPLATFORM "ARM64 " EDGECOMPILER
#define EDGEPRINTBIT "64-bit"
#define E_TITLE "EDGE (64-bit)"  EDGEVERSTR
#else
#error "No platform defined"
#endif
#endif

//#define EDGEBUILDSTR "(205af44)"

#define VERSIONSTR "v1.2.0test3"
#define FINALVERSIONSTR "v2.1.0 (Final)" "x" EDGEPRINTBIT

// patch level (Savegames and Demos)
#define EDGEPATCH  7

// -ES- 2000/03/04 The version of EDGE2.WAD we require.
#define EDGE_WAD_VERSION  800
// ~CA  5.7.2016
#define EDGE_PAK_VERSION  800

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
