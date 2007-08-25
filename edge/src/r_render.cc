//----------------------------------------------------------------------------
//  EDGE OpenGL Rendering (BSP Traversal)
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2007  The EDGE Team.
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
// TODO HERE:
//   * use IM_WIDTH(),IM_BOTTOM(),etc where needed.
//   + optimise first subsector: ignore floors out of view.
//   + split up: rgl_seg.c and rgl_mobj.c.
//   + handle scaling better.
//

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
#include "r_gldefs.h"
#include "r_occlude.h"
#include "r_effects.h"
#include "r_sky.h"
#include "r_things.h"
#include "r_units.h"
#include "r_image.h"
#include "r_view.h"
#include "r_colors.h"

#include "n_network.h"  // N_NetUpdate

#include "r_texgl.h" ///!!!!
#include "epi/image_data.h"

#define DEBUG  0

#define FLOOD_DIST    1024.0f
#define FLOOD_EXPAND  128.0f

#define DLIGHT_PROTOTYPE  1

// #define DEBUG_GREET_NEIGHBOUR



side_t *sidedef;
line_t *linedef;
sector_t *frontsector;
sector_t *backsector;

unsigned int root_node;

extern camera_t *camera;


// -ES- 1999/03/20 Different right & left side clip angles, for asymmetric FOVs.
angle_t clip_left, clip_right;
angle_t clip_scope;


static int checkcoord[12][4] =
{
  {BOXRIGHT, BOXTOP, BOXLEFT, BOXBOTTOM},
  {BOXRIGHT, BOXTOP, BOXLEFT, BOXTOP},
  {BOXRIGHT, BOXBOTTOM, BOXLEFT, BOXTOP},
  {0},
  {BOXLEFT, BOXTOP, BOXLEFT, BOXBOTTOM},
  {0},
  {BOXRIGHT, BOXBOTTOM, BOXRIGHT, BOXTOP},
  {0},
  {BOXLEFT, BOXTOP, BOXRIGHT, BOXBOTTOM},
  {BOXLEFT, BOXBOTTOM, BOXRIGHT, BOXBOTTOM},
  {BOXLEFT, BOXBOTTOM, BOXRIGHT, BOXTOP}
};



// colour of the player's weapon
extern int rgl_weapon_r;
extern int rgl_weapon_g;
extern int rgl_weapon_b;

extern float sprite_skew;

// common stuff

extern sector_t *frontsector;
extern sector_t *backsector;

static subsector_t *cur_sub;
static seg_t *cur_seg;

static bool solid_mode;

static std::list<drawsub_c *> drawsubs;

#ifdef SHADOW_PROTOTYPE
static const image_c *shadow_image = NULL;
#endif


static GLuint glow_tex = 0;





// ========= MIRROR STUFF ===========

#define MAX_MIRRORS  3

static inline void ClipPlaneHorizontalLine(GLdouble *p, const vec2_t& s,
	const vec2_t& e)
{
	p[0] = e.y - s.y;
	p[1] = s.x - e.x;
	p[2] = 0.0f;
	p[3] = e.x * s.y - s.x * e.y;
}

static inline void ClipPlaneEyeAngle(GLdouble *p, angle_t ang)
{
	vec2_t s, e;

	s.Set(viewx, viewy);

	e.Set(viewx + M_Cos(ang), viewy + M_Sin(ang));

	ClipPlaneHorizontalLine(p, s, e);
}


typedef struct
{
	drawmirror_c *def;

	float xc, xx, xy;  // x' = xc + x*xx + y*xy
	float yc, yx, yy;  // y' = yc + x*yx + y*yy

public:
	void ComputeMirror()
	{
		seg_t *seg = def->seg;

		float sdx = seg->v2->x - seg->v1->x;
		float sdy = seg->v2->y - seg->v1->y;

		float len_p2 = seg->length * seg->length;

		float A = (sdx * sdx - sdy * sdy) / len_p2;
		float B = (sdx * sdy * 2.0)       / len_p2;

		xx = A; xy =  B;
		yx = B; yy = -A;

		xc = seg->v1->x * (1.0-A) - seg->v1->y * B;
		yc = seg->v1->y * (1.0+A) - seg->v1->x * B;
	}

	void Flip(float& x, float& y)
	{
		float tx = x, ty = y;

		x = xc + tx*xx + ty*xy;
		y = yc + tx*yx + ty*yy;
	}
}
mirror_info_t;

static mirror_info_t active_mirrors[MAX_MIRRORS];

int num_active_mirrors = 0;


void MIR_Coordinate(float& x, float& y)
{
	for (int i=num_active_mirrors-1; i >= 0; i--)
		active_mirrors[i].Flip(x, y);
}

static void MIR_SetClippers()
{
	glDisable(GL_CLIP_PLANE0);
	glDisable(GL_CLIP_PLANE1);
	glDisable(GL_CLIP_PLANE2);
	glDisable(GL_CLIP_PLANE3);
	glDisable(GL_CLIP_PLANE4);
	glDisable(GL_CLIP_PLANE5);

	if (num_active_mirrors == 0)
		return;

	// setup planes for left and right sides of innermost mirror.
	// Angle clipping has ensured that for multiple mirrors all
	// later mirrors are limited to the earlier mirrors.

	mirror_info_t& inner = active_mirrors[num_active_mirrors-1];

	GLdouble  left_p[4];
	GLdouble right_p[4];

	ClipPlaneEyeAngle(left_p,  inner.def->left);
	ClipPlaneEyeAngle(right_p, inner.def->right + ANG180);

  	glEnable(GL_CLIP_PLANE0);
   	glEnable(GL_CLIP_PLANE1);

	glClipPlane(GL_CLIP_PLANE0, left_p);
  	glClipPlane(GL_CLIP_PLANE1, right_p);


	// now for each mirror, setup a clip plane that removes
	// everything that gets projected in front of that mirror.

	for (int i=0; i < num_active_mirrors; i++)
	{
		mirror_info_t& mir = active_mirrors[i];

		vec2_t v1, v2;

		v1.Set(mir.def->seg->v1->x, mir.def->seg->v1->y);
		v2.Set(mir.def->seg->v2->x, mir.def->seg->v2->y);

		for (int k = i-1; k >= 0; k--)
		{
			vec2_t tmp; tmp = v1; v1 = v2; v2 = tmp;

			active_mirrors[k].Flip(v1.x, v1.y);
			active_mirrors[k].Flip(v2.x, v2.y);
		}
		
		GLdouble front_p[4];

		ClipPlaneHorizontalLine(front_p, v2, v1);

		glEnable(GL_CLIP_PLANE2 + i);

		glClipPlane(GL_CLIP_PLANE2 + i, front_p);
	}
}

static bool MIR_Push(drawmirror_c *mir)
{
	SYS_ASSERT(mir);
	SYS_ASSERT(mir->seg);

	if (num_active_mirrors == MAX_MIRRORS)
		return false;

	active_mirrors[num_active_mirrors].def = mir;
	active_mirrors[num_active_mirrors].ComputeMirror();

	num_active_mirrors++;

	MIR_SetClippers();

	return true;
}

static void MIR_Pop()
{
	SYS_ASSERT(num_active_mirrors > 0);

	num_active_mirrors--;

	MIR_SetClippers();
}


// ============================================================================
// R2_BSP START
// ============================================================================

int detail_level = 1;
int use_dlights = 0;

int doom_fading = 1;


//
// R2_FindDLights
//
static void R2_FindDLights(subsector_t *sub, drawfloor_t *dfloor)
{
	//!!!!!
	float max_dlight_radius = 4*200.0f; // (use_dlights == 1) ? 300.0f : 200.0f;

	int xl, xh, yl, yh;
	int bx, by;

	xl = BLOCKMAP_GET_X(sub->bbox[BOXLEFT]   - max_dlight_radius);
	xh = BLOCKMAP_GET_X(sub->bbox[BOXRIGHT]  + max_dlight_radius);
	yl = BLOCKMAP_GET_Y(sub->bbox[BOXBOTTOM] - max_dlight_radius);
	yh = BLOCKMAP_GET_Y(sub->bbox[BOXTOP]    + max_dlight_radius);

	for (bx = xl; bx <= xh; bx++)
	for (by = yl; by <= yh; by++)
	{
		mobj_t *mo;
		drawthing_t *dl;

		if (bx < 0 || by < 0 || bx >= bmapwidth || by >= bmapheight)
			continue;

		for (mo=blocklights[by * bmapwidth + bx]; mo; mo = mo->dlnext)
		{
			SYS_ASSERT(mo->state);

			if (mo->state->bright <= 0 || mo->dlight[0].r <= 0)
				continue;

			if (mo->ceilingz <= dfloor->f_h || mo->floorz >= dfloor->top_h)
			{
				continue;
			}

			dl = R_GetDrawThing();
			dl->Clear();

			dl->mo = mo;
			dl->tz = mo->z + mo->height * PERCENT_2_FLOAT(mo->info->dlight0.height);

			dl->next = dfloor->dlights;
			dl->prev = NULL;  // NOTE: not used (singly linked)

			dfloor->dlights = dl;
		}
	}
}

// ============================================================================
// R2_BSP END
// ============================================================================


#if 0  // DISABLED FOR TIME BEING
static void ClipPlaneHorizontalEye(GLdouble *p, const vec2_t *v, bool flip)
{
	vec2_t s, e;

	s.x = viewx;
	s.y = viewy;

	e.x = v->x;
	e.y = v->y;

	ClipPlaneHorizontalLine(p, s, e, flip);
}

