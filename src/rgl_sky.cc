//----------------------------------------------------------------------------
//  EDGE OpenGL Rendering (Skies)
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2001  The EDGE Team.
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

// this conditional applies to the whole file
#ifdef USE_GL

#include "i_defs.h"

#include "am_map.h"
#include "dm_defs.h"
#include "dm_state.h"
#include "e_search.h"
#include "m_bbox.h"
#include "m_random.h"
#include "p_local.h"
#include "p_mobj.h"
#include "r_defs.h"
#include "r_main.h"
#include "r_plane.h"
#include "r_sky.h"
#include "r_state.h"
#include "r_things.h"
#include "r2_defs.h"
#include "rgl_defs.h"
#include "v_colour.h"
#include "v_ctx.h"
#include "v_res.h"
#include "w_textur.h"
#include "w_wad.h"
#include "z_zone.h"


#define DEBUG  0


#define RGB_RED(rgbcol)  ((float)((rgbcol >> 16) & 0xFF) / 255.0)
#define RGB_GRN(rgbcol)  ((float)((rgbcol >>  8) & 0xFF) / 255.0)
#define RGB_BLU(rgbcol)  ((float)((rgbcol      ) & 0xFF) / 255.0)


#if 0  // TEMPORARILY DISABLED

static INLINE void DrawSkyTilePoint(const tilesky_info_t *info,
									const image_t *image, int p, int xmul, int ymul, int zmul, 
									float dx, float dy)
{
	float x = tilepoints[p].x * xmul;
	float y = tilepoints[p].y * ymul;
	float z = tilepoints[p].z * zmul;

	float tx = (zmul > 0) ? tilepoints[p].tx : tilepoints[p].bx;
	float ty = (zmul > 0) ? tilepoints[p].ty : tilepoints[p].by;

	if (xmul < 0)
	{
		float tmp = tx; tx = 1 - ty; ty = 1 - tmp;
	}

	if (ymul < 0)
	{
		float tmp = tx; tx = ty; ty = tmp;
	}

	CHECKVAL(info->number);
	CHECKVAL(info->squish);

	glTexCoord2f(tx * info->number + dx, ty * info->number + dy);
	glVertex3f(y * 1000, x * 1000, (z + info->offset) * 1000 / 
		info->squish);
}

static void DrawSkyTilePart(const tilesky_info_t *info,
							const image_t *image, int xmul, int ymul, int zmul, 
							float dx, float dy)
{
	int s;
	int len, top, bot;

	for (s=0; tilestrips[s].len > 0; s++)
	{
		glBegin(GL_TRIANGLE_STRIP);

		len = tilestrips[s].len;
		top = tilestrips[s].top;
		bot = tilestrips[s].bottom;

		for (; len > 0; len--, top++, bot++)
		{
			DrawSkyTilePoint(info, image, top, xmul, ymul, zmul, dx, dy);
			DrawSkyTilePoint(info, image, bot, xmul, ymul, zmul, dx, dy);
		}

		DrawSkyTilePoint(info, image, top, xmul, ymul, zmul, dx, dy);

		glEnd();
	}
}

