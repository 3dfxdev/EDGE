//----------------------------------------------------------------------------
//  EDGE True BSP Rendering (Utility routines)
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2004  The EDGE Team.
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
// TODO HERE:
//   + Implement better 1D buffer code.
//

#include "i_defs.h"

#include "dm_state.h"
#include "m_bbox.h"
#include "m_fixed.h"

#include "r_data.h"
#include "r_defs.h"
#include "r_state.h"
#include "r_main.h"
#include "v_res.h"
#include "z_zone.h"

#include "r2_defs.h"

// arrays of stuff
#define DEFAULT_DRAWARRAY_SIZE

drawwallarray_c  drawwalls;
drawplanearray_c drawplanes;
drawthingarray_c drawthings;
drawfloorarray_c drawfloors;

byte *subsectors_seen = NULL;
static int subsectors_seen_size = -1;

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
	drawwalls.Init();
	drawfloors.Init();
	drawthings.Init();
	drawplanes.Init();
	
	if (subsectors_seen_size != numsubsectors)
	{
		subsectors_seen_size = numsubsectors;

		Z_Resize(subsectors_seen, byte, subsectors_seen_size);
	}

	Z_Clear(subsectors_seen, byte, subsectors_seen_size);
}

void R2_FreeupBSP(void)
{
	drawwalls.Clear();
	drawfloors.Clear();
	drawthings.Clear();
	drawplanes.Clear();
}

// ---> Drawwall container class

//
// drawwallarray_c::GetNew()
//
drawwall_t* drawwallarray_c::GetNew()
{
	DEV_ASSERT(!active_trans, ("[drawwallarray_c::GetNew] called twice"));	

	drawwall_t *dw;

	// Look for a spare entry
	if (commited < array_entries)
	{
		dw = *(drawwall_t**)FetchObjectDirect(commited);
		commited++;
	}
	else  
	{
		dw = new drawwall_t;
		InsertObject((void*)&dw);				
		commited = array_entries;
	}

	memset(dw, 0, sizeof(drawwall_t));

	active_trans = true; 	// We now have an active transaction 
	return NULL;
}

//
// drawwallarray_c::Commit()
//
void drawwallarray_c::Commit(void)
{
	DEV_ASSERT(active_trans, ("[drawwallarray_c::Commit] no active trans"));
	active_trans = false;
}

//
// drawwallarray_c::Rollback()
//
void drawwallarray_c::Rollback(void)
{
	DEV_ASSERT(active_trans, ("[drawwallarray_c::Rollback] no active trans"));
	active_trans = false;
	commited--;
}

// ---> Draw plane container class

//
// drawplanearray_c::GetNew()
//
drawplane_t* drawplanearray_c::GetNew()
{
	DEV_ASSERT(!active_trans, ("[drawplanearray_c::GetNew] called twice"));	

	drawplane_t *dp;

	// Look for a spare entry
	if (commited < array_entries)
	{
		dp = *(drawplane_t**)FetchObjectDirect(commited);
		commited++;
	}
	else  
	{
		dp = new drawplane_t;
		InsertObject((void*)&dp);				
		commited = array_entries;
	}
	
	memset(dp, 0, sizeof(drawplane_t));

	active_trans = true; 	// We now have an active transaction 
	return dp;
}

//
// drawplanearray_c::Commit()
//
void drawplanearray_c::Commit(void)
{
	DEV_ASSERT(active_trans, ("[drawplanearray_c::Commit] no active trans"));
	active_trans = false;
}

//
// drawplanearray_c::Rollback()
//
void drawplanearray_c::Rollback(void)
{
	DEV_ASSERT(active_trans, ("[drawplanearray_c::Rollback] no active trans"));
	active_trans = false;
	commited--;
}

// ---> Draw thing container class

//
// drawthingarray_c::GetNew()
//
drawthing_t* drawthingarray_c::GetNew()
{
	DEV_ASSERT(!active_trans, ("[drawthingarray_c::GetNew] called twice"));	

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
	DEV_ASSERT(active_trans, ("[drawthingarray_c::Commit] no active trans"));
	active_trans = false;
}

