//----------------------------------------------------------------------------
//  EDGE Colour Code
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
//
//  Based on the DOOM source code, released by Id Software under the
//  following copyright:
//
//    Copyright (C) 1993-1996 by id Software, Inc.
//
//----------------------------------------------------------------------------

#include "i_defs.h"
#include "v_colour.h"

#include "dm_defs.h"
#include "dm_state.h"
#include "m_argv.h"
#include "r_main.h"
#include "rgl_defs.h"
#include "st_stuff.h"
#include "v_res.h"
#include "w_image.h"
#include "w_wad.h"
#include "z_zone.h"

// -AJA- 1999/06/30: added this
byte playpal_data[14][256][3];
static byte playpal_greys[256];

// -AJA- 1999/09/18: fixes problem with black text etc.
static bool loaded_playpal = false;

// -AJA- 1999/06/30: moved here from system-specific code.
byte rgb_32k[32][32][32];
unsigned long col2rgb16[65][256][2];
unsigned long col2rgb8[65][256];
unsigned long hicolourtransmask;

// -AJA- 1999/07/03: moved these here from st_stuff.c:
// Palette indices.
// For damage/bonus red-/gold-shifts
#define PAIN_PALS         1
#define BONUS_PALS        9
#define NUM_PAIN_PALS     8
#define NUM_BONUS_PALS    4
// Radiation suit, green shift.
#define RADIATION_PAL     13

// -AJA- 1999/07/03: moved these here from v_res.c:
int usegamma;
int current_gamma;

bool interpolate_colmaps = true;

// general purpose colormaps
const colourmap_t *normal_map = NULL;
const colourmap_t *sky_map  = NULL;
const colourmap_t *shadow_map  = NULL;
const coltable_t *fuzz_coltable = NULL;
const coltable_t *dim_coltable = NULL;

// text translation tables
const byte *font_whitener = NULL;

const colourmap_t *text_red_map    = NULL;
const colourmap_t *text_white_map  = NULL;
const colourmap_t *text_grey_map   = NULL;
const colourmap_t *text_green_map  = NULL;
const colourmap_t *text_brown_map  = NULL;
const colourmap_t *text_blue_map   = NULL;
const colourmap_t *text_yellow_map = NULL;

// automap translation tables
const byte *am_normal_colmap  = NULL;
const byte *am_overlay_colmap = NULL;

byte null_tranmap[256];
byte halo_conv_table[256];

// -AJA- 1999/07/05: added `pixel_values' array.
long pixel_values[256];

// -AJA- whenever colourmaps have to be recomputed, we increase this.
static int colourmap_validcount = 7;

// colour indices from palette
int pal_black, pal_white, pal_gray239;
int pal_red, pal_green, pal_blue;
int pal_yellow, pal_green1, pal_brown1;

static int V_FindPureColour(int which);

//
// V_InitPalette
//
bool V_InitPalette(void)
{
	int t, i, r, g, b, max_file, pal_lump;
	wadtex_resource_t WT;

	const byte *pal = (const byte*)W_CacheLumpName("PLAYPAL");

	max_file = W_GetNumFiles();
	pal_lump = -1;

	// find "GLOBAL" palette (the one in the IWAD)
	for (t = 0; t < max_file; t++)
	{
		W_GetTextureLumps(t, &WT);

		if (WT.palette >= 0)
		{
			pal_lump = WT.palette;
			break;
		}
	}

	if (pal_lump == -1)
		I_Error("Missing PLAYPAL palette lump !\n");

	pal = (const byte*)W_CacheLumpNum(pal_lump);

	// read in palette colours
	for (t = 0; t < 14; t++)
	{
		for (i = 0; i < 256; i++)
		{
			playpal_data[t][i][0] = pal[(t * 256 + i) * 3 + 0];
			playpal_data[t][i][1] = pal[(t * 256 + i) * 3 + 1];
			playpal_data[t][i][2] = pal[(t * 256 + i) * 3 + 2];
		}
	}

	for (i = 0; i < 256; i++)
	{
		r = playpal_data[0][i][0];
		g = playpal_data[0][i][1];
		b = playpal_data[0][i][2];

		// NB: this test is rather lax
		playpal_greys[i] = (r == g) || (g == b);
	}

	W_DoneWithLump(pal);
	loaded_playpal = true;

	// lookup useful colours
	pal_black = V_FindColour(0, 0, 0);
	pal_white = V_FindColour(255, 255, 255);
	pal_gray239 = V_FindColour(239, 239, 239);

	pal_red   = V_FindPureColour(0);
	pal_green = V_FindPureColour(1);
	pal_blue  = V_FindPureColour(2);

	pal_yellow = V_FindColour(255, 255, 0);
	pal_green1 = V_FindColour(64, 128, 48);
	pal_brown1 = V_FindColour(192, 128, 74);

	I_Printf("Loaded global palette.\n");

	return true;
}

