//----------------------------------------------------------------------------
//  EDGE OpenGL Rendering (Skies)
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

#include "i_defs.h"
#include "i_defs_gl.h"

#include <math.h>

#include "epi/image_data.h"

#include "dm_state.h"
#include "m_math.h"
#include "r_misc.h"
#include "w_flat.h"
#include "r_sky.h"
#include "r_gldefs.h"
#include "r_sky.h"
#include "r_units.h"
#include "r_colormap.h"
#include "r_modes.h"
#include "r_image.h"
#include "r_texgl.h"
#include "w_wad.h"
#include "z_zone.h"

#define DEBUG  0


const image_c *sky_image;

bool custom_sky_box;

// needed for SKY
extern epi::image_data_c *ReadAsEpiBlock(image_c *rim);


typedef struct sec_sky_ring_s
{
	// which group of connected skies (0 if none)
	int group;

	// link of sector in RING
	struct sec_sky_ring_s *next;
	struct sec_sky_ring_s *prev;

	// maximal sky height of group
	float max_h;
}
sec_sky_ring_t;

//
// R_ComputeSkyHeights
// 
// This routine computes the sky height field in sector_t, which is
// the maximal sky height over all sky sectors (ceiling only) which
// are joined by 2S linedefs.
//
// Algorithm: Initially all sky sectors are in individual groups.  Now
// we scan the linedef list.  For each 2-sectored line with sky on
// both sides, merge the two groups into one.  Simple :).  We can
// compute the maximal height of the group as we go.
// 
void R_ComputeSkyHeights(void)
{
	int i;
	line_t *ld;
	sector_t *sec;

	// --- initialise ---

	sec_sky_ring_t *rings = new sec_sky_ring_t[numsectors];

	memset(rings, 0, numsectors * sizeof(sec_sky_ring_t));

	for (i=0, sec=sectors; i < numsectors; i++, sec++)
	{
		if (! IS_SKY(sec->ceil))
			continue;

		rings[i].group = (i + 1);
		rings[i].next = rings[i].prev = rings + i;
		rings[i].max_h = sec->c_h;

		// leave some room for tall sprites 
		static const float SPR_H_MAX = 256.0f;

		if (sec->c_h < 30000.0f && (sec->c_h > sec->f_h) &&
			(sec->c_h < sec->f_h + SPR_H_MAX))
		{
			rings[i].max_h = sec->f_h + SPR_H_MAX;
		}
	}

	// --- make the pass over linedefs ---

	for (i=0, ld=lines; i < numlines; i++, ld++)
	{
		sector_t *sec1, *sec2;
		sec_sky_ring_t *ring1, *ring2, *tmp_R;

		if (! ld->side[0] || ! ld->side[1])
			continue;

		sec1 = ld->frontsector;
		sec2 = ld->backsector;

		SYS_ASSERT(sec1 && sec2);

		if (sec1 == sec2)
			continue;

		ring1 = rings + (sec1 - sectors);
		ring2 = rings + (sec2 - sectors);

		// we require sky on both sides
		if (ring1->group == 0 || ring2->group == 0)
			continue;

		// already in the same group ?
		if (ring1->group == ring2->group)
			continue;

		// swap sectors to ensure the lower group is added to the higher
		// group, since we don't need to update the `max_h' fields of the
		// highest group.

		if (ring1->max_h < ring2->max_h)
		{
			tmp_R = ring1;  ring1 = ring2;  ring2 = tmp_R;
		}

		// update the group numbers in the second group

		ring2->group = ring1->group;
		ring2->max_h = ring1->max_h;

		for (tmp_R=ring2->next; tmp_R != ring2; tmp_R=tmp_R->next)
		{
			tmp_R->group = ring1->group;
			tmp_R->max_h = ring1->max_h;
		}

		// merge 'em baby...

		ring1->next->prev = ring2;
		ring2->next->prev = ring1;

		tmp_R = ring1->next; 
		ring1->next = ring2->next;
		ring2->next = tmp_R;
	}

	// --- now store the results, and free up ---

	for (i=0, sec=sectors; i < numsectors; i++, sec++)
	{
		if (rings[i].group > 0)
			sec->sky_h = rings[i].max_h;

#if 0   // DEBUG CODE
		L_WriteDebug("SKY: sec %d  group %d  max_h %1.1f\n", i,
				rings[i].group, rings[i].max_h);
#endif
	}

	delete[] rings;
}

