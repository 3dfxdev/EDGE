//----------------------------------------------------------------------------
//  EDGE Colour Code
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
#include "e_main.h"
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


// text translation tables
const byte *font_whitener = NULL;
const colourmap_c *font_whiten_map = NULL;

const colourmap_c *text_red_map    = NULL;
const colourmap_c *text_white_map  = NULL;
const colourmap_c *text_grey_map   = NULL;
const colourmap_c *text_green_map  = NULL;
const colourmap_c *text_brown_map  = NULL;
const colourmap_c *text_blue_map   = NULL;
const colourmap_c *text_yellow_map = NULL;

// automap translation tables
const byte *am_normal_colmap  = NULL;
const byte *am_overlay_colmap = NULL;


// colour indices from palette
int pal_black, pal_white, pal_gray239;
int pal_red, pal_green, pal_blue;
int pal_yellow, pal_green1, pal_brown1;

static int V_FindPureColour(int which);

//
// V_InitPalette
//
void V_InitPalette(void)
{
	int t, i, r, g, b, max_file, pal_lump;
	wadtex_resource_c WT;

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
	if (font_whitener)
		return;

	// look up the general colmaps & coltables

	font_whiten_map = colourmaps.Lookup("FONTWHITEN");
	font_whitener = V_GetTranslationTable(font_whiten_map);


	am_normal_colmap = V_GetTranslationTable(
		colourmaps.Lookup("AUTOMAP_NORMAL"));

	am_overlay_colmap = V_GetTranslationTable(
		colourmaps.Lookup("AUTOMAP_OVERLAY"));

	// look up the text maps
	text_red_map = colourmaps.Lookup("TEXT_RED");
	text_white_map  = colourmaps.Lookup("TEXT_WHITE");
	text_grey_map   = colourmaps.Lookup("TEXT_GREY");
	text_green_map  = colourmaps.Lookup("TEXT_GREEN");
	text_brown_map  = colourmaps.Lookup("TEXT_BROWN");
	text_blue_map   = colourmaps.Lookup("TEXT_BLUE");
	text_yellow_map = colourmaps.Lookup("TEXT_YELLOW");
}

static int cur_palette = -1;
static int new_palette = 0;

//
// V_InitColour
//
void V_InitColour(void)
{
	// check options
	M_CheckBooleanParm("nointerp", &interpolate_colmaps, true);

	InitTranslationTables();

	// -AJA- 1999/11/07: fix for the "palette corrupt on res change"
	//       bug.  Allegro does not remember the palette when changing
	//       resolution, hence the problem.
	//
	cur_palette = -1;
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
	int palette = 0;

	// -AJA- 1999/09/17: fixes problems with black text etc.
	if (!loaded_playpal)
		return;

	if (amount >= 0.95f)
		amount = 0.95f;

	switch (type)
	{
	case PALETTE_PAIN:
		palette = (int)(PAIN_PALS + amount * NUM_PAIN_PALS);
		break;

	case PALETTE_BONUS:
		palette = (int)(BONUS_PALS + amount * NUM_BONUS_PALS);
		break;

	case PALETTE_SUIT:
		palette = RADIATION_PAL;
		break;
	}

	if (palette == cur_palette)
		return;

	// mark the palette change
	new_palette++;

	cur_palette = palette;
}

//
// LoadColourmap
//
// Computes the right "colourmap" (more precisely, coltable) to put into
// the dc_colourmap & ds_colourmap variables for use by the column &
// span drawers.
//
static void LoadColourmap(const colourmap_c * colm)
{
	int lump;
	int size;
	const byte *data;
	const byte *data_in;

	// we are writing to const marked memory here. Here is the only place
	// the cache struct is touched.
	colmapcache_t *cache = (colmapcache_t *)&colm->cache; // Intentional Const Override

	lump = W_GetNumForName(colm->lump_name);
	size = W_LumpLength(lump);
	data = (const byte*)W_CacheLumpNum(lump);

	if ((colm->start + colm->length) * 256 > size)
		I_Error("Colourmap [%s] is too small ! (LENGTH too big)\n", 
		colm->ddf.name.GetString());

	data_in = data + (colm->start * 256);

	bool whiten = (colm->special & COLSP_Whiten) ? true : false;

	Z_Resize(cache->data, byte, colm->length * 256);

	if (whiten)
		for (int j = 0; j < colm->length * 256; j++)
			cache->data[j] = data_in[font_whitener[j]];
	else
		for (int j = 0; j < colm->length * 256; j++)
			cache->data[j] = data_in[j];

	W_DoneWithLump(data);
}


