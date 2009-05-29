//----------------------------------------------------------------------------
//  NEW RENDERER
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2008  The EDGE Team.
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
#include "i_defs_gl.h"

#include <math.h>

#include "dm_data.h"
#include "dm_defs.h"
#include "dm_state.h"
#include "g_game.h"
#include "m_bbox.h"
#include "p_local.h"
#include "r_defs.h"
#include "r_misc.h"
#include "r_modes.h"
#include "r_gldefs.h"
#include "r_colormap.h"
#include "r_effects.h"
#include "r_image.h"
#include "r_occlude.h"
#include "r_shader.h"
#include "r_sky.h"
#include "r_things.h"
#include "r_units.h"

#include "n_network.h"  // N_NetUpdate


#define DEBUG  0


// #define DEBUG_GREET_NEIGHBOUR


// -ES- 1999/03/20 Different right & left side clip angles, for asymmetric FOVs.
extern angle_t clip_left, clip_right;
extern angle_t clip_scope;

extern std::list<drawsub_c *> drawsubs;

extern mobj_t *view_cam_mo;

extern int rgl_weapon_r;
extern int rgl_weapon_g;
extern int rgl_weapon_b;

extern float sprite_skew;

extern void RGL_WalkSubsector(subsector_t *sub, bool do_segs);
extern void RGL_DrawSubList(std::list<drawsub_c *> &dsubs);


class drawseg2_c
{
public:
	seg_t *seg;

	angle_t left, right, span;

	float dist;

	// segs which we block
	std::list<drawseg2_c *> occludes;

	// count of segs which block us
	int blockers;

public:
	drawseg2_c(seg_t *_seg) : seg(_seg), occludes(), blockers(0)
	{ }

	~drawseg2_c()
	{ }
};


static std::list<drawseg2_c *> line_set;

static int crap_muck;


static void LineSet_Init(void)
{
	line_set.clear();

	crap_muck = 0;
}

static void LineSet_Done(void)
{
	// nothing needed
}

static void LineSet_AddSeg(seg_t *seg)
{
	// CRAP FOR TESTING !!!
	if (crap_muck)
		return;
	else if (line_set.size() >= 100)
		crap_muck = 1;

	if (seg->miniseg)
		return;

	float sx1 = seg->v1->x;
	float sy1 = seg->v1->y;

	float sx2 = seg->v2->x;
	float sy2 = seg->v2->y;

	// FIXME : mirror stuff

	angle_t angle_L = R_PointToAngle(viewx, viewy, sx1, sy1);
	angle_t angle_R = R_PointToAngle(viewx, viewy, sx2, sy2);

	// Clip to view edges.

	angle_t span = angle_L - angle_R;

	// back side ?
	if (span >= ANG180)
		return;

	angle_L -= viewangle;
	angle_R -= viewangle;

	if (clip_scope != ANG180)
	{
		angle_t tspan1 = angle_L - clip_right;
		angle_t tspan2 = clip_left - angle_R;

		if (tspan1 > clip_scope)
		{
			// Totally off the left edge?
			if (tspan2 >= ANG180)
				return;

			angle_L = clip_left;
		}

		if (tspan2 > clip_scope)
		{
			// Totally off the left edge?
			if (tspan1 >= ANG180)
				return;

			angle_R = clip_right;
		}

		span = angle_L - angle_R;
	}
	
	if (span == 0)
		return;

	// visibility test
	if (RGL_1DOcclusionTest(angle_R, angle_L))
		return;

	drawseg2_c *dseg = new drawseg2_c(seg);

	dseg->left  = angle_L;
	dseg->right = angle_R;
	dseg->span  = span;

	// FIXME: better distance calc
	dseg->dist  = R_PointToDist(viewx, viewy, (sx1+sx2)*0.5, (sy1+sy2)*0.5);

	std::list<drawseg2_c *>::iterator LI;

	for (LI = line_set.begin(); LI != line_set.end(); LI++)
	{
		drawseg2_c *other = *LI;

		if (dseg->dist < other->dist)
			break;
	}

	line_set.insert(LI, dseg);
}

static void LineSet_AddSegList(seg_t *first)
{
	for (; first; first = first->sub_next)
		LineSet_AddSeg(first);
}

static drawseg2_c * LineSet_RemoveFirst(void)
{
	if (line_set.empty())
		return NULL;

	drawseg2_c *dseg = line_set.front();

	line_set.pop_front();

	return dseg;
}

static void RGL_WalkSubsector2(subsector_t *sub)
{
	LineSet_AddSegList(sub->segs);

	sub->rend_seen = framecount;

	RGL_WalkSubsector(sub, false);
}

static void RGL_WalkSeg2(drawseg2_c *dseg)
{
	subsector_t *back = dseg->seg->back_sub;

	if (back && back->rend_seen != framecount)
	{
		RGL_WalkSubsector2(back);
	}

	SYS_ASSERT(dseg->seg->linedef);

	// only one-sided walls affect the 1D occlusion buffer
	if (dseg->seg->linedef->blocked)
	{
		RGL_1DOcclusionSet(dseg->right, dseg->left);
	}
}

