//----------------------------------------------------------------------------
//  EDGE OpenGL Rendering (Skies)
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

// this conditional applies to the whole file
#include "i_defs.h"

#include "am_map.h"
#include "dm_defs.h"
#include "dm_state.h"
#include "con_main.h"
#include "e_search.h"
#include "m_bbox.h"
#include "m_random.h"
#include "p_local.h"
#include "p_mobj.h"
#include "r_defs.h"
#include "r_main.h"
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


extern angle_t FIELDOFVIEW;


static bool need_to_draw_sky = false;

int sky_stretch = 0;  // ranges from 0 to 4


typedef struct
{
	int last_stretch;

	const image_t *base_sky;
	const image_t *north, *east, *south, *west;
	const image_t *top, *bottom;
}
skybox_info_t;

static skybox_info_t box_info =
{
	-1,
	NULL,
	NULL, NULL, NULL, NULL,
	NULL, NULL
};


//
// RGL_SetupSkyMatrices
//
void RGL_SetupSkyMatrices(void)
{
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();

	glLoadIdentity();
	glFrustum(rightslope, leftslope, bottomslope, topslope, Z_NEAR, Z_FAR);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();

	glLoadIdentity();
	
	glRotatef(270.0f - ANG_2_FLOAT(viewvertangle), 1.0f, 0.0f, 0.0f);
	glRotatef(90.0f  - ANG_2_FLOAT(viewangle), 0.0f, 0.0f, 1.0f);
//	glTranslatef(0.0f, 0.0f, 0.0f);
}

void RGL_SetupSkyMatrices2D(void)
{
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();

	glLoadIdentity();
	glOrtho(0.0f, (float)SCREENWIDTH, 
			0.0f, (float)SCREENHEIGHT, -1.0f, 1.0f);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();

	glLoadIdentity();
}

//
// RGL_RevertSkyMatrices
//
void RGL_RevertSkyMatrices(void)
{
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}

//
// RGL_BeginSky
//
void RGL_BeginSky(void)
{
	need_to_draw_sky = false;

	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	glDisable(GL_TEXTURE_2D);

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glEdgeFlag(GL_TRUE);
}

//
// RGL_FinishSky
//
void RGL_FinishSky(void)
{
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

	if (! need_to_draw_sky)
		return;

	// draw sky picture, but DON'T affect the depth buffering

	glEnable(GL_TEXTURE_2D);

	glDepthMask(GL_FALSE);

	if (sky_stretch == STRETCH_ORIGINAL)
	{
		RGL_DrawSkyOriginal();
	}
	else
	{
		if (! dumb_sky)
			glDepthFunc(GL_GREATER);

		RGL_DrawSkyBox();
	}

	glDepthFunc(GL_LEQUAL);
	glDepthMask(GL_TRUE);

	glDisable(GL_TEXTURE_2D);

#if 0
	// clear buffer (EXPERIMENTAL) -- causes render problems: ceilings
	// you shouldn't be able to see (MAP05, MAP12).
	glClear(GL_DEPTH_BUFFER_BIT);
#endif
}