static void ClipPlaneVerticalEye(GLdouble *p, const seg_t *seg,
	float h, bool flip)
{
	float x1 = seg->v1->x - viewx;
	float y1 = seg->v1->y - viewy;
	float z1 = (h - viewz);

	float x2 = seg->v2->x - viewx;
	float y2 = seg->v2->y - viewy;
	float z2 = z1;

	// compute plane normal by using the Cross Product

	p[0] = y2*z1 - y1*z2;
	p[1] = z2*x1 - z1*x2;
	p[2] = x2*y1 - x1*y2;
	p[3] = -(viewx * p[0] + viewy * p[1] + viewz * p[2]);

	if (flip)
	{
		p[0] = 0.0f - p[0];
		p[1] = 0.0f - p[1];
		p[2] = 0.0f - p[2];
		p[3] = 0.0f - p[3];
	}
}
#endif

static void FloodSetClipPlanes(const seg_t *seg, float h1, float h2)
{
#if 0
	GLdouble left_p[4];
	GLdouble right_p[4];
	GLdouble bottom_p[4];
	GLdouble top_p[4];

	glEnable(GL_CLIP_PLANE0);  // Left
	glEnable(GL_CLIP_PLANE1);  // Right
	glEnable(GL_CLIP_PLANE2);  // Bottom
	glEnable(GL_CLIP_PLANE3);  // Top

	ClipPlaneHorizontalEye(left_p,  seg->v1, false);
	ClipPlaneHorizontalEye(right_p, seg->v2, true);

	ClipPlaneVerticalEye(bottom_p, seg, h1, false);
	ClipPlaneVerticalEye(top_p, seg, h2, true);

	glClipPlane(GL_CLIP_PLANE0, left_p);
	glClipPlane(GL_CLIP_PLANE1, right_p);
	glClipPlane(GL_CLIP_PLANE2, bottom_p);
	glClipPlane(GL_CLIP_PLANE3, top_p);
#endif
}

static void FloodResetClipPlanes(void)
{
#if 0
	glDisable(GL_CLIP_PLANE0);
	glDisable(GL_CLIP_PLANE1);
	glDisable(GL_CLIP_PLANE2);
	glDisable(GL_CLIP_PLANE3);
#endif
}


//
// LIGHTING RULES:
//
//   1. N/S/E/W changes and infrared are applied before darkening.
//   2. extralight and dlights are applied after darkening.
//   3. for gamma, we rely on the gamma function g() having the
//      property: g(X * Y) = g(X) * g(Y).
//

int RGL_Light(int nominal)
{
	SYS_ASSERT(currmap);

	if (currmap->lighting <= LMODEL_Doomish)
	{
		nominal = rgl_light_map[MIN(255, nominal)];
	}

	nominal += ren_extralight;

	return GAMMA_CONV(MIN(255, nominal));
}


typedef struct wall_plane_data_s
{
	vec3_t normal;
	divline_t div;

	int light;
	int col[3];
	float trans;

	int cmx;

	drawthing_t *dlights;
	const image_c *image;

	float tx, tdx;
	float ty, ty_mul, ty_skew;

	vec2_t x_mat, y_mat;

	bool flood_emu;
	float emu_mx, emu_my;
}
wall_plane_data_t;


#if 0
static inline void Color_Std(local_gl_vert_t *v, int R, int G, int B, float alpha)
{
	v->col[0] = R / 255.0;
	v->col[1] = G / 255.0;
	v->col[2] = B / 255.0;
	v->col[3] = alpha;
}

static inline void Color_Rainbow(local_gl_vert_t *v, int R, int G, int B, float alpha)
{
	v->col[0] = MIN(1.0, R * ren_red_mul / 255.0);
	v->col[1] = MIN(1.0, G * ren_grn_mul / 255.0);
	v->col[2] = MIN(1.0, B * ren_blu_mul / 255.0);
	v->col[3] = alpha;
}

static inline void Color_Dimmed(local_gl_vert_t *v, int R, int G, int B, float mul)
{
	v->col[0] = mul * R / 255.0;
	v->col[1] = mul * G / 255.0;
	v->col[2] = mul * B / 255.0;
	v->col[3] = 1.0;
}

static inline void Color_White(local_gl_vert_t *v)
{
	v->col[0] = 1.0;
	v->col[1] = 1.0;
	v->col[2] = 1.0;
	v->col[3] = 1.0;
}

static inline void Color_Black(local_gl_vert_t *v)
{
	v->col[0] = 0.0;
	v->col[1] = 0.0;
	v->col[2] = 0.0;
	v->col[3] = 1.0;
}

static inline void Vertex_Std(local_gl_vert_t *v, const vec3_t *src, GLboolean edge)
{
	v->x = src->x;
	v->y = src->y;
	v->z = src->z;

	v->edge = edge;
}

static inline void Normal_Std(local_gl_vert_t *v, float nx, float ny, float nz)
{
	v->nx = nx;
	v->ny = ny;
	v->nz = nz;
}

static inline void TexCoord_Wall(local_gl_vert_t *v, int t,
		const divline_t *div, float tx0, float ty0,
		float tx_mul, float ty_mul)
{
	float along;

	if (fabs(div->dx) > fabs(div->dy))
	{
		SYS_ASSERT(0 != div->dx);
		along = (v->x - div->x) / div->dx;
	}
	else
	{
		SYS_ASSERT(0 != div->dy);
		along = (v->y - div->y) / div->dy;
	}

	v->s[t] = tx0 + along * tx_mul;
	v->t[t] = ty0 + v->z  * ty_mul;
}

static inline void TexCoord_Plane(local_gl_vert_t *v, int t,
		float tx0, float ty0, float image_w, float image_h,
		const vec2_t *x_mat, const vec2_t *y_mat)
{
	float rx = (tx0 + v->x) / image_w;
	float ry = (ty0 + v->y) / image_h;

	v->s[t] = rx * x_mat->x + ry * x_mat->y;
	v->t[t] = rx * y_mat->x + ry * y_mat->y;
}


static inline void TexCoord_Shadow(local_gl_vert_t *v, int t)
{
#if 0
	float rx = (v->x + data->tx);
	float ry = (v->y + data->ty);

	v->s[t] = rx * data->x_mat.x + ry * data->x_mat.y;
	v->t[t] = rx * data->y_mat.x + ry * data->y_mat.y;
#endif
}

static inline void TexCoord_FloorGlow(local_gl_vert_t *v, int t, float f_h)
{
	v->s[t] = 0.5;
	v->t[t] = (v->z - f_h) / 64.0;
}

static inline void TexCoord_CeilGlow(local_gl_vert_t *v, int t, float c_h)
{
	v->s[t] = 0.5;
	v->t[t] = (c_h - v->z) / 64.0;
}

static inline void TexCoord_WallGlow(local_gl_vert_t *v, int t,
		float x1, float y1, float nx, float ny)
{
	float dist = (v->x - x1) * nx + (v->y - y1) * ny;

	v->s[t] = 0.5;
	v->t[t] = dist / 192.0;
}

static inline void TexCoord_WallLight(local_gl_vert_t *v, int t)
{
}

static inline void TexCoord_PlaneLight(local_gl_vert_t *v, int t)
{
}
#endif


typedef struct
{
	const vec3_t *vert;

	float R, G, B;

	divline_t div;

	float tx0, ty0;
	float tx_mul, ty_mul;

	vec3_t normal;
}
wall_coord_data_t;


static void WallCoordFunc(void *d, int v_idx,
		vec3_t *pos, float *rgb, vec2_t *texc,
		vec3_t *normal, vec3_t *lit_pos)
{
	const wall_coord_data_t *data = (wall_coord_data_t *)d;

	*pos    = data->vert[v_idx];
	*normal = data->normal;

	rgb[0] = data->R;
	rgb[1] = data->G;
	rgb[2] = data->B;

	float along;

	if (fabs(data->div.dx) > fabs(data->div.dy))
	{
		along = (pos->x - data->div.x) / data->div.dx;
	}
	else
	{
		along = (pos->y - data->div.y) / data->div.dy;
	}

	texc->x = data->tx0 + along  * data->tx_mul;
	texc->y = data->ty0 + pos->z * data->ty_mul;

	*lit_pos = *pos;
}


typedef struct
{
	const vec3_t *vert;

	float R, G, B;

	float tx0, ty0;
	float image_w, image_h;

	vec2_t x_mat;
	vec2_t y_mat;

	vec3_t normal;
}
plane_coord_data_t;


static void PlaneCoordFunc(void *d, int v_idx,
		vec3_t *pos, float *rgb, vec2_t *texc,
		vec3_t *normal, vec3_t *lit_pos)
{
	const plane_coord_data_t *data = (plane_coord_data_t *)d;

	*pos    = data->vert[v_idx];
	*normal = data->normal;

	rgb[0] = data->R;
	rgb[1] = data->G;
	rgb[2] = data->B;

	float rx = (data->tx0 + pos->x) / data->image_w;
	float ry = (data->ty0 + pos->y) / data->image_h;

	texc->x = rx * data->x_mat.x + ry * data->x_mat.y;
	texc->y = rx * data->y_mat.x + ry * data->y_mat.y;

	*lit_pos = *pos;
}


///---void ShadowCoordFunc(vec3_t *src, local_gl_vert_t *vert, void *d)
///---{
///---	wall_plane_data_t *data = (wall_plane_data_t *)d;
///---
///---	float x = src->x;
///---	float y = src->y;
///---	float z = src->z;
///---
///---
///---	float tx = rx * data->x_mat.x + ry * data->x_mat.y;
///---	float ty = rx * data->y_mat.x + ry * data->y_mat.y;
///---
///---	int R = 0;
///---	int G = 0;
///---	int B = 0;
///---
///---	SET_COLOR(LT_RED(R), LT_GRN(G), LT_BLU(B), data->trans);
///---	SET_TEXCOORD(tx, ty);
///---	SET_NORMAL(data->normal.x, data->normal.y, data->normal.z);
///---	SET_EDGE_FLAG(GL_TRUE);
///---	SET_VERTEX(x, y, z);
///---}