//----------------------------------------------------------------------------


extern angle_t FIELDOFVIEW;


static bool need_to_draw_sky = false;


typedef struct
{
	const image_c *base_sky;

	int face_size;

	GLuint tex[6];

	// face images are only present for custom skyboxes.
	// pseudo skyboxes are generated outside of the image system.
	const image_c *face[6];

}
skybox_info_t;

static skybox_info_t box_info =
{
	NULL, 1,
	{ 0,0,0,0,0,0 },
	{ NULL, NULL, NULL, NULL, NULL, NULL }
};


static void RGL_SetupSkyMatrices(float dist)
{
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();

	glLoadIdentity();
	glFrustum(rightslope * var_nearclip, leftslope * var_nearclip,
			  bottomslope * var_nearclip, topslope * var_nearclip,
			  var_nearclip, var_farclip);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();

	glLoadIdentity();
	
	glRotatef(270.0f - ANG_2_FLOAT(viewvertangle), 1.0f, 0.0f, 0.0f);
	glRotatef(90.0f  - ANG_2_FLOAT(viewangle), 0.0f, 0.0f, 1.0f);

	// lower the centre of the skybox (pseudo only!) to match
	// the DOOM sky (which is 128 pixels high on a 200 pixel
	// screen, so it dipped 28 pixels below the horizon).
	if (! custom_sky_box)
		glTranslatef(0.0f, 0.0f, -dist / 4.0);
}

static void RGL_SetupSkyMatrices2D(void)
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

	if (level_flags.mlook || custom_sky_box)
	{
		if (! dumb_sky)
			glDepthFunc(GL_GREATER);

		RGL_DrawSkyBox();
	}
	else
	{
		RGL_DrawSkyOriginal();
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
	float dist = var_farclip / 2.0f;

	RGL_UpdateSkyBoxTextures();
	RGL_SetupSkyMatrices(dist);

	float v0 = 0.0f;
	float v1 = 1.0f;

	if (dumb_clamp)
	{
		float size = box_info.face_size;

		v0 = 0.5f / size;
		v1 = 1.0f - v0;
	}

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
	glBindTexture(GL_TEXTURE_2D, box_info.tex[WSKY_Top]);
        glNormal3i(0, 0, -1);

	glBegin(GL_QUADS);
	glTexCoord2f(v0, v0); glVertex3f(-dist,  dist, +dist);
	glTexCoord2f(v0, v1); glVertex3f(-dist, -dist, +dist);
	glTexCoord2f(v1, v1); glVertex3f( dist, -dist, +dist);
	glTexCoord2f(v1, v0); glVertex3f( dist,  dist, +dist);
	glEnd();

	// bottom
	glBindTexture(GL_TEXTURE_2D, box_info.tex[WSKY_Bottom]);
        glNormal3i(0, 0, +1);

	glBegin(GL_QUADS);
	glTexCoord2f(v0, v0); glVertex3f(-dist, -dist, -dist);
	glTexCoord2f(v0, v1); glVertex3f(-dist,  dist, -dist);
	glTexCoord2f(v1, v1); glVertex3f( dist,  dist, -dist);
	glTexCoord2f(v1, v0); glVertex3f( dist, -dist, -dist);
	glEnd();

	// north
	glBindTexture(GL_TEXTURE_2D, box_info.tex[WSKY_North]);
        glNormal3i(0, -1, 0);

	glBegin(GL_QUADS);
	glTexCoord2f(v0, v0); glVertex3f(-dist,  dist, -dist);
	glTexCoord2f(v0, v1); glVertex3f(-dist,  dist, +dist);
	glTexCoord2f(v1, v1); glVertex3f( dist,  dist, +dist);
	glTexCoord2f(v1, v0); glVertex3f( dist,  dist, -dist);
	glEnd();

	// east
	glBindTexture(GL_TEXTURE_2D, box_info.tex[WSKY_East]);
        glNormal3i(-1, 0, 0);

	glBegin(GL_QUADS);
	glTexCoord2f(v0, v0); glVertex3f( dist,  dist, -dist);
	glTexCoord2f(v0, v1); glVertex3f( dist,  dist, +dist);
	glTexCoord2f(v1, v1); glVertex3f( dist, -dist, +dist);
	glTexCoord2f(v1, v0); glVertex3f( dist, -dist, -dist);
	glEnd();

	// south
	glBindTexture(GL_TEXTURE_2D, box_info.tex[WSKY_South]);
        glNormal3i(0, +1, 0);

	glBegin(GL_QUADS);
	glTexCoord2f(v0, v0); glVertex3f( dist, -dist, -dist);
	glTexCoord2f(v0, v1); glVertex3f( dist, -dist, +dist);
	glTexCoord2f(v1, v1); glVertex3f(-dist, -dist, +dist);
	glTexCoord2f(v1, v0); glVertex3f(-dist, -dist, -dist);
	glEnd();

	// west
	glBindTexture(GL_TEXTURE_2D, box_info.tex[WSKY_West]);
        glNormal3i(+1, 0, 0);

	glBegin(GL_QUADS);
	glTexCoord2f(v0, v0); glVertex3f(-dist, -dist, -dist);
	glTexCoord2f(v0, v1); glVertex3f(-dist, -dist, +dist);
	glTexCoord2f(v1, v1); glVertex3f(-dist,  dist, +dist);
	glTexCoord2f(v1, v0); glVertex3f(-dist,  dist, -dist);
	glEnd();

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

	GLuint tex_id = W_ImageCache(sky_image);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, tex_id);
 
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