//
// V_InitTranslationTables
//
// Reads the translation tables for various things, especially text
// colours.  The text colourmaps to use are currently hardcoded, when
// HUD.DDF (or MENU.DDF) comes along then the colours will be user
// configurable.
//
// -ACB- 1998/09/10 Replaced the old procedure with this.
// -AJA- 2000/03/04: Moved here from r_draw1/2.c.
// -AJA- 2000/03/05: Uses colmap.ddf instead of PALREMAP lump.
//
static void InitTranslationTables(void)
{
	int i;

	if (normal_map)
		return;

	// look up the general colmaps & coltables
	normal_map = DDF_ColmapLookup("NORMAL");
	sky_map  = DDF_ColmapLookup("SKY");
	shadow_map = DDF_ColmapLookup("SHADOW");

	fuzz_coltable = V_GetRawColtable(
		DDF_ColmapLookup("FUZZY"), 127);

	dim_coltable = V_GetRawColtable(
		DDF_ColmapLookup("DIMSCREEN"), 127);

	font_whitener = V_GetTranslationTable(
		DDF_ColmapLookup("FONTWHITEN"));

	am_normal_colmap = V_GetTranslationTable(
		DDF_ColmapLookup("AUTOMAP_NORMAL"));

	am_overlay_colmap = V_GetTranslationTable(
		DDF_ColmapLookup("AUTOMAP_OVERLAY"));

	// look up the text maps
	text_red_map = DDF_ColmapLookup("TEXT_RED");
	text_white_map  = DDF_ColmapLookup("TEXT_WHITE");
	text_grey_map   = DDF_ColmapLookup("TEXT_GREY");
	text_green_map  = DDF_ColmapLookup("TEXT_GREEN");
	text_brown_map  = DDF_ColmapLookup("TEXT_BROWN");
	text_blue_map   = DDF_ColmapLookup("TEXT_BLUE");
	text_yellow_map = DDF_ColmapLookup("TEXT_YELLOW");

	for (i=0; i < 256; i++)
		null_tranmap[i] = i;

	// compute halo table (software mode)
	for (i=0; i < 256; i++)
	{
		int j = font_whitener[i];

		if (GRAY <= j && j < GRAY+GRAY_LEN)
			halo_conv_table[i] = 128 - ((j & 0x1F) << 2);
		else
			halo_conv_table[i] = 0;
	}
}


/* progress indicator for the translucency table calculations */
static void ColourCallbackFunc(int pos)
{
	static int cur_pos = 0;

	if (pos <= cur_pos)
		return;

	while (pos >= cur_pos + 16)
	{
		I_Printf(".");
		cur_pos += 16;
	}
}

/* CreateRGBTable:

* -ES- 1998/10/29: Added this. 
* -AJA- 1999/06/30: Ditto :).
* 
*  This is a modified version of Allegro's create_rgb_table, by Jan
*  Hubicka. The only difference is that this version will insert colour
*  0 into the table, and Allegro's version won't (since that's Allegro's
*  transparent colour).
*
*  Fills an RGB_MAP lookup table with conversion data for the specified
*  palette. This is the faster version by Jan Hubicka.
*
*  Uses alg. similiar to foodfill - it adds one seed per every colour in 
*  palette to its best possition. Then areas around seed are filled by 
*  same colour because it is best aproximation for them, and then areas 
*  about them etc...
*
*  It does just about 80000 tests for distances and this is about 100
*  times better than normal 256*32000 tests so the caluclation time
*  is now less than one second at all computers I tested.
*/