#ifdef DLIGHT_PROTOTYPE

//!!!! FIXME big hack crap
static int dl_R, dl_G, dl_B;
static wall_plane_data_t *dl_WP;

#if 0
void DLightWallCoordFunc(vec3_t *src, local_gl_vert_t *vert, void *d)
{
	wall_plane_data_t *data = (wall_plane_data_t *)d;

	float x = src->x;
	float y = src->y;
	float z = src->z;

	float along;

	// compute texture coord
	if (fabs(data->div.dx) > fabs(data->div.dy))
	{
		SYS_ASSERT(0 != data->div.dx);
		along = (x - data->div.x) / data->div.dx;
	}
	else
	{
		SYS_ASSERT(0 != data->div.dy);
		along = (y - data->div.y) / data->div.dy;
	}

	float tx = data->tx + along * data->tdx;
	float ty = data->ty + z * data->ty_mul + along * data->ty_skew;

	// compute texture coord
	if (fabs(dl_WP->div.dx) > fabs(dl_WP->div.dy))
	{
		SYS_ASSERT(0 != dl_WP->div.dx);
		along = (x - dl_WP->div.x) / dl_WP->div.dx;
	}
	else
	{
		SYS_ASSERT(0 != dl_WP->div.dy);
		along = (y - dl_WP->div.y) / dl_WP->div.dy;
	}
	float tx0 = dl_WP->tx + along * dl_WP->tdx;
	float ty0 = dl_WP->ty + z * dl_WP->ty_mul + along * dl_WP->ty_skew;

	int R = int(dl_R * data->trans);
	int G = int(dl_G * data->trans);
	int B = int(dl_B * data->trans);

	SET_COLOR(LT_RED(R), LT_GRN(G), LT_BLU(B), 1.0f);
	SET_TEXCOORD(tx0, ty0);
	SET_TEX2COORD(tx, ty);
	SET_NORMAL(data->normal.x, data->normal.y, data->normal.z);
	SET_EDGE_FLAG(GL_TRUE);
	SET_VERTEX(x, y, z);
}

void DLightPlaneCoordFunc(vec3_t *src, local_gl_vert_t *vert, void *d)
{
	wall_plane_data_t *data = (wall_plane_data_t *)d;

	float x = src->x;
	float y = src->y;
	float z = src->z;

	float rx = (x + data->tx);
	float ry = (y + data->ty);
	float tx = rx * data->x_mat.x + ry * data->x_mat.y;
	float ty = rx * data->y_mat.x + ry * data->y_mat.y;

	float rx0 = (x + dl_WP->tx) / IM_WIDTH(dl_WP->image);
	float ry0 = (y + dl_WP->ty) / IM_HEIGHT(dl_WP->image);
	float tx0 = rx0 * dl_WP->x_mat.x + ry0 * dl_WP->x_mat.y;
	float ty0 = rx0 * dl_WP->y_mat.x + ry0 * dl_WP->y_mat.y;

	int R = int(dl_R * data->trans);
	int G = int(dl_G * data->trans);
	int B = int(dl_B * data->trans);

	SET_COLOR(LT_RED(R), LT_GRN(G), LT_BLU(B), 1.0f);
	SET_TEXCOORD(tx0, ty0);
	SET_TEX2COORD(tx, ty);
	SET_NORMAL(data->normal.x, data->normal.y, data->normal.z);
	SET_EDGE_FLAG(GL_TRUE);
	SET_VERTEX(x, y, z);
}
#endif

static void ComputeDLParameters(float dist, mobj_t *mo,
	float *radius, float *intensity)
{
	*radius = mo->dlight[0].r;

	*intensity = 0;

	if (dist > mo->dlight[0].r)
		return;

	dist /= mo->dlight[0].r;
	dist = fabs(dist);  // needed???

	*intensity = exp(-5.44*dist);

#if 0
	if (mo->info->dlight0.type == DLITE_Linear)
	{
		*radius = DL_OUTER * dist;
		*intensity = mo->dlight[0].r / 8.0f / dist;

		if (*intensity > 1.0f)
		{
			*radius *= (*intensity);
			*intensity = 1.0f;
		}
	}
	else  /* DLITE_Quadratic */
	{
  		*radius = DL_OUTER_SQRT * dist;
  		*intensity = mo->dlight[0].r * 2.0f / dist / dist;
  
  		if (*intensity > 1.0f)
  		{
  			*radius *= sqrt(*intensity);
  			*intensity = 1.0f;
  		}
	}
#endif
}

#endif // DLIGHT_PROTOTYPE


#define MAX_EDGE_VERT  20

static inline void GreetNeighbourSector(float *hts, int& num,
		vertex_seclist_t *seclist)
{
	if (! seclist)
		return;

	for (int k=0; k < (seclist->num * 2); k++)
	{
		sector_t *sec = sectors + seclist->sec[k/2];

		float h = (k & 1) ? sec->c_h : sec->f_h;

		// does not intersect current height range?
		if (h <= hts[0]+0.1 || h >= hts[num-1]-0.1)
			continue;

		// check if new height already present, and at same time
		// find place to insert new height.
		
		int pos;

		for (pos = 1; pos < num; pos++)
		{
			if (h < hts[pos] - 0.1)
				break;

			if (h < hts[pos] + 0.1)
			{
				pos = -1; // already present
				break;
			}
		}

		if (pos > 0 && pos < num)
		{
			for (int i = num; i > pos; i--)
				hts[i] = hts[i-1];

			hts[pos] = h;

			num++;

			if (num >= MAX_EDGE_VERT)
				return;
		}
	}
}


//
// RGL_DrawWall
//
// Note: mid_masked is 2 & 3 for horiz. sliding door.
//
static void RGL_DrawWall(drawfloor_t *dfloor, float top,
						 float bottom, float tex_top_h, wall_tile_t *wt,
						 int mid_masked, bool opaque, float x_offset,
						 region_properties_t *masked_props)