#define PIXEL_RED(pix)  (what_palette[pix*3 + 0])
#define PIXEL_GRN(pix)  (what_palette[pix*3 + 1])
#define PIXEL_BLU(pix)  (what_palette[pix*3 + 2])

///---// -AJA- Another hack, this variable holds the current sky when
///---// compute sky merging.  We hold onto the image, because there are
///---// six sides to compute, and we don't want to load the original
///---// image six times.  Removing this hack requires caching it in the
///---// cache system (which is not possible right now).
///---static epi::image_data_c *merging_sky_image;
///---
///---void W_ImageClearMergingSky(void)
///---{
///---	if (merging_sky_image)
///---		delete merging_sky_image;
///---
///---	merging_sky_image = NULL;
///---}


static bool SkyIsNarrow(const image_c *sky)
{
	// check the aspect of the image
	return (IM_WIDTH(sky) / IM_HEIGHT(sky)) < 2.28f;
}


static void CalcSkyCoord(int px, int py, int pw, int ph, int face,
		bool narrow, float *tx, float *ty)
{
	// the 0.5 here ensures we never hit exactly zero
	float ax = ((float)px + 0.5f) / (float)pw * 2.0f - 1.0f;
	float ay = ((float)py + 0.5f) / (float)ph * 2.0f - 1.0f;

	float sx, sy, sz;

	switch (face)
	{
		case WSKY_North:
			sx = ax; sy = 1.0f; sz = ay; break;

		case WSKY_South:
			sx = -ax; sy = -1.0f; sz = ay; break;

		case WSKY_East:
			sx = 1.0f; sy = -ax; sz = ay; break;

		case WSKY_West:
			sx = -1.0f; sy = ax; sz = ay; break;

		case WSKY_Top:
			sx = ax; sy = -ay; sz = 1.0f; break;

		case WSKY_Bottom:
			sx = ax; sy = ay; sz = -1.0f; break;

		default:
			I_Error("CalcSkyCoord: INTERNAL ERROR (lost face)\n");
			sx = sy = sz = 0;
			break; /* NOT REACHED */
	}

	float len2 = sqrt((sx) * (sx) + (sy) * (sy));

	angle_t H = ANG0  + R_PointToAngle(0, 0, sx, sy);
	angle_t V = ANG90 + R_PointToAngle(0, 0, len2, sz);

	if (narrow)
		*tx = (float)(H >> 1) / (float)(1 << 30);
	else
		*tx = (float)(H >> 2) / (float)(1 << 30);

	// want yy to range from 0.0 (bottom) to 2.0 (top)
	float yy = (float)(V) / (float)(1 << 30);

	// mirror it (vertically)
	if (yy > 1.0f) yy = 2.0f - yy;

	*ty = 1.0f - (yy * yy);
}


