//----------------------------------------------------------------------------
//  EDGE Texture Upload
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

#include <limits.h>

#include "epi/image_data.h"

#include "e_search.h"
#include "e_main.h"
#include "m_argv.h"
#include "m_misc.h"
#include "p_local.h"
#include "r_gldefs.h"
#include "r_image.h"
#include "r_sky.h"
#include "r_texgl.h"
#include "r_colormap.h"

#include "w_texture.h"
#include "w_wad.h"


int alpha_mip_thresh = 1;


int W_MakeValidSize(int value)
{
	SYS_ASSERT(value > 0);

	if (value <=    1) return    1;
	if (value <=    2) return    2;
	if (value <=    4) return    4;
	if (value <=    8) return    8;
	if (value <=   16) return   16;
	if (value <=   32) return   32;
	if (value <=   64) return   64;
	if (value <=  128) return  128;
	if (value <=  256) return  256;
	if (value <=  512) return  512;
	if (value <= 1024) return 1024;
	if (value <= 2048) return 2048;
	if (value <= 4096) return 4096;

	I_Error("Texture size (%d) too large !\n", value);
	return -1; /* NOT REACHED */
}


epi::image_data_c *R_PalettisedToRGB(epi::image_data_c *src,
									 const byte *palette, int opacity)
{
	int bpp = (opacity == OPAC_Solid) ? 3 : 4;

	epi::image_data_c *dest = new epi::image_data_c(src->width, src->height, bpp);	

	dest->used_w = src->used_w;
	dest->used_h = src->used_h;

	for (int y=0; y < src->height; y++)
	for (int x=0; x < src->width;  x++)
	{
		byte src_pix = src->PixelAt(x, y)[0];

		byte *dest_pix = dest->PixelAt(x, y);

		if (src_pix == TRANS_PIXEL)
		{
			dest_pix[0] = dest_pix[1] = dest_pix[2] = 0;
			
			if (bpp == 4)
				dest_pix[3] = 0;
		}
		else
		{
			dest_pix[0] = palette[src_pix*3 + 0];
			dest_pix[1] = palette[src_pix*3 + 1];
			dest_pix[2] = palette[src_pix*3 + 2];

			if (bpp == 4)
				dest_pix[3] = 255;
		}
	}

	return dest;
}