{
	surface_t *surf = wt->surface;

	float x1 = cur_seg->v1->x;
	float y1 = cur_seg->v1->y;
	float x2 = cur_seg->v2->x;
	float y2 = cur_seg->v2->y;

	MIR_Coordinate(x1, y1);
	MIR_Coordinate(x2, y2);

	if (num_active_mirrors % 2)
	{
		float tx = x1; x1 = x2; x2 = tx;
		float ty = y1; y1 = y2; y2 = ty;
	}

	float xy_ofs = cur_seg->offset;
	float xy_len = cur_seg->length;

	float tex_x1 = 0;
	float tex_x2 = 0;

	float trans = surf->translucency;
	bool blended;



	region_properties_t *props = masked_props ? masked_props :
		surf->override_p ? surf->override_p : dfloor->props;

	R_ColmapPipe_SetProps(props);


	// ignore non-solid walls in solid mode (& vice versa)
	blended = (surf->translucency <= 0.99f) ? true : false;
	if ((blended || mid_masked) == solid_mode)
		return;

	SYS_ASSERT(currmap);

	int lit_adjust = 0;

	// do the N/S/W/E bizzo...
	if (currmap->lighting == LMODEL_Doom && props->lightlevel > 0)
	{
		if (cur_seg->v1->y == cur_seg->v2->y)
			lit_adjust -= 16;
		else if (cur_seg->v1->x == cur_seg->v2->x)
			lit_adjust += 16;
	}


	SYS_ASSERT(surf->image);

	GLuint tex_id = W_ImageCache(surf->image);


	x_offset += xy_ofs;

	tex_x1 = x_offset;
	tex_x2 = tex_x1 + xy_len;

	// horizontal sliding door hack
	if (mid_masked >= 2)
	{
		slider_move_t *smov = cur_seg->linedef->slider_move;
		line_t *L;

		float start, end;
		float seg_start, seg_end;

		if (cur_seg->side == 0)
		{
			seg_start = xy_ofs;
			seg_end   = seg_start + xy_len;
		}
		else
		{
			seg_end   = smov->line_len - xy_ofs;
			seg_start = seg_end - xy_len;
		}

		SYS_ASSERT(smov);

		switch (smov->info->type * 2 + (mid_masked & 1))
		{
			case (SLIDE_Left * 2 + 0):
				start = 0;
				end = smov->line_len - smov->opening;
				break;

			case (SLIDE_Right * 2 + 0):
				start = smov->opening;
				end = smov->line_len;
				break;

			case (SLIDE_Center * 2 + 0):
				start = 0;
				end = (smov->line_len - smov->opening) / 2;
				break;

			case (SLIDE_Center * 2 + 1):
				start = (smov->line_len + smov->opening) / 2;
				end = smov->line_len;
				break;

			default:
				return;
		}

		start = MAX(start, seg_start);
		end = MIN(end, seg_end);

		// no overlap ?
		if (end <= start + 1)
			return;

		L = cur_seg->linedef;
		SYS_ASSERT(smov->line_len > 0);

		x1 = L->v1->x + L->dx * start / smov->line_len;
		y1 = L->v1->y + L->dy * start / smov->line_len;

		x2 = L->v1->x + L->dx * end / smov->line_len;
		y2 = L->v1->y + L->dy * end / smov->line_len;

		switch (smov->info->type * 2 + (mid_masked & 1))
		{
		case (SLIDE_Left * 2 + 0):
			tex_x1 = seg_start + smov->opening;
			tex_x2 = seg_end   + smov->opening;
			break;

		case (SLIDE_Right * 2 + 0):
			tex_x1 = seg_start - smov->opening;
			tex_x2 = seg_end   - smov->opening;
			break;

		case (SLIDE_Center * 2 + 0):
			tex_x1 = seg_start + smov->opening/2;
			tex_x2 = seg_end   + smov->opening/2;
			break;

		case (SLIDE_Center * 2 + 1):
			tex_x1 = seg_start - smov->opening/2;
			tex_x2 = seg_end   - smov->opening/2;
			break;
		}

		if (cur_seg->side == 1)
		{
			float tex_tmp = tex_x1;
			tex_x1 = tex_x2;
			tex_x2 = tex_tmp;
		}
	}
	else if (mid_masked == 1 && cur_seg->linedef->special &&
		cur_seg->linedef->special->s.type != SLIDE_None)
	{
		if (cur_seg->side == 1)
		{
			tex_x1 = IM_WIDTH(surf->image) - tex_x1;
			tex_x2 = IM_WIDTH(surf->image) - tex_x2;
		}
	}

	float total_w = IM_TOTAL_WIDTH( surf->image);
	float total_h = IM_TOTAL_HEIGHT(surf->image);

	total_w *= surf->x_mat.x;
	total_h *= surf->y_mat.y;

	tex_x1 = tex_x1 / total_w;
	tex_x2 = tex_x2 / total_w;

//??	if (num_active_mirrors % 2)
//??	{
//??		float t = tex_x1; tex_x1 = tex_x2; tex_x2 = t;
//??	}

	float tx0    = tex_x1;
	float tx_mul = tex_x2 - tex_x1;

	float ty_mul = 1.0f / total_h;
	float ty0    = IM_TOP(surf->image) - tex_top_h * ty_mul;

#if (DEBUG >= 3) 
	L_WriteDebug( "WALL (%d,%d,%d) -> (%d,%d,%d)\n", 
		(int) x1, (int) y1, (int) top, (int) x2, (int) y2, (int) bottom);
#endif


	// -AJA- 2007/08/07: ugly code here ensures polygon edges
	//       match up with adjacent linedefs (otherwise small
	//       gaps can appear which look bad).

	float  left_h[MAX_EDGE_VERT]; int  left_num=2;
	float right_h[MAX_EDGE_VERT]; int right_num=2;

	left_h[0]  = bottom; left_h[1]  = top;
	right_h[0] = bottom; right_h[1] = top;

	if (solid_mode) /// && ! (wt->flags & WTILF_IsExtra)
	{
		GreetNeighbourSector(left_h,  left_num,  cur_seg->nb_sec[0]);
		GreetNeighbourSector(right_h, right_num, cur_seg->nb_sec[1]);

#if DEBUG_GREET_NEIGHBOUR
		SYS_ASSERT(left_num  <= MAX_EDGE_VERT);
		SYS_ASSERT(right_num <= MAX_EDGE_VERT);

		for (int k = 0; k < MAX_EDGE_VERT; k++)
		{
			if (k+1 < left_num)
			{
				SYS_ASSERT(left_h[k]  <= left_h[k+1]);
			}
			if (k+1 < right_num)
			{
				SYS_ASSERT(right_h[k] <= right_h[k+1]);
			}
		}
#endif
	}

	vec3_t vertices[MAX_EDGE_VERT * 2];

	int v_count = 0;

	for (int LI = 0; LI < left_num; LI++)
	{
		vertices[v_count].x = x1;
		vertices[v_count].y = y1;
		vertices[v_count].z = left_h[LI];
		v_count++;
	}

	for (int RI = right_num-1; RI >= 0; RI--)
	{
		vertices[v_count].x = x2;
		vertices[v_count].y = y2;
		vertices[v_count].z = right_h[RI];
		v_count++;
	}

	int blending = (blended ? BL_Alpha : 0) | (mid_masked ? BL_Masked : 0);

	// -AJA- 2006-06-22: fix for midmask wrapping bug
	if (mid_masked == 1)
		blending |= BL_ClampY;


	wall_coord_data_t data;

	data.vert = vertices;

	data.R = data.G = data.B = 1.0f;

	data.div.x  = x1;
	data.div.y  = y1;
	data.div.dx = x2 - x1;
	data.div.dy = y2 - y1;

	data.tx0 = tx0;
	data.ty0 = ty0;
	data.tx_mul = tx_mul;
	data.ty_mul = ty_mul;

	// TODO: make a unit vector
	data.normal.Set( (y2-y1), (x1-x2), 0 );

	R_ColmapPipe_AdjustLight(lit_adjust);

	R_RunPipeline(GL_POLYGON, v_count, tex_id,
			      trans, blending, PIPEF_NONE,
				  &data, WallCoordFunc);

	R_ColmapPipe_AdjustLight(0);

#if 0
	int pass = 0;

	local_gl_vert_t *glvert;
	
	glvert = RGL_BeginUnit(GL_POLYGON, v_count,
			GL_MODULATE, tex_id, GL_MODULATE, tex2_id,
			pass, blending);
	pass++;

	for (int kk=0; kk < v_count; kk++)
	{
		Color_Rainbow(glvert+kk, 255, 255, 255, trans);
		Vertex_Std   (glvert+kk, vertices+kk, GL_TRUE);
		Normal_Std   (glvert+kk, nx, ny, nz);

		TexCoord_Wall (glvert+kk, 0, &div, tx0, ty0, tx_mul, ty_mul);
		TexCoord_Fader(glvert+kk, 1, lit_Nom, false);
	}

	RGL_EndUnit(v_count);
#endif

#if 0  // COLORMAP ADD SHIT

	data.cmx = 1;

	RGL_RenderPolyQuad(poly, &data, WallCoordFunc, 0,tex2_id,
		/* pass */ 1, blending | BL_Add);

	RGL_FreePolyQuad(poly);
#endif

#if 0  // GLOW TEST !!!!
	if (! glow_tex)
		MakeGlowTexture();

	glvert = RGL_BeginUnit(GL_POLYGON, v_count,
			GL_MODULATE, tex_id, GL_MODULATE, glow_tex,
			pass, BL_Add);
	pass++;

	for (int kk=0; kk < v_count; kk++)
	{
		Color_Rainbow(glvert+kk, 127, 255, 63, 0.9*trans);
		Vertex_Std   (glvert+kk, vertices+kk, GL_TRUE);
		Normal_Std   (glvert+kk, nx, ny, nz);

		TexCoord_Wall (glvert+kk, 0, &div, tx0, ty0, tx_mul, ty_mul);
		TexCoord_FloorGlow(glvert+kk, 1, cur_seg->frontsector->f_h);
	}

	RGL_EndUnit(v_count);
#endif

#if 0 ///!!!! def DLIGHT_PROTOTYPE
	if (use_dlights == 1 && solid_mode)
	{
RGL_DrawUnits(); //!!!!

		wall_plane_data_t dat2;
		memcpy(&dat2, &data, sizeof(dat2));

		dat2.dlights = NULL;
		dat2.trans = 0.5f;

		for (drawthing_t *dl=dfloor->dlights; dl; dl=dl->next)
		{
			mobj_t *mo = dl->mo;

			float dist = (mo->x - dat2.div.x) * dat2.div.dy -
			             (mo->y - dat2.div.y) * dat2.div.dx;

			// light behind the plane ?    
			if (dist < 0)
				continue;

			dist /= cur_seg->length;

			for (int DL=0; DL < 2; DL++)
			{
				const dlight_info_c *info = (DL == 0) ? &mo->info->dlight0 : &mo->info->dlight1;

				if (info->type == DLITE_None ||
					mo->dlight[DL].r <= 0 || ! mo->dlight[DL].image)
					continue;

				dl_R = (info->colour >> 16) & 0xFF;
				dl_G = (info->colour >>  8) & 0xFF;
				dl_B = (info->colour      ) & 0xFF;
				dl_WP = &data;

				GLuint tex2_id = W_ImageCache(mo->dlight[DL].image);

				float fx_radius;
				ComputeDLParameters(dist, mo, &fx_radius, &dat2.trans);

				dat2.ty = (mo->z + mo->height * PERCENT_2_FLOAT(info->height));

				dat2.ty = (dat2.ty / fx_radius) + 0.5f;
				dat2.ty_mul = -1.0f / fx_radius;
				dat2.ty_skew = 0;

				dat2.tx = (mo->x - dat2.div.x) * dat2.div.dx +
						  (mo->y - dat2.div.y) * dat2.div.dy;
				dat2.tx /= cur_seg->length;

				dat2.tx = (dat2.tx / -fx_radius) + 0.5f;

				dat2.tdx = cur_seg->length * 1.0f / fx_radius;

				poly = RGL_NewPolyQuad(4);

				PQ_ADD_VERT(poly, x1, y1, bottom);
				PQ_ADD_VERT(poly, x1, y1, top);
				PQ_ADD_VERT(poly, x2, y2, top);
				PQ_ADD_VERT(poly, x2, y2, bottom);

				RGL_BoundPolyQuad(poly);

				RGL_RenderPolyQuad(poly, &dat2, DLightWallCoordFunc,
(info->type == DLITE_Add ? 0 : tex_id),tex2_id,
					/* pass */ 2+DL, BL_Add);

				RGL_FreePolyQuad(poly);
RGL_DrawUnits();
			}
		}
	}
#endif // DLIGHT_PROTOTYPE
}


#define MAX_FLOOD_VERT  16

typedef struct
{
	vec3_t vert[2 * (MAX_FLOOD_VERT + 1)];

	float R, G, B;

	float plane_h;

	float tx0, ty0;
	float image_w, image_h;

	vec2_t x_mat;
	vec2_t y_mat;

	vec3_t normal;
}
flood_emu_data_t;


