//----------------------------------------------------------------------------
//  EDGE New Demo Handling (Global info)
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
//  Based on the DOOM source code, released by Id Software under the
//  following copyright:
//
//    Copyright (C) 1993-1996 by id Software, Inc.
//
//----------------------------------------------------------------------------

#ifndef __DEM_GLOB__
#define __DEM_GLOB__

#include "i_defs.h"
#include "p_local.h"
#include "sv_main.h"
#include "epi/files.h"

saveglobals_t *DEM_NewGLOB(void);
saveglobals_t *DEM_LoadGLOB(void);

void DEM_SaveGLOB(saveglobals_t *globs);
void DEM_FreeGLOB(saveglobals_t *globs);


//
//  DEBUGGING
//

void DEM_DumpDemo(epi::file_c *fp);


#endif  // __DEM_GLOB__