///---// use a common buffer for image shrink operations, saving the
///---// overhead of allocating a new buffer for every image.
///---static byte *img_shrink_buffer;
///---static int img_shrink_buf_size;
///---
///---
///---static byte *ShrinkGetBuffer(int required)
///---{
///---	if (img_shrink_buffer && img_shrink_buf_size >= required)
///---		return img_shrink_buffer;
///---	
///---	if (img_shrink_buffer)
///---		delete[] img_shrink_buffer;
///---	
///---	img_shrink_buffer   = new byte[required];
///---	img_shrink_buf_size = required;
///---
///---	return img_shrink_buffer;
///---}
///---
///---//
///---// ShrinkBlockRGBA
///---//
///---// Just like ShrinkBlock() above, but the returned format is RGBA.
///---// Source format is column-major (i.e. normal block), whereas result
///---// is row-major (ano note that GL textures are _bottom up_ rather than
///---// the usual top-down ordering).  The new size should be scaled down
///---// to fit into glmax_tex_size.
///---//
///---static byte *ShrinkBlockRGBA(byte *src, int total_w, int total_h,
///---							 int new_w, int new_h, const byte *what_palette)
///---{
///---	byte *dest;
///--- 
///---	int x, y, dx, dy;
///---	int step_x, step_y;
///---
///---	SYS_ASSERT(new_w > 0);
///---	SYS_ASSERT(new_h > 0);
///---	SYS_ASSERT(new_w <= glmax_tex_size);
///---	SYS_ASSERT(new_h <= glmax_tex_size);
///---	SYS_ASSERT((total_w % new_w) == 0);
///---	SYS_ASSERT((total_h % new_h) == 0);
///---
///---	dest = ShrinkGetBuffer(new_w * new_h * 4);
///---
///---	step_x = total_w / new_w;
///---	step_y = total_h / new_h;
///---
///---	// faster method for the usual case (no shrinkage)
///---
///---	if (step_x == 1 && step_y == 1)
///---	{
///---		for (y=0; y < total_h; y++)
///---		for (x=0; x < total_w; x++)
///---		{
///---			byte src_pix = src[y * total_w + x];
///---			byte *dest_pix = dest + (((y) * total_w + x) * 4);
///---
///---			if (src_pix == TRANS_PIXEL)
///---			{
///---				dest_pix[0] = dest_pix[1] = dest_pix[2] = dest_pix[3] = 0;
///---			}
///---			else
///---			{
///---				dest_pix[0] = GAMMA_RED(src_pix);
///---				dest_pix[1] = GAMMA_GRN(src_pix);
///---				dest_pix[2] = GAMMA_BLU(src_pix);
///---				dest_pix[3] = 255;
///---			}
///---		}
///---		return dest;
///---	}
///---
///---	// slower method, as we must shrink the bugger...
///---
///---	for (y=0; y < new_h; y++)
///---	for (x=0; x < new_w; x++)
///---	{
///---		byte *dest_pix = dest + (((y) * new_w + x) * 4);
///---
///---		int px = x * step_x;
///---		int py = y * step_y;
///---
///---		int tot_r=0, tot_g=0, tot_b=0, a_count=0, alpha;
///---		int total = step_x * step_y;
///---
///---		// compute average colour of block
///---		for (dx=0; dx < step_x; dx++)
///---		for (dy=0; dy < step_y; dy++)
///---		{
///---			byte src_pix = src[(py+dy) * total_w + (px+dx)];
///---
///---			if (src_pix == TRANS_PIXEL)
///---				a_count++;
///---			else
///---			{
///---				tot_r += GAMMA_RED(src_pix);
///---				tot_g += GAMMA_GRN(src_pix);
///---				tot_b += GAMMA_BLU(src_pix);
///---			}
///---		}
///---
///---		if (a_count >= total)
///---		{
///---			// all pixels were translucent.  Keep r/g/b as zero.
///---			alpha = 0;
///---		}
///---		else
///---		{
///---			alpha = (total - a_count) * 255 / total;
///---
///---			total -= a_count;
///---
///---			tot_r /= total;
///---			tot_g /= total;
///---			tot_b /= total;
///---		}
///---
///---		dest_pix[0] = tot_r;
///---		dest_pix[1] = tot_g;
///---		dest_pix[2] = tot_b;
///---		dest_pix[3] = alpha;
///---	}
///---
///---	return dest;
///---}
///---
///---static byte *ShrinkNormalRGB(byte *rgb, int total_w, int total_h,
///---							  int new_w, int new_h)
///---{
///---	byte *dest;
///--- 
///---	int i, x, y, dx, dy;
///---	int step_x, step_y;
///---
///---	SYS_ASSERT(new_w > 0);
///---	SYS_ASSERT(new_h > 0);
///---	SYS_ASSERT(new_w <= glmax_tex_size);
///---	SYS_ASSERT(new_h <= glmax_tex_size);
///---	SYS_ASSERT((total_w % new_w) == 0);
///---	SYS_ASSERT((total_h % new_h) == 0);
///---
///---	dest = ShrinkGetBuffer(new_w * new_h * 3);
///---
///---	step_x = total_w / new_w;
///---	step_y = total_h / new_h;
///---
///---	// faster method for the usual case (no shrinkage)
///---
///---	if (step_x == 1 && step_y == 1)
///---	{
///---		for (y=0; y < total_h; y++)
///---		for (x=0; x < total_w; x++)
///---		{
///---			for (i=0; i < 3; i++)
///---				dest[(y * total_w + x) * 3 + i] = GAMMA_CONV(
///---					rgb[(y * total_w + x) * 3 + i]);
///---		}
///---		return dest;
///---	}
///---
///---	// slower method, as we must shrink the bugger...
///---
///---	for (y=0; y < new_h; y++)
///---	for (x=0; x < new_w; x++)
///---	{
///---		byte *dest_pix = dest + ((y * new_w + x) * 3);
///---
///---		int px = x * step_x;
///---		int py = y * step_y;
///---
///---		int tot_r=0, tot_g=0, tot_b=0;
///---		int total = step_x * step_y;
///---
///---		// compute average colour of block
///---		for (dx=0; dx < step_x; dx++)
///---		for (dy=0; dy < step_y; dy++)
///---		{
///---			byte *src_pix = rgb + (((py+dy) * total_w + (px+dx)) * 3);
///---
///---			tot_r += GAMMA_CONV(src_pix[0]);
///---			tot_g += GAMMA_CONV(src_pix[1]);
///---			tot_b += GAMMA_CONV(src_pix[2]);
///---		}
///---
///---		dest_pix[0] = tot_r / total;
///---		dest_pix[1] = tot_g / total;
///---		dest_pix[2] = tot_b / total;
///---	}
///---
///---	return dest;
///---}
///---
///---static byte *ShrinkNormalRGBA(byte *rgba, int total_w, int total_h,
///---							  int new_w, int new_h)
///---{
///---	byte *dest;
///--- 
///---	int i, x, y, dx, dy;
///---	int step_x, step_y;
///---
///---	SYS_ASSERT(new_w > 0);
///---	SYS_ASSERT(new_h > 0);
///---	SYS_ASSERT(new_w <= glmax_tex_size);
///---	SYS_ASSERT(new_h <= glmax_tex_size);
///---	SYS_ASSERT((total_w % new_w) == 0);
///---	SYS_ASSERT((total_h % new_h) == 0);
///---
///---	dest = ShrinkGetBuffer(new_w * new_h * 4);
///---
///---	step_x = total_w / new_w;
///---	step_y = total_h / new_h;
///---
///---	// faster method for the usual case (no shrinkage)
///---
///---	if (step_x == 1 && step_y == 1)
///---	{
///---		for (y=0; y < total_h; y++)
///---		for (x=0; x < total_w; x++)
///---		{
///---			for (i=0; i < 4; i++)
///---				dest[(y * total_w + x) * 4 + i] = GAMMA_CONV(
///---					rgba[(y * total_w + x) * 4 + i]);
///---		}
///---		return dest;
///---	}
///---
///---	// slower method, as we must shrink the bugger...
///---
///---	for (y=0; y < new_h; y++)
///---	for (x=0; x < new_w; x++)
///---	{
///---		byte *dest_pix = dest + ((y * new_w + x) * 4);
///---
///---		int px = x * step_x;
///---		int py = y * step_y;
///---
///---		int tot_r=0, tot_g=0, tot_b=0, tot_a=0;
///---		int total = step_x * step_y;
///---
///---		// compute average colour of block
///---		for (dx=0; dx < step_x; dx++)
///---		for (dy=0; dy < step_y; dy++)
///---		{
///---			byte *src_pix = rgba + (((py+dy) * total_w + (px+dx)) * 4);
///---
///---			tot_r += GAMMA_CONV(src_pix[0]);
///---			tot_g += GAMMA_CONV(src_pix[1]);
///---			tot_b += GAMMA_CONV(src_pix[2]);
///---			tot_a += GAMMA_CONV(src_pix[3]);
///---		}
///---
///---		dest_pix[0] = tot_r / total;
///---		dest_pix[1] = tot_g / total;
///---		dest_pix[2] = tot_b / total;
///---		dest_pix[3] = tot_a / total;
///---	}
///---
///---	return dest;
///---}