static GLuint BuildFace(const epi::image_data_c *sky, int face,
				  	    const byte *what_palette)
			
{
	int img_size = box_info.face_size;

	epi::image_data_c img(img_size, img_size, 3);


	bool narrow = SkyIsNarrow(box_info.base_sky);

	const byte *src = sky->pixels;

	int sky_w = sky->width;
	int sky_h = sky->height;

	for (int y=0; y < img_size; y++)
	{
		u8_t *dest = img.PixelAt(0, y);

		for (int x=0; x < img_size; x++, dest += 3)
		{
			float tx, ty;

			CalcSkyCoord(x, y, img_size, img_size, face, narrow, &tx, &ty);

			// Bilinear Filtering

			int TX = (int)(tx * sky_w * 16);
			int TY = (int)(ty * sky_h * 16);

			// negative values shouldn't occur, but just in case...
			TX = (TX + sky_w * 64) % (sky_w * 16);
			TY = (TY + sky_h * 64) % (sky_h * 16);

			SYS_ASSERT(TX >= 0 && TY >= 0);

			///--- TX = sky->width*16 - 1 - TX;

			int FX = TX % 16; TX >>= 4;
			int FY = TY % 16; TY >>= 4;

			SYS_ASSERT(TX < sky_w && TY < sky_h);


			int TX2 = (TX + 1) % sky_w;
			int TY2 = (TY < sky_h-1) ? (TY+1) : TY;

			byte rA, rB, rC, rD;
			byte gA, gB, gC, gD;
			byte bA, bB, bC, bD;

			switch (sky->bpp)
			{
				case 1:
				{
					byte src_A = src[TY  * sky_w + TX];
					byte src_B = src[TY  * sky_w + TX2];
					byte src_C = src[TY2 * sky_w + TX];
					byte src_D = src[TY2 * sky_w + TX2];

					rA = PIXEL_RED(src_A); rB = PIXEL_RED(src_B);
					rC = PIXEL_RED(src_C); rD = PIXEL_RED(src_D);

					gA = PIXEL_GRN(src_A); gB = PIXEL_GRN(src_B);
					gC = PIXEL_GRN(src_C); gD = PIXEL_GRN(src_D);

					bA = PIXEL_BLU(src_A); bB = PIXEL_BLU(src_B);
					bC = PIXEL_BLU(src_C); bD = PIXEL_BLU(src_D);
				}
				break;

				case 3:
				{
					rA = src[(TY * sky_w + TX) * 3 + 0];
					gA = src[(TY * sky_w + TX) * 3 + 1];
					bA = src[(TY * sky_w + TX) * 3 + 2];

					rB = src[(TY * sky_w + TX2) * 3 + 0];
					gB = src[(TY * sky_w + TX2) * 3 + 1];
					bB = src[(TY * sky_w + TX2) * 3 + 2];

					rC = src[(TY2 * sky_w + TX) * 3 + 0];
					gC = src[(TY2 * sky_w + TX) * 3 + 1];
					bC = src[(TY2 * sky_w + TX) * 3 + 2];

					rD = src[(TY2 * sky_w + TX2) * 3 + 0];
					gD = src[(TY2 * sky_w + TX2) * 3 + 1];
					bD = src[(TY2 * sky_w + TX2) * 3 + 2];
				}
				break;

				case 4:
				{
					rA = src[(TY * sky_w + TX) * 4 + 0];
					gA = src[(TY * sky_w + TX) * 4 + 1];
					bA = src[(TY * sky_w + TX) * 4 + 2];

					rB = src[(TY * sky_w + TX2) * 4 + 0];
					gB = src[(TY * sky_w + TX2) * 4 + 1];
					bB = src[(TY * sky_w + TX2) * 4 + 2];

					rC = src[(TY2 * sky_w + TX) * 4 + 0];
					gC = src[(TY2 * sky_w + TX) * 4 + 1];
					bC = src[(TY2 * sky_w + TX) * 4 + 2];

					rD = src[(TY2 * sky_w + TX2) * 4 + 0];
					gD = src[(TY2 * sky_w + TX2) * 4 + 1];
					bD = src[(TY2 * sky_w + TX2) * 4 + 2];
				}
				break;

				default:  // remove compiler warning
					rA = rB = rC = rD = 0;
					gA = gB = gC = gD = 0;
					bA = bB = bC = bD = 0;
					break;
			}

			int r = (int)rA * (FX^15) * (FY^15) +
					(int)rB * (FX   ) * (FY^15) +
					(int)rC * (FX^15) * (FY   ) +
					(int)rD * (FX   ) * (FY   );

			int g = (int)gA * (FX^15) * (FY^15) +
					(int)gB * (FX   ) * (FY^15) +
					(int)gC * (FX^15) * (FY   ) +
					(int)gD * (FX   ) * (FY   );

			int b = (int)bA * (FX^15) * (FY^15) +
					(int)bB * (FX   ) * (FY^15) +
					(int)bC * (FX^15) * (FY   ) +
					(int)bD * (FX   ) * (FY   );

			dest[0] = r / 225;
			dest[1] = g / 225;
			dest[2] = b / 225;
		}
	}

	return R_UploadTexture(&img, NULL, UPL_Smooth|UPL_Clamp);
}