/* 1.5k lookup table for colour matching */
static unsigned col_diff[3 * 128];

/* bestfit_init:
*  Colour matching is done with weighted squares, which are much faster
*  if we pregenerate a little lookup table...
*/
static void BestfitInit(void)
{
	int i;

	for (i = 1; i < 64; i++)
	{
		int k = i * i;

		col_diff[0 + i] = col_diff[0 + 128 - i] = k * (59 * 59);
		col_diff[128 + i] = col_diff[128 + 128 - i] = k * (30 * 30);
		col_diff[256 + i] = col_diff[256 + 128 - i] = k * (11 * 11);
	}
}

static void CreateRGBTable(byte table[32][32][32],
						   byte pal[256][3],
						   void (*callback) (int pos))
{
#define UNUSED 65535
#define LAST 65532

	// macro ADD adds to single linked list
#define ADD(i)    (next[(i)] == UNUSED ? (next[(i)] = LAST, \
	(first != LAST ? (next[last] = (i)) : (first = (i))), \
	(last = (i))) : 0)

	// same but w/o checking for first element
#define ADD1(i)   (next[(i)] == UNUSED ? (next[(i)] = LAST, \
	next[last] = (i), \
	(last = (i))) : 0)

	// calculates distance between two colours
#define DIST(a1, a2, a3, b1, b2, b3) \
	(col_diff[ ((a2) - (b2)) & 0x7F] + \
	(col_diff + 128)[((a1) - (b1)) & 0x7F] + \
	(col_diff + 256)[((a3) - (b3)) & 0x7F])

	// converts r,g,b to position in array and back
#define POS(r, g, b) \
	(((r) / 2) * 32 * 32 + ((g) / 2) * 32 + ((b) / 2))

#define DEPOS(pal0, r, g, b) \
	((b) = ((pal0)        & 31) * 2, \
	(g) = (((pal0) >> 5)  & 31) * 2, \
	(r) = (((pal0) >> 10) & 31) * 2)

	// is current colour better than pal1?
#define BETTER(r1, g1, b1, pal1) \
	(((int)DIST((r1), (g1), (b1), \
	(pal1)[0], (pal1)[1], (pal1)[2])) > (int)dist2)

	// checking of position
#define DOPOS(rp, gp, bp, ts) \
	if ((rp > -1 || r > 0) && (rp < 1 || r < 61) && \
	(gp > -1 || g > 0) && (gp < 1 || g < 61) && \
	(bp > -1 || b > 0) && (bp < 1 || b < 61)) \
	{ \
	i = first + rp * 32 * 32 + gp * 32 + bp; \
	if (ts ? data[i] != val : !data[i]) \
	{ \
	dist2 = (rp ? (col_diff+128)[(r+2*rp-pal[val][0]) & 0x7F] : r2) + \
	(gp ? (col_diff)[(g+2*gp-pal[val][1]) & 0x7F] : g2) + \
	(bp ? (col_diff+256)[(b+2*bp-pal[val][2]) & 0x7F] : b2); \
	if (BETTER((r+2*rp), (g+2*gp), (b+2*bp), pal[data[i]])) \
	{ \
	data[i] = val; \
	ADD1 (i); \
	} \
	} \
	}

	int i, curr, r, g, b, val, r2, g2, b2, dist2;
	unsigned short next[32 * 32 * 32];
	unsigned char *data;
	int first = LAST;
	int last = LAST;
	int count = 0;
	int cbcount = 0;