static void RGL_WalkLevel(void)
{
	LineSet_Init();

	RGL_WalkSubsector2(viewsubsector);

	for (;;)
	{
		drawseg2_c *dseg = LineSet_RemoveFirst();

		if (! dseg)  // all done?
			break;

		RGL_WalkSeg2(dseg);
	}

	LineSet_Done();
}


static void RGL_RenderNEW(void)
{
	// clear extra light on player's weapon
	rgl_weapon_r = rgl_weapon_g = rgl_weapon_b = 0;

	FUZZ_Update();

	R2_ClearBSP();
	RGL_1DOcclusionClear();

	drawsubs.clear();

	player_t *v_player = view_cam_mo->player;
	
	// handle powerup effects
	RGL_RainbowEffect(v_player);


	if (hom_detect)
	{
		glClearColor(0.0f, 1.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);
	}

	RGL_SetupMatrices3D();

	glClear(GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);

	RGL_BeginSky();

	RGL_WalkLevel();

	RGL_FinishSky();

	RGL_DrawSubList(drawsubs);

//!!!	DoWeaponModel();

	glDisable(GL_DEPTH_TEST);

	// now draw 2D stuff like psprites, and add effects
	RGL_SetupMatrices2D();

	if (v_player)
	{
		RGL_DrawWeaponSprites(v_player);

		RGL_ColourmapEffect(v_player);
		RGL_PaletteEffect(v_player);

		RGL_DrawCrosshair(v_player);
	}

#if (DEBUG >= 3) 
	L_WriteDebug( "\n\n");
#endif
}


static void InitCamera(mobj_t *mo)
{
	leftslope  = M_Tan(leftangle);
	rightslope = M_Tan(rightangle);

	float slopeoffset;

	slopeoffset = M_Tan(FIELDOFVIEW / 2) * aspect_ratio;
	slopeoffset = slopeoffset * viewwindow_h / viewwindow_w;
	slopeoffset = slopeoffset * SCREENWIDTH / SCREENHEIGHT;

	topslope    =  slopeoffset;
	bottomslope = -slopeoffset;


	viewx = mo->x;
	viewy = mo->y;
	viewz = mo->z;
	viewangle = mo->angle;

	if (mo->player)
		viewz += mo->player->viewz;
	else
		viewz += mo->height * 9 / 10;

	viewsubsector = mo->subsector;
	viewvertangle = mo->vertangle;
	view_props = R_PointGetProps(viewsubsector, viewz);

	if (mo->player)
	{
		viewvertangle += M_ATan(mo->player->kick_offset);

		if (! level_flags.mlook)
			viewvertangle = 0;

		// No heads above the ceiling
		if (viewz > mo->player->mo->ceilingz - 2)
			viewz = mo->player->mo->ceilingz - 2;

		// No heads below the floor, please
		if (viewz < mo->player->mo->floorz + 2)
			viewz = mo->player->mo->floorz + 2;
	}


	// do some more stuff
	viewsin = M_Sin(viewangle);
	viewcos = M_Cos(viewangle);

	float lk_sin = M_Sin(viewvertangle);
	float lk_cos = M_Cos(viewvertangle);

	viewforward.x = lk_cos * viewcos;
	viewforward.y = lk_cos * viewsin;
	viewforward.z = lk_sin;

	viewup.x = -lk_sin * viewcos;
	viewup.y = -lk_sin * viewsin;
	viewup.z =  lk_cos;

	// cross product
	viewright.x = viewforward.y * viewup.z - viewup.y * viewforward.z;
	viewright.y = viewforward.z * viewup.x - viewup.z * viewforward.x;
	viewright.z = viewforward.x * viewup.y - viewup.x * viewforward.y;


	// compute the 1D projection of the view angle
	angle_t oned_side_angle;
	{
		float k, d;

		// k is just the mlook angle (in radians)
		k = ANG_2_FLOAT(viewvertangle);
		if (k > 180.0) k -= 360.0;
		k = k * M_PI / 180.0f;

		sprite_skew = tan((-k) / 2.0);

		k = fabs(k);

		// d is just the distance horizontally forward from the eye to
		// the top/bottom edge of the view rectangle.
		d = cos(k) - sin(k) * topslope;

		oned_side_angle = (d <= 0.01f) ? ANG180 : M_ATan(leftslope / d);
	}

	// setup clip angles
	if (oned_side_angle != ANG180)
	{
		clip_left  = 0 + oned_side_angle;
		clip_right = 0 - oned_side_angle;
		clip_scope = clip_left - clip_right;
	}
	else
	{
		// not clipping to the viewport.  Dummy values.
		clip_scope = ANG180;
		clip_left  = 0 + ANG45;
		clip_right = 0 - ANG45;
	}
}


void R_Render(int x, int y, int w, int h, mobj_t *camera)
{
	viewwindow_x = x;
	viewwindow_y = y;
	viewwindow_w = w;
	viewwindow_h = h;

	view_cam_mo = camera;

	// FIXME: SCISSOR TEST


	// Load the details for the camera
	InitCamera(camera);

	// Profiling
	framecount++;
	validcount++;

	RGL_RenderNEW();
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