//
// drawthingarray_c::Rollback()
//
void drawthingarray_c::Rollback(void)
{
	DEV_ASSERT(active_trans, ("[drawthingarray_c::Rollback] no active trans"));
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
	DEV_ASSERT(!active_trans, ("[drawfloorarray_c::GetNew] called twice"));	

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
	DEV_ASSERT(active_trans, ("[drawfloorarray_c::Commit] no active trans"));
	active_trans = false;
}

//
// drawfloorarray_c::Rollback()
//
void drawfloorarray_c::Rollback(void)
{
	DEV_ASSERT(active_trans, ("[drawfloorarray_c::Rollback] no active trans"));
	active_trans = false;
	commited--;
}
	
//----------------------------------------------------------------------------
//
//  1D OCCLUSION BUFFER CODE
//

// NOTE!  temporary code to test new 1D buffer concept

static byte cruddy_1D_buf[2048];

//
// R2_1DOcclusionClear
//
void R2_1DOcclusionClear(int x1, int x2)
{
	DEV_ASSERT2(0 <= x1 && x1 <= x2);

	for (; x1 <= x2; x1++)
		cruddy_1D_buf[x1] = 1;
}

//
// R2_1DOcclusionSet
//
void R2_1DOcclusionSet(int x1, int x2)
{
	DEV_ASSERT2(0 <= x1 && x1 <= x2);

	for (; x1 <= x2; x1++)
		cruddy_1D_buf[x1] = 0;
}

//
// R2_1DOcclusionTest
//
// Test if the range from x1..x2 is completely blocked, returning true
// if it is, otherwise false.
//
bool R2_1DOcclusionTest(int x1, int x2)
{
	DEV_ASSERT2(0 <= x1 && x1 <= x2);

	for (; x1 <= x2; x1++)
		if (cruddy_1D_buf[x1])
			return false;

	return true;
}

//
// R2_1DOcclusionTestShrink
//
// Similiar to the above routine, but shrinks the given range as much
// as possible.  Returns true if totally occluded.
//
bool R2_1DOcclusionTestShrink(int *x1, int *x2)
{
	DEV_ASSERT2(0 <= (*x1) && (*x1) <= (*x2));

	for (; (*x1) <= (*x2); (*x1)++)
		if (cruddy_1D_buf[*x1])
			break;

	for (; (*x1) <= (*x2); (*x2)--)
		if (cruddy_1D_buf[*x2])
			break;

	return (*x1) > (*x2) ? true : false;
}

//
// R2_1DOcclusionClose
//
void R2_1DOcclusionClose(int x1, int x2, Y_range_t *ranges)
{
	int i;

	DEV_ASSERT2(0 <= x1 && x1 <= x2);

	for (i=x1; i <= x2; i++)
	{
		if (cruddy_1D_buf[i] == 0)
		{
			ranges[i - x1].y1 = 1;
			ranges[i - x1].y2 = 0;
		}
	}
}


//----------------------------------------------------------------------------
//
//  2D OCCLUSION BUFFER CODE
//

Y_range_t Screen_clip[2048];

//
// R2_2DOcclusionClear
//
void R2_2DOcclusionClear(int x1, int x2)
{
	DEV_ASSERT2(0 <= x1 && x1 <= x2);

	for (; x1 <= x2; x1++)
	{
		Screen_clip[x1].y1 = 0;
		Screen_clip[x1].y2 = viewheight-1;
	}
}

//
// R2_2DOcclusionClose
//
void R2_2DOcclusionClose(int x1, int x2, Y_range_t *ranges,
    bool connect_low, bool connect_high, bool solid)
{
	int i;

	DEV_ASSERT2(0 <= x1 && x1 <= x2);

	for (i=x1; i <= x2; i++)
	{
		short y1 = ranges[i - x1].y1;
		short y2 = ranges[i - x1].y2;

		if (y1 > y2)
			continue;

		ranges[i - x1].y1 = MAX(y1, Screen_clip[i].y1);
		ranges[i - x1].y2 = MIN(y2, Screen_clip[i].y2);

		if (connect_low || (solid && y2 >= Screen_clip[i].y2))
			Screen_clip[i].y2 = MIN(Screen_clip[i].y2, y1-1);

		if (connect_high || (solid && y1 <= Screen_clip[i].y1))
			Screen_clip[i].y1 = MAX(Screen_clip[i].y1, y2+1);
	}
}

