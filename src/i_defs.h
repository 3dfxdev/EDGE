//----------------------------------------------------------------------------
//  EDGE2 System Specific Header
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2009  The EDGE2 Team.
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

#ifndef __SYSTEM_SPECIFIC_DEFS__
#define __SYSTEM_SPECIFIC_DEFS__

#include "../epi/epi.h"

#include "con_var.h"
#include "i_system.h"

/// `CA- 6.5.2016: quick hacks to change these in Visual Studio (less warnings). 
#ifdef _MSC_VER
#define strdup _strdup
#define stricmp _stricmp
#define strnicmp _strnicmp
#endif


#endif /*__SYSTEM_SPECIFIC_DEFS__*/

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