GLuint R_UploadTexture(epi::image_data_c *img, int flags, int max_pix)
{
	/* Send the texture data to the GL, and returns the texture ID
	 * assigned to it.
	 */

	SYS_ASSERT(img->bpp == 3 || img->bpp == 4);

	bool clamp  = (flags & UPL_Clamp)  ? true : false;
	bool nomip  = (flags & UPL_MipMap) ? false : true;
	bool smooth = (flags & UPL_Smooth) ? true : false;
	
  	int total_w = img->width;
	int total_h = img->height;

	int new_w, new_h;

	// scale down, if necessary, to fix the maximum size
	for (new_w = total_w; new_w > glmax_tex_size; new_w /= 2)
	{ /* nothing here */ }

	for (new_h = total_h; new_h > glmax_tex_size; new_h /= 2)
	{ /* nothing here */ }

	while (new_w * new_h > max_pix)
	{
		if (new_h >= new_w)
			new_h /= 2;
		else
			new_w /= 2;
	}

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	GLuint id;

	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);

	int tmode = GL_REPEAT;

	if (clamp)
		tmode = dumb_clamp ? GL_CLAMP : GL_CLAMP_TO_EDGE;

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, tmode);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, tmode);

	// magnification mode
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
					smooth ? GL_LINEAR : GL_NEAREST);

	// minification mode
	var_mipmapping = MIN(2, MAX(0, var_mipmapping));

	static GLuint minif_modes[2*3] =
	{
		GL_NEAREST,
		GL_NEAREST_MIPMAP_NEAREST,
		GL_NEAREST_MIPMAP_LINEAR,
	  
		GL_LINEAR,
		GL_LINEAR_MIPMAP_NEAREST,
		GL_LINEAR_MIPMAP_LINEAR
	};

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, 
					minif_modes[(smooth ? 3 : 0) +
							   (nomip ? 0 : var_mipmapping)]);

	for (int mip=0; ; mip++)
	{
		if (img->width != new_w || img->height != new_h)
		{
			img->ShrinkMasked(new_w, new_h);

			if (flags & UPL_Thresh)
				img->ThresholdAlpha(alpha_mip_thresh);
		}

///---		byte *rgba_src = 0;
///---
///---		switch (img->bpp)
///---		{
///---			case 1:
///---				rgba_src = ShrinkBlockRGBA(img->pixels, total_w, total_h,
///---					new_w, new_h, palette);
///---				break;
///---
///---			case 3:
///---				rgba_src = ShrinkNormalRGB(img->pixels, total_w, total_h, new_w, new_h);
///---				break;
///---    
///---			case 4:
///---				rgba_src = ShrinkNormalRGBA(img->pixels, total_w, total_h, new_w, new_h);
///---				break;
///---		}
///---
///---		SYS_ASSERT(rgba_src);
    
		glTexImage2D(GL_TEXTURE_2D, mip, (img->bpp == 3) ? GL_RGB : GL_RGBA,
					 new_w, new_h, 0 /* border */,
					 (img->bpp == 3) ? GL_RGB : GL_RGBA,
					 GL_UNSIGNED_BYTE, img->PixelAt(0,0));

		// stop if mipmapping disabled or we have reached the end
		if (nomip || !var_mipmapping || (new_w == 1 && new_h == 1))
			break;

		new_w = MAX(1, new_w / 2);
		new_h = MAX(1, new_h / 2);

		// -AJA- 2003/12/05: workaround for Radeon 7500 driver bug, which
		//       incorrectly draws the 1x1 mip texture as black.
#ifndef WIN32
		if (new_w == 1 && new_h == 1)
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, mip);
#endif
	}

	return id;
}