//
// R2_2DOcclusionCopy
//
// Copy the 1D/2D occlusion buffer.
//
void R2_2DOcclusionCopy(int x1, int x2, Y_range_t *ranges)
{
	int i;

	DEV_ASSERT2(0 <= x1 && x1 <= x2);

	for (i=x1; i <= x2; i++)
	{
		if (cruddy_1D_buf[i] == 0)
		{
			ranges[i - x1].y1 = 1;
			ranges[i - x1].y2 = 0;
		}
		else
		{
			ranges[i - x1].y1 = Screen_clip[i].y1;
			ranges[i - x1].y2 = Screen_clip[i].y2;
		}
	}
}

//
// R2_2DUpdate1D
//
// Check the 2D occlusion buffer for complete closure, updating the 1D
// occlusion buffer where necessary.
//
void R2_2DUpdate1D(int x1, int x2)
{
	int i;

	DEV_ASSERT2(0 <= x1 && x1 <= x2);

	for (i=x1; i <= x2; i++)
	{
		if (Screen_clip[i].y1 > Screen_clip[i].y2)
			cruddy_1D_buf[i] = 0;
	}
}


//----------------------------------------------------------------------------
//
//  LEVEL-OF-DETAIL STUFF
//
//  These functions return a "LOD" number.  1 means the maximum
//  detail, 2 is half the detail, 4 is a quarter, etc...
//

int lod_base_cube = 256;


//
// R2_GetPointLOD
//
int R2_GetPointLOD(float x, float y, float z)
{
	x = fabs(x - viewx);
	y = fabs(y - viewy);
	z = fabs(z - viewz);

	x = MAX(x, MAX(y, z));

	return 1 + (int)(x / lod_base_cube);
}

//
// R2_GetBBoxLOD
//
int R2_GetBBoxLOD(float x1, float y1, float z1,
		float x2, float y2, float z2)
{
	x1 -= viewx;  x2 -= viewx;
	y1 -= viewy;  y2 -= viewy;
	z1 -= viewz;  z2 -= viewz;

	// check signs to handle BBOX crossing the axis
	x1 = ((x1<0) != (x2<0)) ? 0 : MIN(fabs(x1), fabs(x2));
	y1 = ((y1<0) != (y2<0)) ? 0 : MIN(fabs(y1), fabs(y2));
	z1 = ((z1<0) != (z2<0)) ? 0 : MIN(fabs(z1), fabs(z2));

	x1 = MAX(x1, MAX(y1, z1));

	return 1 + (int)(x1 / lod_base_cube);
}

//
// R2_GetWallLOD
//
// Note: code is currently the same as in R2_GetBBoxLOD above, though
// this could be improved to handle diagonal lines better (at the
// moment we just use the line's bbox).
//
int R2_GetWallLOD(float x1, float y1, float z1,
		float x2, float y2, float z2)
{
	x1 -= viewx;  x2 -= viewx;
	y1 -= viewy;  y2 -= viewy;
	z1 -= viewz;  z2 -= viewz;

	// check signs to handle BBOX crossing the axis
	x1 = ((x1<0) != (x2<0)) ? 0 : MIN(fabs(x1), fabs(x2));
	y1 = ((y1<0) != (y2<0)) ? 0 : MIN(fabs(y1), fabs(y2));
	z1 = ((z1<0) != (z2<0)) ? 0 : MIN(fabs(z1), fabs(z2));

	x1 = MAX(x1, MAX(y1, z1));

	return 1 + (int)(x1 / lod_base_cube);
}

//
// R2_GetPlaneLOD
//
int R2_GetPlaneLOD(subsector_t *sub, float h)
{
	// get BBOX of plane
	float x1 = sub->bbox[BOXLEFT]   - viewx;
	float x2 = sub->bbox[BOXRIGHT]  - viewx;
	float y1 = sub->bbox[BOXBOTTOM] - viewy;
	float y2 = sub->bbox[BOXTOP]    - viewy;

	x1 = ((x1<0) != (x2<0)) ? 0 : MIN(fabs(x1), fabs(x2));
	y1 = ((y1<0) != (y2<0)) ? 0 : MIN(fabs(y1), fabs(y2));

	h = MAX(fabs(h - viewz), MAX(x1, y1));

	return 1 + (int)(h / lod_base_cube);
}

