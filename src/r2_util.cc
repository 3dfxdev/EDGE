//----------------------------------------------------------------------------
//  EDGE True BSP Rendering (Utility routines)
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
//
// -AJA- 1999/08/31: Wrote this file.
//

#include "i_defs.h"

#include "dm_state.h"
#include "m_bbox.h"

#include "r_data.h"
#include "r_defs.h"
#include "r_state.h"
#include "r_main.h"
#include "r_modes.h"

#include "r2_defs.h"

#include <math.h>
#include <string.h>


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