void RGL_DrawSkyBox(void)
{
	RGL_UpdateSkyBoxTextures();
	RGL_SetupSkyMatrices();

	float dist = Z_FAR / 2.0f;

	float v0 = 0.0f;
	float v1 = 1.0f;

	if (! glcap_edgeclamp)
	{
		float size = IM_WIDTH(box_info.north);

		v0 = 0.5f / size;
		v1 = 1.0f - v0;
	}

	const cached_image_t *cim_N, *cim_E, *cim_S, *cim_W;
	const cached_image_t *cim_T, *cim_B;

	cim_N = W_ImageCache(box_info.north);
	cim_E = W_ImageCache(box_info.east);
	cim_S = W_ImageCache(box_info.south);
	cim_W = W_ImageCache(box_info.west);
	cim_T = W_ImageCache(box_info.top);
	cim_B = W_ImageCache(box_info.bottom);

	glEnable(GL_TEXTURE_2D);

	float col[4];

	col[0] = LT_RED(255);
	col[1] = LT_GRN(255);
	col[2] = LT_BLU(255);
	col[3] = 1.0f;

	if (use_color_material || ! use_lighting)
		glColor4fv(col);
	else
	{
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, col);
		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, col);
	}

	// top
	glBindTexture(GL_TEXTURE_2D, W_ImageGetOGL(cim_T));

	glBegin(GL_QUADS);
	glTexCoord2f(v0, v0); glVertex3f(-dist, -dist, +dist);
	glTexCoord2f(v0, v1); glVertex3f(-dist,  dist, +dist);
	glTexCoord2f(v1, v1); glVertex3f( dist,  dist, +dist);
	glTexCoord2f(v1, v0); glVertex3f( dist, -dist, +dist);
	glEnd();

	// bottom
	glBindTexture(GL_TEXTURE_2D, W_ImageGetOGL(cim_B));

	glBegin(GL_QUADS);
	glTexCoord2f(v0, v0); glVertex3f(-dist, -dist, -dist);
	glTexCoord2f(v0, v1); glVertex3f(-dist,  dist, -dist);
	glTexCoord2f(v1, v1); glVertex3f( dist,  dist, -dist);
	glTexCoord2f(v1, v0); glVertex3f( dist, -dist, -dist);
	glEnd();

	// north
	glBindTexture(GL_TEXTURE_2D, W_ImageGetOGL(cim_N));

	glBegin(GL_QUADS);
	glTexCoord2f(v0, v0); glVertex3f(-dist,  dist, -dist);
	glTexCoord2f(v0, v1); glVertex3f(-dist,  dist, +dist);
	glTexCoord2f(v1, v1); glVertex3f( dist,  dist, +dist);
	glTexCoord2f(v1, v0); glVertex3f( dist,  dist, -dist);
	glEnd();

	// east
	glBindTexture(GL_TEXTURE_2D, W_ImageGetOGL(cim_E));

	glBegin(GL_QUADS);
	glTexCoord2f(v0, v0); glVertex3f( dist,  dist, -dist);
	glTexCoord2f(v0, v1); glVertex3f( dist,  dist, +dist);
	glTexCoord2f(v1, v1); glVertex3f( dist, -dist, +dist);
	glTexCoord2f(v1, v0); glVertex3f( dist, -dist, -dist);
	glEnd();

	// south
	glBindTexture(GL_TEXTURE_2D, W_ImageGetOGL(cim_S));

	glBegin(GL_QUADS);
	glTexCoord2f(v0, v0); glVertex3f( dist, -dist, -dist);
	glTexCoord2f(v0, v1); glVertex3f( dist, -dist, +dist);
	glTexCoord2f(v1, v1); glVertex3f(-dist, -dist, +dist);
	glTexCoord2f(v1, v0); glVertex3f(-dist, -dist, -dist);
	glEnd();

	// west
	glBindTexture(GL_TEXTURE_2D, W_ImageGetOGL(cim_W));

	glBegin(GL_QUADS);
	glTexCoord2f(v0, v0); glVertex3f(-dist, -dist, -dist);
	glTexCoord2f(v0, v1); glVertex3f(-dist, -dist, +dist);
	glTexCoord2f(v1, v1); glVertex3f(-dist,  dist, +dist);
	glTexCoord2f(v1, v0); glVertex3f(-dist,  dist, -dist);
	glEnd();

	W_ImageDone(cim_N); W_ImageDone(cim_E);
	W_ImageDone(cim_S); W_ImageDone(cim_W);
	W_ImageDone(cim_T); W_ImageDone(cim_B);

	glDisable(GL_TEXTURE_2D);

	RGL_RevertSkyMatrices();
}

void RGL_DrawSkyOriginal(void)
{
	RGL_SetupSkyMatrices2D();

	float white[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

	if (use_color_material || ! use_lighting)
		glColor4fv(white);
	else
	{
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, white);
		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, white);
	}

	const cached_image_t *cim = W_ImageCache(sky_image);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, W_ImageGetOGL(cim));
 
	// divide screen into 32 vertical strips, since mapping is non-linear
	glBegin(GL_QUAD_STRIP);
 
	float focal_len = M_Tan((FIELDOFVIEW) / 2);
	float centerxfrac = SCREENWIDTH / 2.0f;

	float ty1 = 200.0f / 128.0f;
	float ty2 = 0;

	for (int i = 0; i <= 32; i++)
	{
		int sx = i * SCREENWIDTH / 32;

		// use formula from original Doom code
		angle_t ang = ANG180 + M_ATan((1.0f - sx / centerxfrac) * focal_len);

		// some mucking about here to prevent wrap-around
		float tx = (((viewangle >> 2) + (ang >> 2) + (ANG180 >> 2)) >> 20);

		if ((IM_WIDTH(sky_image) / IM_HEIGHT(sky_image)) < 2.28f)
			tx = tx / 256.0f;
		else
			tx = tx / 1024.0f;

#if 0  // DEBUGGING
I_Printf("[%i] --> %1.2f  tx %1.4f\n", i, ANG_2_FLOAT(ang), tx);
#endif
		glTexCoord2f(tx, 1.0f - ty1);
		glVertex2i(sx, 0);

		glTexCoord2f(tx, 1.0f - ty2); 
		glVertex2i(sx, SCREENHEIGHT);
 	}

	glEnd();

	glDisable(GL_TEXTURE_2D);

	W_ImageDone(cim);
	RGL_RevertSkyMatrices();
}

