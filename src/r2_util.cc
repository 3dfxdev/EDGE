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

// arrays of stuff
#define DEFAULT_DRAWARRAY_SIZE

drawthingarray_c drawthings;
drawfloorarray_c drawfloors;

//
// R2_InitUtil
//
// One-time initialisation routine.
//
void R2_InitUtil(void)
{
}

// bsp clear function

void R2_ClearBSP(void)
{
	drawfloors.Init();
	drawthings.Init();
}

void R2_FreeupBSP(void)
{
	drawfloors.Clear();
	drawthings.Clear();
}

// ---> Draw thing container class

//
// drawthingarray_c::GetNew()
//
drawthing_t* drawthingarray_c::GetNew()
{
	SYS_ASSERT_MSG(!active_trans, ("[drawthingarray_c::GetNew] called twice"));	

	drawthing_t *dt;

	// Look for a spare entry
	if (commited < array_entries)
	{
		dt = *(drawthing_t**)FetchObjectDirect(commited);
		commited++;
	}
	else  
	{
		dt = new drawthing_t;
		InsertObject((void*)&dt);				
		commited = array_entries;
	}
	
	memset(dt, 0, sizeof(drawthing_t));
	
	active_trans = true; 	// We now have an active transaction 
	return dt;
}

//
// drawthingarray_c::Commit()
//
void drawthingarray_c::Commit(void)
{
	SYS_ASSERT_MSG(active_trans, ("[drawthingarray_c::Commit] no active trans"));
	active_trans = false;
}

//
// drawthingarray_c::Rollback()
//
void drawthingarray_c::Rollback(void)
{
	SYS_ASSERT_MSG(active_trans, ("[drawthingarray_c::Rollback] no active trans"));
	active_trans = false;
	commited--;
}
	
// ---> Draw floor container class

//
// drawfloorarray_c::GetNew()
//
// -ACB- 2004/08/04 
//
drawfloor_t* drawfloorarray_c::GetNew()
{
	SYS_ASSERT_MSG(!active_trans, ("[drawfloorarray_c::GetNew] called twice"));	

	drawfloor_t *df;

	// Look for a spare entry
	if (commited < array_entries)
	{
		df = *(drawfloor_t**)FetchObjectDirect(commited);
		commited++;
	}
	else  
	{
		df = new drawfloor_t;
		InsertObject((void*)&df);				
		commited = array_entries;
	}
	
	memset(df, 0, sizeof(drawfloor_t));

	active_trans = true; 	// We now have an active transaction 
	return df;
}

//
// drawfloorarray_c::Commit()
//
void drawfloorarray_c::Commit(void)
{
	SYS_ASSERT_MSG(active_trans, ("[drawfloorarray_c::Commit] no active trans"));
	active_trans = false;
}

//
// drawfloorarray_c::Rollback()
//
void drawfloorarray_c::Rollback(void)
{
	SYS_ASSERT_MSG(active_trans, ("[drawfloorarray_c::Rollback] no active trans"));
	active_trans = false;
	commited--;
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