static void RGL_DrawTiledSky(void)
{
	int i;
	const image_t *image;
	const cached_image_t *cim;
	const tilesky_info_t *info;

	side_t *side;
	float trans;
	float dx, dy;

	RGL_SetupMatricesTiledSky();

	for (i=0; i < MAX_TILESKY; i++)
	{
		if (! sky_tiles[i].active)
			continue;

		info = sky_tiles[i].info;
		side = sky_tiles[i].line->side[0];
		trans = side->middle.translucency;
		dx = side->middle.x_offset * info->number / 1024.0f;
		dy = side->middle.y_offset * info->number / 1024.0f;

		if (info->type == TILESKY_Flat)
			image = sky_tiles[i].line->frontsector->floor.image;
		else
			image = side->middle.image;

		if (!image || trans < 0.01)
			continue;

		glEnable(GL_TEXTURE_2D);

		cim = W_ImageCache(image, IMG_OGL, 0, true);
		glBindTexture(GL_TEXTURE_2D, W_ImageGetOGL(cim));

		if (trans <= 0.99 || !image->solid)
			glEnable(GL_BLEND);

		// sky is always 100% bright
		glColor4f(1.0, 1.0, 1.0, trans);

		DrawSkyTilePart(info, image,  1,  1,  1, dx, dy);
		DrawSkyTilePart(info, image, -1,  1,  1, dx, dy);
		DrawSkyTilePart(info, image, -1, -1,  1, dx, dy);
		DrawSkyTilePart(info, image,  1, -1,  1, dx, dy);

		DrawSkyTilePart(info, image,  1,  1, -1, dx, dy);
		DrawSkyTilePart(info, image, -1,  1, -1, dx, dy);
		DrawSkyTilePart(info, image, -1, -1, -1, dx, dy);
		DrawSkyTilePart(info, image,  1, -1, -1, dx, dy);

		W_ImageDone(cim);

		glDisable(GL_TEXTURE_2D);
		glDisable(GL_BLEND);
	}
}

//
// RGL_DrawSky
//
void RGL_DrawSky(void)
{
	int x, y, w, h;
	float right, bottom;

	const cached_image_t *cim;
	GLuint tex_id;

	float mlook_rad;
	int top_L, bottom_L;
	int base_a = viewangle >> 6;

	int sx1, sy1, sx2, sy2;  // screen coords
	float tx1, tx2, ty, bx1, bx2, by;  // tex coords

	if (! draw_sky)
		return;

	// reset draw-sky flag
	draw_sky = false;

	if (sky_tiles_active > 0)
	{
		RGL_DrawTiledSky();
		return;
	}

	DEV_ASSERT2(sky_image);
	cim = W_ImageCache(sky_image, IMG_OGL, 0, true);
	tex_id = W_ImageGetOGL(cim);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, tex_id);

	w = sky_image->actual_w;
	h = sky_image->actual_h;
	right  = w / (float)sky_image->total_w;
	bottom = h / (float)sky_image->total_h;

	// sky is always 100% bright
	glColor4f(1.0, 1.0, 1.0, 1.0);

	sx1 = viewwindowx;
	sx2 = viewwindowx + viewwindowwidth;

	sy1 = viewwindowy;
	sy2 = viewwindowy + viewwindowheight;

	// compute sky horizontally tex coords
	mlook_rad = atan(viewvertangle);

	if (mlook_rad >= M_PI/4)
		top_L = ANG90;
	else if (mlook_rad <= -M_PI/4)
		top_L = ANG45;
	else
	{
		// d is just the distance horizontally forward from the eye to
		// the top edge of the view rectangle.
		float d = M_ROOT2 * sin(M_PI/2 - (mlook_rad + M_PI/4));

		top_L = ANG90 - M_ATan(d);
	}

	if (mlook_rad <= -M_PI/4)
		bottom_L = ANG90;
	else if (mlook_rad >= M_PI/4)
		bottom_L = ANG45;
	else
	{
		// d is just the distance horizontally forward from the eye to
		// the bottom edge of the view rectangle.
		float d = M_ROOT2 * sin(M_PI/2 - (M_PI/4 - mlook_rad));

		bottom_L = ANG90 - M_ATan(d);
	}

	if (! level_flags.stretchsky)
	{
		top_L = bottom_L = ANG90;
	}