#define AVERAGE_COUNT   18000

	BestfitInit();
	memset(next, 255, sizeof(next));
	memset(table, 0, sizeof(byte) * 32 * 32 * 32);

	data = (unsigned char *)table;

	// add starting seeds for foodfill
	for (i = 1; i < 256; i++)
	{
		curr = POS(pal[i][0], pal[i][1], pal[i][2]);
		if (next[curr] == UNUSED)
		{
			data[curr] = i;
			ADD(curr);
		}
	}

	// main foodfill: two versions of loop for faster growing in blue axis
	while (first != LAST)
	{
		DEPOS(first, r, g, b);

		// calculate distance of current colour
		val = data[first];
		r2 = (col_diff + 128)[((pal[val][0]) - (r)) & 0x7F];
		g2 = (col_diff)[((pal[val][1]) - (g)) & 0x7F];
		b2 = (col_diff + 256)[((pal[val][2]) - (b)) & 0x7F];

		// try to grow to all directions
		DOPOS(0, 0, 1, 1);
		DOPOS(0, 0, -1, 1);
		DOPOS(1, 0, 0, 1);
		DOPOS(-1, 0, 0, 1);
		DOPOS(0, 1, 0, 1);
		DOPOS(0, -1, 0, 1);

		// faster growing of blue direction
		if ((b > 0) && (data[first - 1] == val))
		{
			b -= 2;
			first--;
			b2 = (col_diff + 256)[((pal[val][2]) - (b)) & 0x7F];

			DOPOS(-1, 0, 0, 0);
			DOPOS(1, 0, 0, 0);
			DOPOS(0, -1, 0, 0);
			DOPOS(0, 1, 0, 0);

			first++;
		}

		// get next from list
		i = first;
		first = next[first];
		next[i] = UNUSED;

		// second version of loop
		if (first != LAST)
		{
			DEPOS(first, r, g, b);

			val = data[first];
			r2 = (col_diff + 128)[((pal[val][0]) - (r)) & 0x7F];
			g2 = (col_diff)[((pal[val][1]) - (g)) & 0x7F];
			b2 = (col_diff + 256)[((pal[val][2]) - (b)) & 0x7F];

			DOPOS(0, 0, 1, 1);
			DOPOS(0, 0, -1, 1);
			DOPOS(1, 0, 0, 1);
			DOPOS(-1, 0, 0, 1);
			DOPOS(0, 1, 0, 1);
			DOPOS(0, -1, 0, 1);

			if ((b < 61) && (data[first + 1] == val))
			{
				b += 2;
				first++;
				b2 = (col_diff + 256)[((pal[val][2]) - (b)) & 0x7f];

				DOPOS(-1, 0, 0, 0);
				DOPOS(1, 0, 0, 0);
				DOPOS(0, -1, 0, 0);
				DOPOS(0, 1, 0, 0);

				first--;
			}

			i = first;
			first = next[first];
			next[i] = UNUSED;
		}

		count++;
		if (count == (cbcount + 1) * AVERAGE_COUNT / 256)
		{
			if (cbcount < 256)
			{
				if (callback)
					callback(cbcount);
				cbcount++;
			}
		}
	}

	if (callback)
		while (cbcount < 256)
			callback(cbcount++);

#undef AVERAGE_COUNT
#undef UNUSED
#undef LAST
#undef ADD
#undef ADD1
#undef DIST
#undef POS
#undef DEPOS
#undef DOPOS
#undef BETTER
}

typedef struct
{
	int bits, mask, shift;
	int frac_bits, up_shift;

}
rgb16info_t;

static rgb16info_t rgb16info[3];

#define RI rgb16info

static void SwapEm(int a, int b)
{
	rgb16info_t tmp;
	tmp = RI[a];
	RI[a] = RI[b];
	RI[b] = tmp;
}

static void V_ComputeRGBInfo(truecol_info_t * ti)
{
	if (BPP != 2)
		return;

	RI[0].bits = ti->red_bits;
	RI[1].bits = ti->green_bits;
	RI[2].bits = ti->blue_bits;

	RI[0].mask = ti->red_mask;
	RI[1].mask = ti->green_mask;
	RI[2].mask = ti->blue_mask;

	RI[0].shift = ti->red_shift;
	RI[1].shift = ti->green_shift;
	RI[2].shift = ti->blue_shift;

	// sort bitfields from right to left:

	if (RI[0].shift > RI[1].shift)
		SwapEm(0, 1);
	if (RI[0].shift > RI[2].shift)
		SwapEm(0, 2);
	if (RI[1].shift > RI[2].shift)
		SwapEm(1, 2);

	if ((RI[0].shift != 0) || (RI[1].shift != RI[0].bits) ||
		(RI[2].shift != RI[0].bits + RI[1].bits))
	{
		I_Error("Non-contiguous 16 bit mode !");
	}

	RI[0].frac_bits = 16 - RI[1].bits - RI[0].bits;
	RI[1].frac_bits = RI[0].bits;
	RI[2].frac_bits = RI[1].bits;

	RI[0].up_shift = 16 - RI[0].frac_bits;
	RI[1].up_shift = 0;
	RI[2].up_shift = 16 + RI[0].bits;

#define MASK(n)  (((1L << RI[n].frac_bits) - 1L) << RI[n].up_shift)

	hicolourtransmask = MASK(0) | MASK(1) | MASK(2);

#undef MASK
}

