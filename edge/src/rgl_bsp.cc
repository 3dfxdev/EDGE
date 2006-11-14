//----------------------------------------------------------------------------
//  EDGE OpenGL Rendering (BSP Traversal)
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
#include "r_main.h"
#include "rgl_defs.h"
#include "rgl_occ.h"
#include "rgl_fx.h"
#include "rgl_sky.h"
#include "rgl_thing.h"
#include "rgl_unit.h"
#include "w_image_gl.h"
#include "v_colour.h"

#include <math.h>

#define DEBUG  0

#define HALOS    0

#define FLOOD_DIST    1024.0f
#define FLOOD_EXPAND  128.0f


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

static subsector_t *drawsubs_head;
static subsector_t *drawsubs_tail;

#ifdef SHADOW_PROTOTYPE
static const image_t *shadow_image = NULL;
#endif
#ifdef DLIGHT_PROTOTYPE
static const image_t *linear_image = NULL;
static const image_t *quad_image = NULL;
#endif

// ============================================================================
// R2_BSP START
// ============================================================================

int detail_level = 1;
int use_dlights = 0;

bool doom_fading = true;


//
// R2_AddColourDLights
//
// Increases the arrays of colour levels at the points (x,y,z).
//
void R2_AddColourDLights(int num, int *r, int *g, int *b, 
						 float *x, float *y, float *z, mobj_t *mo)
{
	float base_qty = mo->dlight_qty;
	float mo_z = mo->z + mo->height * PERCENT_2_FLOAT(mo->info->dlight.height);

	rgbcol_t col = mo->info->dlight.colour;

#if 0  // TEST CODE
	if (col == RGB_NO_VALUE)
	{
		bool flip;
		const image_t *img = R2_GetThingSprite(mo, &flip);
		const cached_image_t *cim = W_ImageCache(img);
		col = W_ImageGetHue(cim);
		W_ImageDone(cim);
	}
#endif
	if (col == RGB_NO_VALUE)
		col = RGB_MAKE(255,255,255);

	int R = (col >> 16) & 0xFF;
	int G = (col >>  8) & 0xFF;
	int B = (col      ) & 0xFF;

	DEV_ASSERT2(num > 0);
	DEV_ASSERT2(base_qty >= 0);

	switch (mo->info->dlight.type)
	{
		case DLITE_None:
			I_Error("R2_AddColourDLights: bad dynamic light\n");

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
}

//
// R2_FindDLights
//
static void R2_FindDLights(subsector_t *sub, drawfloor_t *dfloor)
{
	float max_dlight_radius = 200.0f; // (use_dlights == 1) ? 300.0f : 200.0f;

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
				if (! mo->bright || mo->dlight_qty <= 0)
					continue;

				if (mo->ceilingz <= dfloor->f_h || mo->floorz >= dfloor->top_h)
				{
					continue;
				}

				dl = drawthings.GetNew();
				drawthings.Commit();

				dl->mo = mo;
				dl->tz = mo->z + mo->height * PERCENT_2_FLOAT(mo->info->dlight.height);

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
	DEV_ASSERT2(currmap);

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

	drawthing_t *dlights;
	const image_t *image;

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
	ty = data->ty - z * data->ty_mul + along * data->ty_skew;

	// emulated Doom lighting (fades away)
	if (doom_fading)
	{
		float dist = APPROX_DIST3(fabs(x - viewx), fabs(y - viewy),
								  fabs(z - viewz));

		int L = EMU_LIGHT(data->light, dist);

		L = RGL_LightEmu((L < 0) ? 0 : (L > 255) ? 255 : L);

		R = (R * L) >> 8;
		G = (G * L) >> 8;
		B = (B * L) >> 8;
	}

	// dynamic lighting
	if (use_dlights)  // == 2 (COMPAT)
	{
		drawthing_t *dl;

		for (dl=data->dlights; dl; dl=dl->next)
		{
			// light behind seg ?    
			if (P_PointOnDivlineSide(dl->mo->x, dl->mo->y, &data->div) != 0)
				continue;

			R2_AddColourDLights(1, &R, &G, &B, &x, &y, &z, dl->mo);
		}
	}

	SET_COLOR(LT_RED(R), LT_GRN(G), LT_BLU(B), data->trans);
	SET_TEXCOORD(tx, ty);
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

	// emulated Doom lighting (fades away)
	if (doom_fading)
	{
		float dist;

		if (data->flood_emu) // -AJA- HORRIBLE HACK
			dist = APPROX_DIST3(fabs(data->emu_mx - viewx), fabs(data->emu_my - viewy),
								  fabs(z - viewz));
		else
			dist = APPROX_DIST3(fabs(x - viewx), fabs(y - viewy),
								  fabs(z - viewz));

		int L = EMU_LIGHT(data->light, dist);

		L = RGL_LightEmu((L < 0) ? 0 : (L > 255) ? 255 : L);

		R = (R * L) >> 8;
		G = (G * L) >> 8;
		B = (B * L) >> 8;
	}

	// dynamic lighting
	if (use_dlights)  // == 2 (COMPAT)
	{
		drawthing_t *dl;

		for (dl=data->dlights; dl; dl=dl->next)
		{
			// light behind the plane ?    
			if ((dl->tz > z) != (data->normal.z > 0))
				continue;

			R2_AddColourDLights(1, &R, &G, &B, &x, &y, &z, dl->mo);
		}
	}

	SET_COLOR(LT_RED(R), LT_GRN(G), LT_BLU(B), data->trans);
	SET_TEXCOORD(tx, ty);
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
	float ty = data->ty - z * data->ty_mul + along * data->ty_skew;

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
	float ty0 = dl_WP->ty - z * dl_WP->ty_mul + along * dl_WP->ty_skew;

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
	if (dist < 4)
		dist = 4;

	if (mo->info->dlight.type == DLITE_Linear)
	{
		*radius = DL_OUTER * dist;
		*intensity = mo->dlight_qty / 8.0f / dist;

		if (*intensity > 1.0f)
		{
			*radius *= (*intensity);
			*intensity = 1.0f;
		}
	}
	else  /* DLITE_Quadratic */
	{
		*radius = DL_OUTER_SQRT * dist;
		*intensity = mo->dlight_qty * 2.0f / dist / dist;

		if (*intensity > 1.0f)
		{
			*radius *= sqrt(*intensity);
			*intensity = 1.0f;
		}
	}
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

	float xy_ofs = cur_seg->offset;
	float xy_len = cur_seg->length;

	float tex_x1 = 0, tex_y1 = 0;
	float tex_x2 = 0, tex_y2 = 0, tex_dy = 0;

	float trans = part->translucency;
	bool blended;

	GLuint tex_id;
	const cached_image_t *cim;

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

	DEV_ASSERT2(currmap);

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

	data.col[0] = (int)((doom_fading ? 255 : lit_Nom) * c_r);
	data.col[1] = (int)((doom_fading ? 255 : lit_Nom) * c_g);
	data.col[2] = (int)((doom_fading ? 255 : lit_Nom) * c_b);

	data.trans = trans;

	data.dlights = dfloor->dlights;
	data.image   = part->image;

	DEV_ASSERT2(part->image);
	cim = W_ImageCache(part->image);
	tex_id = W_ImageGetOGL(cim);

	// Note: normally this would be wrong, since we're using the GL
	// texture ID later on (after W_ImageDone).  The W_LockImagesOGL
	// call saves us though.
	W_ImageDone(cim);

	x_offset += xy_ofs;

	tex_x1 = x_offset;
	tex_x2 = tex_x1 + xy_len;

	tex_y1 = tex_top_h - top;
	tex_y2 = tex_top_h - bottom;
	tex_dy = part->y_mat.x * xy_len;

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

		DEV_ASSERT2(smov);

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

	tex_y1 = 1.0f - tex_y1 / total_h;
	tex_y2 = 1.0f - tex_y2 / total_h;
	tex_dy = 1.0f - tex_dy / total_h;

	data.ty_mul = -1.0f / total_h;
	data.ty = 1.0f + tex_top_h * data.ty_mul;
	data.ty_skew = 0;  //!!!! FIXME

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

	poly = RGL_NewPolyQuad(4, true);

	PQ_ADD_VERT(poly, x1, y1, bottom);
	PQ_ADD_VERT(poly, x1, y1, top);
	PQ_ADD_VERT(poly, x2, y2, bottom);
	PQ_ADD_VERT(poly, x2, y2, top);

	RGL_BoundPolyQuad(poly);

	if ((use_dlights /* == 2 (COMPAT) */ && data.dlights) || doom_fading)
		RGL_SplitPolyQuadLOD(poly, 1, 128 >> detail_level);

	int blending = (blended ? BL_Alpha : 0) | (mid_masked ? BL_Masked : 0);

	// -AJA- 2006-06-22: fix for midmask wrapping bug
	if (mid_masked == 1)
		blending |= BL_ClampY;

	RGL_RenderPolyQuad(poly, &data, WallCoordFunc, tex_id,0,
		/* pass */ 0, blending);

	RGL_FreePolyQuad(poly);

#ifdef DLIGHT_PROTOTYPE
	if (use_dlights == 1 && solid_mode)
	{
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

			dl_R = (mo->info->dlight.colour >> 16) & 0xFF;
			dl_G = (mo->info->dlight.colour >>  8) & 0xFF;
			dl_B = (mo->info->dlight.colour      ) & 0xFF;
			dl_WP = &data;

			cim = W_ImageCache((mo->info->dlight.type == DLITE_Linear) ?
				linear_image : quad_image);

			GLuint tex2_id = W_ImageGetOGL(cim);
			// Note: normally this would be wrong, since we're using the GL
			// texture ID later on (after W_ImageDone).  The W_LockImagesOGL
			// call saves us though.
			W_ImageDone(cim);

			float fx_radius;
			ComputeDLParameters(dist, mo, &fx_radius, &dat2.trans);

			dat2.ty = (mo->z + mo->height * PERCENT_2_FLOAT(mo->info->dlight.height));

			dat2.ty = (dat2.ty / fx_radius) + 0.5f;
			dat2.ty_mul = 1.0f / fx_radius;
			dat2.ty_skew = 0;

			dat2.tx = (mo->x - dat2.div.x) * dat2.div.dx +
			          (mo->y - dat2.div.y) * dat2.div.dy;
			dat2.tx /= cur_seg->length;

			dat2.tx = (dat2.tx / -fx_radius) + 0.5f;

			dat2.tdx = cur_seg->length * 1.0f / fx_radius;

			poly = RGL_NewPolyQuad(4, true);

			PQ_ADD_VERT(poly, x1, y1, bottom);
			PQ_ADD_VERT(poly, x1, y1, top);
			PQ_ADD_VERT(poly, x2, y2, bottom);
			PQ_ADD_VERT(poly, x2, y2, top);

			RGL_BoundPolyQuad(poly);

			RGL_RenderPolyQuad(poly, &dat2, DLightWallCoordFunc, tex_id,tex2_id,
				/* pass */ 1, BL_Multi|BL_Add);

			RGL_FreePolyQuad(poly);
		}
	}
#endif // DLIGHT_PROTOTYPE
}