//
// RGL_DrawSkyPlane
//
void RGL_DrawSkyPlane(subsector_t *sub, float h)
{
	need_to_draw_sky = true;

	if (dumb_sky)
		return;

	glNormal3f(0, 0, (viewz > h) ? 1.0f : -1.0f);

	glBegin(GL_POLYGON);

	for (seg_t *seg=sub->segs; seg; seg=seg->sub_next)
	{
		glVertex3f(seg->v1->x, seg->v1->y, h);
	}

	glEnd();
}

//
// RGL_DrawSkyWall
//
void RGL_DrawSkyWall(seg_t *seg, float h1, float h2)
{
	need_to_draw_sky = true;

	if (dumb_sky)
		return;

	float x1 = seg->v1->x;
	float y1 = seg->v1->y;
	float x2 = seg->v2->x;
	float y2 = seg->v2->y;

	glNormal3f(y2 - y1, x1 - x2, 0);

	glBegin(GL_QUADS);

	glVertex3f(x1, y1, h1);
	glVertex3f(x1, y1, h2);
	glVertex3f(x2, y2, h2);
	glVertex3f(x2, y2, h1);

	glEnd();
}

//----------------------------------------------------------------------------

extern void W_ImageClearMergingSky(void);

static const float stretches[5] =
{ 0.55f, 0.78f, 1.0f, 1.0f /* MIRROR */, 1.0f /* ORIGINAL */};

void RGL_CalcSkyCoord(float sx, float sy, float sz, bool narrow, float *tx, float *ty)
{
	angle_t H = R_PointToAngle(0, 0, sx, sy);
	angle_t V = R_PointToAngle(0, 0, sz, R_PointToDist(0, 0, sx, sy));

	H = 0 - H;
	V = V;

	if (narrow)
		*tx = (float)(H >> 7) / (float)(1 << 24);
	else
		*tx = (float)(H >> 8) / (float)(1 << 24);

	float k = (float)(V >> 7) / (float)(1 << 24);

	if (sky_stretch >= STRETCH_MIRROR)
	{
		k *= 2.0f;
		*ty = (k > 1.0f) ? 2.0f - k : k;
	}
	else
	{
		// FIXME: optimise
		k = k * 2.0f - 1.0f;

		if (k < 0)
			k = -pow(-k, stretches[sky_stretch]);
		else
			k = pow(k, stretches[sky_stretch]);

		// if (k < -0.99) k = -0.99;
		// if (k > +0.99) k = +0,99;

		*ty = (k + 1.0f) / 2.0f;
	}
}

void RGL_UpdateSkyBoxTextures(void)
{
	if (box_info.base_sky == sky_image &&
		box_info.last_stretch == sky_stretch)
	{
		return;
	}

	W_ImageClearMergingSky(); // hack (see w_image.cpp)

	box_info.base_sky = sky_image;
	box_info.last_stretch = sky_stretch;

	if (sky_stretch != STRETCH_ORIGINAL)
	{
		box_info.north  = W_ImageFromSkyMerge(sky_image, WSKY_North);
		box_info.east   = W_ImageFromSkyMerge(sky_image, WSKY_East);
		box_info.top    = W_ImageFromSkyMerge(sky_image, WSKY_Top);
		box_info.bottom = W_ImageFromSkyMerge(sky_image, WSKY_Bottom);
		box_info.south  = W_ImageFromSkyMerge(sky_image, WSKY_South);
		box_info.west   = W_ImageFromSkyMerge(sky_image, WSKY_West);
	}
}

void RGL_PreCacheSky(void)
{
	if (sky_stretch == STRETCH_ORIGINAL)
		return;

	W_ImagePreCache(box_info.north);
	W_ImagePreCache(box_info.east);
	W_ImagePreCache(box_info.top);

	if (box_info.south != box_info.north)
		W_ImagePreCache(box_info.south);

	if (box_info.west != box_info.east)
		W_ImagePreCache(box_info.west);

	if (box_info.bottom != box_info.top)
		W_ImagePreCache(box_info.bottom);
}