static unsigned long V_CalcRGB(int hicol, int level)
{
	unsigned long A = (hicol & RI[0].mask) >> RI[0].shift;
	unsigned long B = (hicol & RI[1].mask) >> RI[1].shift;
	unsigned long C = (hicol & RI[2].mask) >> RI[2].shift;

	A = ((A * level) >> (6 - RI[0].frac_bits)) << RI[0].up_shift;
	B = ((B * level) >> (6 - RI[1].frac_bits)) << RI[1].up_shift;
	C = ((C * level) >> (6 - RI[2].frac_bits)) << RI[2].up_shift;

	return A | B | C;
}

#undef RI

static void CalcTranslucencyTable(void)
{
	int x, y, i;
	truecol_info_t ti;

	byte palette[256][3];  // Note: 6 bits per colour.

	I_Printf("Calculating translucency table");

	for (i = 0; i < 256; i++)
	{
		palette[i][0] = playpal_data[0][i][0] >> 2;
		palette[i][1] = playpal_data[0][i][1] >> 2;
		palette[i][2] = playpal_data[0][i][2] >> 2;
	}

	CreateRGBTable(rgb_32k, palette, ColourCallbackFunc);

	// Convert to R/B/G
	for (x = 0; x < 32; x++)
		for (y = 0; y < 32; y++)
			for (i = 0; i < y; i++)
			{
				byte temp;

				temp = rgb_32k[x][i][y];
				rgb_32k[x][i][y] = rgb_32k[x][y][i];
				rgb_32k[x][y][i] = temp;
			}

			// fixup some common colours
			rgb_32k[0][0][0] = pal_black;
			rgb_32k[31][31][31] = pal_white;

			I_GetTruecolInfo(&ti);
			V_ComputeRGBInfo(&ti);

			for (x = 0; x < 65; x++)
			{
				for (y = 0; y < 256; y++)
				{
					unsigned long rgb_lo;
					unsigned long rgb_hi;

					col2rgb8[x][y] = ((playpal_data[0][y][0] * x >> 4) << 22) |
						((playpal_data[0][y][1] * x >> 2) << 10) |
						(playpal_data[0][y][2] * x >> 4);

					rgb_lo = V_CalcRGB(y, x);
					rgb_hi = V_CalcRGB(y << 8, x);

					col2rgb16[x][y][0] = rgb_lo;
					col2rgb16[x][y][1] = rgb_hi;
				}
			}

			I_Printf("\n");
}

static int cur_palette = -1;
static int cur_pal_bpp = -1;
static int new_palette = 0;

//
// V_InitColour
//
void V_InitColour(void)
{
	// check options
	M_CheckBooleanParm("nointerp", &interpolate_colmaps, true);

	InitTranslationTables();
	CalcTranslucencyTable();

	// -AJA- 1999/11/07: fix for the "palette corrupt on res change"
	//       bug.  Allegro does not remember the palette when changing
	//       resolution, hence the problem.
	//
	cur_palette = -1;
	cur_pal_bpp = -1;
}

//
// V_FindColour
// 
// Find the closest matching colour in the palette.
// 
int V_FindColour(int r, int g, int b)
{
	int i;

	int best = 0;
	int best_dist = 1 << 30;

	for (i=0; i < 256; i++)
	{
		int d_r = ABS(r - playpal_data[0][i][0]);
		int d_g = ABS(g - playpal_data[0][i][1]);
		int d_b = ABS(b - playpal_data[0][i][2]);

		int dist = d_r * d_r + d_g * d_g + d_b * d_b;

		if (dist == 0)
			return i;

		if (dist < best_dist)
		{
			best = i;
			best_dist = dist;
		}
	}

	return best;
}