static void FloodCoordFunc(void *d, int v_idx,
		vec3_t *pos, float *rgb, vec2_t *texc,
		vec3_t *normal, vec3_t *lit_pos)
{
	const flood_emu_data_t *data = (flood_emu_data_t *)d;

	*pos    = data->vert[v_idx];
	*normal = data->normal;

	rgb[0] = data->R;
	rgb[1] = data->G;
	rgb[2] = data->B;

	float along = (viewz - data->plane_h) / (viewz - pos->z);

	lit_pos->x = viewx + along * (pos->x - viewx);
	lit_pos->y = viewy + along * (pos->y - viewy);
	lit_pos->z = data->plane_h;

	float rx = (data->tx0 + lit_pos->x) / data->image_w;
	float ry = (data->ty0 + lit_pos->y) / data->image_h;

	texc->x = rx * data->x_mat.x + ry * data->x_mat.y;
	texc->y = rx * data->y_mat.x + ry * data->y_mat.y;
}


static void EmulateFloodPlane(const drawfloor_t *dfloor,
	const sector_t *flood_ref, int face_dir, float h1, float h2)
{
	if (num_active_mirrors > 0)
		return;

	const surface_t *surf = (face_dir > 0) ? &flood_ref->floor :
		&flood_ref->ceil;

	// ignore sky and invisible planes
	if (IS_SKY(*surf) || surf->translucency < 0.04f)
		return;

	// ignore transparent doors (TNT MAP02)
	if (flood_ref->f_h >= flood_ref->c_h)
		return;

	// ignore fake 3D bridges (Batman MAP03)
	if (cur_seg->linedef &&
	    cur_seg->linedef->frontsector == cur_seg->linedef->backsector)
		return;

	const region_properties_t *props = surf->override_p ?
		surf->override_p : &flood_ref->props;

	R_ColmapPipe_SetProps(props);


	SYS_ASSERT(surf->image);

	GLuint tex_id = W_ImageCache(surf->image);


	flood_emu_data_t data;

	data.R = data.G = data.B = 1.0f;

	data.plane_h = (face_dir > 0) ? h2 : h1;

	data.tx0 = surf->offset.x;
	data.ty0 = surf->offset.y;
	data.image_w = IM_WIDTH(surf->image);
	data.image_h = IM_HEIGHT(surf->image);

	data.x_mat = surf->x_mat;
	data.y_mat = surf->y_mat;

	data.normal.Set(0, 0, face_dir);


	// determine number of pieces to subdivide the area into.
	// The more the better, upto a limit of 64 pieces, and
	// also limiting the size of the pieces.

	float piece_w = cur_seg->length;
	float piece_h = h2 - h1;

	int piece_col = 1;
	int piece_row = 1;

	while (piece_w > 16 || piece_h > 16)
	{
		if (piece_col * piece_row >= 64)
			break;

		if (piece_col >= MAX_FLOOD_VERT && piece_row >= MAX_FLOOD_VERT)
			break;

		if (piece_h >= piece_w && piece_row < MAX_FLOOD_VERT)
		{
			piece_h /= 2.0;
			piece_row *= 2;
		}
		else
		{
			piece_w /= 2.0;
			piece_col *= 2;
		}
	}

	float sx = cur_seg->v1->x;
	float sy = cur_seg->v1->y;

	float dx = cur_seg->v2->x - sx;
	float dy = cur_seg->v2->y - sy;
	float dh = h2 - h1;

	for (int row=0; row < piece_row; row++)
	{
		float z = h1 + dh * row / (float)piece_row;

		for (int col=0; col <= piece_col; col++)
		{
			float x = sx + dx * col / (float)piece_col;
			float y = sy + dy * col / (float)piece_col;

			data.vert[col*2 + 0].Set(x, y, z);
			data.vert[col*2 + 1].Set(x, y, z + dh / piece_row);
		}

#if 0  // DEBUGGING AIDE
		data.R = (64 + 190 * (row & 1)) / 255.0;
		data.B = (64 + 90 * (row & 2))  / 255.0;
#endif

		R_RunPipeline(GL_QUAD_STRIP, (piece_col+1) * 2,
				      tex_id, 1.0, BL_NONE, PIPEF_NONE,
					  &data, FloodCoordFunc);
	}
}

//
// RGL_BuildWalls
//
// Analyses floor/ceiling heights, and add corresponding walls/floors
// to the drawfloor.  Returns true if the whole region was "solid".
//
static bool RGL_BuildWalls(drawfloor_t *dfloor, seg_t *seg)
{
	cur_seg = seg;

	SYS_ASSERT(!seg->miniseg && seg->linedef);

	// mark the segment on the automap
	seg->linedef->flags |= MLF_Mapped;

	frontsector = seg->front_sub->sector;
	backsector  = NULL;

	if (seg->back_sub)
		backsector = seg->back_sub->sector;

	side_t *sd = cur_seg->sidedef;

	float f1 = dfloor->f_h;
	float c1 = dfloor->c_h;

	float f, c, x_offset;
	float tex_top_h;

	bool opaque;

#if (DEBUG >= 3)
	L_WriteDebug( "   BUILD WALLS %1.1f .. %1.1f\n", f1, c1);
#endif

	// handle TRANSLUCENT + THICK floors (a bit of a hack)
	if (dfloor->ef && dfloor->higher &&
		(dfloor->ef->ef_info->type & EXFL_Thick) &&
		(dfloor->ef->top->translucency <= 0.99f))
	{
		c1 = dfloor->ef->top_h;
	}

	for (int j=0; j < sd->tile_used; j++)
	{
		wall_tile_t *wt = sd->tiles + j;

		c = MIN(c1, wt->z2);
		f = MAX(f1, wt->z1);

		// not visible ?
		if (f >= c)
			continue;

		SYS_ASSERT(wt->surface->image);

		tex_top_h = wt->tex_z + wt->surface->offset.y;
		x_offset  = wt->surface->offset.x;

		if (wt->flags & WTILF_ExtraX)
		{
			x_offset += cur_seg->sidedef->middle.offset.x;
		}
		if (wt->flags & WTILF_ExtraY)
		{
			// needed separate Y flag to maintain compatibility
			tex_top_h += cur_seg->sidedef->middle.offset.y;
		}

		opaque = (! cur_seg->backsector) ||
			(wt->surface->translucency > 0.99f && wt->surface->image->img_solid);

		// check for horizontal sliders
		if ((wt->flags & WTILF_MidMask) && cur_seg->linedef->special && 
			cur_seg->linedef->special->s.type != SLIDE_None &&
			cur_seg->linedef->slider_move)
		{
			RGL_DrawWall(dfloor, c, f, tex_top_h,
				wt, 2, opaque, x_offset, NULL);

			if (cur_seg->linedef->special->s.type == SLIDE_Center)
			{
				RGL_DrawWall(dfloor, c, f, tex_top_h,
					wt, 3, opaque, x_offset, NULL);
			}
			continue;
		}

		RGL_DrawWall(dfloor, c, f, tex_top_h, wt,
			(wt->flags & WTILF_MidMask) ? 1 : 0, 
			opaque, x_offset, (wt->flags & WTILF_MidMask) ?
			  &cur_seg->sidedef->sector->props : NULL);
	}

	// -AJA- 2004/04/21: Emulate Flat-Flooding TRICK
	if (! hom_detect && solid_mode && dfloor->prev == NULL &&
		sd->bottom.image == NULL && cur_seg->back_sub &&
		cur_seg->back_sub->sector->f_h > cur_seg->front_sub->sector->f_h &&
		cur_seg->back_sub->sector->f_h < viewz)
	{
		EmulateFloodPlane(dfloor, cur_seg->back_sub->sector, +1,
			cur_seg->front_sub->sector->f_h,
			cur_seg->back_sub->sector->f_h);
	}

	if (! hom_detect && solid_mode && dfloor->prev == NULL &&
		sd->top.image == NULL && cur_seg->back_sub &&
		cur_seg->back_sub->sector->c_h < cur_seg->front_sub->sector->c_h &&
		cur_seg->back_sub->sector->c_h > viewz)
	{
		EmulateFloodPlane(dfloor, cur_seg->back_sub->sector, -1,
			cur_seg->back_sub->sector->c_h,
			cur_seg->front_sub->sector->c_h);
	}

	if (cur_seg->sidedef->middle.image == NULL)
	{
		// -AJA- hack for transparent doors (this test would normally be
		// above this block, not inside it).
		//
		if (f1 >= c1)
			return true;

		return false;
	}

	// handle sliders that are totally solid and closed
	if (cur_seg->linedef->special &&
		cur_seg->linedef->special->s.type != SLIDE_None &&
		! cur_seg->linedef->special->s.see_through &&
		! cur_seg->linedef->slider_move)
	{
		return true;
	}

	return false;
}


static void RGL_WalkBSPNode(unsigned int bspnum);


static void RGL_WalkMirror(drawsub_c *dsub, seg_t *seg,
						   angle_t left, angle_t right)
{
	drawmirror_c *mir = R_GetDrawMirror();
	mir->Clear(seg);

	mir->left  = viewangle + left;
	mir->right = viewangle + right;

	dsub->mirrors.push_back(mir);

	// push mirror (translation matrix)
	MIR_Push(mir);

	subsector_t *save_sub = cur_sub;

	angle_t save_clip_L   = clip_left;
	angle_t save_clip_R   = clip_right;
	angle_t save_scope    = clip_scope;

	clip_left  = left;
	clip_right = right;
	clip_scope = left - right;

	// perform another BSP walk
	RGL_WalkBSPNode(root_node);

	cur_sub = save_sub;

	clip_left  = save_clip_L;
	clip_right = save_clip_R;
	clip_scope = save_scope;

	// pop mirror
	MIR_Pop();
}


