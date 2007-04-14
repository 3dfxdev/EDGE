//----------------------------------------------------------------------------
//  EDGE Main Rendering Organisation Code
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
// -KM- 1998/09/27 Dynamic Colourmaps
//

#include "i_defs.h"
#include "i_defs_gl.h"
#include "r_main.h"

#include "e_main.h"
#include "gui_main.h"
#include "m_misc.h"
#include "n_network.h"
#include "r_vbinit.h"
#include "r_view.h"
#include "rgl_defs.h"
#include "rgl_unit.h"
#include "p_mobj.h"
#include "p_local.h"
#include "st_stuff.h"
#include "v_ctx.h"
#include "v_colour.h"
#include "r_modes.h"

#include <math.h>


// -ES- 1999/03/14 Dynamic Field Of View
// Fineangles in the viewwidth wide window.
angle_t FIELDOFVIEW = ANG90;

// The used aspect ratio. A normal texel will look aspect_ratio*4/3
// times wider than high on the monitor
static const float aspect_ratio = 200.0f / 320.0f;

// the extreme angles of the view
static angle_t rightangle;
static angle_t leftangle;

float leftslope;
float rightslope;
float topslope;
float bottomslope;

int viewwidth;
int viewheight;
int viewwindowx;
int viewwindowy;
int viewwindowwidth;
int viewwindowheight;

angle_t viewangle = 0;
angle_t viewvertangle = 0;

angle_t normalfov, zoomedfov;
bool viewiszoomed = false;

// increment every time a check is made
int validcount = 1;

// just for profiling purposes
int framecount;
int linecount;

subsector_t *viewsubsector;
region_properties_t *view_props;

float viewx;
float viewy;
float viewz;

float viewcos;
float viewsin;

player_t *viewplayer;

camera_t *camera = NULL;

camera_t *background_camera = NULL;
mobj_t *background_camera_mo = NULL;

//
// precalculated math tables
//

// -ES- 1999/03/20 Different right & left side clip angles, for asymmetric FOVs.
angle_t leftclipangle, rightclipangle;
angle_t clipscope;

angle_t viewanglebaseoffset;
angle_t viewangleoffset;

int telept_starttic;
int telept_active = 0;

//
// R_Render
//
void R_Render(void)
{
	// Load the details for the camera
	// FIXME!! Organise camera handling 
	if (camera)
		R_CallCallbackList(camera->frame_start);

	// do some more stuff
	viewsin = M_Sin(viewangle);
	viewcos = M_Cos(viewangle);

	// Profiling
	framecount++;
	validcount++;
	
	N_NetUpdate();	// check for new console commands.

	RGL_RenderTrueBSP();

	N_NetUpdate();	// Check for new console commands.
}

//
// R_PointToAngle
//
// To get a global angle from cartesian coordinates,
// the coordinates are flipped until they are in
// the first octant of the coordinate system, then
// the y (<=x) is scaled and divided by x to get a
// tangent (slope) value which is looked up in the
// tantoangle[] table.
//
angle_t R_PointToAngle(float x1, float y1, float x, float y)
{
	x -= x1;
	y -= y1;

	if ((x == 0) && (y == 0))
		return 0;

	if (x >= 0)
	{
		// x >=0
		if (y >= 0)
		{
			// y>= 0

			if (x > y)
			{
				// octant 0
				return M_ATan(y / x);
			}
			else
			{
				// octant 1
				return ANG90 - 1 - M_ATan(x / y);
			}
		}
		else
		{
			// y<0
			y = -y;

			if (x > y)
			{
				// octant 8
				// -ACB- 1999/09/27 Fixed MSVC Compiler warning
				return 0 - M_ATan(y / x);
			}
			else
			{
				// octant 7
				return ANG270 + M_ATan(x / y);
			}
		}
	}
	else
	{
		// x<0
		x = -x;

		if (y >= 0)
		{
			// y>= 0
			if (x > y)
			{
				// octant 3
				return ANG180 - 1 - M_ATan(y / x);
			}
			else
			{
				// octant 2
				return ANG90 + M_ATan(x / y);
			}
		}
		else
		{
			// y<0
			y = -y;

			if (x > y)
			{
				// octant 4
				return ANG180 + M_ATan(y / x);
			}
			else
			{
				// octant 5
				return ANG270 - 1 - M_ATan(x / y);
			}
		}
	}
}

//
// R_PointToDist
//
float R_PointToDist(float x1, float y1, float x2, float y2)
{
	angle_t angle;
	float dx;
	float dy;
	float temp;
	float dist;

	dx = (float)fabs(x2 - x1);
	dy = (float)fabs(y2 - y1);

	if (dx == 0.0f)
		return dy;
	else if (dy == 0.0f)
		return dx;

	if (dy > dx)
	{
		temp = dx;
		dx = dy;
		dy = temp;
	}

	angle = M_ATan(dy / dx) + ANG90;

	// use as cosine
	dist = dx / M_Sin(angle);

	return dist;
}

//
// R_SetViewSize
//
// Do not really change anything here,
// because it might be in the middle of a refresh.
//
// The change will take effect next refresh.

bool setsizeneeded;
int set_hud;
bool setresfailed = false;

void R_SetViewSize(int hud)
{
	setsizeneeded = true;

	set_hud = hud;
}