//----------------------------------------------------------------------------


void R_PaletteRemapRGBA(epi::image_data_c *img,
	const byte *new_pal, const byte *old_pal)
{
	const int max_prev = 16;

	// cache of previously looked-up colours (in pairs)
	u8_t previous[max_prev * 6];
	int num_prev = 0;

	for (int y = 0; y < img->height; y++)
	for (int x = 0; x < img->width;  x++)
	{
		u8_t *cur = img->PixelAt(x, y);

		// skip completely transparent pixels
		if (img->bpp == 4 && cur[3] == 0)
			continue;

		// optimisation: if colour matches previous one, don't need
		// to compute the remapping again.
		int i;
		for (i = 0; i < num_prev; i++)
		{
			if (previous[i*6+0] == cur[0] &&
				previous[i*6+1] == cur[1] &&
				previous[i*6+2] == cur[2])
			{
				break;
			}
		}

		if (i < num_prev)
		{
			// move to front (Most Recently Used)
#if 1
			if (i != 0)
			{
				u8_t tmp[6];

				memcpy(tmp, previous, 6);
				memcpy(previous, previous + i*6, 6);
				memcpy(previous + i*6, tmp, 6);
			}
#endif
			cur[0] = previous[3];
			cur[1] = previous[4];
			cur[2] = previous[5];

			continue;
		}

		if (num_prev < max_prev)
		{
			memmove(previous+6, previous, num_prev*6);
			num_prev++;
		}

		// most recent lookup is at the head
		previous[0] = cur[0];
		previous[1] = cur[1];
		previous[2] = cur[2];

		int best = 0;
		int best_dist = (1 << 30);

		int R = int(cur[0]);
		int G = int(cur[1]);
		int B = int(cur[2]);

		for (int p = 0; p < 256; p++)
		{
			int dR = int(old_pal[p*3+0]) - R;
			int dG = int(old_pal[p*3+1]) - G;
			int dB = int(old_pal[p*3+2]) - B;

			int dist = dR * dR + dG * dG + dB * dB;

			if (dist < best_dist)
			{
				best_dist = dist;
				best = p;
			}
		}

		// if this colour is not affected by the colourmap, then
		// keep the original colour (which has more precision).
		if (old_pal[best*3+0] != new_pal[best*3+0] ||
			old_pal[best*3+1] != new_pal[best*3+1] ||
			old_pal[best*3+2] != new_pal[best*3+2])
		{
			cur[0] = new_pal[best*3+0];
			cur[1] = new_pal[best*3+1];
			cur[2] = new_pal[best*3+2];
		}

		previous[3] = cur[0];
		previous[4] = cur[1];
		previous[5] = cur[2];
	}
}