//
// V_FindPureColour
// 
// Find the best match for the pure colour.  `which' is 0 for red, 1
// for green and 2 for blue.
// 
static int V_FindPureColour(int which)
{
	int i;

	int best = 0;
	int best_dist = 1 << 30;

	for (i=0; i < 256; i++)
	{
		int a = playpal_data[0][i][which];
		int b = playpal_data[0][i][(which+1)%3];
		int c = playpal_data[0][i][(which+2)%3];
		int d = MAX(b, c);

		int dist = 255 - (a - d);

		// the pure colour must shine through
		if (a <= d)
			continue;

		if (dist < best_dist)
		{
			best = i;
			best_dist = dist;
		}
	}

	return best;
}

//
// V_SetPalette
//
void V_SetPalette(int type, float amount)
{
	int i;
	int palette = 0;
	truecol_info_t ti;

	// -AJA- 1999/09/17: fixes problems with black text etc.
	if (!loaded_playpal)
		return;

	if (amount >= 1.0)
		amount = 1.0f;

	switch (type)
	{
	case PALETTE_PAIN:
		palette = (int)(PAIN_PALS + amount * (NUM_PAIN_PALS-1));
		break;

	case PALETTE_BONUS:
		palette = (int)(BONUS_PALS + amount * (NUM_BONUS_PALS-1));
		break;

	case PALETTE_SUIT:
		palette = RADIATION_PAL;
		break;
	}

	if (palette == cur_palette && cur_pal_bpp == BPP)
		return;

	// mark the palette change
	new_palette++;

	cur_palette = palette;
	cur_pal_bpp = BPP;

	I_GetTruecolInfo(&ti);

	for (i = 0; i < 256; i++)
	{
		pixel_values[i] = I_Colour2Pixel(playpal_data[cur_palette], i);

		if (BPP != 1 && playpal_greys[i])
			pixel_values[i] &= ti.grey_mask;
	}

	stbar_update = true;
}

static INLINE long MakeRGB(truecol_info_t *ti, long r, long g, long b)
{
	r = (r >> (8 - ti->red_bits))   << ti->red_shift;
	g = (g >> (8 - ti->green_bits)) << ti->green_shift;
	b = (b >> (8 - ti->blue_bits))  << ti->blue_shift;

	return r | g | b;
}

//
// MakeColourmapRange
//
// Creates a colourmap table in `dest_colmaps' for use by the column &
// span drawers.  Each colourmap is an array of `num' coltables.  Each
// coltable in the destination is a lookup table from indexed colour
// (0-255) to framebuffer pixel.  Each coltable in the source is a
// lookup table from indexed colour to indexed colour.  Only used for
// non-8-bit modes.  Note that the current gamma level must also be
// applied in the conversion.
//
static void MakeColourmapRange(void *dest_colmaps, byte palette[256][3],
							   const byte *src_colmaps, int num)
{
	const byte *gtable = gammatable[usegamma];
	unsigned short *colmap16 = (unsigned short*)dest_colmaps;
	byte tempr[256], tempg[256], tempb[256];
	truecol_info_t ti;
	int i, j;

	DEV_ASSERT2(BPP == 2);

	I_GetTruecolInfo(&ti);

	if (interpolate_colmaps && num >= 2)
	{
		for (j = 0; j < 256; j++)
		{
			int c1 = src_colmaps[0 * 256 + j];
			int c2 = src_colmaps[(num - 1) * 256 + j];

			int r1 = palette[c1][0];
			int g1 = palette[c1][1];
			int b1 = palette[c1][2];

			int r2 = palette[c2][0];
			int g2 = palette[c2][1];
			int b2 = palette[c2][2];

			for (i = 0; i < num; i++)
			{
				int r = gtable[r1 + (r2 - r1) * i / (num - 1)];
				int g = gtable[g1 + (g2 - g1) * i / (num - 1)];
				int b = gtable[b1 + (b2 - b1) * i / (num - 1)];

				colmap16[i * 256 + j] = MakeRGB(&ti, r, g, b);

				if (playpal_greys[i])
					colmap16[i * 256 + j] &= ti.grey_mask;
			}
		}

		return;
	}

	// use the old (but faster) method:

	for (i = 0; i < 256; i++)
	{
		tempr[i] = gtable[palette[i][0]];
		tempg[i] = gtable[palette[i][1]];
		tempb[i] = gtable[palette[i][2]];
	}

	for (j = 0; j < 256; j++)
		for (i = 0; i < num; i++)
		{
			int c = src_colmaps[i * 256 + j];

			colmap16[i * 256 + j] = MakeRGB(&ti, tempr[c], tempg[c], tempb[c]);

			if (playpal_greys[j])
				colmap16[i * 256 + j] &= ti.grey_mask;
		}
}

