//----------------------------------------------------------------------------
//  EDGE2 Main Rendering Organisation Code
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

#include "system/i_defs.h"
#include "system/i_defs_gl.h"

#include <math.h>

#include "dm_defs.h"
#include "dm_state.h"
#include "e_main.h"
#include "m_misc.h"
#include "n_network.h"
#include "p_local.h"
#include "p_mobj.h"
#include "r_colormap.h"
#include "r_defs.h"
#include "r_draw.h"
#include "r_gldefs.h"
#include "r_misc.h"
#include "r_modes.h"
#include "r_units.h"


cvar_c r_fov;
cvar_c r_zoomfov;

int viewwindow_x;
int viewwindow_y;
int viewwindow_w;
int viewwindow_h;

angle_t viewangle = 0;
angle_t viewvertangle = 0;


vec3_t viewforward;
vec3_t viewup;
vec3_t viewright;

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

mobj_t *background_camera_mo = NULL;

//
// precalculated math tables
//

angle_t viewanglebaseoffset;
angle_t viewangleoffset;

extern float gamma_settings;
extern float fade_gdelta;
extern float fade_gamma;
int fade_starttic;
bool fade_active = false;

int telept_effect = 0;
int telept_flash = 1;
int telept_reverse = 0;
int simple_shadows = 0;

int var_invul_fx;


// e6y
// The precision of the code above is abysmal so use the CRT atan2 function instead!

// FIXME - use of this function should be disabled on architectures with
// poor floating point support! Imagine how slow this would be on ARM, say.

#if 0
angle_t R_PointToAngleEx(float x, float y)
{
	static uint64_t old_y_viewy;
	static uint64_t old_x_viewx;
	static int old_result;

	uint64_t y_viewy = (uint64_t)y - viewy;
	uint64_t x_viewx = (uint64_t)x - viewx;

	if (!render_precise)
	{
		// e6y: here is where "slime trails" can SOMETIMES occur

		if (y_viewy < INT_MAX / 4 && x_viewx < INT_MAX / 4
			&& y_viewy > -INT_MAX / 4 && x_viewx > -INT_MAX / 4)

			return R_PointToAngle(x, y);
	}

	if (old_y_viewy != y_viewy || old_x_viewx != x_viewx)
	{
		old_y_viewy = y_viewy;
		old_x_viewx = x_viewx;

		old_result = (int)((float)atan2((float)y_viewy, (float)x_viewx) * (ANG180 / M_PI));
	}
	return old_result;
}

#endif // 0

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
	return (x == 0) && (y == 0) ? 0 : FLOAT_2_ANG(atan2(y, x) * (180 / M_PI));
}

// faster less precise PointToAngle
angle_t BSP_PointToAngle(float x1, float y1, float x, float y)
{
	float vecx = x - x1;
	float vecy = y - y1;

	if (vecx == 0 && vecy == 0)
	{
		return 0;
	}
	else
	{
		float result = vecy / (fabs(vecx) + fabs(vecy));
		if (vecx < 0)
		{
			result = 2.f - result;
		}
		return (angle_t)(result * (1 << 30));
	}
}

angle_t R_PointToPseudoAngle (double x, double y)
{
  // Note: float won't work here as it's less precise than the BAM values being passed as parameters!
  double vecx = (double)x - viewx;
  double vecy = (double)y - viewy;

  if (vecx == 0 && vecy == 0)
  {
    return 0;
  }
  else
  {
    double result = vecy / (fabs(vecx) + fabs(vecy));
    if (vecx < 0)
    {
      result = 2.0 - result;
    }
    return (angle_t)(result * (1 << 30));
  }
}

//angle_t R_GetVertexViewAngleGL(vertex_t *v)
//{
//  if (v->angletime != framecount)
//  {
//    v->angletime = framecount;
//    v->viewangle = R_PointToPseudoAngle(v->x, v->y);
//  }
//  return v->viewangle;
//}


#if 0
angle_t R_GetVertexViewAngle(vertex_t *v)
{
  if (v->angletime != framecount)
  {
    v->angletime = framecount;
    v->viewangle = R_PointToAngle(v->px, v->py);
  }
  return v->viewangle;
}

