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

#include "g_game.h"
#include "m_bbox.h"
#include "p_local.h"
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

#include <math.h>

#define DEBUG  0

#define HALOS    0

#define FLOOD_DIST    1024.0f
#define FLOOD_EXPAND  128.0f

#define DLIGHT_PROTOTYPE  1



side_t *sidedef;
line_t *linedef;
sector_t *frontsector;
sector_t *backsector;

unsigned int root_node;

extern camera_t *camera;


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

static const image_c *fading_image = NULL;

#ifdef SHADOW_PROTOTYPE
static const image_c *shadow_image = NULL;
#endif


// ========= MIRROR STUFF ===========

static void ClipPlaneHorizontalLine(GLdouble *p, const vec2_t& s,
	const vec2_t& e, bool flip);

typedef struct
{
	drawmirror_c *def;

	float xc, xx, xy;  // x' = xc + x*xx + y*xy
	float yc, yx, yy;  // y' = yc + x*yx + y*yy

public:
	void ComputeMirror()
	{
		line_t *line = def->line;

		float len_p2 = line->length * line->length;

		float A = (line->dx * line->dx - line->dy * line->dy) / len_p2;
		float B = (line->dx * line->dy + line->dx * line->dy) / len_p2;

		xx = A; xy =  B;
		yx = B; yy = -A;

		xc = line->v1->x * (1.0-A) - line->v1->y * B;
		yc = line->v1->y * (1.0+A) - line->v1->x * B;
	}

	void Flip(float& x, float& y)
	{
		float tx = x, ty = y;

		x = xc + tx*xx + ty*xy;
		y = yc + tx*yx + ty*yy;
	}
}
mirror_info_t;

#define MAX_MIRRORS  1

static mirror_info_t active_mirrors[MAX_MIRRORS];

int num_active_mirrors = 0;


void MIR_Coordinate(float& x, float& y)
{
	for (int i=0; i < num_active_mirrors; i++)
		active_mirrors[i].Flip(x, y);
}

static void MIR_SetClippers()
{
	if (num_active_mirrors == 0)
	{
		glDisable(GL_CLIP_PLANE0);
		glDisable(GL_CLIP_PLANE1);
		glDisable(GL_CLIP_PLANE2);
		glDisable(GL_CLIP_PLANE3);
		glDisable(GL_CLIP_PLANE4);
		glDisable(GL_CLIP_PLANE5);

		return;
	}

	// FIXME: multiple mirrors
	
	glEnable(GL_CLIP_PLANE0);  // front
  	glEnable(GL_CLIP_PLANE1);  // left
   	glEnable(GL_CLIP_PLANE2);  // right

	GLdouble front_p[4];
	GLdouble  left_p[4];
	GLdouble right_p[4];

	line_t *ld = active_mirrors[num_active_mirrors-1].def->line;

	vec2_t left_v;
	vec2_t right_v;
	vec2_t eye_v;

	 left_v.Set(ld->v1->x, ld->v1->y);
	right_v.Set(ld->v2->x, ld->v2->y);
	  eye_v.Set(viewx, viewy);

	ClipPlaneHorizontalLine(front_p, left_v, right_v, true);
	ClipPlaneHorizontalLine( left_p, eye_v,   left_v, false);
  	ClipPlaneHorizontalLine(right_p, eye_v,  right_v, true);

	glClipPlane(GL_CLIP_PLANE0, front_p);
	glClipPlane(GL_CLIP_PLANE1, left_p);
  	glClipPlane(GL_CLIP_PLANE2, right_p);
}