//
// RGL_WalkSeg
//
// Visit a single seg of the subsector, and for one-sided lines update
// the 1D occlusion buffer.
//
static void RGL_WalkSeg(drawsub_c *dsub, seg_t *seg)
{
	// ignore segs sitting on current mirror
	if (num_active_mirrors > 0 && seg ==
		active_mirrors[num_active_mirrors-1].def->seg)
		return;

	float sx1 = seg->v1->x;
	float sy1 = seg->v1->y;

	float sx2 = seg->v2->x;
	float sy2 = seg->v2->y;

	// when there are active mirror planes, segs not only need to
	// be flipped across them but also clipped across them.
	if (num_active_mirrors > 0)
	{
		for (int i=num_active_mirrors-1; i >= 0; i--)
		{
			seg_t *seg = active_mirrors[i].def->seg;

			divline_t div;

			div.x  = seg->v1->x;
			div.y  = seg->v1->y;
			div.dx = seg->v2->x - div.x;
			div.dy = seg->v2->y - div.y;

			int s1 = P_PointOnDivlineSide(sx1, sy1, &div);
			int s2 = P_PointOnDivlineSide(sx2, sy2, &div);

			// seg lies completely in the back space?
			if (s1 == 1 && s2 == 1)
				return;

			if (s1 != s2)
			{
				// seg crosses line, need to split it
				float ix, iy;

				P_ComputeIntersection(&div, sx1, sy1, sx2, sy2, &ix, &iy);

				if (s2 == 1)
					sx2 = ix, sy2 = iy;
				else
					sx1 = ix, sy1 = iy;
			}

			active_mirrors[i].Flip(sx1, sy1);
			active_mirrors[i].Flip(sx2, sy2);

      		float tx = sx1; sx1 = sx2; sx2 = tx;
      		float ty = sy1; sy1 = sy2; sy2 = ty;
		}
	}

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

	// The seg is in the view range,
	// but not necessarily visible.

#if 1
	// check if visible
	if (span > (ANG1/4) && RGL_1DOcclusionTest(angle_R, angle_L))
	{
		return;
	}
#endif

	dsub->visible = true;

	if (seg->miniseg || span == 0)
		return;

	if ((seg->linedef->flags & MLF_Mirror) &&
		(num_active_mirrors < MAX_MIRRORS))
	{
		RGL_WalkMirror(dsub, seg, angle_L, angle_R);

		RGL_1DOcclusionSet(angle_R, angle_L);
		return;
	}

	drawseg_c *dseg = R_GetDrawSeg();
	dseg->seg = seg;

	dsub->segs.push_back(dseg);


	sector_t *frontsector = seg->front_sub->sector;
	sector_t *backsector  = NULL;

	if (seg->back_sub)
		backsector = seg->back_sub->sector;
		
	// only 1 sided walls affect the 1D occlusion buffer

	if (seg->linedef->blocked)
	{
		RGL_1DOcclusionSet(angle_R, angle_L);
	}

	// --- handle sky (using the depth buffer) ---

	bool upper_sky = false;
	bool lower_sky = false;

	if (backsector && IS_SKY(frontsector->floor) && IS_SKY(backsector->floor))
		lower_sky = true;

	if (backsector && IS_SKY(frontsector->ceil) && IS_SKY(backsector->ceil))
		upper_sky = true;

	if (lower_sky && frontsector->f_h < backsector->f_h)
	{
		RGL_DrawSkyWall(seg, frontsector->f_h, backsector->f_h);
	}

	if (IS_SKY(frontsector->ceil))
	{
		if (frontsector->c_h < frontsector->sky_h &&
			(! backsector || ! IS_SKY(backsector->ceil) ||
			backsector->f_h >= frontsector->c_h))
		{
			RGL_DrawSkyWall(seg, frontsector->c_h, frontsector->sky_h);
		}
		else if (backsector && IS_SKY(backsector->ceil))
		{
			float max_f = MAX(frontsector->f_h, backsector->f_h);

			if (backsector->c_h <= max_f && max_f < frontsector->sky_h)
			{
				RGL_DrawSkyWall(seg, max_f, frontsector->sky_h);
			}
		}
	}
	// -AJA- 2004/08/29: Emulate Sky-Flooding TRICK
	else if (! hom_detect && backsector && IS_SKY(backsector->ceil) &&
			 seg->sidedef->top.image == NULL &&
			 backsector->c_h < frontsector->c_h)
	{
		RGL_DrawSkyWall(seg, backsector->c_h, frontsector->c_h);
	}
}

//
// RGL_CheckBBox
//
// Checks BSP node/subtree bounding box.
// Returns true if some part of the bbox might be visible.
//
// Placed here to be close to RGL_WalkSeg(), which has similiar angle
// clipping stuff in it.
//

bool RGL_CheckBBox(float *bspcoord)
{
	if (num_active_mirrors > 0)
	{
		// a flipped bbox may no longer be axis aligned, hence we
		// need to find the bounding area of the transformed box.
		static float new_bbox[4];

		M_ClearBox(new_bbox);

		for (int p=0; p < 4; p++)
		{
			float tx = bspcoord[(p & 1) ? BOXLEFT   : BOXRIGHT];
			float ty = bspcoord[(p & 2) ? BOXBOTTOM : BOXTOP];

			MIR_Coordinate(tx, ty);

			M_AddToBox(new_bbox, tx, ty);
		}

		bspcoord = new_bbox;
	}
			
	int boxx, boxy;

	// Find the corners of the box
	// that define the edges from current viewpoint.
	if (viewx <= bspcoord[BOXLEFT])
		boxx = 0;
	else if (viewx < bspcoord[BOXRIGHT])
		boxx = 1;
	else
		boxx = 2;

	if (viewy >= bspcoord[BOXTOP])
		boxy = 0;
	else if (viewy > bspcoord[BOXBOTTOM])
		boxy = 1;
	else
		boxy = 2;

	int boxpos = (boxy << 2) + boxx;

	if (boxpos == 5)
		return true;

	float x1 = bspcoord[checkcoord[boxpos][0]];
	float y1 = bspcoord[checkcoord[boxpos][1]];
	float x2 = bspcoord[checkcoord[boxpos][2]];
	float y2 = bspcoord[checkcoord[boxpos][3]];

	// check clip list for an open space
	angle_t angle_L = R_PointToAngle(viewx, viewy, x1, y1);
	angle_t angle_R = R_PointToAngle(viewx, viewy, x2, y2);

	angle_t span = angle_L - angle_R;

	// Sitting on a line?
	if (span >= ANG180)
		return true;

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
				return false;

			angle_L = clip_left;
		}

		if (tspan2 > clip_scope)
		{
			// Totally off the right edge?
			if (tspan1 >= ANG180)
				return false;

			angle_R = clip_right;
		}

		if (angle_L == angle_R)
			return false;
	}

	return ! RGL_1DOcclusionTest(angle_R, angle_L);
}