#endif // 0

float R_PointToDist(float x1, float y1, float x2, float y2)
{
	return sqrt(pow((x1-x2), 2)+pow((y1-y2), 2)); //pythagorean distance formula. Wow, wasn't that easier?
}



//
// Called once at startup, to initialise some rendering stuff.
//
void R_Init(void)
{
	E_ProgressMessage(language["RefreshDaemon"]);

	framecount = 0;

	// -AJA- 1999/07/01: Setup colour tables.
	V_InitColour();

	RGL_LoadLights();
}


subsector_t *R_PointInSubsector(float x, float y)
{
	node_t *node;
	int side;
	unsigned int nodenum;

	nodenum = root_node;

	while (!(nodenum & NF_V5_SUBSECTOR))
	{
		node = &nodes[nodenum];
		//		side = R_PointOnSide(x, y, &node->div); // we're not ready for this, yet
		side = P_PointOnDivlineSide(x, y, &node->div);
		nodenum = node->children[side];
	}

	return &subsectors[nodenum & ~NF_V5_SUBSECTOR];
}


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

void R_StartFading(int start, int range)
{
	if (range == 0)
		return;
	fade_active = true;
	fade_starttic = start + leveltime;
	fade_gamma = range > 0 ? 0.0f : 1.0f;
	fade_gdelta = 1.0f / range;
}


//----------------------------------------------------------------------------

static std::vector<drawthing_t  *> drawthings;
static std::vector<drawfloor_t  *> drawfloors;
static std::vector<drawseg_c    *> drawsegs;
static std::vector<drawsub_c    *> drawsubs;
static std::vector<drawmirror_c *> drawmirrors;

static int drawthing_pos;
static int drawfloor_pos;
static int drawseg_pos;
static int drawsub_pos;
static int drawmirror_pos;


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
	drawthing_pos  = 0;
	drawfloor_pos  = 0;
	drawseg_pos    = 0;
	drawsub_pos    = 0;
	drawmirror_pos = 0;
}

void R2_FreeupBSP(void)
{
	int i;

	for (i=0; i < (int)drawthings .size(); i++) delete drawthings [i];
	for (i=0; i < (int)drawfloors .size(); i++) delete drawfloors [i];
	for (i=0; i < (int)drawsegs   .size(); i++) delete drawsegs   [i];
	for (i=0; i < (int)drawsubs   .size(); i++) delete drawsubs   [i];
	for (i=0; i < (int)drawmirrors.size(); i++) delete drawmirrors[i];

	drawthings .erase(drawthings .begin(), drawthings .end());
	drawfloors .erase(drawfloors .begin(), drawfloors .end());
	drawsegs   .erase(drawsegs   .begin(), drawsegs   .end());
	drawsubs   .erase(drawsubs   .begin(), drawsubs   .end());
	drawmirrors.erase(drawmirrors.begin(), drawmirrors.end());

	R2_ClearBSP();
}


drawthing_t *R_GetDrawThing()
{
	if (drawthing_pos >= (int)drawthings.size())
	{
		drawthings.push_back(new drawthing_t);
	}

	return drawthings[drawthing_pos++];
}

drawfloor_t *R_GetDrawFloor()
{
	if (drawfloor_pos >= (int)drawfloors.size())
	{
		drawfloors.push_back(new drawfloor_t);
	}

	return drawfloors[drawfloor_pos++];
}

drawseg_c *R_GetDrawSeg()
{
	if (drawseg_pos >= (int)drawsegs.size())
	{
		drawsegs.push_back(new drawseg_c);
	}

	return drawsegs[drawseg_pos++];
}

drawsub_c *R_GetDrawSub()
{
	if (drawsub_pos >= (int)drawsubs.size())
	{
		drawsubs.push_back(new drawsub_c);
	}

	return drawsubs[drawsub_pos++];
}

drawmirror_c *R_GetDrawMirror()
{
	if (drawmirror_pos >= (int)drawmirrors.size())
	{
		drawmirrors.push_back(new drawmirror_c);
	}

	return drawmirrors[drawmirror_pos++];
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