//
// V_GetTranslationTable
//
const byte *V_GetTranslationTable(const colourmap_c * colmap)
{
	// Do we need to load or recompute this colourmap ?

	if (colmap->cache.data == NULL)
		LoadColourmap(colmap);

	return (const byte*)colmap->cache.data;
}


//
// V_GetColmapRGB
//
void V_GetColmapRGB(const colourmap_c *colmap,
					float *r, float *g, float *b, bool font)
{
	if (colmap->cache.data == NULL)
	{
		// Intention Const Override
		colmapcache_t *cache = (colmapcache_t *) &colmap->cache;
		byte *table;

		int r, g, b;

		LoadColourmap(colmap);

		table = (byte *) cache->data;

		if (font)
		{
			// for fonts, we only care about the GRAY colour
			r = playpal_data[0][table[pal_gray239]][0];
			g = playpal_data[0][table[pal_gray239]][1];
			b = playpal_data[0][table[pal_gray239]][2];

			r = r * 255 / 239;
			g = g * 255 / 239;
			b = b * 255 / 239;
		}
		else
		{
			/* analyse whole colourmap */
			int r_diffs = 0;
			int g_diffs = 0;
			int b_diffs = 0;
			int total   = 0;

			for (int j = 0; j < 256; j++)
			{
				int r0 = playpal_data[0][j][0];
				int g0 = playpal_data[0][j][1];
				int b0 = playpal_data[0][j][2];

				int r1 = playpal_data[0][table[j]][0];
				int g1 = playpal_data[0][table[j]][1];
				int b1 = playpal_data[0][table[j]][2];

				int intensity = MAX(r0, MAX(g0, b0));

				if (intensity < 32)
				{
					continue;
				}

				// give the grey-scales more importance
				int weight = (r0 == g0 && g0 == b0) ? 3 : 1;

				int r_delta = (r1 - r0) * 255 / intensity;
				int g_delta = (g1 - g0) * 255 / intensity;
				int b_delta = (b1 - b0) * 255 / intensity;

				// limit the delta range, since dark colours can produce
				// very high values (> 1000) which skew the results.

				r_delta = MIN(200, MAX(-250, r_delta));
				g_delta = MIN(200, MAX(-250, g_delta));
				b_delta = MIN(200, MAX(-250, b_delta));

				r_diffs += r_delta * weight;
				g_diffs += g_delta * weight;
				b_diffs += b_delta * weight;
				total   += weight;
			}

			// ASSERT(total > 0)

			// exaggerate the results to give more oomph..
			r = 255 + r_diffs * 4 / 3 / total;
			g = 255 + g_diffs * 4 / 3 / total;
			b = 255 + b_diffs * 4 / 3 / total;

#if 0  // DEBUGGING
			I_Printf("COLMAP [%s] --> (%d, %d, %d)\n",
				colmap->ddf.name.GetString(), r, g, b);
#endif

#if 0  // OLD METHOD
			r = playpal_data[0][table[pal_red]][0];
			g = playpal_data[0][table[pal_green]][1];
			b = playpal_data[0][table[pal_blue]][2];
#endif
		}

		r = MIN(255, MAX(0, r));
		g = MIN(255, MAX(0, g));
		b = MIN(255, MAX(0, b));

		cache->gl_colour = (r << 16) | (g << 8) | b;
	}

	(*r) = GAMMA_CONV((colmap->cache.gl_colour >> 16) & 0xFF) / 255.0f;
	(*g) = GAMMA_CONV((colmap->cache.gl_colour >>  8) & 0xFF) / 255.0f;
	(*b) = GAMMA_CONV((colmap->cache.gl_colour      ) & 0xFF) / 255.0f;
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