static bool MIR_Push(drawmirror_c *mir)
{
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
// R2_AddColourDLights
//
// Increases the arrays of colour levels at the points (x,y,z).
//
void R2_AddColourDLights(int num, int *r, int *g, int *b, 
						 float *x, float *y, float *z, mobj_t *mo)
{
#if 0
	float base_qty = mo->dlight[0].r;
	float mo_z = mo->z + mo->height * PERCENT_2_FLOAT(mo->info->dlight0.height);

	rgbcol_t col = mo->info->dlight0.colour;

#if 0  // TEST CODE
	if (col == RGB_NO_VALUE)
	{
		bool flip;
		const image_c *img = R2_GetThingSprite(mo, &flip);
		col = W_ImageGetHue(img);
	}
#endif
	if (col == RGB_NO_VALUE)
		col = RGB_MAKE(255,255,255);

	int R = (col >> 16) & 0xFF;
	int G = (col >>  8) & 0xFF;
	int B = (col      ) & 0xFF;

	SYS_ASSERT(num > 0);
	SYS_ASSERT(base_qty >= 0);

	switch (mo->info->dlight0.type)
	{
		case DLITE_None:
			I_Error("R2_AddColourDLights: bad dynamic light\n");

		default:  //!!!!! FIXME
			break;

		case DLITE_Linear:
			for (; num > 0; num--, r++, g++, b++, x++, y++, z++)
			{
				float dist = APPROX_DIST3(fabs((*x) - mo->x),
					fabs((*y) - mo->y), fabs((*z) - mo_z));

				int qty = (int)(base_qty * 32.0f / MAX(1.0f, dist));

				(*r) += qty * R / 255;
				(*g) += qty * G / 255;
				(*b) += qty * B / 255;
			}
			break;

		case DLITE_Quadratic:
			for (; num > 0; num--, r++, g++, b++, x++, y++, z++)
			{
				float dist = APPROX_DIST3(fabs((*x) - mo->x),
					fabs((*y) - mo->y), fabs((*z) - mo_z));

				int qty = (int)(base_qty * 1024.0f / MAX(1.0f, dist * dist));

				(*r) += qty * R / 255;
				(*g) += qty * G / 255;
				(*b) += qty * B / 255;
			}
			break;
	}
#endif
}

//
// R2_FindDLights
//
static void R2_FindDLights(subsector_t *sub, drawfloor_t *dfloor)
{
	//!!!!!
	float max_dlight_radius = 5*200.0f; // (use_dlights == 1) ? 300.0f : 200.0f;

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
			if (! mo->bright || mo->dlight[0].r <= 0)
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


static void ClipPlaneHorizontalLine(GLdouble *p, const vec2_t& s,
	const vec2_t& e, bool flip)
{
	if (! flip)
	{
		p[0] = e.y - s.y;
		p[1] = s.x - e.x;
		p[2] = 0.0f;
		p[3] = e.x * s.y - s.x * e.y;
	}
	else
	{
		p[0] = s.y - e.y;
		p[1] = e.x - s.x;
		p[2] = 0.0f;
		p[3] = - e.x * s.y + s.x * e.y;
	}
}

static void ClipPlaneHorizontalEye(GLdouble *p, const vertex_t *v, bool flip)
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

static void FloodSetClipPlanes(const seg_t *seg, float h1, float h2)
{
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
}

static void FloodResetClipPlanes(void)
{
	glDisable(GL_CLIP_PLANE0);
	glDisable(GL_CLIP_PLANE1);
	glDisable(GL_CLIP_PLANE2);
	glDisable(GL_CLIP_PLANE3);
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

int RGL_LightEmu(int nominal)
{
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


// TODO: merge Wall and Plane coordinate functions into one.
//       (After removing vertex lighting).

void WallCoordFunc(vec3_t *src, local_gl_vert_t *vert, void *d)
{
	wall_plane_data_t *data = (wall_plane_data_t *)d;

	int R = data->col[0];
	int G = data->col[1];
	int B = data->col[2];

	float x = src->x;
	float y = src->y;
	float z = src->z;

	float tx, ty;
	float along;

	// compute texture coord
	if (fabs(data->div.dx) > fabs(data->div.dy))
	{
		CHECKVAL(data->div.dx);
		along = (x - data->div.x) / data->div.dx;
	}
	else
	{
		CHECKVAL(data->div.dy);
		along = (y - data->div.y) / data->div.dy;
	}

	tx = data->tx + along * data->tdx;
	ty = data->ty + z * data->ty_mul + along * data->ty_skew;

	// FADING CALC
	float tx2, ty2;
	{
		// distance from viewplane: (point - camera) . viewvec

		float lk_sin = M_Sin(viewvertangle);
		float lk_cos = M_Cos(viewvertangle);

		vec3_t viewvec;

		viewvec.x = lk_cos * viewcos;
		viewvec.y = lk_cos * viewsin;
		viewvec.z = lk_sin;

		float vx = (x - viewx) * viewvec.x;
		float vy = (y - viewy) * viewvec.y;
		float vz = (z - viewz) * viewvec.z;

		tx2 = (vx + vy + vz) / 2048.0;

		ty2 = data->light / 256.0 / 2.0 + (data->cmx==1 ? 0.5 : 0);
		if (ty2 < 0.01) ty2 = 0.01;
		if (ty2 > 0.99) ty2 = 0.99;
	}

	SET_COLOR(LT_RED(R), LT_GRN(G), LT_BLU(B), data->trans);
	SET_TEXCOORD(tx, ty);
	SET_TEX2COORD(tx2, ty2);
	SET_NORMAL(data->normal.x, data->normal.y, data->normal.z);
	SET_EDGE_FLAG(GL_TRUE);
	SET_VERTEX(x, y, z);
}


void PlaneCoordFunc(vec3_t *src, local_gl_vert_t *vert, void *d)
{
	wall_plane_data_t *data = (wall_plane_data_t *)d;

	float x = src->x;
	float y = src->y;
	float z = src->z;

	float rx = (x + data->tx) / IM_WIDTH(data->image);
	float ry = (y + data->ty) / IM_HEIGHT(data->image);

	float tx = rx * data->x_mat.x + ry * data->x_mat.y;
	float ty = rx * data->y_mat.x + ry * data->y_mat.y;

	int R = data->col[0];
	int G = data->col[1];
	int B = data->col[2];

	// FADING CALC
	float tx2, ty2;
	{
		// distance from viewplane: (point - camera) . viewvec

		float lk_sin = M_Sin(viewvertangle);
		float lk_cos = M_Cos(viewvertangle);

		vec3_t viewvec;

		viewvec.x = lk_cos * viewcos;
		viewvec.y = lk_cos * viewsin;
		viewvec.z = lk_sin;

		float vx = (x - viewx) * viewvec.x;
		float vy = (y - viewy) * viewvec.y;
		float vz = (z - viewz) * viewvec.z;

		tx2 = (vx + vy + vz) / 2048.0;

		ty2 = data->light / 256.0 / 2.0 + (data->cmx==1 ? 0.5 : 0);
		if (ty2 < 0.01) ty2 = 0.01;
		if (ty2 > 0.99) ty2 = 0.99;
	}

	SET_COLOR(LT_RED(R), LT_GRN(G), LT_BLU(B), data->trans);
	SET_TEXCOORD(tx, ty);
	SET_TEX2COORD(tx2, ty2);
	SET_NORMAL(data->normal.x, data->normal.y, data->normal.z);
	SET_EDGE_FLAG(GL_TRUE);
	SET_VERTEX(x, y, z);
}


void ShadowCoordFunc(vec3_t *src, local_gl_vert_t *vert, void *d)
{
	wall_plane_data_t *data = (wall_plane_data_t *)d;

	float x = src->x;
	float y = src->y;
	float z = src->z;

	float rx = (x + data->tx);
	float ry = (y + data->ty);

	float tx = rx * data->x_mat.x + ry * data->x_mat.y;
	float ty = rx * data->y_mat.x + ry * data->y_mat.y;

	int R = 0;
	int G = 0;
	int B = 0;

	SET_COLOR(LT_RED(R), LT_GRN(G), LT_BLU(B), data->trans);
	SET_TEXCOORD(tx, ty);
	SET_NORMAL(data->normal.x, data->normal.y, data->normal.z);
	SET_EDGE_FLAG(GL_TRUE);
	SET_VERTEX(x, y, z);
}

#ifdef DLIGHT_PROTOTYPE

//!!!! FIXME big hack crap
static int dl_R, dl_G, dl_B;
static wall_plane_data_t *dl_WP;

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
		CHECKVAL(data->div.dx);
		along = (x - data->div.x) / data->div.dx;
	}
	else
	{
		CHECKVAL(data->div.dy);
		along = (y - data->div.y) / data->div.dy;
	}

	float tx = data->tx + along * data->tdx;
	float ty = data->ty + z * data->ty_mul + along * data->ty_skew;

	// compute texture coord
	if (fabs(dl_WP->div.dx) > fabs(dl_WP->div.dy))
	{
		CHECKVAL(dl_WP->div.dx);
		along = (x - dl_WP->div.x) / dl_WP->div.dx;
	}
	else
	{
		CHECKVAL(dl_WP->div.dy);
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


//
// RGL_DrawWall
//
// Note: mid_masked is 2 & 3 for horiz. sliding door.
//
static void RGL_DrawWall(drawfloor_t *dfloor, float top,
						 float bottom, float tex_top_h, surface_t *part,
						 int mid_masked, bool opaque, float x_offset,
						 region_properties_t *masked_props)

{
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

	float trans = part->translucency;
	bool blended;

	GLuint tex_id=0, tex_id2=0;

	raw_polyquad_t *poly;
	wall_plane_data_t data;

	region_properties_t *props = masked_props ? masked_props :
		part->override_p ? part->override_p : dfloor->props;

	int lit_Nom = props->lightlevel;
	const colourmap_c *colmap = props->colourmap;
	float c_r, c_g, c_b;

	// ignore non-solid walls in solid mode (& vice versa)
	blended = (part->translucency <= 0.99f) ? true : false;
	if ((blended || mid_masked) == solid_mode)
		return;

	SYS_ASSERT(currmap);

	// do the N/S/W/E bizzo...
	if (currmap->lighting == LMODEL_Doom && lit_Nom > 0)
	{
		if (cur_seg->v1->y == cur_seg->v2->y)
			lit_Nom -= 8;
		else if (cur_seg->v1->x == cur_seg->v2->x)
			lit_Nom += 8;

		// limit to 0..255 range
		lit_Nom = MAX(0, MIN(255, lit_Nom));
	}

	data.light = lit_Nom;
	data.flood_emu = false;

	lit_Nom = RGL_Light(lit_Nom);

	V_GetColmapRGB(colmap, &c_r, &c_g, &c_b, false);

	data.col[0] = 255; //!!!!! (int)((doom_fading ? 255 : lit_Nom) * c_r);
	data.col[1] = 255; //!!!!! (int)((doom_fading ? 255 : lit_Nom) * c_g);
	data.col[2] = 255; //!!!!! (int)((doom_fading ? 255 : lit_Nom) * c_b);

	data.trans = trans;

	data.dlights = dfloor->dlights;
	data.image   = part->image;

	SYS_ASSERT(part->image);
	tex_id = W_ImageCache(part->image);

	// FADING MAP
	{
		tex_id2 = W_ImageCache(fading_image);
	}

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

		// which side is seg on ?
		int side = 0;

		if ((cur_seg->v2->x - cur_seg->v1->x) * cur_seg->linedef->dx < 0 ||
			(cur_seg->v2->y - cur_seg->v1->y) * cur_seg->linedef->dy < 0)
		{
			side = 1;
		}

		if (side == 0)
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
		CHECKVAL(smov->line_len);

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

		if (side == 1)
		{
			float tex_tmp = tex_x1;
			tex_x1 = tex_x2;
			tex_x2 = tex_tmp;
		}
	}
	else if (mid_masked == 1 && cur_seg->linedef->special &&
		cur_seg->linedef->special->s.type != SLIDE_None)
	{
		// which side is seg on ?
		int side = 0;

		if ((cur_seg->v2->x - cur_seg->v1->x) * cur_seg->linedef->dx < 0 ||
			(cur_seg->v2->y - cur_seg->v1->y) * cur_seg->linedef->dy < 0)
		{
			side = 1;
		}

		if (side == 1)
		{
			tex_x1 = IM_WIDTH(part->image) - tex_x1;
			tex_x2 = IM_WIDTH(part->image) - tex_x2;
		}
	}

	float total_w = IM_TOTAL_WIDTH( part->image);
	float total_h = IM_TOTAL_HEIGHT(part->image);

	total_w *= part->x_mat.x;
	total_h *= part->y_mat.y;

	tex_x1 = tex_x1 / total_w;
	tex_x2 = tex_x2 / total_w;

	data.tx  = tex_x1;
	data.tdx = tex_x2 - tex_x1;

	data.ty_mul = 1.0f / total_h;
	data.ty = IM_TOP(part->image) - tex_top_h * data.ty_mul;
	data.ty_skew = 0;

#if (DEBUG >= 3) 
	L_WriteDebug( "WALL (%d,%d,%d) -> (%d,%d,%d)\n", 
		(int) x1, (int) y1, (int) top, (int) x2, (int) y2, (int) bottom);
#endif

	data.normal.x = (y2 - y1);
	data.normal.y = (x1 - x2);
	data.normal.z = 0;

	data.div.x  = cur_seg->v1->x;
	data.div.y  = cur_seg->v1->y;
	data.div.dx = cur_seg->v2->x - data.div.x;
	data.div.dy = cur_seg->v2->y - data.div.y;

	data.cmx = 0;

	poly = RGL_NewPolyQuad(4);

	PQ_ADD_VERT(poly, x1, y1, bottom);
	PQ_ADD_VERT(poly, x1, y1, top);
	PQ_ADD_VERT(poly, x2, y2, top);
	PQ_ADD_VERT(poly, x2, y2, bottom);

	RGL_BoundPolyQuad(poly);

	int blending = (blended ? BL_Alpha : 0) | (mid_masked ? BL_Masked : 0);

	// -AJA- 2006-06-22: fix for midmask wrapping bug
	if (mid_masked == 1)
		blending |= BL_ClampY;

	RGL_RenderPolyQuad(poly, &data, WallCoordFunc, tex_id,tex_id2,
		/* pass */ 0, blending | BL_Multi);

#if 1  // COLORMAP ADD SHIT

	data.cmx = 1;

	RGL_RenderPolyQuad(poly, &data, WallCoordFunc, 0,tex_id2,
		/* pass */ 1, blending | BL_Multi | BL_Add);
#endif

	RGL_FreePolyQuad(poly);

#ifdef DLIGHT_PROTOTYPE
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
					/* pass */ 2+DL, BL_Multi|BL_Add);

				RGL_FreePolyQuad(poly);
RGL_DrawUnits();
			}
		}
	}
