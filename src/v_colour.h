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

#ifndef __V_COLOUR_H__
#define __V_COLOUR_H__

#include "dm_type.h"
#include "dm_defs.h"

#include "ddf_main.h"
#include "r_defs.h"

void V_InitColour(void);

// -ACB- 1999/10/11 Gets an RGB colour from the current palette
// -AJA- Added `nominal' version, which gets colour from palette 0.
void V_IndexColourToRGB(int indexcol, byte *returncol);
void V_IndexNominalToRGB(int indexcol, byte *returncol);

// -ES- 1998/11/29 Added translucency tables
// -AJA- 1999/06/30: Moved 'em here, from dm_state.h.
extern byte rgb_32k[32][32][32];  // 32K RGB table, for 8-bit translucency
extern unsigned long col2rgb16[65][256][2];
extern unsigned long col2rgb8[65][256];
extern unsigned long hicolourtransmask;  // OR mask used for translucency

// -AJA- 1999/07/03: moved here from v_res.h.
extern byte gammatable[5][256];
extern int usegamma;
extern int current_gamma;

#define GAMMA_CONV(light)  (gammatable[usegamma][light])

// -AJA- 1999/07/03: Some palette stuff. Should be replaced later on with
// some DDF system (e.g. "palette.ddf").

extern byte playpal_data[14][256][3];

#define PALETTE_NORMAL   0
#define PALETTE_PAIN     1
#define PALETTE_BONUS    2
#define PALETTE_SUIT     3

void V_SetPalette(int type, float_t amount);
void V_ColourNewFrame(void);

// -AJA- 1999/07/05: Added this, to be used instead of the
// Allegro-specific `palette_color' array.

extern long pixel_values[256];

// -AJA- 1999/07/10: Some stuff for colmap.ddf.

typedef enum
{
  VCOL_Sky = 0x0001
}
vcol_flags_e;

const coltable_t *V_GetRawColtable(const colourmap_t * nominal, int level);
const coltable_t *V_GetColtable
 (const colourmap_t * nominal, int lightlevel, vcol_flags_e flags);

// translation support
const byte *V_GetTranslationTable(const colourmap_t * colmap);

const coltable_t *V_GetTranslatedColtable(const coltable_t *src,
    const byte *trans);

// support for GL
#ifdef USE_GL
void V_GetColmapRGB(const colourmap_t *colmap,
    float_t *r, float_t *g, float_t *b, boolean_t font);
#endif

// general purpose colormaps & coltables
extern const colourmap_t *normal_map;
extern const colourmap_t *sky_map;
extern const colourmap_t *shadow_map;
extern const coltable_t *fuzz_coltable;
extern const coltable_t *dim_coltable;

// text translation tables
extern const byte *font_whitener;

extern const colourmap_t *text_red_map;
extern const colourmap_t *text_white_map;
extern const colourmap_t *text_grey_map;
extern const colourmap_t *text_green_map;
extern const colourmap_t *text_brown_map;
extern const colourmap_t *text_blue_map;
extern const colourmap_t *text_yellow_map;

// automap translation tables
extern const byte *am_normal_colmap;
extern const byte *am_overlay_colmap;

// do-nothing translation table
extern byte null_tranmap[256];

// halo translation table
extern byte halo_conv_table[256];

// colour values.  These assume the standard Doom palette.  Maybe
// remove most of these one day -- will take some work though...
// Note: some of the ranges begin with a bright (often white) colour.

#define BLACK   0
#define WHITE   4

#define PINK         0x10
#define PINK_LEN     32
#define BROWN        0x30
#define BROWN_LEN    32
#define GRAY         0x50
#define GRAY_LEN     32
#define GREEN        0x70
#define GREEN_LEN    16
#define BEIGE        0x80
#define BEIGE_LEN    16
#define RED          0xB0
#define RED_LEN      16
#define CYAN         0xC0
#define CYAN_LEN     8
#define BLUE         0xC8
#define BLUE_LEN     8
#define ORANGE       0xD8
#define ORANGE_LEN   8
#define YELLOW       0xE7
#define YELLOW_LEN   1
#define DBLUE        0xF0
#define DBLUE_LEN    8

#endif