int R_DetermineOpacity(epi::image_data_c *img)
{
	if (img->bpp == 3)
		return OPAC_Solid;

	if (img->bpp == 1)
	{
		for (int y=0; y < img->used_h; y++)
		for (int x=0; x < img->used_w; x++)
		{
			u8_t pix = img->PixelAt(x, y)[0];

			if (pix == TRANS_PIXEL)
				return OPAC_Masked;
		}

		return OPAC_Solid;
	}
	else
	{
		SYS_ASSERT(img->bpp == 4);

		bool is_masked = false;

		for (int y=0; y < img->used_h; y++)
		for (int x=0; x < img->used_w; x++)
		{
			u8_t alpha = img->PixelAt(x, y)[3];

			if (alpha == 0)
				is_masked = true;
			else if (alpha != 255)
				return OPAC_Complex;
		}

		return is_masked ? OPAC_Masked : OPAC_Solid;
	}
}

void R_BlackenClearAreas(epi::image_data_c *img)
{
	// makes sure that any totally transparent pixel (alpha == 0)
	// has a colour of black.

	byte *dest = img->pixels;

	int count = img->width * img->height;

	if (img->bpp == 1)
	{
		for (; count > 0; count--, dest++)
		{
			if (*dest == TRANS_PIXEL)
				*dest = pal_black;
		}
	}
	else if (img->bpp == 4)
	{
		for (; count > 0; count--, dest += 4)
		{
			if (dest[3] == 0)
			{
				dest[0] = dest[1] = dest[2] = 0;
			}
		}
	}
}

void R_DumpImage(epi::image_data_c *img)
{
	L_WriteDebug("DUMP IMAGE: size=%dx%d [%dx%d] bpp=%d\n",
			img->used_w, img->used_h,
			img->width, img->height, img->bpp);

	for (int y=img->height-1; y >= 0; y--)
	{
		for (int x=0; x < img->width; x++)
		{
			u8_t pixel = img->PixelAt(x,y)[0];

			// L_WriteDebug("%02x", pixel);
			L_WriteDebug("%c", 'A' + (pixel % 26));
		}

		L_WriteDebug("\n");
	}
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