//
// LoadColourmap
//
// Computes the right "colourmap" (more precisely, coltable) to put into
// the dc_colourmap & ds_colourmap variables for use by the column &
// span drawers.
//
static void LoadColourmap(const colourmap_t * colm, int bpp)
{
	int lump;
	int size;
	const byte *data;
	const byte *data_in;

	// we are writing to const marked memory here. Here is the only place
	// the cache struct is touched.
	colmapcache_t *cache = (colmapcache_t *)&colm->cache; // Intentional Const Override

	cache->validcount = colourmap_validcount;
	cache->bpp = bpp;

	lump = W_GetNumForName(colm->lump_name);
	size = W_LumpLength(lump);
	data = (const byte*)W_CacheLumpNum(lump);

	if ((colm->start + colm->length) * 256 > size)
		I_Error("Colourmap [%s] is too small ! (LENGTH too big)\n", 
		colm->ddf.name);

	data_in = data + (colm->start * 256);

	switch (bpp)
	{
		case 1:
			Z_Resize(cache->baseptr, char, colm->length * 256 + 255);
			cache->data = (void *)(((unsigned long)cache->baseptr + 255) & ~255);
			Z_MoveData((byte *)cache->data, data_in, byte, colm->length * 256);
			break;

		case 2:
			Z_Resize(cache->baseptr, char, colm->length * 512 + 511);
			cache->data = (void *)(((unsigned long)cache->baseptr + 511) & ~511);

			MakeColourmapRange(cache->data, playpal_data[cur_palette],
				data_in, colm->length);
			break;
	}

	W_DoneWithLump(data);
}

//
// V_GetRawColtable
//
const coltable_t *V_GetRawColtable(const colourmap_t * nominal, int level)
{
	coltable_t *result;
	int index;

	// Do we need to load or recompute this colourmap ?

	if (nominal->cache.data == NULL || 
		nominal->cache.validcount != colourmap_validcount ||
		nominal->cache.bpp != BPP)
	{
		LoadColourmap(nominal, BPP);
	}

	result = (coltable_t*)nominal->cache.data;
	index = (nominal->length * level) >> 8;

	return result + index * 256 * BPP;
}

//
// V_GetTranslationTable
//
const byte *V_GetTranslationTable(const colourmap_t * colmap)
{
	// Do we need to load or recompute this colourmap ?

	if (colmap->cache.data == NULL || 
		colmap->cache.validcount != colourmap_validcount ||
		colmap->cache.bpp != 1)
	{
		LoadColourmap(colmap, 1);
	}

	return (const byte*)colmap->cache.data;
}

//
// V_GetColtable
//
// Finds the right coltable for the dc_colourmap & ds_colourmap
// variables used by the column & span drawers.
//
const coltable_t *V_GetColtable(const colourmap_t * nominal, 
								int lightlevel, vcol_flags_e flags)
{
	DEV_ASSERT2(nominal);

	if (effect_colourmap)
	{
		if (effect_colourmap->length > 1)
		{
			nominal = effect_colourmap;
			lightlevel = (int)(255 * effect_strength);
		}
		else if (effect_strength >= 1.0 || ((int)(effect_strength * 16) & 2))
		{
			nominal = effect_colourmap;
			// doesn't matter
			lightlevel = 127;
		}
	}
	else if (flags & VCOL_Sky)
	{
		nominal = sky_map;
		lightlevel = 255;
	}
	else
	{ 
		if (effect_infrared)
			lightlevel += (int)(effect_strength * 255);

		if (! (nominal->special & COLSP_NoFlash))
			lightlevel += extralight * 16;
	}

	lightlevel = MAX(0, MIN(255, lightlevel));

	return V_GetRawColtable(nominal, 255 - lightlevel);
}