static void EmulateFlooding(const drawfloor_t *dfloor,
	const sector_t *flood_ref, int face_dir, float h1, float h2)
{
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
	const cached_image_t *cim;

	int lit_Nom = RGL_Light(props->lightlevel);
	const colourmap_c *colmap = props->colourmap;
	float c_r, c_g, c_b;

	data.light = props->lightlevel;

	data.flood_emu = true;
	data.emu_mx = seg_mx;
	data.emu_my = seg_my;

	V_GetColmapRGB(colmap, &c_r, &c_g, &c_b, false);

	data.col[0] = (int)((doom_fading ? 255 : lit_Nom) * c_r);
	data.col[1] = (int)((doom_fading ? 255 : lit_Nom) * c_g);
	data.col[2] = (int)((doom_fading ? 255 : lit_Nom) * c_b);

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

	DEV_ASSERT2(info->image);
	cim = W_ImageCache(info->image);
	tex_id = W_ImageGetOGL(cim);

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

	W_ImageDone(cim);

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
static bool RGL_BuildWalls(drawfloor_t *dfloor)
{
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

		DEV_ASSERT2(wt->surface->image);

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

//
// RGL_DrawSeg
//
static void RGL_DrawSeg(seg_t *seg)
{
	drawfloor_t *dfloor;

	cur_seg = seg;

#if (DEBUG >= 2)
	L_WriteDebug("   DRAW SEG %p\n", seg);
#endif

	DEV_ASSERT2(!seg->miniseg && seg->linedef);

	// mark the segment on the automap
	seg->linedef->flags |= ML_Mapped;

	// --- handle each floor ---

	frontsector = seg->front_sub->sector;
	backsector  = NULL;

	if (seg->back_sub)
		backsector = seg->back_sub->sector;

	for (dfloor=cur_sub->floors; dfloor; dfloor=dfloor->next)
	{
		RGL_BuildWalls(dfloor);
	}
}

//
// RGL_WalkSeg
//
// Visit a single seg of the subsector, and for one-sided lines update
// the 1D occlusion buffer.
//
static void RGL_WalkSeg(seg_t *seg)
{
	angle_t angle1, angle2;
	angle_t span, tspan1, tspan2;

	seg->visible = false;

	// compute distances
	{
		float tx1 = seg->v1->x - viewx;
		float ty1 = seg->v1->y - viewy;
		float tx2 = seg->v2->x - viewx;
		float ty2 = seg->v2->y - viewy;

		seg->tx1 = tx1 * viewsin - ty1 * viewcos;
		seg->tz1 = tx1 * viewcos + ty1 * viewsin;

		seg->tx2 = tx2 * viewsin - ty2 * viewcos;
		seg->tz2 = tx2 * viewcos + ty2 * viewsin;
	}

	angle1 = R_PointToAngle(viewx, viewy, seg->v1->x, seg->v1->y);
	angle2 = R_PointToAngle(viewx, viewy, seg->v2->x, seg->v2->y);

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

	seg->visible = span > (ANG1 / 64);

	if (seg->miniseg)
		return;

	sector_t *frontsector = seg->front_sub->sector;
	sector_t *backsector  = NULL;

	if (seg->back_sub)
		backsector = seg->back_sub->sector;
		
	// only 1 sided walls affect the 1D occlusion buffer

	if (seg->linedef->blocked)
	{
		RGL_1DOcclusionSet(angle2, angle1);
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
extern int checkcoord[12][4];

bool RGL_CheckBBox(float *bspcoord)
{
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
						  surface_t *info, int face_dir)
{
	seg_t *seg;

	bool mid_masked = ! info->image->img_solid;
	bool blended;

	wall_plane_data_t data;
	raw_polyquad_t *poly;

	GLuint tex_id;
	const cached_image_t *cim;

	int num_vert, i;

	region_properties_t *props = dfloor->props;

	// more deep water hackitude
	if (cur_sub->deep_ref &&
		((face_dir > 0 && dfloor->prev == NULL) ||
		 (face_dir < 0 && dfloor->next == NULL)))
	{
		props = &cur_sub->deep_ref->props;
	}

	if (info->override_p)
		props = info->override_p;

	const colourmap_c *colmap = props->colourmap;
	float c_r, c_g, c_b;

	float trans = info->translucency;

	// ignore sky
	if (IS_SKY(*info))
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

	int lit_Nom = RGL_Light(props->lightlevel);

	data.light = props->lightlevel;
	data.flood_emu = false;

	V_GetColmapRGB(colmap, &c_r, &c_g, &c_b, false);

	data.col[0] = (int)((doom_fading ? 255 : lit_Nom) * c_r);
	data.col[1] = (int)((doom_fading ? 255 : lit_Nom) * c_g);
	data.col[2] = (int)((doom_fading ? 255 : lit_Nom) * c_b);

	data.trans = trans;

	data.dlights = dfloor->dlights;
	data.image   = info->image;

	data.normal.x = 0;
	data.normal.y = 0;
	data.normal.z = (viewz > h) ? 1.0f : -1.0f;

	DEV_ASSERT2(info->image);
	cim = W_ImageCache(info->image);
	tex_id = W_ImageGetOGL(cim);

	// Note: normally this would be wrong, since we're using the GL
	// texture ID later on (after W_ImageDone).  The W_LockImagesOGL
	// call saves us though.
	//
	W_ImageDone(cim);

	data.tx = info->offset.x;
	data.ty = info->offset.y;

	data.x_mat = info->x_mat;
	data.y_mat = info->y_mat;

	poly = RGL_NewPolyQuad(num_vert, false);

	for (seg=cur_sub->segs, i=0; seg && (i < MAX_PLVERT); 
		seg=seg->sub_next, i++)
	{
		PQ_ADD_VERT(poly, seg->v1->x, seg->v1->y, h);
	}

	RGL_BoundPolyQuad(poly);

	if ((use_dlights /* == 2 (COMPAT) */ && data.dlights) || doom_fading)
		RGL_SplitPolyQuadLOD(poly, 1, 128 >> detail_level);

	int blending = (blended ? BL_Alpha : 0) | (mid_masked ? BL_Masked : 0);

	RGL_RenderPolyQuad(poly, &data, PlaneCoordFunc, tex_id,0,
		/* pass */ 0, blending);

	RGL_FreePolyQuad(poly);

#ifdef SHADOW_PROTOTYPE
	if (level_flags.shadows && solid_mode && face_dir > 0)
	{
		wall_plane_data_t dat2;
		memcpy(&dat2, &data, sizeof(dat2));

		dat2.dlights = NULL;
		dat2.trans = 0.5;
		dat2.image = shadow_image;

		cim = W_ImageCache(dat2.image);
		tex_id = W_ImageGetOGL(cim);

		// Note: normally this would be wrong, since we're using the GL
		// texture ID later on (after W_ImageDone).  The W_LockImagesOGL
		// call saves us though.
		//
		W_ImageDone(cim);

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

			poly = RGL_NewPolyQuad(num_vert, false);

			for (seg=cur_sub->segs, i=0; seg && (i < MAX_PLVERT); 
				seg=seg->sub_next, i++)
			{
				PQ_ADD_VERT(poly, seg->v1->x, seg->v1->y, h);
			}

			RGL_BoundPolyQuad(poly);

			RGL_RenderPolyQuad(poly, &data, ShadowCoordFunc, tex_id,0,
				/* pass */ 1, BL_Alpha);

			RGL_FreePolyQuad(poly);
		}
	}
#endif

#ifdef DLIGHT_PROTOTYPE
	if (use_dlights == 1 && solid_mode)
	{
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

			dl_R = (mo->info->dlight.colour >> 16) & 0xFF;
			dl_G = (mo->info->dlight.colour >>  8) & 0xFF;
			dl_B = (mo->info->dlight.colour      ) & 0xFF;
			dl_WP = &data;

			float dist = ABS(dl->tz - h);

			cim = W_ImageCache((mo->info->dlight.type == DLITE_Linear) ?
				linear_image : quad_image);
			GLuint tex2_id = W_ImageGetOGL(cim);
			// Note: normally this would be wrong, since we're using the GL
			// texture ID later on (after W_ImageDone).  The W_LockImagesOGL
			// call saves us though.
			W_ImageDone(cim);

			float fx_radius;
			ComputeDLParameters(dist, mo, &fx_radius, &dat2.trans);

            fx_radius /= 2.0f;

			dat2.tx = -(mo->x - fx_radius);
			dat2.ty = -(mo->y - fx_radius);

			dat2.x_mat.x = 0.5f / fx_radius;
			dat2.x_mat.y = 0;

			dat2.y_mat.y = 0.5f / fx_radius;
			dat2.y_mat.x = 0;

			poly = RGL_NewPolyQuad(num_vert, false);

			for (seg=cur_sub->segs, i=0; seg && (i < MAX_PLVERT); 
				seg=seg->sub_next, i++)
			{
				PQ_ADD_VERT(poly, seg->v1->x, seg->v1->y, h);
			}

			RGL_BoundPolyQuad(poly);

			RGL_RenderPolyQuad(poly, &dat2, DLightPlaneCoordFunc, tex_id,tex2_id,
				/* pass */ 2, BL_Multi|BL_Add);

			RGL_FreePolyQuad(poly);
		}
	}
#endif // DLIGHT_PROTOTYPE
}

static INLINE void AddNewDrawFloor(extrafloor_t *ef,
								   float f_h, float c_h, float top_h,
								   surface_t *floor, surface_t *ceil,
								   region_properties_t *props)
{
	drawfloor_t *dfloor;
	drawfloor_t *tail;

	dfloor = drawfloors.GetNew();
	drawfloors.Commit();

	dfloor->f_h   = f_h;
	dfloor->c_h   = c_h;
	dfloor->top_h = top_h;
	dfloor->floor = floor;
	dfloor->ceil  = ceil;
	dfloor->ef    = ef;
	dfloor->props = props;

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

	// add to tail of height order list (for sprite clipping)
	for (tail=cur_sub->z_floors; tail && tail->higher; tail=tail->higher)
	{ /* nothing here */ }

	dfloor->higher = NULL;
	dfloor->lower = tail;

	if (tail)
		tail->higher = dfloor;
	else
		cur_sub->z_floors = dfloor;

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

	sub->ranges = NULL;
	sub->floors = sub->z_floors = NULL;
	sub->raw_things = NULL;

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

		DEV_ASSERT2(C);

		// ignore liquids in the middle of THICK solids, or below real
		// floor or above real ceiling
		//
		if (C->bottom_h < floor_h || C->bottom_h > sector->c_h)
			continue;

		bool de_f = (cur_sub->deep_ref && cur_sub->z_floors == NULL);

		AddNewDrawFloor(C,
			de_f ? cur_sub->deep_ref->f_h : floor_h,
			C->bottom_h, C->top_h,
			de_f ? &cur_sub->deep_ref->floor : floor_s,
			C->bottom, C->p);

		floor_s = C->top;
		floor_h = C->top_h;
	}

	// -AJA- 2004/04/22: emulate the Deep-Water TRICK (above too)
	bool de_f = (cur_sub->deep_ref && cur_sub->z_floors == NULL);
	bool de_c = (cur_sub->deep_ref != NULL);

	AddNewDrawFloor(NULL,
		de_f ? cur_sub->deep_ref->f_h : floor_h,
		de_c ? cur_sub->deep_ref->c_h : sector->c_h,
		de_c ? cur_sub->deep_ref->c_h : sector->c_h,
		de_f ? &cur_sub->deep_ref->floor : floor_s,
		de_c ? &cur_sub->deep_ref->ceil : &sector->ceil, sector->p);

	// handle each sprite in the subsector.  Must be done before walls,
	// since the wall code will update the 1D occlusion buffer.

	for (mo=cur_sub->thinglist; mo; mo=mo->snext)
	{
		RGL_WalkThing(mo, cur_sub);
	}

	// clip 1D occlusion buffer.
	for (seg=sub->segs; seg; seg=seg->sub_next)
	{
		RGL_WalkSeg(seg);
	}

	// add drawsub to list
	// (add to head, thus the eventual order is furthest -> closest)

	sub->rend_next = drawsubs_head;
	sub->rend_prev = NULL;

	if (drawsubs_head)
		drawsubs_head->rend_prev = sub;
	else
		drawsubs_tail = sub;

	drawsubs_head = sub;
}


//
// RGL_DrawSubsector
//
static void RGL_DrawSubsector(subsector_t *sub)
{
	drawfloor_t *dfloor;
	seg_t *seg;
	int vis_num = 0;

#if (DEBUG >= 1)
	L_WriteDebug("\nREVISITING SUBSEC %d\n\n", (int)(sub - subsectors));
#endif

	cur_sub = sub;

	// handle each seg in the subsector
	for (seg=sub->segs; seg; seg=seg->sub_next)
	{
		if (seg->visible)
		{
			vis_num++;

			if (! seg->miniseg)
				RGL_DrawSeg(seg);
		}
	}

	// are any segs visible ?  If not, abort now
	if (vis_num == 0)
		return;

	// handle each floor, drawing planes and things
	for (dfloor=sub->floors; dfloor; dfloor=dfloor->next)
	{
		RGL_DrawPlane(dfloor, dfloor->c_h, dfloor->ceil,  -1);
		RGL_DrawPlane(dfloor, dfloor->f_h, dfloor->floor, +1);

		if (solid_mode)
			continue;

		RGL_DrawSortThings(dfloor);
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
	side = P_PointOnDivlineSide(viewx, viewy, &node->div);

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
#ifdef DLIGHT_PROTOTYPE
	linear_image = W_ImageLookup("DLIGHT_LINEAR");
	quad_image   = W_ImageLookup("DLIGHT_QUAD");
#endif

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
	subsector_t *cur;

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

	drawsubs_head = drawsubs_tail = NULL;

	// handle powerup effects
	RGL_RainbowEffect(players[displayplayer]);

	if (hom_detect)
	{
		glClearColor(0.0f, 1.0f, 0.0f, 0.0f);
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

	// make sure we don't delete any GL textures
	W_LockImagesOGL();

	// draw all solid walls and planes
	solid_mode = true;
	RGL_StartUnits(solid_mode);

	for (cur=drawsubs_head; cur; cur=cur->rend_next)
		RGL_DrawSubsector(cur);

	RGL_FinishUnits();

	// draw all sprites and masked/translucent walls/planes
	solid_mode = false;
	RGL_StartUnits(solid_mode);

	for (cur=drawsubs_head; cur; cur=cur->rend_next)
		RGL_DrawSubsector(cur);

	RGL_FinishUnits();
	glDisable(GL_DEPTH_TEST);

	W_UnlockImagesOGL();

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


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