//
// RGL_DrawPlane
//
static void RGL_DrawPlane(drawfloor_t *dfloor, float h,
						  surface_t *surf, int face_dir)
{
	seg_t *seg;

	bool mid_masked = ! surf->image->img_solid;
	bool blended;

	int num_vert, i;


	region_properties_t *props = dfloor->props;

	// more deep water hackitude
	if (cur_sub->deep_ref &&
		((face_dir > 0 && dfloor->prev == NULL) ||
		 (face_dir < 0 && dfloor->next == NULL)))
	{
		props = &cur_sub->deep_ref->props;
	}

	if (surf->override_p)
		props = surf->override_p;

	R_ColmapPipe_SetProps(props);


	float trans = surf->translucency;

	// ignore sky
	if (IS_SKY(*surf))
		return;

	// ignore invisible planes
	if (trans < 0.04f)
		return;

	// ignore non-facing planes
	if ((viewz > h) != (face_dir > 0))
		return;

	// ignore non-solid planes in solid_mode (& vice versa)
	blended = (trans <= 0.99f) ? true : false;
	if (blended == solid_mode)
		return;

	// ignore dud regions (floor >= ceiling)
	if (dfloor->f_h > dfloor->c_h)
		return;

	// ignore empty subsectors
	if (cur_sub->segs == NULL)
		return;

	// count number of actual vertices
	for (seg=cur_sub->segs, num_vert=0; seg; seg=seg->sub_next, num_vert++)
	{
		/* nothing here */
	}

	// -AJA- make sure polygon has enough vertices.  Sometimes a subsector
	// ends up with only 1 or 2 segs due to level problems (e.g.  MAP22).
	if (num_vert < 3)
		return;

	if (num_vert > MAX_PLVERT)
		num_vert = MAX_PLVERT;



	SYS_ASSERT(surf->image);

	GLuint tex_id = W_ImageCache(surf->image);


	vec3_t vertices[MAX_PLVERT];

	int v_count = 0;

	for (seg=cur_sub->segs, i=0; seg && (i < MAX_PLVERT); 
		seg=seg->sub_next, i++)
	{
		if (v_count < MAX_PLVERT)
		{
			float x = seg->v1->x;
			float y = seg->v1->y;

			MIR_Coordinate(x, y);

			vertices[v_count].x = x;
			vertices[v_count].y = y;
			vertices[v_count].z = h;

			v_count++;
		}
	}

	int blending = (blended ? BL_Alpha : 0) | (mid_masked ? BL_Masked : 0);


	plane_coord_data_t data;

	data.vert = vertices;

	data.R = data.G = data.B = 1.0f;

	data.tx0 = surf->offset.x;
	data.ty0 = surf->offset.y;
	data.image_w = IM_WIDTH(surf->image);
	data.image_h = IM_HEIGHT(surf->image);

	data.x_mat = surf->x_mat;
	data.y_mat = surf->y_mat;

	data.normal.Set(0, 0, (viewz > h) ? +1 : -1);

	R_RunPipeline(GL_POLYGON, v_count, tex_id,
			      trans, blending, PIPEF_NONE,
				  &data, PlaneCoordFunc);

#if 0  // OLD WAY
	int pass = 0;

	local_gl_vert_t *glvert = RGL_BeginUnit(GL_POLYGON, v_count,
			GL_MODULATE, tex_id, GL_MODULATE, tex2_id,
			pass, blending);
	pass++;

	for (int kk=0; kk < v_count; kk++)
	{
		Color_Rainbow(glvert+kk, 255, 255, 255, trans);
		Vertex_Std   (glvert+kk, vertices+kk, GL_TRUE);

		Normal_Std   (glvert+kk, nx, ny, nz);

		TexCoord_Plane(glvert+kk, 0, tx0, ty0, image_w, image_h,
				&surf->x_mat, &surf->y_mat);
		TexCoord_Fader(glvert+kk, 1, lit_Nom, false);
	}

	RGL_EndUnit(v_count);
#endif

#if 0  // COLORMAP ADD SHIT

	data.cmx = 1;

	RGL_RenderPolyQuad(poly, &data, WallCoordFunc, 0,tex2_id,
		/* pass */ 1, blending | BL_Add);

	RGL_FreePolyQuad(poly);
#endif

#if 0  // FLOOR GLOW TEST !!!!
	if (! glow_tex)
		MakeGlowTexture();

if (nz > 0)
{
	glvert = RGL_BeginUnit(GL_POLYGON, v_count,
			GL_MODULATE, tex_id, GL_MODULATE, glow_tex,
			pass, BL_Add);
	pass++;

	for (int kk=0; kk < v_count; kk++)
	{
		Color_Rainbow(glvert+kk, 127, 255, 63, 0.9*trans);
		Vertex_Std   (glvert+kk, vertices+kk, GL_TRUE);
		Normal_Std   (glvert+kk, nx, ny, nz);

		TexCoord_Plane(glvert+kk, 0, tx0, ty0, image_w, image_h,
				&surf->x_mat, &surf->y_mat);
		TexCoord_FloorGlow(glvert+kk, 1, h);
	}

	RGL_EndUnit(v_count);
}
#endif

#if 0  // WALL GLOW TEST !!!!
	if (! glow_tex)
		MakeGlowTexture();

	glvert = RGL_BeginUnit(GL_POLYGON, v_count,
			GL_MODULATE, tex_id, GL_MODULATE, glow_tex,
			pass, BL_Add);
	pass++;

	for (int kk=0; kk < v_count; kk++)
	{
		Color_Rainbow(glvert+kk, 255, 255, 255, 0.9*trans);
		Vertex_Std   (glvert+kk, vertices+kk, GL_TRUE);
		Normal_Std   (glvert+kk, nx, ny, nz);

		TexCoord_Plane(glvert+kk, 0, tx0, ty0, image_w, image_h,
				&surf->x_mat, &surf->y_mat);
		TexCoord_WallGlow(glvert+kk, 1, 2560,2880, 0,1);
	}

	RGL_EndUnit(v_count);
#endif

#ifdef SHADOW_PROTOTYPE
	if (level_flags.shadows && solid_mode && face_dir > 0)
	{
		wall_plane_data_t dat2;
		memcpy(&dat2, &data, sizeof(dat2));

		dat2.dlights = NULL;
		dat2.trans = 0.5;
		dat2.image = shadow_image;

		tex_id = W_ImageCache(dat2.image);

		for (drawthing_t *dthing=dfloor->things; dthing; dthing=dthing->next)
		{
			if (dthing->mo->info->shadow_trans <= 0 || dthing->mo->floorz >= viewz)
				continue;

			dat2.tx = -(dthing->mo->x - dthing->mo->radius);
			dat2.ty = -(dthing->mo->y - dthing->mo->radius);

			dat2.x_mat.x = 0.5f / dthing->mo->radius;
			dat2.x_mat.y = 0;

			dat2.y_mat.y = 0.5f / dthing->mo->radius;
			dat2.y_mat.x = 0;

			poly = RGL_NewPolyQuad(num_vert);

			for (seg=cur_sub->segs, i=0; seg && (i < MAX_PLVERT); 
				seg=seg->sub_next, i++)
			{
				PQ_ADD_VERT(poly, seg->v1->x, seg->v1->y, h);
			}

			RGL_BoundPolyQuad(poly);

			RGL_RenderPolyQuad(poly, &data, ShadowCoordFunc, tex_id,0,
				/* pass */ 2, BL_Alpha);

			RGL_FreePolyQuad(poly);
		}
	}
#endif

#if 0 //!!!! def DLIGHT_PROTOTYPE
	if (use_dlights == 1 && solid_mode)
	{
RGL_DrawUnits();

		wall_plane_data_t dat2;
		memcpy(&dat2, &data, sizeof(dat2));

		dat2.dlights = NULL;
		dat2.trans = 0.5f;

		for (drawthing_t *dl=dfloor->dlights; dl; dl=dl->next)
		{
			mobj_t *mo = dl->mo;

			// light behind the plane ?    
			if ((dl->tz > h) != (dat2.normal.z > 0))
				continue;

			for (int DL=0; DL < 2; DL++)
			{
				const dlight_info_c *info = (DL == 0) ? &mo->info->dlight0 : &mo->info->dlight1;

				if (info->type == DLITE_None ||
					mo->dlight[DL].r <= 0 || ! mo->dlight[DL].image)
					continue;

				dl_R = (info->colour >> 16) & 0xFF;
				dl_G = (info->colour >>  8) & 0xFF;
				dl_B = (info->colour      ) & 0xFF;
				dl_WP = &data;

				float dist = ABS(dl->tz - h);

				GLuint tex2_id = W_ImageCache(mo->dlight[DL].image);

				float fx_radius;
				ComputeDLParameters(dist, mo, &fx_radius, &dat2.trans);

				fx_radius /= 2.0f;

				dat2.tx = -(mo->x - fx_radius);
				dat2.ty = -(mo->y - fx_radius);

				dat2.x_mat.x = 0.5f / fx_radius;
				dat2.x_mat.y = 0;

				dat2.y_mat.y = 0.5f / fx_radius;
				dat2.y_mat.x = 0;

				poly = RGL_NewPolyQuad(num_vert);

				for (seg=cur_sub->segs, i=0; seg && (i < MAX_PLVERT); 
					seg=seg->sub_next, i++)
				{
					PQ_ADD_VERT(poly, seg->v1->x, seg->v1->y, h);
				}

				RGL_BoundPolyQuad(poly);

				RGL_RenderPolyQuad(poly, &dat2, DLightPlaneCoordFunc,
(info->type == DLITE_Add ? 0 : tex_id),tex2_id,
					/* pass */ 2+DL, BL_Add);

				RGL_FreePolyQuad(poly);
RGL_DrawUnits();

			}
		}
	}
#endif // DLIGHT_PROTOTYPE
}

static inline void AddNewDrawFloor(drawsub_c *dsub, extrafloor_t *ef,
								   float f_h, float c_h, float top_h,
								   surface_t *floor, surface_t *ceil,
								   region_properties_t *props)
{
	drawfloor_t *dfloor;

	dfloor = R_GetDrawFloor();
	dfloor->Clear();

	dfloor->f_h   = f_h;
	dfloor->c_h   = c_h;
	dfloor->top_h = top_h;
	dfloor->floor = floor;
	dfloor->ceil  = ceil;
	dfloor->ef    = ef;
	dfloor->props = props;

#if 0
	drawfloor_t *tail;

	// link it in
	// (order is very important)
	if (cur_sub->floors == NULL || f_h > viewz)
	{
		// add to head
		dfloor->next = cur_sub->floors;
		dfloor->prev = NULL;

		if (cur_sub->floors)
			cur_sub->floors->prev = dfloor;

		cur_sub->floors = dfloor;
	}
	else
	{
		// add to tail
		for (tail=cur_sub->floors; tail->next; tail=tail->next)
		{ /* nothing here */ }

		dfloor->next = NULL;
		dfloor->prev = tail;

		tail->next = dfloor;
	}
#endif

	dsub->floors.push_back(dfloor);

	if (use_dlights)
	{
		R2_FindDLights(cur_sub, dfloor);

#if 0  // FIXME:
		if (cur_sub == viewsubsector && f_h <= viewz && viewz <= c_h)
		    RememberDlightsForPSprites()
#endif
	}
}


