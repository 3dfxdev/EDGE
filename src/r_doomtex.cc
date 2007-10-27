//----------------------------------------------------------------------------
//  EDGE Generalised Image Handling
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
// -AJA- 2000/06/25: Began this image generalisation, based on Erik
//       Sandberg's w_textur.c/h code.
//
// TODO HERE:
//   -  faster search methods.
//   -  do some optimisation
//

#include "i_defs.h"
#include "i_defs_gl.h"

#include <limits.h>

#include "epi/endianess.h"
#include "epi/file.h"
#include "epi/filesystem.h"

#include "epi/image_data.h"
#include "epi/image_hq2x.h"
#include "epi/image_png.h"
#include "epi/image_jpeg.h"
#include "epi/image_tga.h"

#include "dm_state.h"
#include "e_search.h"
#include "e_main.h"
#include "m_argv.h"
#include "m_misc.h"
#include "p_local.h"
#include "r_defs.h"
#include "r_image.h"
#include "r_gldefs.h"
#include "r_sky.h"
#include "r_colors.h"
#include "w_texture.h"
#include "w_wad.h"
#include "z_zone.h"


// posts are runs of non masked source pixels
typedef struct
{
	// -1 is the last post in a column
	byte topdelta;

    // length data bytes follows
	byte length;  // length data bytes follows
}
post_t;

// column_t is a list of 0 or more post_t, (byte)-1 terminated
typedef post_t column_t;


#define TRANS_REPLACE  pal_black