#endif // DLIGHT_PROTOTYPE
}

static void EmulateFlooding(const drawfloor_t *dfloor,
	const sector_t *flood_ref, int face_dir, float h1, float h2)
{
	if (num_active_mirrors > 0) return;

	const surface_t *info = (face_dir > 0) ? &flood_ref->floor :
		&flood_ref->ceil;

	const region_properties_t *props = info->override_p ?
		info->override_p : &flood_ref->props;

	float h;
	float bbox[4];

	// ignore sky and invisible planes
	if (IS_SKY(*info) || info->translucency < 0.04f)
		return;

	// ignore transparent doors (TNT MAP02)
	if (flood_ref->f_h >= flood_ref->c_h)
		return;

	// ignore fake 3D bridges (Batman MAP03)
	if (cur_seg->linedef &&
	    cur_seg->linedef->frontsector == cur_seg->linedef->backsector)
		return;

	FloodSetClipPlanes(cur_seg, h1, h2);

	h = (face_dir > 0) ? h2 : h1;

	// --- compute required area ---

	M_ClearBox(bbox);
	M_AddToBox(bbox, cur_seg->v1->x, cur_seg->v1->y);
	M_AddToBox(bbox, cur_seg->v2->x, cur_seg->v2->y);

	// include viewpoint in BBOX, but limited to FLOOD_DIST
	float seg_mx = (cur_seg->v1->x + cur_seg->v2->x) / 2.0f;
	float seg_my = (cur_seg->v1->y + cur_seg->v2->y) / 2.0f;
	float seg_vdx = viewx - seg_mx;
	float seg_vdy = viewy - seg_my;

	if (seg_vdx < -FLOOD_DIST) seg_vdx = -FLOOD_DIST;
	if (seg_vdx > +FLOOD_DIST) seg_vdx = +FLOOD_DIST;

	if (seg_vdy < -FLOOD_DIST) seg_vdy = -FLOOD_DIST;
	if (seg_vdy > +FLOOD_DIST) seg_vdy = +FLOOD_DIST;
	
	M_AddToBox(bbox, seg_mx + seg_vdx, seg_my + seg_vdy);

	bbox[BOXLEFT]   -= FLOOD_EXPAND;
	bbox[BOXRIGHT]  += FLOOD_EXPAND;
	bbox[BOXBOTTOM] -= FLOOD_EXPAND;
	bbox[BOXTOP]    += FLOOD_EXPAND;

	// stuff from DrawPlane (FIXME: factor out common stuff ?)

	wall_plane_data_t data;

	GLuint tex_id;

//!!!!!!	int lit_Nom = RGL_Light(props->lightlevel);
	const colourmap_c *colmap = props->colourmap;
	float c_r, c_g, c_b;

	data.light = props->lightlevel;

	data.flood_emu = true;
	data.emu_mx = seg_mx;
	data.emu_my = seg_my;

	V_GetColmapRGB(colmap, &c_r, &c_g, &c_b, false);

	data.col[0] = 255; //!!!!! (int)((doom_fading ? 255 : lit_Nom) * c_r);
	data.col[1] = 255; //!!!!! (int)((doom_fading ? 255 : lit_Nom) * c_g);
	data.col[2] = 255; //!!!!! (int)((doom_fading ? 255 : lit_Nom) * c_b);

	data.trans = 1.0f;

	data.dlights = dfloor->dlights;
	data.image   = info->image;

	data.normal.x = 0;
	data.normal.y = 0;
	data.normal.z = face_dir;

	data.tx = info->offset.x;
	data.ty = info->offset.y;

	data.x_mat = info->x_mat;
	data.y_mat = info->y_mat;

	SYS_ASSERT(info->image);
	tex_id = W_ImageCache(info->image);

	vec3_t vbox[4];

	vbox[0].Set(bbox[BOXLEFT ], bbox[BOXBOTTOM], h);
	vbox[1].Set(bbox[BOXLEFT ], bbox[BOXTOP   ], h);
	vbox[2].Set(bbox[BOXRIGHT], bbox[BOXTOP   ], h);
	vbox[3].Set(bbox[BOXRIGHT], bbox[BOXBOTTOM], h);

	// update the depth buffer (prevent overdraw from behind)
	if (! dumb_sky)
	{
		glColor3f(0.0f, 0.0f, 0.0f);
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

		glBegin(GL_QUAD_STRIP);
		glVertex3f(cur_seg->v1->x, cur_seg->v1->y, h1);
		glVertex3f(cur_seg->v1->x, cur_seg->v1->y, h2);
		glVertex3f(cur_seg->v2->x, cur_seg->v2->y, h1);
		glVertex3f(cur_seg->v2->x, cur_seg->v2->y, h2);
		glEnd();

		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	}

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, tex_id);

	glDepthMask(GL_FALSE);

	glColor3f(1.0f, 1.0f, 1.0f);
	glBegin(GL_POLYGON);

	for (int j = 0; j < 4; j++)
	{
		local_gl_vert_t V;

  		PlaneCoordFunc(vbox + j, &V, &data);

		RGL_SendRawVector(&V);		
	}

	glEnd();

	glDepthMask(GL_TRUE);
	glDisable(GL_TEXTURE_2D);

	FloodResetClipPlanes();
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

	int j;
	wall_tile_t *wt;
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

	for (j=0; j < sd->tile_used; j++)
	{
		wt = sd->tiles + j;

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
				wt->surface, 2, opaque, x_offset, NULL);

			if (cur_seg->linedef->special->s.type == SLIDE_Center)
			{
				RGL_DrawWall(dfloor, c, f, tex_top_h,
					wt->surface, 3, opaque, x_offset, NULL);
			}
			continue;
		}

		RGL_DrawWall(dfloor, c, f, tex_top_h,
			wt->surface, (wt->flags & WTILF_MidMask) ? 1 : 0, 
			opaque, x_offset, (wt->flags & WTILF_MidMask) ?
			  &cur_seg->sidedef->sector->props : NULL);
	}

	// -AJA- 2004/04/21: Emulate Flat-Flooding TRICK
	if (! hom_detect && solid_mode && dfloor->prev == NULL &&
		sd->bottom.image == NULL && cur_seg->back_sub &&
		cur_seg->back_sub->sector->f_h > cur_seg->front_sub->sector->f_h &&
		cur_seg->back_sub->sector->f_h < viewz)
	{
		EmulateFlooding(dfloor, cur_seg->back_sub->sector, +1,
			cur_seg->front_sub->sector->f_h,
			cur_seg->back_sub->sector->f_h);
	}

	if (! hom_detect && solid_mode && dfloor->prev == NULL &&
		sd->top.image == NULL && cur_seg->back_sub &&
		cur_seg->back_sub->sector->c_h < cur_seg->front_sub->sector->c_h &&
		cur_seg->back_sub->sector->c_h > viewz)
	{
		EmulateFlooding(dfloor, cur_seg->back_sub->sector, -1,
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


static void RGL_WalkMirror(drawsub_c *dsub, seg_t *seg)
{
	subsector_t *save_sub = cur_sub;

	drawmirror_c *mir = R_GetDrawMirror();
	mir->Clear(seg->linedef);

	dsub->mirrors.push_back(mir);

	// push mirror (translation matrix)
	MIR_Push(mir);

	// perform another BSP walk
	RGL_WalkBSPNode(root_node);

	// pop mirror
	MIR_Pop();

	cur_sub = save_sub;
}


//
// RGL_WalkSeg
//
// Visit a single seg of the subsector, and for one-sided lines update
// the 1D occlusion buffer.
//
static void RGL_WalkSeg(drawsub_c *dsub, seg_t *seg)
{
	angle_t angle1, angle2;
	angle_t span, tspan1, tspan2;

	// ignore segs sitting on current mirror
	if (num_active_mirrors > 0 && seg->linedef ==
		active_mirrors[num_active_mirrors-1].def->line)
		return;

//  float l_viewx = viewx, l_viewy = viewy;
//	MIR_Coordinate(l_viewx, l_viewy);
	
	float sx1 = seg->v1->x;
	float sy1 = seg->v1->y;

	float sx2 = seg->v2->x;
	float sy2 = seg->v2->y;

	MIR_Coordinate(sx1, sy1);
	MIR_Coordinate(sx2, sy2);

	if (num_active_mirrors % 2)
	{
		float tx = sx1; sx1 = sx2; sx2 = tx;
		float ty = sy1; sy1 = sy2; sy2 = ty;
	}

	angle1 = R_PointToAngle(viewx, viewy, sx1, sy1);
	angle2 = R_PointToAngle(viewx, viewy, sx2, sy2);

#if (DEBUG >= 3)
	L_WriteDebug( "INIT ANGLE1 = %1.2f  ANGLE2 = %1.2f\n", 
		ANG_2_FLOAT(angle1), ANG_2_FLOAT(angle2));
#endif

	// Clip to view edges.
	// -ES- 1999/03/20 Replaced clipangle with clipscope/leftclipangle/rightclipangle

	span = angle1 - angle2;

	// back side ?
	if (span >= ANG180)
		return;

	angle1 -= viewangle;
	angle2 -= viewangle;

#if (DEBUG >= 3)
	L_WriteDebug( "ANGLE1 = %1.2f  ANGLE2 = %1.2f  (linedef %d)\n", 
		ANG_2_FLOAT(angle1), ANG_2_FLOAT(angle2), seg->linedef - lines);
#endif

	if (oned_side_angle != ANG180)
	{
		tspan1 = angle1 - rightclipangle;
		tspan2 = leftclipangle - angle2;

		if (tspan1 > clipscope)
		{
			// Totally off the left edge?
			if (tspan2 >= ANG180)
				return;

			angle1 = leftclipangle;
		}

		if (tspan2 > clipscope)
		{
			// Totally off the left edge?
			if (tspan1 >= ANG180)
				return;

			angle2 = rightclipangle;
		}
	}

#if (DEBUG >= 3)
	L_WriteDebug( "CLIPPED ANGLE1 = %1.2f  ANGLE2 = %1.2f\n", 
		ANG_2_FLOAT(angle1), ANG_2_FLOAT(angle2));
#endif

	// The seg is in the view range,
	// but not necessarily visible.

#if (DEBUG >= 2)
	L_WriteDebug( "  %sSEG %p (%1.1f, %1.1f) -> (%1.1f, %1.1f)\n",
		seg->miniseg ? "MINI" : "", seg, seg->v1->x, seg->v1->y, 
		seg->v2->x, seg->v2->y);
#endif

	// check if visible
#if 1
	if (span > (ANG1/4) && RGL_1DOcclusionTest(angle2, angle1))
	{
		return;
	}
#endif

	dsub->visible = true;

	if (seg->miniseg)
		return;

	if ((seg->linedef->flags & MLF_Mirror) &&
		(num_active_mirrors < MAX_MIRRORS))
	{
		// FIXME: check if linedef already seen
		// if (XXX) return;

		RGL_WalkMirror(dsub, seg);

//!!!!!!		RGL_1DOcclusionSet(angle2, angle1);
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
//!!!!!!		RGL_1DOcclusionSet(angle2, angle1);
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
return true; //!!!!!! MIRRORS
	int boxx, boxy;
	int boxpos;

	float x1, y1, x2, y2;

	angle_t angle1, angle2;
	angle_t span, tspan1, tspan2;

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

	boxpos = (boxy << 2) + boxx;

	if (boxpos == 5)
		return true;

	x1 = bspcoord[checkcoord[boxpos][0]];
	y1 = bspcoord[checkcoord[boxpos][1]];
	x2 = bspcoord[checkcoord[boxpos][2]];
	y2 = bspcoord[checkcoord[boxpos][3]];

	// check clip list for an open space
	angle1 = R_PointToAngle(viewx, viewy, x1, y1) - viewangle;
	angle2 = R_PointToAngle(viewx, viewy, x2, y2) - viewangle;

	span = angle1 - angle2;

	// Sitting on a line?
	if (span >= ANG180)
		return true;

	// -ES- 1999/03/20 Replaced clipangle with clipscope/leftclipangle/rightclipangle

	if (oned_side_angle != ANG180)
	{
		tspan1 = angle1 - rightclipangle;
		tspan2 = leftclipangle - angle2;

		if (tspan1 > clipscope)
		{
			// Totally off the left edge?
			if (tspan2 >= ANG180)
				return false;

			angle1 = leftclipangle;
		}

		if (tspan2 > clipscope)
		{
			// Totally off the right edge?
			if (tspan1 >= ANG180)
				return false;

			angle2 = rightclipangle;
		}
	}

	return ! RGL_1DOcclusionTest(angle2, angle1);
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

	wall_plane_data_t data;
	raw_polyquad_t *poly;

	GLuint tex_id=0, tex_id2=0;

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

	const colourmap_c *colmap = props->colourmap;
	float c_r, c_g, c_b;

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

//!!!!!!	int lit_Nom = RGL_Light(props->lightlevel);

	data.light = props->lightlevel;
	data.flood_emu = false;

	V_GetColmapRGB(colmap, &c_r, &c_g, &c_b, false);

	data.col[0] = 255; //!!!!! (int)((doom_fading ? 255 : lit_Nom) * c_r);
	data.col[1] = 255; //!!!!! (int)((doom_fading ? 255 : lit_Nom) * c_g);
	data.col[2] = 255; //!!!!! (int)((doom_fading ? 255 : lit_Nom) * c_b);

	data.trans = trans;

	data.dlights = dfloor->dlights;
	data.image   = surf->image;

	data.normal.x = 0;
	data.normal.y = 0;
	data.normal.z = (viewz > h) ? 1.0f : -1.0f;

	SYS_ASSERT(surf->image);
	tex_id = W_ImageCache(surf->image);

	// FADING MAP
	{
		tex_id2 = W_ImageCache(fading_image);
	}

	data.tx = surf->offset.x;
	data.ty = surf->offset.y;

	data.x_mat = surf->x_mat;
	data.y_mat = surf->y_mat;

	data.cmx = 0;

	poly = RGL_NewPolyQuad(num_vert);

	for (seg=cur_sub->segs, i=0; seg && (i < MAX_PLVERT); 
		seg=seg->sub_next, i++)
	{
		float x = seg->v1->x;
		float y = seg->v1->y;
			
MIR_Coordinate(x, y);

		PQ_ADD_VERT(poly, x, y, h);
	}

	RGL_BoundPolyQuad(poly);

	int blending = (blended ? BL_Alpha : 0) | (mid_masked ? BL_Masked : 0);

	RGL_RenderPolyQuad(poly, &data, PlaneCoordFunc, tex_id,tex_id2,
		/* pass */ 0, blending | BL_Multi);

#if 1  // COLORMAP ADD SHIT

	data.cmx = 1;

	RGL_RenderPolyQuad(poly, &data, WallCoordFunc, 0,tex_id2,
		/* pass */ 1, blending | BL_Multi | BL_Add);
#endif

	RGL_FreePolyQuad(poly);

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

#ifdef DLIGHT_PROTOTYPE
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
					/* pass */ 2+DL, BL_Multi|BL_Add);

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

		if (cur_sub == viewsubsector && f_h <= viewz && viewz <= c_h)
			RGL_LightUpPlayerWeapon(players[displayplayer], dfloor);
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

	float alpha = 0.1; //!!!! 0.12 * (num_active_mirrors + 1);

	glColor4f(0.0, 1.0, 0.0, alpha);

	float x1 = mir->line->v1->x;
	float y1 = mir->line->v1->y;
	float z1 = mir->line->frontsector->f_h;

	float x2 = mir->line->v2->x;
	float y2 = mir->line->v2->y;
	float z2 = mir->line->frontsector->c_h;

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
	// FIXME: push mirror (clip planes | translation matrix)

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

		std::list<drawseg_c *>::iterator SEGI;

		for (SEGI = dsub->segs.begin(); SEGI != dsub->segs.end(); SEGI++)
		{
			RGL_BuildWalls(dfloor, (*SEGI)->seg);
		}

		RGL_DrawPlane(dfloor, dfloor->c_h, dfloor->ceil,  -1);
		RGL_DrawPlane(dfloor, dfloor->f_h, dfloor->floor, +1);

		if (! solid_mode)
		{
			RGL_DrawSortThings(dfloor);
  		}
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

//	float l_viewx = viewx, l_viewy = viewy;
//  MIR_Coordinate(l_viewx, l_viewy);

	divline_t nd_div;
	nd_div.x = node->div.x;
	nd_div.y = node->div.y;
	nd_div.dx = node->div.x + node->div.dx;
	nd_div.dy = node->div.y + node->div.dy;

	MIR_Coordinate(nd_div.x, nd_div.y);
	MIR_Coordinate(nd_div.dx, nd_div.dy);

	nd_div.dx -= nd_div.x;
	nd_div.dy -= nd_div.y;
	
	side = P_PointOnDivlineSide(viewx, viewy, &nd_div);

#if (DEBUG >= 2)
	L_WriteDebug( "NODE %d (%1.1f, %1.1f) -> (%1.1f, %1.1f)  SIDE %d\n",
		bspnum, node->div.x, node->div.y, node->div.x +
		node->div.dx, node->div.y + node->div.dy, side);
#endif

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
	fading_image = W_ImageLookup("COLMAP_TEST");

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
		leftclipangle  = 0 + oned_side_angle;
		rightclipangle = 0 - oned_side_angle;
		clipscope = leftclipangle - rightclipangle;
	}
	else
	{
		// not clipping to the viewport.  Dummy values.
		leftclipangle  = 0 + ANG5;
		rightclipangle = 0 - ANG5;
		clipscope = leftclipangle - rightclipangle;
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
		glClearColor(0.5f, 0.0f, 0.5f, 0.0f);
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

	// NOTE: this call will move for layer system (should be topmost).
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
