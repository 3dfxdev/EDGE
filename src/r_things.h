//----------------------------------------------------------------------------
//  EDGE Rendering things (objects as sprites) Code
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2000  The EDGE Team.
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

#ifndef __R_THINGS__
#define __R_THINGS__

#include "e_player.h"
#include "r_defs.h"
#include "z_zone.h"

// Constant arrays used for psprite clipping
//  and initializing clipping.
extern int negonearray;
#define screenheightarray 0

// vars for R_DrawMaskedColumn
extern int mfloorclip;
extern int mceilingclip;
extern float_t spryscale;
extern float_t sprtopscreen;
extern float_t mheightmaximum;  // -AJA- 1999/08/16.

extern float_t pspritescale;
extern float_t pspriteiscale;
extern float_t pspritescale2;
extern float_t pspriteiscale2;
extern float_t masked_translucency;

extern int extra_psp_light;

void R_DrawMaskedColumn(const column_t * column);
void R_SortVisSprites(void);

void R_AddSprite(mobj_t *thing);
void R_AddPSprites(void);
void R_DrawSprites(void);
void R_InitSprites(char **namelist);
void R_ClearSprites(void);
void R_DrawMasked(void);

// -ES- 1999/05/31 Changed player sprite system.
void R_DrawPlayerSprites(player_t * p);

void R_ClipVisSprite(vissprite_t * vis, int xl, int xh);

#endif
