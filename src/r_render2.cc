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

extern void InitCamera(mobj_t *mo);


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


static std::list<drawseg2_c *> free_lines;
static std::list<drawseg2_c *> blocked_lines;


static void LineSet_Init(void)
{
	free_lines.clear();
	blocked_lines.clear();
}

static void LineSet_Done(void)
{
	// nothing needed
}

static void LineSet_InsertFree(drawseg2_c *dseg)
{
	std::list<drawseg2_c *>::iterator LI;

	for (LI = free_lines.begin(); LI != free_lines.end(); LI++)
	{
		drawseg2_c *other = *LI;

		if (dseg->dist < other->dist)
			break;
	}

	free_lines.insert(LI, dseg);
}

static inline int LineSet_Overlap(drawseg2_c *A, drawseg2_c *B)
{
	// RETURN:  0 if no overlap
	//         -1 if A is closer than (blocks) B
	//         +1 if A is further than (is blocked by) B

	// subsectors are convex, hence no two segs from the
	// same subsector can overlap
	if (A->seg->front_sub == B->seg->front_sub)
		return 0;
	
	// test angle range
	angle_t L = B->left  - A->right;
	angle_t R = B->right - A->right;

	if (L >= A->span && R >= A->span)
		return 0;

	// test line-line-camera relationship
	divline_t A_div;
	divline_t B_div;

	A_div.x  = A->seg->v1->x;
	A_div.y  = A->seg->v1->y;
	A_div.dx = A->seg->v2->x - A_div.x;
	A_div.dy = A->seg->v2->y - A_div.y;

	int side1 = P_PointOnDivlineSide(B->seg->v1->x, B->seg->v1->y, &A_div);
	int side2 = P_PointOnDivlineSide(B->seg->v2->x, B->seg->v2->y, &A_div);

	if (side1 == side2)
		return (P_PointOnDivlineSide(viewx, viewy, &A_div) == side1) ? +1 : -1;

	B_div.x  = B->seg->v1->x;
	B_div.y  = B->seg->v1->y;
	B_div.dx = B->seg->v2->x - B_div.x;
	B_div.dy = B->seg->v2->y - B_div.y;

	int side3 = P_PointOnDivlineSide(A->seg->v1->x, A->seg->v1->y, &B_div);
	// we assume side4 would be the same

	return (P_PointOnDivlineSide(viewx, viewy, &B_div) == side3) ? -1 : +1;
}

static void LineSet_TestBlocking(drawseg2_c *dseg)
{
	std::list<drawseg2_c *>::iterator LI;

	for (LI = free_lines.begin(); LI != free_lines.end(); LI++)
	{
		drawseg2_c *other = *LI;

		int cmp = LineSet_Overlap(dseg, other);

		if (cmp < 0)
		{
			// the new seg occludes an existing 'free' line,
			// hence it must be moved to the blocked list.
			free_lines.remove(other);
			blocked_lines.push_back(other);

			dseg->occludes.push_back(other);
			other->blockers++;
		}
		else if (cmp > 0)
		{
			// the new seg is occluded
			other->occludes.push_back(dseg);
			dseg->blockers++;
		}
	}

	for (LI = blocked_lines.begin(); LI != blocked_lines.end(); LI++)
	{
		drawseg2_c *other = *LI;

		int cmp = LineSet_Overlap(dseg, other);

		if (cmp < 0)
		{
			dseg->occludes.push_back(other);
			other->blockers++;
		}
		else if (cmp > 0)
		{
			other->occludes.push_back(dseg);
			dseg->blockers++;
		}
	}
}

static void LineSet_AddSeg(seg_t *seg)
{
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

	dseg->dist = R_PointToDist(viewx, viewy, (sx1+sx2)*0.5, (sy1+sy2)*0.5);

	LineSet_TestBlocking(dseg);

	if (dseg->blockers == 0)
		LineSet_InsertFree(dseg);
	else
		blocked_lines.push_back(dseg);
}

static void LineSet_AddSegList(seg_t *first)
{
	for (; first; first = first->sub_next)
		LineSet_AddSeg(first);
}

static drawseg2_c * LineSet_RemoveFirst(void)
{
	if (free_lines.empty())
	{
		SYS_ASSERT(blocked_lines.empty());
		return NULL;
	}

	drawseg2_c *dseg = free_lines.front();

	free_lines.pop_front();

	// removing this seg causes other lines to become unblocked
	std::list<drawseg2_c *>::iterator LI;
	for (LI = dseg->occludes.begin(); LI != dseg->occludes.end(); LI++)
	{
		drawseg2_c *other = *LI;

		SYS_ASSERT(other->blockers >= 1);
		other->blockers--;

		if (other->blockers == 0)
		{
			blocked_lines.remove(other);
			LineSet_InsertFree(other);
		}
	}

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

	// only one-sided walls affect the 1D occlusion buffer
	if (dseg->seg->linedef &&
	    dseg->seg->linedef->blocked)
	{
		RGL_1DOcclusionSet(dseg->right, dseg->left);
	}
	else if (back && back->rend_seen != framecount)
	{
		RGL_WalkSubsector2(back);
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


void R_Render2(int x, int y, int w, int h, mobj_t *camera)
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