#ifdef USE_GL
//
// V_GetColmapRGB
//
void V_GetColmapRGB(const colourmap_t *colmap,
					float *r, float *g, float *b, bool font)
{
	if (colmap->cache.data == NULL)
	{
		// Intention Const Override
		colmapcache_t *cache = (colmapcache_t *) &colmap->cache;
		coltable_t *table;

		int r, g, b;

		LoadColourmap(colmap, 1);

		table = (coltable_t *) cache->data;

		if (font)
		{
			// for fonts, we only care about the GRAY colour
			r = playpal_data[0][table[pal_gray239]][0];
			g = playpal_data[0][table[pal_gray239]][1];
			b = playpal_data[0][table[pal_gray239]][2];
		}
		else
		{
			// use the RED,GREEN,BLUE colours, see how they change
			r = playpal_data[0][table[pal_red]][0];
			g = playpal_data[0][table[pal_green]][1];
			b = playpal_data[0][table[pal_blue]][2];
		}

		cache->gl_colour = (r << 16) | (g << 8) | b;
	}

	(*r) = GAMMA_CONV((colmap->cache.gl_colour >> 16) & 0xFF) / 255.0f;
	(*g) = GAMMA_CONV((colmap->cache.gl_colour >>  8) & 0xFF) / 255.0f;
	(*b) = GAMMA_CONV((colmap->cache.gl_colour      ) & 0xFF) / 255.0f;
}
#endif

//
// V_GetTranslatedColtable
//
// Used for remapped sprites.  Will convert the input coltable into
// the output coltable (which is static) through the given translation
// array.
//
static byte translated_coltable[1024];

const coltable_t *V_GetTranslatedColtable(const coltable_t *src,
										  const byte *trans)
{
	int i;

	switch (BPP)
	{
	case 1:
		for (i=0; i < 256; i++)
		{
			translated_coltable[i] = ((byte *)src)[trans[i]];
		}
		break;

	case 2:
		for (i=0; i < 256; i++)
		{
			((unsigned short *)translated_coltable)[i] =
				((unsigned short *)src)[trans[i]];
		}
		break;
	}

	return (const coltable_t *)translated_coltable;
}


//
// V_ColourNewFrame
//
// Call this at the start of each frame (before any rendering or
// render-related work has been done).  Will update the palette and/or
// gamma settings if they have changed since the last call.
//
void V_ColourNewFrame(void)
{
	if (current_gamma != usegamma)
	{
		W_ResetImages();
	}

	// -AJA- 1999/08/03: This fixes, once and for all, the now infamous
	//       "gamma too late on walls" bug.
	//
	if (new_palette > 0 || current_gamma != usegamma)
	{
		new_palette = 0;
		usegamma = current_gamma;

		// -AJA- FIXME: GL doesn't need whole colourmaps recomputed

		if (BPP == 1)
			I_SetPalette(playpal_data[cur_palette]);
		else
			colourmap_validcount++;  // invalidate all colourmaps.
	}

	return;
}

//
// V_IndexColourToRGB
//
// Returns an RGB value from an index value - used the current
// palette.  The byte pointer is assumed to point a 3-byte array.

void V_IndexColourToRGB(int indexcol, byte *returncol)
{
	returncol[0] = playpal_data[cur_palette][indexcol][0];
	returncol[1] = playpal_data[cur_palette][indexcol][1];
	returncol[2] = playpal_data[cur_palette][indexcol][2];
}

void V_IndexNominalToRGB(int indexcol, byte *returncol)
{
	returncol[0] = playpal_data[0][indexcol][0];
	returncol[1] = playpal_data[0][indexcol][1];
	returncol[2] = playpal_data[0][indexcol][2];
}