//
// RGL_WalkSubsector
//
// Visit a subsector, and collect information, such as where the
// walls, planes (ceilings & floors) and things need to be drawn.
//
static void RGL_WalkSubsector(int num)
{
	subsector_t *sub = &subsectors[num];
	seg_t *seg;
	mobj_t *mo;
	sector_t *sector;
	surface_t *floor_s;
	float floor_h;

	extrafloor_t *S, *L, *C;

#if (DEBUG >= 1)
	L_WriteDebug( "\nVISITING SUBSEC %d (sector %d)\n\n", num, sub->sector - sectors);
#endif

	cur_sub = sub;
	sector = cur_sub->sector;

	drawsub_c *K = R_GetDrawSub();
	K->Clear(sub);

	// --- handle sky (using the depth buffer) ---

	if (IS_SKY(cur_sub->sector->floor) && 
		viewz > cur_sub->sector->f_h)
	{
		RGL_DrawSkyPlane(cur_sub, cur_sub->sector->f_h);
	}

	if (IS_SKY(cur_sub->sector->ceil) && 
		viewz < cur_sub->sector->sky_h)
	{
		RGL_DrawSkyPlane(cur_sub, cur_sub->sector->sky_h);
	}

	// add in each extrafloor, traversing strictly upwards

	floor_s = &sector->floor;
	floor_h = sector->f_h;

	S = sector->bottom_ef;
	L = sector->bottom_liq;

	// Handle the BOOMTEX flag (Boom compatibility)
	extrafloor_t *boom_ef = sector->bottom_liq ? sector->bottom_liq : sector->bottom_ef;

	if (boom_ef && (boom_ef->ef_info->type & EXFL_BoomTex))
		floor_s = &boom_ef->ef_line->frontsector->floor;

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
		if (C->bottom_h < floor_h || C->bottom_h > sector->c_h)
			continue;

		bool de_f = (cur_sub->deep_ref && K->floors.size() == 0);

		AddNewDrawFloor(K, C,
			de_f ? cur_sub->deep_ref->f_h : floor_h,
			C->bottom_h, C->top_h,
			de_f ? &cur_sub->deep_ref->floor : floor_s,
			C->bottom, C->p);

		floor_s = C->top;
		floor_h = C->top_h;
	}

	// -AJA- 2004/04/22: emulate the Deep-Water TRICK (above too)
	bool de_f = (cur_sub->deep_ref && K->floors.size() == 0);
	bool de_c = (cur_sub->deep_ref != NULL);

	AddNewDrawFloor(K, NULL,
		de_f ? cur_sub->deep_ref->f_h : floor_h,
		de_c ? cur_sub->deep_ref->c_h : sector->c_h,
		de_c ? cur_sub->deep_ref->c_h : sector->c_h,
		de_f ? &cur_sub->deep_ref->floor : floor_s,
		de_c ? &cur_sub->deep_ref->ceil : &sector->ceil, sector->p);

	// handle each sprite in the subsector.  Must be done before walls,
	// since the wall code will update the 1D occlusion buffer.

	for (mo=cur_sub->thinglist; mo; mo=mo->snext)
	{
		RGL_WalkThing(K, mo);
	}

	// clip 1D occlusion buffer.
	for (seg=sub->segs; seg; seg=seg->sub_next)
	{
		RGL_WalkSeg(K, seg);
	}

	// add drawsub to list (closest -> furthest)

	if (num_active_mirrors > 0)
		active_mirrors[num_active_mirrors-1].def->drawsubs.push_back(K);
	else
		drawsubs.push_back(K);
}


static void RGL_DrawSubsector(drawsub_c *dsub);


static void RGL_DrawSubList(std::list<drawsub_c *> &dsubs)
{
	// draw all solid walls and planes
	solid_mode = true;
	RGL_StartUnits(solid_mode);

	std::list<drawsub_c *>::iterator FI;  // Forward Iterator

	for (FI = dsubs.begin(); FI != dsubs.end(); FI++)
		RGL_DrawSubsector(*FI);

	RGL_FinishUnits();

	// draw all sprites and masked/translucent walls/planes
	solid_mode = false;
	RGL_StartUnits(solid_mode);

	std::list<drawsub_c *>::reverse_iterator RI;

	for (RI = dsubs.rbegin(); RI != dsubs.rend(); RI++)
		RGL_DrawSubsector(*RI);

	RGL_FinishUnits();
}


static void DrawMirrorPolygon(drawmirror_c *mir)
{
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	float alpha = 0.14 * (num_active_mirrors + 1);

	line_t *ld = mir->seg->linedef;
	SYS_ASSERT(ld);

	if (ld->special)
	{
#if 0 
		float trans = PERCENT_2_FLOAT(ld->special->translucency);

		// ignore the default value, which is 100%
		if (trans < 0.95) alpha = MAX(alpha, trans);
#endif
		float R = RGB_RED(ld->special->fx_color) / 255.0;
		float G = RGB_GRN(ld->special->fx_color) / 255.0;
		float B = RGB_BLU(ld->special->fx_color) / 255.0;

		// looks better with reduced color in multiple reflections
		R *= 1.0 - 0.4 * num_active_mirrors;
		G *= 1.0 - 0.4 * num_active_mirrors;
		B *= 1.0 - 0.4 * num_active_mirrors;

		glColor4f(R, G, B, alpha);
	}
	else
		glColor4f(0.0, 0.0, 0.0, alpha);

	float x1 = mir->seg->v1->x;
	float y1 = mir->seg->v1->y;
	float z1 = ld->frontsector->f_h;

	float x2 = mir->seg->v2->x;
	float y2 = mir->seg->v2->y;
	float z2 = ld->frontsector->c_h;

	MIR_Coordinate(x1, y1);
	MIR_Coordinate(x2, y2);

	glBegin(GL_POLYGON);

	glVertex3f(x1, y1, z1);
	glVertex3f(x1, y1, z2);
	glVertex3f(x2, y2, z2);
	glVertex3f(x2, y2, z1);

	glEnd();

	glDisable(GL_BLEND);
}

static void RGL_DrawMirror(drawmirror_c *mir)
{
	MIR_Push(mir);

	RGL_FinishUnits();

	RGL_DrawSubList(mir->drawsubs);

	MIR_Pop();

	solid_mode = true;
	RGL_StartUnits(solid_mode);

	DrawMirrorPolygon(mir);
}


static void RGL_DrawSubsector(drawsub_c *dsub)
{
	subsector_t *sub = dsub->sub;

#if (DEBUG >= 1)
	L_WriteDebug("\nREVISITING SUBSEC %d\n\n", (int)(sub - subsectors));
#endif

	cur_sub = sub;

	if (solid_mode)
	{
		std::list<drawmirror_c *>::iterator MRI;

		for (MRI = dsub->mirrors.begin(); MRI != dsub->mirrors.end(); MRI++)
		{
			RGL_DrawMirror(*MRI);
		}
	}

	cur_sub = sub;

	//!!!!! FIXME
	// if (! dsub->sorted) std::sort(dsub->floors.begin(), dsub->floors.end(), SORTER)

	std::vector<drawfloor_t *>::iterator DFI;

	// handle each floor, drawing planes and things
	for (DFI = dsub->floors.begin(); DFI != dsub->floors.end(); DFI++)
	{
		drawfloor_t *dfloor = *DFI;

		R_LightPipe_SetList(dfloor->dlights);

		std::list<drawseg_c *>::iterator SEGI;

		for (SEGI = dsub->segs.begin(); SEGI != dsub->segs.end(); SEGI++)
		{
			RGL_BuildWalls(dfloor, (*SEGI)->seg);
		}

		RGL_DrawPlane(dfloor, dfloor->c_h, dfloor->ceil,  -1);
		RGL_DrawPlane(dfloor, dfloor->f_h, dfloor->floor, +1);

		if (! solid_mode)
		{
			R_ColmapPipe_SetProps(dfloor->props);

			RGL_DrawSortThings(dfloor);
  		}

		R_LightPipe_SetList(NULL);
	}
}

//
// RGL_WalkBSPNode
//
// Walks all subsectors below a given node, traversing subtree
// recursively, collecting information.  Just call with BSP root.
//
static void RGL_WalkBSPNode(unsigned int bspnum)
{
	node_t *node;
	int side;

	// Found a subsector?
	if (bspnum & NF_V5_SUBSECTOR)
	{
		RGL_WalkSubsector(bspnum & (~NF_V5_SUBSECTOR));
		return;
	}

	node = &nodes[bspnum];

	// Decide which side the view point is on.

	divline_t nd_div;

	nd_div.x  = node->div.x;
	nd_div.y  = node->div.y;
	nd_div.dx = node->div.x + node->div.dx;
	nd_div.dy = node->div.y + node->div.dy;

	MIR_Coordinate(nd_div.x,  nd_div.y);
	MIR_Coordinate(nd_div.dx, nd_div.dy);

	if (num_active_mirrors % 2)
	{
		float tx = nd_div.x; nd_div.x = nd_div.dx; nd_div.dx = tx;
		float ty = nd_div.y; nd_div.y = nd_div.dy; nd_div.dy = ty;
	}

	nd_div.dx -= nd_div.x;
	nd_div.dy -= nd_div.y;
	
	side = P_PointOnDivlineSide(viewx, viewy, &nd_div);

	// Recursively divide front space.
	if (RGL_CheckBBox(node->bbox[side]))
		RGL_WalkBSPNode(node->children[side]);

	// Recursively divide back space.
	if (RGL_CheckBBox(node->bbox[side ^ 1]))
		RGL_WalkBSPNode(node->children[side ^ 1]);
}

//
// RGL_LoadLights
//
void RGL_LoadLights(void)
{
#ifdef SHADOW_PROTOTYPE
	shadow_image = W_ImageLookup("SHADOW_STD");
#endif
}

//
// RGL_RenderTrueBSP
//
// OpenGL BSP rendering.  Initialises all structures, then walks the
// BSP tree collecting information, then renders each subsector:
// firstly front to back (drawing all solid walls & planes) and then
// from back to front (drawing everything else, sprites etc..).
//
void RGL_RenderTrueBSP(void)
{
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

	// clear extra light on player's weapon
	rgl_weapon_r = rgl_weapon_g = rgl_weapon_b = 0;

	R2_ClearBSP();
	RGL_1DOcclusionClear();
	RGL_UpdateTheFuzz();

	drawsubs.clear();

	// handle powerup effects
	RGL_RainbowEffect(players[displayplayer]);

	if (true) //!!!!! hom_detect)
	{
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);
	}

	RGL_SetupMatrices3D();

	glClear(GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);

	// needed for drawing the sky
	RGL_BeginSky();

	// walk the bsp tree
	RGL_WalkBSPNode(root_node);

	RGL_FinishSky();

	RGL_DrawSubList(drawsubs);

	glDisable(GL_DEPTH_TEST);

	// now draw 2D stuff like psprites, and add effects
	RGL_SetupMatrices2D();

	RGL_DrawPlayerSprites(players[displayplayer]);

	RGL_ColourmapEffect(players[displayplayer]);
	RGL_PaletteEffect(players[displayplayer]);

#if (DEBUG >= 3) 
	L_WriteDebug( "\n\n");
#endif
}


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


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