// Dummy image, for when texture/flat/graphic is unknown.  Row major
// order.  Could be packed, but why bother ?
#define DUMMY_X  16
#define DUMMY_Y  16
static byte dummy_graphic[DUMMY_X * DUMMY_Y] =
{
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,1,1,1,1,1,1,1,1,0,0,0,0,
	0,0,0,1,1,1,1,1,1,1,1,1,1,0,0,0,
	0,0,1,1,1,1,0,0,0,0,0,1,1,1,0,0,
	0,0,0,1,1,0,0,0,0,0,0,1,1,1,0,0,
	0,0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,
	0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,0,
	0,0,0,0,0,0,0,0,1,1,1,0,0,0,0,0,
	0,0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,
	0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,
	0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,
	0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

// -AJA- Another hack, this variable holds the current sky when
// compute sky merging.  We hold onto the image, because there are
// six sides to compute, and we don't want to load the original
// image six times.  Removing this hack requires caching it in the
// cache system (which is not possible right now).
static epi::image_data_c *merging_sky_image;


//
//  UTILITY
//

#define PIXEL_RED(pix)  (what_palette[pix*3 + 0])
#define PIXEL_GRN(pix)  (what_palette[pix*3 + 1])
#define PIXEL_BLU(pix)  (what_palette[pix*3 + 2])

#define GAMMA_RED(pix)  GAMMA_CONV(PIXEL_RED(pix))
#define GAMMA_GRN(pix)  GAMMA_CONV(PIXEL_GRN(pix))
#define GAMMA_BLU(pix)  GAMMA_CONV(PIXEL_BLU(pix))

//
// DrawColumnIntoBlock
//
// Clip and draw an old-style column from a patch into a block.
//
static void DrawColumnIntoEpiBlock(image_c *rim, epi::image_data_c *img,
   const column_t *patchcol, int x, int y)
{
	SYS_ASSERT(patchcol);

	int w1 = rim->actual_w;
	int h1 = rim->actual_h;
	int w2 = rim->total_w;
//	int h2 = rim->total_h;

	// clip horizontally
	if (x < 0 || x >= w1)
		return;

	while (patchcol->topdelta != P_SENTINEL)
	{
		int top = y + (int) patchcol->topdelta;
		int count = patchcol->length;

		byte *src = (byte *) patchcol + 3;
		byte *dest = img->pixels + x;

		if (top < 0)
		{
			count += top;
			top = 0;
		}

		if (top + count > h1)
			count = h1 - top;

		// copy the pixels, remapping any TRANS_PIXEL values
		for (; count > 0; count--, src++, top++)
		{
			if (*src == TRANS_PIXEL)
				dest[(h1-1-top) * w2] = TRANS_REPLACE;
			else
				dest[(h1-1-top) * w2] = *src;
		}

		patchcol = (const column_t *) ((const byte *) patchcol + 
									   patchcol->length + 4);
	}
}

//
// CheckBlockSolid
//
// FIXME: Avoid future checks.
//
#define MAX_STRAY_PIXELS  2

static void CheckEpiBlockSolid(image_c *rim, epi::image_data_c *img)
{
	SYS_ASSERT(img->bpp == 1);

	int w1 = rim->actual_w;
	int h1 = rim->actual_h;
	int w2 = rim->total_w;
	int h2 = rim->total_h;

	int total_num = w1 * h1;
	int stray_count=0;

	int x, y;

	for (x=0; x < w1; x++)
	for (y=0; y < h1; y++)
	{
		byte src_pix = img->pixels[y * img->width + x];

		if (src_pix != TRANS_PIXEL)
			continue;

		stray_count++;

		// only ignore stray pixels on large images
		if (total_num < 256 || stray_count > MAX_STRAY_PIXELS)
			return;
	}

	// image is totally solid.  Blacken any transparent parts.
	rim->img_solid = true;

	for (x=0; x < w2; x++)
	for (y=0; y < h2; y++)
	{
		if (x >= w1 || y >= h1 ||
			img->pixels[y * img->width + x] == TRANS_PIXEL)
		{
			img->pixels[y * img->width + x] = pal_black;
		}
	}
}

//------------------------------------------------------------------------

//
//  BLOCK READING STUFF
//

//
// ReadFlatAsBlock
//
// Loads a flat from the wad and returns the image block for it.
// Doesn't do any mipmapping (this is too "raw" if you follow).
//
static epi::image_data_c *ReadFlatAsEpiBlock(image_c *rim)
{
	SYS_ASSERT(rim->source_type == IMSRC_Flat ||
				rim->source_type == IMSRC_Raw320x200);

	int tw = MAX(rim->total_w, 1);
	int th = MAX(rim->total_h, 1);

	int w = rim->actual_w;
	int h = rim->actual_h;

	epi::image_data_c *img = new epi::image_data_c(tw, th, 1);

	byte *dest = img->pixels;

#ifdef MAKE_TEXTURES_WHITE
	img->Clear(pal_white);
	return img;
#endif

	// clear initial image to black
	img->Clear(pal_black);

	// read in pixels
	const byte *src = (const byte*)W_CacheLumpNum(rim->source.flat.lump);

	for (int y=0; y < h; y++)
	for (int x=0; x < w; x++)
	{
		byte src_pix = src[y * w + x];

		byte *dest_pix = &dest[(h-1-y) * tw + x];

		// make sure TRANS_PIXEL values (which do not occur naturally in
		// Doom images) are properly remapped.
		if (src_pix == TRANS_PIXEL)
			dest_pix[0] = TRANS_REPLACE;
		else
			dest_pix[0] = src_pix;
	}

	W_DoneWithLump(src);

	return img;
}

//
// ReadTextureAsBlock
//
// Loads a texture from the wad and returns the image block for it.
// Doesn't do any mipmapping (this is too "raw" if you follow).
// This routine will also update the `solid' flag if texture turns
// out to be solid.
//
static epi::image_data_c *ReadTextureAsEpiBlock(image_c *rim)
{
	SYS_ASSERT(rim->source_type == IMSRC_Texture);

	texturedef_t *tdef = rim->source.texture.tdef;
	SYS_ASSERT(tdef);

	int tw = rim->total_w;
	int th = rim->total_h;

	epi::image_data_c *img = new epi::image_data_c(tw, th, 1);

#ifdef MAKE_TEXTURES_WHITE
	img->Clear(pal_white);
	return img;
#endif

	// Clear initial pixels to either totally transparent, or totally
	// black (if we know the image should be solid).  If the image turns
	// out to be solid instead of transparent, the transparent pixels
	// will be blackened.
  
	if (rim->img_solid)
		img->Clear(pal_black);
	else
		img->Clear(TRANS_PIXEL);

	int i;
	texpatch_t *patch;

	// Composite the columns into the block.
	for (i=0, patch=tdef->patches; i < tdef->patchcount; i++, patch++)
	{
		const patch_t *realpatch = (const patch_t*)W_CacheLumpNum(patch->patch);

		int realsize = W_LumpLength(patch->patch);

		int x1 = patch->originx;
		int y1 = patch->originy;
		int x2 = x1 + EPI_LE_S16(realpatch->width);

		int x = MAX(0, x1);

		x2 = MIN(tdef->width, x2);

		for (; x < x2; x++)
		{
			int offset = EPI_LE_S32(realpatch->columnofs[x - x1]);

			if (offset < 0 || offset >= realsize)
				I_Error("Bad image offset 0x%08x in image [%s]\n", offset, rim->name);

			const column_t *patchcol = (const column_t *)
				((const byte *) realpatch + offset);

			DrawColumnIntoEpiBlock(rim, img, patchcol, x, y1);
		}

		W_DoneWithLump(realpatch);
	}

	// update solid flag, if needed
	if (! rim->img_solid)
		CheckEpiBlockSolid(rim, img);

	return img;
}

//
// ReadPatchAsBlock
//
// Loads a patch from the wad and returns the image block for it.
// Very similiar to ReadTextureAsBlock() above.  Doesn't do any
// mipmapping (this is too "raw" if you follow).  This routine will
// also update the `solid' flag if it turns out to be 100% solid.
//
static epi::image_data_c *ReadPatchAsEpiBlock(image_c *rim)
{
	SYS_ASSERT(rim->source_type == IMSRC_Graphic ||
				rim->source_type == IMSRC_Sprite);

	int tw = rim->total_w;
	int th = rim->total_h;

	epi::image_data_c *img = new epi::image_data_c(tw, th, 1);

	// Clear initial pixels to either totally transparent, or totally
	// black (if we know the image should be solid).  If the image turns
	// out to be solid instead of transparent, the transparent pixels
	// will be blackened.
  
	if (rim->img_solid)
		img->Clear(pal_black);
	else
		img->Clear(TRANS_PIXEL);

	// Composite the columns into the block.
	const patch_t *realpatch = (const patch_t*)W_CacheLumpNum(rim->source.graphic.lump);

	int realsize = W_LumpLength(rim->source.graphic.lump);

	SYS_ASSERT(rim->actual_w == EPI_LE_S16(realpatch->width));
	SYS_ASSERT(rim->actual_h == EPI_LE_S16(realpatch->height));
  
	for (int x=0; x < rim->actual_w; x++)
	{
		int offset = EPI_LE_S32(realpatch->columnofs[x]);

		if (offset < 0 || offset >= realsize)
			I_Error("Bad image offset 0x%08x in image [%s]\n", offset, rim->name);

		const column_t *patchcol = (const column_t *)
			((const byte *) realpatch + offset);

		DrawColumnIntoEpiBlock(rim, img, patchcol, x, 0);
	}

	W_DoneWithLump(realpatch);

	// update solid flag, if needed
	if (! rim->img_solid)
		CheckEpiBlockSolid(rim, img);

	return img;
}

static void CalcSphereCoord(int px, int py, int pw, int ph, int face,
	float *sx, float *sy, float *sz)
{
	float ax = ((float)px + 0.5f) / (float)pw * 2.0f - 1.0f;
	float ay = ((float)py + 0.5f) / (float)ph * 2.0f - 1.0f;

///????	ay = -ay;

	switch (face)
	{
		case WSKY_North:
			*sx = ax; *sy = 1.0f; *sz = ay; break;

		case WSKY_South:
			*sx = -ax; *sy = -1.0f; *sz = ay; break;

		case WSKY_East:
			*sx = 1.0f; *sy = -ax; *sz = ay; break;

		case WSKY_West:
			*sx = -1.0f; *sy = ax; *sz = ay; break;

		case WSKY_Top:
			*sx = ax; *sy = -ay; *sz = 1.0f; break;

		case WSKY_Bottom:
			*sx = ax; *sy = ay; *sz = -1.0f; break;

		default:
			*sx = *sy = *sz = 0; break;
	}

	// normalise the vector (FIXME: optimise the sqrt)
	float len = sqrt((*sx) * (*sx) + (*sy) * (*sy) + (*sz) * (*sz));

	if (len > 0)
	{
		*sx /= len; *sy /= len; *sz /= len;
	}
}

static inline bool SkyIsNarrow(const image_c *sky)
{
	// check the aspect of the image
	return (IM_WIDTH(sky) / IM_HEIGHT(sky)) < 2.28f;
}

// needed for SKY
extern epi::image_data_c *ReadAsEpiBlock(image_c *rim);

//
// ReadSkyMergeAsBlock
//
static epi::image_data_c *ReadSkyMergeAsEpiBlock(image_c *rim)
{
	SYS_ASSERT(rim->source_type == IMSRC_SkyMerge);
	SYS_ASSERT(rim->actual_w == rim->total_w);
	SYS_ASSERT(rim->actual_h == rim->total_h);

	int tw = rim->total_w;
	int th = rim->total_h;

	// Yuck! Recursive call into image system. Hope nothing breaks...
	const image_c *sky = rim->source.merge.sky;
	image_c *sky_rim = (image_c *) sky; // Intentional Const Override

	// get correct palette
	const byte *what_palette = (const byte *) &playpal_data[0];
	bool what_pal_cached = false;

	if (sky_rim->source_palette >= 0)
	{
		what_palette = (const byte *) W_CacheLumpNum(sky_rim->source_palette);
		what_pal_cached = true;
	}

	// big hack (see note near top of file)
	if (! merging_sky_image)
		merging_sky_image = ReadAsEpiBlock(sky_rim);

	epi::image_data_c *sky_img = merging_sky_image;

	epi::image_data_c *img = new epi::image_data_c(tw, th, 3);

#if 0 // DEBUG
	I_Printf("SkyMerge: Image %p face %d\n", rim, rim->source.merge.face);
#endif
	bool narrow = SkyIsNarrow(sky);

	byte *src = sky_img->pixels;
	byte *dest = img->pixels;

	int sk_w = sky_img->width;
	int ds_w = img->width;
	int ds_h = img->height;

	for (int y=0; y < rim->total_h; y++)
	for (int x=0; x < rim->total_w; x++)
	{
		float sx, sy, sz;
		float tx, ty;

		CalcSphereCoord(x, y, rim->total_w, rim->total_h,
			rim->source.merge.face, &sx, &sy, &sz);

		RGL_CalcSkyCoord(sx, sy, sz, narrow, &tx, &ty);

		int TX = (int)(tx * sky_img->width  * 16);
		int TY = (int)(ty * sky_img->height * 16);

		TX = (TX + sky_img->width  * 64) % (sky_img->width  * 16);
		TY = (TY + sky_img->height * 64) % (sky_img->height * 16);

		if (TX < 0) TX = 0;
		if (TY < 0) TY = 0;

		TX = sky_img->width*16-1-TX;

///---	// FIXME: handle images everywhere with bottom-up coords
///---	if (sky_img->bpp >= 3) TY = sky_img->height*16-1-TY;

		int FX = TX % 16;
		int FY = TY % 16;

		TX = TX / 16;
		TY = TY / 16;
		
#if 0 // DEBUG
		if ((x==0 || x==rim->total_w-1) && (y==0 || y==rim->total_h-1))
		{
			I_Printf("At (%d,%d) : sphere (%1.2f,%1.2f,%1.2f)  tex (%1.4f,%1.4f)\n",
			x, y, sx, sy, sz, tx, ty);
		}
#endif

		// bilinear filtering

		int TY2 = (TY >= sky_img->height-1) ? TY : (TY+1);
		int TX2 = (TX + 1) % sky_img->width;

		byte rA, rB, rC, rD;
		byte gA, gB, gC, gD;
		byte bA, bB, bC, bD;

		switch (sky_img->bpp)
		{
			case 1:
			{
				byte src_A = src[TY  * sk_w + TX];
				byte src_B = src[TY  * sk_w + TX2];
				byte src_C = src[TY2 * sk_w + TX];
				byte src_D = src[TY2 * sk_w + TX2];

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
				rA = src[(TY * sk_w + TX) * 3 + 0];
				gA = src[(TY * sk_w + TX) * 3 + 1];
				bA = src[(TY * sk_w + TX) * 3 + 2];

				rB = src[(TY * sk_w + TX2) * 3 + 0];
				gB = src[(TY * sk_w + TX2) * 3 + 1];
				bB = src[(TY * sk_w + TX2) * 3 + 2];

				rC = src[(TY2 * sk_w + TX) * 3 + 0];
				gC = src[(TY2 * sk_w + TX) * 3 + 1];
				bC = src[(TY2 * sk_w + TX) * 3 + 2];

				rD = src[(TY2 * sk_w + TX2) * 3 + 0];
				gD = src[(TY2 * sk_w + TX2) * 3 + 1];
				bD = src[(TY2 * sk_w + TX2) * 3 + 2];
			}
			break;

			case 4:
			{
				rA = src[(TY * sk_w + TX) * 4 + 0];
				gA = src[(TY * sk_w + TX) * 4 + 1];
				bA = src[(TY * sk_w + TX) * 4 + 2];

				rB = src[(TY * sk_w + TX2) * 4 + 0];
				gB = src[(TY * sk_w + TX2) * 4 + 1];
				bB = src[(TY * sk_w + TX2) * 4 + 2];

				rC = src[(TY2 * sk_w + TX) * 4 + 0];
				gC = src[(TY2 * sk_w + TX) * 4 + 1];
				bC = src[(TY2 * sk_w + TX) * 4 + 2];

				rD = src[(TY2 * sk_w + TX2) * 4 + 0];
				gD = src[(TY2 * sk_w + TX2) * 4 + 1];
				bD = src[(TY2 * sk_w + TX2) * 4 + 2];
			}
			break;

			default:  // remove compiler warning
				rA = rB = rC = rD = 0;
				gA = gB = gC = gD = 0;
				bA = bB = bC = bD = 0;
				break;
		}

		int r = (int)rA * (15-FX) * (15-FY) +
				(int)rB * (   FX) * (15-FY) +
				(int)rC * (15-FX) * (   FY) +
				(int)rD * (   FX) * (   FY);

		int g = (int)gA * (15-FX) * (15-FY) +
				(int)gB * (   FX) * (15-FY) +
				(int)gC * (15-FX) * (   FY) +
				(int)gD * (   FX) * (   FY);

		int b = (int)bA * (15-FX) * (15-FY) +
				(int)bB * (   FX) * (15-FY) +
				(int)bC * (15-FX) * (   FY) +
				(int)bD * (   FX) * (   FY);

		r /= 225; g /= 225; b /= 225;

		int yy = ds_h - 1 - y;

		dest[(y * ds_w + x) * 3 + 0] = r;
		dest[(y * ds_w + x) * 3 + 1] = g;
		dest[(y * ds_w + x) * 3 + 2] = b;
	}

	if (what_pal_cached)
		W_DoneWithLump(what_palette);

	return img;
}


//
// ReadDummyAsBlock
//
// Creates a dummy image.
//
static epi::image_data_c *ReadDummyAsEpiBlock(image_c *rim)
{
	SYS_ASSERT(rim->source_type == IMSRC_Dummy);
	SYS_ASSERT(rim->actual_w == rim->total_w);
	SYS_ASSERT(rim->actual_h == rim->total_h);
	SYS_ASSERT(rim->total_w == DUMMY_X);
	SYS_ASSERT(rim->total_h == DUMMY_Y);

	epi::image_data_c *img = new epi::image_data_c(DUMMY_X, DUMMY_Y, 4);

	// copy pixels
	for (int y=0; y < DUMMY_Y; y++)
	for (int x=0; x < DUMMY_X; x++)
	{
		byte *dest_pix = img->PixelAt(x, y);

		if (dummy_graphic[(DUMMY_Y-1 - y) * DUMMY_X + x])
		{
			*dest_pix++ = (rim->source.dummy.fg & 0xFF0000) >> 16;
			*dest_pix++ = (rim->source.dummy.fg & 0x00FF00) >> 8;
			*dest_pix++ = (rim->source.dummy.fg & 0x0000FF);
			*dest_pix++ = 255;
		}
		else if (rim->source.dummy.bg == TRANS_PIXEL)
		{
			*dest_pix++ = 0;
			*dest_pix++ = 0;
			*dest_pix++ = 0;
			*dest_pix++ = 0;
		}
		else
		{
			*dest_pix++ = (rim->source.dummy.bg & 0xFF0000) >> 16;
			*dest_pix++ = (rim->source.dummy.bg & 0x00FF00) >> 8;
			*dest_pix++ = (rim->source.dummy.bg & 0x0000FF);
			*dest_pix++ = 255;
		}
	}

	return img;
}

static void NormalizeClearAreas(epi::image_data_c *img)
{
	// makes sure that any totally transparent pixel (alpha == 0)
	// has a colour of black.  This shows up when smoothing is on.

	SYS_ASSERT(img->bpp == 4);

	byte *dest = img->pixels;

	for (int y = 0; y < img->height; y++)
	for (int x = 0; x < img->width;  x++)
	{
		if (dest[3] == 0)
		{
			dest[0] = dest[1] = dest[2] = 0;
		}

		dest += 4;
	}
}

static void CreateUserColourImage(epi::image_data_c *img, imagedef_c *def)
{
	byte *dest = img->pixels;

	for (int y = 0; y < img->height; y++)
	for (int x = 0; x < img->width;  x++)
	{
		*dest++ = (def->colour & 0xFF0000) >> 16;  // R
		*dest++ = (def->colour & 0x00FF00) >>  8;  // G
		*dest++ = (def->colour & 0x0000FF);        // B

		if (img->bpp == 4)
			*dest++ = 0xFF;
	}
}

static void FourWaySymmetry(epi::image_data_c *img)
{
	// the already-drawn corner has the lowest x and y values.  When
	// width or height is odd, the middle column/row must already be
	// drawn.

	int w2 = (img->width  + 1) / 2;
	int h2 = (img->height + 1) / 2;

	for (int y = 0; y < h2; y++)
	for (int x = 0; x < w2; x++)
	{
		int ix = img->width  - 1 - x;
		int iy = img->height - 1 - y;

		img->CopyPixel(x, y, ix,  y);
		img->CopyPixel(x, y,  x, iy);
		img->CopyPixel(x, y, ix, iy);
	}
}

static void EightWaySymmetry(epi::image_data_c *img)
{
	// Note: the corner already drawn has lowest x and y values, and
	// the triangle piece is where x >= y.  The diagonal (x == y) must
	// already be drawn.

	SYS_ASSERT(img->width == img->height);

	int hw = (img->width + 1) / 2;

	for (int y = 0;   y < hw; y++)
	for (int x = y+1; x < hw; x++)
	{
		img->CopyPixel(x, y, y, x);
	}

	FourWaySymmetry(img);
}

static void CreateUserBuiltinLinear(epi::image_data_c *img, imagedef_c *def)
{
	SYS_ASSERT(img->bpp == 4);
	SYS_ASSERT(img->width == img->height);

	int hw = (img->width + 1) / 2;

	for (int y = 0; y < hw; y++)
	for (int x = y; x < hw; x++)
	{
		byte *dest = img->pixels + (y * img->width + x) * 4;

		float dx = (hw-1 - x) / float(hw);
		float dy = (hw-1 - y) / float(hw);

		float hor_p2 = dx * dx + dy * dy;
		float sq = 1.0f / sqrt(1.0f + DL_OUTER * DL_OUTER * hor_p2);

		// ramp intensity down to zero at the outer edge
		float horiz = sqrt(hor_p2);
		if (horiz > 0.80f)
			sq = sq * (0.98f - horiz) / (0.98f - 0.80f);

		int v = int(sq * 255.4f);

		if (v < 0 || 
			x == 0 || x == img->width-1 ||
			y == 0 || y == img->height-1)
		{
			v = 0;
		}

		dest[0] = dest[1] = dest[2] = v;
		dest[3] = 255;
	}

	EightWaySymmetry(img);
}

static void CreateUserBuiltinQuadratic(epi::image_data_c *img, imagedef_c *def)
{
	SYS_ASSERT(img->bpp == 4);
	SYS_ASSERT(img->width == img->height);

	int hw = (img->width + 1) / 2;

	for (int y = 0; y < hw; y++)
	for (int x = y; x < hw; x++)
	{
		byte *dest = img->pixels + (y * img->width + x) * 4;

		float dx = (hw-1 - x) / float(hw);
		float dy = (hw-1 - y) / float(hw);

		float hor_p2 = dx * dx + dy * dy;

		float sq = 0.3f / (1.0f + DL_OUTER * hor_p2) +
		           0.7f * MIN(1.0f, 2.0f / (1.0f + DL_OUTER * 2.0f * hor_p2));

		// ramp intensity down to zero at the outer edge
		float horiz = sqrt(hor_p2);
		if (horiz > 0.80f)
			sq = sq * (0.98f - horiz) / (0.98f - 0.80f);

// hor_p2 = cos(hor_p2*6.2)/2+0.5; //!!!! RING SHAPE ??
sq = exp(-5.44 * hor_p2);
		
 	int v = int(sq * 255.4f);  ////  * sin(hor_p2*7.7));

		if (v < 0 ||
			x == 0 || x == img->width-1 ||
			y == 0 || y == img->height-1)
		{
			v = 0;
		}

		dest[0] = dest[1] = dest[2] = v;
		dest[3] = 255;
	}

	EightWaySymmetry(img);
}

static void CreateUserBuiltin_RING(epi::image_data_c *img, imagedef_c *def)
{
	SYS_ASSERT(img->bpp == 4);
	SYS_ASSERT(img->width == img->height);

	int hw = (img->width + 1) / 2;

	for (int y = 0; y < hw; y++)
	for (int x = y; x < hw; x++)
	{
		byte *dest = img->pixels + (y * img->width + x) * 4;

		float dx = (hw-1 - x) / float(hw);
		float dy = (hw-1 - y) / float(hw);

		float hor_p2 = dx * dx + dy * dy;

		float sq = 0.3f / (1.0f + DL_OUTER * hor_p2) +
		           0.7f * MIN(1.0f, 2.0f / (1.0f + DL_OUTER * 2.0f * hor_p2));

		// ramp intensity down to zero at the outer edge
		float horiz = sqrt(hor_p2);
		if (horiz > 0.80f)
			sq = sq * (0.98f - horiz) / (0.98f - 0.80f);

//hor_p2 = hor_p2*2; if (hor_p2 > 1) hor_p2 = 2.0 - hor_p2;
//hor_p2 = 1 - hor_p2;

sq = exp(-5.44 * hor_p2);

		int v = int(sq * 255.4f);

		if (v < 0 ||
			x == 0 || x == img->width-1 ||
			y == 0 || y == img->height-1)
		{
			v = 0;
		}

		dest[0] = v;
		dest[1] = v*v/255;
		dest[2] = sqrt(v*255);
		dest[3] = 255;
	}

	EightWaySymmetry(img);
}

static void CreateUserBuiltinShadow(epi::image_data_c *img, imagedef_c *def)
{
	SYS_ASSERT(img->bpp == 4);
	SYS_ASSERT(img->width == img->height);

	int hw = (img->width + 1) / 2;

	for (int y = 0; y < hw; y++)
	for (int x = y; x < hw; x++)
	{
		byte *dest = img->pixels + (y * img->width + x) * 4;

		float dx = (hw-1 - x) / float(hw);
		float dy = (hw-1 - y) / float(hw);

		float horiz = sqrt(dx * dx + dy * dy);
		float sq = 1.0f - horiz;

		int v = int(sq * 170.4f);

		if (v < 0 ||
			x == 0 || x == img->width-1 ||
			y == 0 || y == img->height-1)
		{
			v = 0;
		}

		*dest++ = 0;
		*dest++ = 0;
		*dest++ = 0;
		*dest   = v;
	}

	EightWaySymmetry(img);
}

epi::file_c *OpenUserFileOrLump(imagedef_c *def)
{
	if (def->type == IMGDT_File)
	{
		// -AJA- 2005/01/15: filenames in DDF relative to GAMEDIR
		return M_OpenComposedEPIFile(game_dir.c_str(), def->name.c_str());
	}
	else  /* LUMP */
	{
		int lump = W_CheckNumForName(def->name.c_str());

		if (lump < 0)
			return NULL;

		return W_OpenLump(lump);
///---		const byte *lump_data = (byte *)W_CacheLumpNum(lump);
///---		int length = W_LumpLength(lump);
///---
///---		return new epi::mem_file_c(lump_data, length, false /* no copying */);
	}
}

void CloseUserFileOrLump(imagedef_c *def, epi::file_c *f)
{
	delete f;

///---	if (def->type == IMGDT_File)
///---	{
///---		delete f;
///---	}
///---	else  /* LUMP */
///---	{
///---		// FIXME: create sub-class of mem_file_c in WAD code
///---		epi::mem_file_c *mf = (epi::mem_file_c *) f;
///---
///---		W_DoneWithLump(mf->data);
///---
///---		delete f;
///---	}
}

static epi::image_data_c *CreateUserFileImage(image_c *rim, imagedef_c *def)
{
	epi::file_c *f = OpenUserFileOrLump(def);

	if (! f)
		I_Error("Missing image file: %s\n", def->name.c_str());

	epi::image_data_c *img;

	if (def->format == LIF_JPEG)
		img = epi::JPEG_Load(f, epi::IRF_Round_POW2);
	else if (def->format == LIF_TGA)
		img = epi::TGA_Load (f, epi::IRF_Round_POW2);
	else
		img = epi::PNG_Load (f, epi::IRF_Round_POW2);

	CloseUserFileOrLump(def, f);

	if (! img)
		I_Error("Error occurred loading image file: %s\n",
			def->name.c_str());

#if 1  // DEBUGGING
	L_WriteDebug("CREATE IMAGE [%s] %dx%d < %dx%d %s --> %p %dx%d bpp %d\n",
	rim->name,
	rim->actual_w, rim->actual_h,
	rim->total_w, rim->total_h,
	rim->img_solid ? "SOLID" : "MASKED",
	img, img->width, img->height, img->bpp);
#endif

	if (img->bpp == 4)
		NormalizeClearAreas(img);

	SYS_ASSERT(rim->total_w == img->width);
	SYS_ASSERT(rim->total_h == img->height);

	return img;
}

//
// ReadUserAsBlock
//
// Loads or Creates the user defined image.
// Doesn't do any mipmapping (this is too "raw" if you follow).
//
static epi::image_data_c *ReadUserAsEpiBlock(image_c *rim)
{
	SYS_ASSERT(rim->source_type == IMSRC_User);

	int tw = MAX(rim->total_w, 1);
	int th = MAX(rim->total_h, 1);

	int bpp = rim->img_solid ? 3 : 4;

	// clear initial image to black / transparent
	/// ALREADY DONE: memset(dest, pal_black, tw * th * bpp);

	imagedef_c *def = rim->source.user.def;

	switch (def->type)
	{
		case IMGDT_Colour:
		{
			epi::image_data_c *img = new epi::image_data_c(tw, th, bpp);
			CreateUserColourImage(img, def);
			return img;
		}

		case IMGDT_Builtin:
		{
			epi::image_data_c *img = new epi::image_data_c(tw, th, bpp);
			switch (def->builtin)
			{
				case BLTIM_Linear:
					CreateUserBuiltin_RING(img, def); //!!!!!
					break;

				case BLTIM_Quadratic:
					CreateUserBuiltinQuadratic(img, def);
					break;

				case BLTIM_Shadow:
					CreateUserBuiltinShadow(img, def);
					break;

				default:
					I_Error("ReadUserAsEpiBlock: Unknown builtin %d\n", def->builtin);
					break;
			}
			return img;
		}

		case IMGDT_File:
		case IMGDT_Lump:
		    return CreateUserFileImage(rim, def);

		default:
			I_Error("ReadUserAsEpiBlock: Coding error, unknown type %d\n", def->type);
	}

	return NULL;  /* NOT REACHED */
}

//
// ReadAsEpiBlock
//
// Read the image from the wad into an image_data_c class.
// The image returned is normally palettised (bpp == 1), and the
// palette must be determined from rim->source_palette.  Mainly
// just a switch to more specialised image readers.
//
// Never returns NULL.
//
epi::image_data_c *ReadAsEpiBlock(image_c *rim) 
{
	switch (rim->source_type)
	{
		case IMSRC_Flat:
		case IMSRC_Raw320x200:
			return ReadFlatAsEpiBlock(rim);

		case IMSRC_Texture:
			return ReadTextureAsEpiBlock(rim);

		case IMSRC_Graphic:
		case IMSRC_Sprite:
			return ReadPatchAsEpiBlock(rim);

		case IMSRC_SkyMerge:
			return ReadSkyMergeAsEpiBlock(rim);

		case IMSRC_Dummy:
			return ReadDummyAsEpiBlock(rim);
    
		case IMSRC_User:
			return ReadUserAsEpiBlock(rim);
      
		default:
			I_Error("ReadAsBlock: unknown source_type %d !\n", rim->source_type);
			return NULL;
	}
}

void W_ImageClearMergingSky(void)
{
	if (merging_sky_image)
		delete merging_sky_image;

	merging_sky_image = NULL;
}



//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