#define NEWSKYSHIFT  ((float)(1 << (ANGLEBITS - 16)))

	top_L >>= 6;
	bottom_L >>= 6;

	base_a *= 2;

	CHECKVAL(w);
	tx1 = (float)(base_a + top_L) / NEWSKYSHIFT / (float)w;
	tx2 = (float)(base_a - top_L) / NEWSKYSHIFT / (float)w;

	bx1 = (float)(base_a + bottom_L) / NEWSKYSHIFT / (float)w;
	bx2 = (float)(base_a - bottom_L) / NEWSKYSHIFT / (float)w;

	if (w <= 512)
	{
		tx1 /= 2.0f; tx2 /= 2.0f;
		bx1 /= 2.0f; bx2 /= 2.0f;
	}

	// compute sky vertical tex coords
	{
		float top_a    = (M_PI/2 - mlook_rad - M_PI/4) / M_PI;
		float bottom_a = (M_PI/2 - mlook_rad + M_PI/4) / M_PI;

		if (top_a < 0)
			top_a = 0;

		if (bottom_a > 1.0)
			bottom_a = 1.0f;

		DEV_ASSERT2(bottom_a > top_a);

		ty = top_a * bottom;
		by = bottom_a * bottom;
	}

	glBegin(GL_QUADS);

	// divide screen into many squares, to reduce distortion
	for (y=0; y < 8; y++)
		for (x=0; x < 8; x++)
		{
			int xa = sx1 + (sx2 - sx1) * x     / 8;
			int xb = sx1 + (sx2 - sx1) * (x+1) / 8;

			int ya = sy1 + (sy2 - sy1) * y     / 8;
			int yb = sy1 + (sy2 - sy1) * (y+1) / 8;

			float la = tx1 + (bx1 - tx1) * y     / 8;
			float ra = tx2 + (bx2 - tx2) * y     / 8;
			float lb = tx1 + (bx1 - tx1) * (y+1) / 8;
			float rb = tx2 + (bx2 - tx2) * (y+1) / 8;

			float txa = la + (ra - la) * x     / 8;
			float bxa = lb + (rb - lb) * x     / 8;
			float txb = la + (ra - la) * (x+1) / 8;
			float bxb = lb + (rb - lb) * (x+1) / 8;

			float tya = ty + (by - ty) * y     / 8;
			float bya = ty + (by - ty) * (y+1) / 8;

			glTexCoord2f(txa, 1.0 - bottom * tya);
			glVertex2i(xa, SCREENHEIGHT - ya);

			glTexCoord2f(txb, 1.0 - bottom * tya);
			glVertex2i(xb, SCREENHEIGHT - ya);

			glTexCoord2f(bxb, 1.0 - bottom * bya);
			glVertex2i(xb, SCREENHEIGHT - yb);

			glTexCoord2f(bxa, 1.0 - bottom * bya);
			glVertex2i(xa, SCREENHEIGHT - yb);
		}

		glEnd();

		glDisable(GL_TEXTURE_2D);

		W_ImageDone(cim);
}

#endif  // TEMPORARILY DISABLED


static INLINE void CalcSkyTexCoord(float x, float y, float z, 
								   float *tx, float *ty)
{
	float dist;
	float tile_num = 1;
	float base_val, angle_val;

	angle_t angle;

	x -= viewx;
	y -= viewy;
	z -= viewz;

	// OPTIMISE
	dist = (float)sqrt(x*x + y*y + z*z);

	if (dist < 1) 
		dist = 1;

	x /= dist;
	y /= dist;
	z /= dist;

	angle = viewangle - R_PointToAngle(0, 0, x, y);

	base_val = ANG_2_FLOAT(viewangle);

	if (angle < ANG180)
		angle_val = ANG_2_FLOAT(angle);
	else
		angle_val = ANG_2_FLOAT(angle) - 360.0f;

	(*tx) = tile_num * (base_val - angle_val) /
		(sky_image->actual_w > 256 ? 360.0f : 180.0f);
	(*ty) = tile_num * ((1 + z) / 2);  /// * IM_BOTTOM(sky_image);
	return;

#if 0  // EXPERIMENTAL
	dist = sqrt(x*x + z*z);

	if (dist > 0.01)
	{
		x /= dist;
		z /= dist;
	}

	x = (1 + x) / 2;
	z = (1 + z) / 2;

	if (0)  // sky_image->actual_w > 256)
	{
		x /= 2;

		if (y < 0)
			x = 1.0 - x;
	}
	else
	{
		if (y < 0)
			x = -x;
	}

	(*tx) = x;
	(*ty) = z * IM_BOTTOM(sky_image);
#endif
}

