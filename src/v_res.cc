//----------------------------------------------------------------------------
//  EDGE Video Code  
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

#include "i_defs.h"
#include "v_res.h"

#include "am_map.h"
#include "con_cvar.h"
#include "con_defs.h" // BCC Needs to know what funclist_s is.
#include "e_event.h"
#include "dm_defs.h"
#include "dm_state.h"
#include "m_argv.h"
#include "m_fixed.h"
#include "r_draw1.h"
#include "r_draw2.h"
#include "r_plane.h"
#include "r_state.h"
#include "r_things.h"
#include "r_vbinit.h"
#include "st_stuff.h"
#include "v_screen.h"
#include "v_video1.h"
#include "v_video2.h"
#include "z_zone.h"

//
//v_video.c stuff
//
screen_t *main_scr;
screen_t *back_scr;

int SCREENWIDTH;
int SCREENHEIGHT;
int SCREENPITCH;
int SCREENBITS;
bool SCREENWINDOW;
bool graphicsmode = false;

float DX, DY, DXI, DYI, DY2, DYI2;
int SCALEDWIDTH, SCALEDHEIGHT, X_OFFSET, Y_OFFSET;

float BASEYCENTER;
float BASEXCENTER;

// Screen Modes
screenmode_t *scrmode;
int numscrmodes;

static void SetRes(void)
{
	// -ES- 1999/03/04 Removed weird aspect ratio warning - bad ratios don't look awful anymore :-)
	SCALEDWIDTH = (SCREENWIDTH - (SCREENWIDTH % 320));
	SCALEDHEIGHT = 200 * (SCALEDWIDTH / 320);

	// -KM- 1999/01/31 Add specific check for this: resolutions such as 640x350
	//  used to fail.
	if (SCALEDHEIGHT > SCREENHEIGHT)
	{
		SCALEDHEIGHT = (SCREENHEIGHT - (SCREENHEIGHT % 200));
		SCALEDWIDTH = 320 * (SCALEDHEIGHT / 200);
	}

	// -ES- 1999/03/29 Allow very low resolutions
	if (SCALEDWIDTH < 320 || SCALEDHEIGHT < 200)
	{
		SCALEDWIDTH = SCREENWIDTH;
		SCALEDHEIGHT = SCREENHEIGHT;
		X_OFFSET = Y_OFFSET = 0;
	}
	else
	{
		X_OFFSET = (SCREENWIDTH - SCALEDWIDTH) / 2;
		Y_OFFSET = (SCREENHEIGHT - SCALEDHEIGHT) / 2;
	}

	//
	// Weapon Centering
	// Calculates the weapon height, relative to the aspect ratio.
	//
	// Moved here from a #define in r_things.c  -ACB- 1998/08/04
	//
	// -ES- 1999/03/04 Better psprite scaling
	BASEYCENTER = 100;
	BASEXCENTER = 160;

#if 0  // -AJA- This message meaningless at the moment
	// -KM- 1998/07/31 Cosmetic indenting
	I_Printf("  Scaled Resolution: %d x %d\n", SCALEDWIDTH, SCALEDHEIGHT);
#endif

	DX = SCALEDWIDTH / 320.0f;
	DXI = 320.0f / SCALEDWIDTH;
	DY = SCALEDHEIGHT / 200.0f;
	DYI = 200.0f / SCALEDHEIGHT;
	DY2 = DY / 2;
	DYI2 = DYI * 2;

	if (back_scr)
		V_DestroyScreen(back_scr);

	back_scr = V_CreateScreen(SCREENWIDTH, SCREENHEIGHT, BPP);

	ST_ReInit();

	AM_InitResolution();
}

static void SetBPP(void)
{
}

//
// V_InitResolution
// Inits everything resolution-dependent to SCREENWIDTH x SCREENHEIGHT x BPP
//
void V_InitResolution(void)
{
	SetBPP();
	SetRes();
}

//
// V_MultiResInit
//
// Called once at startup to initialise first V_InitResolution
//
// -ACB- 1999/09/19 Removed forcevga reference
//
bool V_MultiResInit(void)
{
	I_Printf("Resolution: %d x %d x %dc\n", SCREENWIDTH, SCREENHEIGHT, 
			1 << SCREENBITS);
	return true;
}

//
// V_AddAvailableResolution
//
// Adds a resolution to the scrmodes list. This is used so we can
// select it within the video options menu.
//
// -ACB- 1999/10/03 Written
//
void V_AddAvailableResolution(screenmode_t *mode)
{
	int i;

	// Unused depth: do not add.
	if (mode->depth != 8 && mode->depth != 16 && mode->depth != 24)
		return;

	L_WriteDebug("V_AddAvailableResolution: Res %d x %d - %dbpp\n", mode->width,
			mode->height, mode->depth);

	if (!scrmode)
	{
		scrmode = Z_New(screenmode_t, 1);
		scrmode[0] = mode[0];
		numscrmodes = 1;
		return;
	}

	// Go through existing list and check width and height do not already exist
	for(i = 0; i < numscrmodes; i++)
	{
		if (scrmode[i].width == mode->width && scrmode[i].height == mode->height &&
				scrmode[i].depth == mode->depth && scrmode[i].windowed == mode->windowed)
			return;

		if ((scrmode[i].width > mode->width || scrmode[i].height > mode->height) &&
				scrmode[i].depth == mode->depth)
			break;
	}

	Z_Resize(scrmode, screenmode_t, numscrmodes+1);

	if (i != numscrmodes)
		Z_MoveData(&scrmode[i+1], &scrmode[i], screenmode_t, numscrmodes - i);

	scrmode[i] = mode[0];
	numscrmodes++;

	return;
}

//
// V_FindClosestResolution
// 
// Finds the closest available resolution to the one specified.
// Returns an index into scrmode[].  The samesize/samedepth will limit
// the search, so -1 is returned if there were no matches.  The search
// only considers modes with the same `windowed' flag.
//
#define DEPTH_MUL  25  // relative important of depth

int V_FindClosestResolution(screenmode_t *mode,
		bool samesize, bool samedepth)
{
	int i;

	int best_idx = -1;
	int best_dist = INT_MAX;

	for (i=0; i < numscrmodes; i++)
	{
		int dw = ABS(scrmode[i].width  - mode->width);
		int dh = ABS(scrmode[i].height - mode->height);
		int dd = ABS(scrmode[i].depth  - mode->depth) * DEPTH_MUL;
		int dist;

		if (scrmode[i].windowed != mode->windowed)
			continue;

		// an exact match is always good...
		if (dw == 0 && dh == 0 && dd == 0)
			return i;

		if (samesize && !(dw == 0 && dh == 0))
			continue;

		if (samedepth && dd != 0)
			continue;

		dist = dw * dw + dh * dh + dd * dd;

		if (dist >= best_dist)
			continue;

		// found a better match
		best_idx = i;
		best_dist = dist;
	}

	return best_idx;
}

//
// V_CompareModes
//
// Returns -1 for less than, 0 if same, or +1 for greater than.
//
int V_CompareModes(screenmode_t *A, screenmode_t *B)
{
	if (A->width < B->width)
		return -1;
	else if (A->width > B->width)
		return +1;

	if (A->height < B->height)
		return -1;
	else if (A->height > B->height)
		return +1;

	if (A->depth < B->depth)
		return -1;
	else if (A->depth > B->depth)
		return +1;

	if (A->windowed < B->windowed)
		return -1;
	else if (A->windowed > B->windowed)
		return +1;

	return 0;
}