static const char *UserSkyFaceName(const char *base, int face)
{
	static char buffer[64];
	static const char letters[] = "NESWTB";

	sprintf(buffer, "%s_%c", base, letters[face]);
	return buffer;
}


void RGL_UpdateSkyBoxTextures(void)
{
	if (box_info.base_sky == sky_image)
	{
		return;
	}

///---	W_ImageClearMergingSky(); // hack (see r_image.cc)

	box_info.base_sky = sky_image;


	// check for custom sky images
	box_info.face[WSKY_North] = W_ImageLookup(
			UserSkyFaceName(sky_image->name, WSKY_North), INS_Texture,
			ILF_Null);

	if (box_info.face[WSKY_North])
	{
		custom_sky_box = true;

		box_info.face_size = box_info.face[WSKY_North]->total_w;

		for (int i = WSKY_East; i < 6; i++)
			box_info.face[i] = W_ImageLookup(
					UserSkyFaceName(sky_image->name, i), INS_Texture);

		for (int k = 0; k < 6; k++)
			box_info.tex[k] = W_ImageCache(box_info.face[k], false);

		return;
	}


	// Create pseudo sky box

//???	// use low-res box sides for low-res sky images
//???	int size = (sky->actual_h < 228) ? 128 : 256;

	box_info.face_size = 256;

	custom_sky_box = false;

	
	// Intentional Const Override
	const epi::image_data_c *block = ReadAsEpiBlock((image_c*)sky_image);
	SYS_ASSERT(block);

	// get correct palette
	const byte *what_pal = (const byte *) &playpal_data[0];
	bool what_pal_cached = false;

	if (sky_image->source_palette >= 0)
	{
		what_pal = (const byte *) W_CacheLumpNum(sky_image->source_palette);
		what_pal_cached = true;
	}

	box_info.tex[WSKY_North]  = BuildFace(block, WSKY_North,  what_pal);
	box_info.tex[WSKY_East]   = BuildFace(block, WSKY_East,   what_pal);
	box_info.tex[WSKY_Top]    = BuildFace(block, WSKY_Top,    what_pal);
	box_info.tex[WSKY_Bottom] = BuildFace(block, WSKY_Bottom, what_pal);

	// optimisation: can share side textures when narrow

	box_info.tex[WSKY_South] = SkyIsNarrow(sky_image) ? box_info.tex[WSKY_North] :
						 BuildFace(block, WSKY_South, what_pal );

	box_info.tex[WSKY_West]  = SkyIsNarrow(sky_image) ? box_info.tex[WSKY_East] :
						 BuildFace(block, WSKY_West, what_pal );

	delete block;

	if (what_pal_cached)
		W_DoneWithLump(what_pal);
}


void RGL_PreCacheSky(void)
{
#if 0   // ????
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
#endif
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
