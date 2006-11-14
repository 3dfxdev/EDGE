//----------------------------------------------------------------------------
//  EDGE System Specific Header
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2005  The EDGE Team.
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
//
// -ACB- 1999/09/19 Written
//

#ifndef __SYSTEM_SPECIFIC_DEFS__
#define __SYSTEM_SPECIFIC_DEFS__

// MinGW 
#ifdef WIN32
#ifdef __GNUC__

#define WIN32_LEAN_AND_MEAN

#include "epi/epi.h"

#define EDGECONFIGFILE "EDGE.CFG"
#define EDGELOGFILE    "EDGE.LOG"
#define EDGEHOMESUBDIR "Application Data\\Edge"
#define REQUIREDWAD    "EDGE"

#define DIRSEPARATOR '\\'

#define NAME        "The EDGE Engine"
#define OUTPUTNAME  "EDGECONSOLE"
#define TITLE       "EDGE Engine"
#define OUTPUTTITLE "EDGE System Console"

#define MAXPATH _MAX_PATH

#define alloca _alloca
#define I_MoveData memmove

#include "w32_compen.h"
#include "i_system.h"

#endif
#endif  // MinGW

// Microsoft Visual C++ V6.0 for Win32
#ifdef WIN32 
#ifdef _GATESY_

#define WIN32_LEAN_AND_MEAN

#include "epi/epi.h"

#define EDGECONFIGFILE "EDGE.CFG"
#define EDGELOGFILE    "EDGE.LOG"
#define EDGEHOMESUBDIR "Application Data\\Edge"
#define REQUIREDWAD    "EDGE"

#define DIRSEPARATOR '\\'

#define NAME        "The EDGE Engine"
#define OUTPUTNAME  "EDGECONSOLE"
#define TITLE       "EDGE Engine"
#define OUTPUTTITLE "EDGE System Console"

// Access() define values. Nicked from DJGPP's <unistd.h>
#define R_OK    0x02
#define W_OK    0x04

// PI define. Nicked from DJGPP's <math.h>
#define M_PI 3.14159265358979323846

#define MAXPATH _MAX_PATH

#define I_MoveData memmove

#define DIRSEPARATOR '\\'

#include "w32_compen.h"
#include "i_system.h"

#endif
#endif  // Visual C++

// Borland C++ V5.5 for Win32
#ifdef WIN32 
#ifdef __BORLANDC__

#include "epi/epi.h"

#define EDGECONFIGFILE "EDGE.CFG"
#define EDGELOGFILE    "EDGE.LOG"
#define EDGEHOMESUBDIR "Application Data\\Edge"
#define REQUIREDWAD    "EDGE"

#define DIRSEPARATOR '\\'

#define NAME        "EDGE"
#define OUTPUTNAME  "EDGECONSOLE"
#define TITLE       "EDGE Engine"
#define OUTPUTTITLE "EDGE System Console"

// Access() define values. Nicked from DJGPP's <unistd.h>
#define R_OK    0x02
#define W_OK    0x04

// PI define. Nicked from DJGPP's <math.h>
#define M_PI 3.14159265358979323846

#define I_MoveData memmove

#define DIRSEPARATOR '\\'

#include "w32_compen.h"
#include "i_system.h"

#endif
#endif  // Borland C++

// LINUX GCC
#ifdef LINUX

#include "epi/epi.h"

#define EDGECONFIGFILE "edge.cfg"
#define EDGELOGFILE    "edge.log"
#define EDGEHOMESUBDIR ".edge"
#define REQUIREDWAD    "edge"

#define NAME        "EDGE"
#define OUTPUTNAME  "EDGECONSOLE"
#define TITLE       "EDGE Engine"
#define OUTPUTTITLE "EDGE Engine console"

#define I_MoveData memmove

#define DIRSEPARATOR '/'

#include "unx_compen.h"
#include "i_system.h"

#endif // LINUX GCC

// MacOSX GCC
#ifdef MACOSX

#include "epi/epi.h"

#define EDGECONFIGFILE "edge.cfg"
#define EDGELOGFILE    "edge.log"
#define EDGEHOMESUBDIR ".edge"
#define REQUIREDWAD    "edge"

#define DIRSEPARATOR '/'

#define NAME        "EDGE"
#define OUTPUTNAME  "EDGECONSOLE"
#define TITLE       "EDGE Engine"
#define OUTPUTTITLE "EDGE Engine console"

#define I_MoveData memmove

#define DIRSEPARATOR '/'

// moved; compile failure if ASSEM=Y
#include "unx_compen.h"
#include "i_system.h"
//#include "linux/i_compen.h"

#endif // MACOSX GCC

#endif /*__SYSTEM_SPECIFIC_DEFS__*/

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