typedef struct sky_data_s
{
	vec3_t normal;
}
sky_data_t;

void SkyPolyCoordFunc(vec3_t *src, local_gl_vert_t *vert, void *d)
{
	sky_data_t *data = (sky_data_t *)d;

	float tx, ty;

	CalcSkyTexCoord(src->x, src->y, src->z, &tx, &ty);

	SET_COLOR(1.0, 1.0, 1.0, 1.0);
	SET_TEXCOORD(tx, ty);
	SET_NORMAL(data->normal.x, data->normal.y, data->normal.z);
	SET_EDGE_FLAG(GL_TRUE);
	SET_VERTEX(src->x, src->y, src->z);
}

//
// RGL_DrawSkyPlane
//
void RGL_DrawSkyPlane(subsector_t *sub, float h)
{
	raw_polyquad_t *poly;
	sky_data_t data;

	seg_t *seg;

	GLuint tex_id;
	const cached_image_t *cim;

	int num_vert, i;

	// count number of actual vertices
	for (seg=sub->segs, num_vert=0; seg; seg=seg->sub_next, num_vert++)
	{ /* nothing here */ }

	if (num_vert > MAX_PLVERT)
		num_vert = MAX_PLVERT;

	DEV_ASSERT2(sky_image);
	cim = W_ImageCache(sky_image, IMG_OGL, 0, true);
	tex_id = W_ImageGetOGL(cim);

	// normally this is wrong -- W_LockImagesOGL saves us though.
	W_ImageDone(cim);

	data.normal.x = 0;
	data.normal.y = 0;
	data.normal.z = (viewz > h) ? 1.0f : -1.0f;

	// create PolyQuad and transfer vertices

	poly = RGL_NewPolyQuad(num_vert, false);

	for (seg=sub->segs, i=0; seg && (i < MAX_PLVERT); 
		seg=seg->sub_next, i++)
	{
		PQ_ADD_VERT(poly, seg->v1->x, seg->v1->y, h);
	}

	RGL_BoundPolyQuad(poly);
	RGL_SplitPolyQuadLOD(poly, 1, 128 >> detail_level);

	RGL_RenderPolyQuad(poly, &data, &SkyPolyCoordFunc, tex_id, 
		false, false);

	RGL_FreePolyQuad(poly);
}

//
// RGL_DrawSkyWall
//
void RGL_DrawSkyWall(seg_t *seg, float h1, float h2)
{
	float x1 = seg->v1->x;
	float y1 = seg->v1->y;
	float x2 = seg->v2->x;
	float y2 = seg->v2->y;

	GLuint tex_id;
	const cached_image_t *cim;

	sky_data_t data;
	raw_polyquad_t *poly;

	poly = RGL_NewPolyQuad(4, true);

	PQ_ADD_VERT(poly, x1, y1, h1);
	PQ_ADD_VERT(poly, x1, y1, h2);
	PQ_ADD_VERT(poly, x2, y2, h1);
	PQ_ADD_VERT(poly, x2, y2, h2);

	// get texture id for sky
	DEV_ASSERT2(sky_image);
	cim = W_ImageCache(sky_image, IMG_OGL, 0, true);
	tex_id = W_ImageGetOGL(cim);

	// normally this is wrong -- W_LockImagesOGL saves us though.
	W_ImageDone(cim);

	data.normal.x = y2 - y1;
	data.normal.y = x1 - x2;
	data.normal.z = 0;

	RGL_BoundPolyQuad(poly);
	RGL_SplitPolyQuadLOD(poly, 1, 128 >> detail_level);

	RGL_RenderPolyQuad(poly, &data, &SkyPolyCoordFunc, tex_id, 
		false, false);

	RGL_FreePolyQuad(poly);
}


#endif  // USE_GL