//
// R_ExecuteSetViewSize
//
void R_ExecuteSetViewSize(void)
{
	float slopeoffset;
	int sbar_height;

	setsizeneeded = false;

	sbar_height = FROM_200(ST_HEIGHT);

	viewwidth  = SCREENWIDTH;
	viewheight = SCREENHEIGHT;

	if (set_hud == HUD_Full && ! background_camera_mo)
	{
		viewheight -= sbar_height;
	}

	viewwindowwidth  = viewwidth;
	viewwindowheight = viewheight;
	viewwindowx = 0;
	viewwindowy = 0;

	leftslope = M_Tan(leftangle);
	rightslope = M_Tan(rightangle);

	slopeoffset = M_Tan(FIELDOFVIEW / 2) * aspect_ratio;
	slopeoffset = slopeoffset * viewwindowheight / viewwindowwidth;
	slopeoffset = slopeoffset * SCREENWIDTH / SCREENHEIGHT;

	topslope = slopeoffset;
	bottomslope = -slopeoffset;

	{
		// -AJA- FIXME: cameras should be renewed when starting a new
		//       level (since there will be new mobjs).

		// -AJA- 1999/10/22: background cameras.  This code really sucks
		//       arse, needs improving.
		if (background_camera_mo && !background_camera)
		{
			background_camera = R_CreateCamera();
			R_InitCamera_StdObject(background_camera, background_camera_mo);
			camera = background_camera;
		}

		if (!camera || (background_camera && !background_camera_mo))
		{
			camera = R_CreateCamera();
			R_InitCamera_StdPlayer(camera);
			background_camera = NULL;
		}
	}
}

//
// R_SetFOV
//
// Sets the specified field of view
//
void R_SetFOV(angle_t fov)
{
	// can't change fov to angle below 5 or above 175 deg (approx). Round so
	// that 5 and 175 are allowed for sure.
	if (fov < ANG90 / 18)
		fov = ANG90 / 18;
	if (fov > ((ANG90 + 17) / 18) * 35)
		fov = ANG90 / 18 * 35;

	setsizeneeded = true;

	leftangle   = fov / 2;
	rightangle  = (fov/2) * -1; // -ACB- 1999/09/27 Fixed MSVC Compiler Problem
	FIELDOFVIEW = leftangle - rightangle;
}

void R_SetNormalFOV(angle_t newfov)
{
	menunormalfov = (newfov - ANG45 / 18) / (ANG45 / 9);
	cfgnormalfov = (newfov + ANG45 / 90) / (ANG180 / 180);
	normalfov = newfov;
	if (!viewiszoomed)
		R_SetFOV(normalfov);
}
void R_SetZoomedFOV(angle_t newfov)
{
	menuzoomedfov = (newfov - ANG45 / 18) / (ANG45 / 9);
	cfgzoomedfov = (newfov + ANG45 / 90) / (ANG180 / 180);
	zoomedfov = newfov;
	if (viewiszoomed)
		R_SetFOV(zoomedfov);
}


//
// R_Init
//
// Called once at startup, to initialise some rendering stuff.
//
void R_Init(void)
{
	E_ProgressMessage(language["RefreshDaemon"]);

	R_SetViewSize(screen_hud);
	R_SetNormalFOV((angle_t)(cfgnormalfov * (angle_t)((float)ANG45 / 45.0f)));
	R_SetZoomedFOV((angle_t)(cfgzoomedfov * (angle_t)((float)ANG45 / 45.0f)));
	R_SetFOV(normalfov);

	framecount = 0;

	// -AJA- 1999/07/01: Setup colour tables.
	V_InitColour();

	RGL_LoadLights();
}

//
// R_PointInSubsector
//
subsector_t *R_PointInSubsector(float x, float y)
{
	node_t *node;
	int side;
	unsigned int nodenum;

	nodenum = root_node;

	while (!(nodenum & NF_V5_SUBSECTOR))
	{
		node = &nodes[nodenum];
		side = P_PointOnDivlineSide(x, y, &node->div);
		nodenum = node->children[side];
	}

	return &subsectors[nodenum & ~NF_V5_SUBSECTOR];
}

//
// R_PointGetProps
//
region_properties_t *R_PointGetProps(subsector_t *sub, float z)
{
	extrafloor_t *S, *L, *C;
	float floor_h;

	// traverse extrafloors upwards

	floor_h = sub->sector->f_h;

	S = sub->sector->bottom_ef;
	L = sub->sector->bottom_liq;

	while (S || L)
	{
		if (!L || (S && S->bottom_h < L->bottom_h))
		{
			C = S;  S = S->higher;
		}
		else
		{
			C = L;  L = L->higher;
		}

		SYS_ASSERT(C);

		// ignore liquids in the middle of THICK solids, or below real
		// floor or above real ceiling
		//
		if (C->bottom_h < floor_h || C->bottom_h > sub->sector->c_h)
			continue;

		if (z < C->top_h)
			return C->p;

		floor_h = C->top_h;
	}

	// extrafloors were exhausted, must be top area
	return sub->sector->p;
}

int telept_effect = 0;
int telept_flash = 1;
int telept_reverse = 0;

void R_StartFading(int start, int range)
{
	telept_active = true;
	telept_starttic = start + leveltime;
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
