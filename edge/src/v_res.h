//----------------------------------------------------------------------------
//  EDGE Video Code
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
//
// Original Author: Chi Hoang
//

#ifndef __MULTIRES_H__
#define __MULTIRES_H__

#include "dm_type.h"
#include "dm_defs.h"
#include "r_data.h"
#include "v_screen.h"
#include "v_video1.h"

typedef enum
{
  VID_DEPTH_8 = 0x01,
  VID_DEPTH_16 = 0x02,
  VID_DEPTH_24 = 0x04
}
viddepth_t;

typedef struct screenmode_s
{
  int width;
  int height;
  byte depthbit;
}
screenmode_t;

boolean_t V_MultiResInit(void);
void V_InitResolution(void);
void V_AddAvailableResolution(int width, int height, int depth);

//
//start with v_video.h
//
#define CENTERY                 (SCREENHEIGHT/2)

extern screen_t *main_scr;
extern screen_t *back_scr;

// Allocates buffer screens, call before R_Init.

/* JUST IN CASE I DON'T TRUST THIS SYSTEM....
#define V_Init V_Init8
#define V_CopyRect V_CopyRect8
#define V_CopyScreen V_CopyScreen8
#define V_DrawPatch V_DrawPatch8
#define V_DrawPatchFlipped V_DrawPatchFlipped8
#define V_DrawPatchDirect V_DrawPatchDirect8
#define V_DrawPatchInDirect V_DrawPatchInDirect8
#define V_DrawPatchInDirectFlipped V_DrawPatchInDirectFlipped8
#define V_DrawPatchTrans V_DrawPatchTrans8
#define V_DrawPatchInDirectTrans V_DrawPatchInDirectTrans8
#define V_DrawPatchShrink V_DrawPatchShrink8
#define V_DrawBlock V_DrawBlock8
#define V_GetBlock V_GetBlock8
#define V_DarkenScreen V_DarkenScreen8
#define V_TextureBackScreen V_TextureBackScreen8
#define V_DrawPixel V_DrawPixel8
#define V_DrawLine V_DrawLine8
#define V_DrawBox V_DrawBox8
#define V_DrawPatchClip V_DrawPatchClip8

#define resinit_r_draw_c resinit_r_draw_c8
#define R_DrawColumn R_DrawColumn8_CVersion
#define R_DrawFuzzColumn R_DrawFuzzColumn8
#define R_DrawTranslucentColumn R_DrawTranslucentColumn8
#define R_DrawTranslatedColumn R_DrawTranslatedColumn8
#define R_DrawTranslucentTranslatedColumn R_DrawTranslucentTranslatedColumn8
#define R_VideoErase R_VideoErase8
#define R_DrawSpan R_DrawSpan8_CVersion
#define R_DrawTranslucentSpan R_DrawTranslucentSpan8
#define R_FillBackScreen R_FillBackScreen8 
#define R_DrawViewBorder R_DrawViewBorder8
*/

// Allocates buffer screens, call before R_Init.
extern void (*V_Init) (void);
extern void (*V_CopyRect) (screen_t * dest, screen_t * src, int srcx, int srcy, int width, int height, int destx, int desty);
extern void (*V_CopyScreen) (screen_t * dest, screen_t * src);
extern void (*V_DrawPatch) (screen_t * scr, int x, int y, const patch_t * patch);
extern void (*V_DrawPatchFlipped) (screen_t * scr, int x, int y, const patch_t * patch);
extern void (*V_DrawPatchDirect) (screen_t * scr, int x, int y, const patch_t * patch);
extern void (*V_DrawPatchInDirect) (screen_t * scr, int x, int y, const patch_t * patch);
extern void (*V_DrawPatchInDirectFlipped) (screen_t * scr, int x, int y, const patch_t * patch);
extern void (*V_DrawPatchTrans) (screen_t * scr, int x, int y, const byte *trans, const patch_t * patch);
extern void (*V_DrawPatchInDirectTrans) (screen_t * scr, int x, int y, const byte *trans, const patch_t * patch);
extern void (*V_DrawPatchShrink) (screen_t * scr, int x, int y, const patch_t * patch);
extern void (*V_DrawBlock) (screen_t * dest, int x, int y, int width, int height, byte * src);
extern void (*V_GetBlock) (screen_t * scr, int x, int y, int width, int height, byte * dest);
extern void (*V_DarkenScreen) (screen_t * scr);
extern void (*V_TextureBackScreen) (screen_t * scr, const char *flatname, int left, int top, int right, int bottom);
extern void (*V_DrawPixel) (screen_t * scr, int x, int y, int c);
extern void (*V_DrawLine) (screen_t * scr, int x1, int y1, int x2, int y2, int c);
extern void (*V_DrawBox) (screen_t * scr, int x, int y, int w, int h, int c);
extern void (*V_DrawPatchClip) (screen_t * scr, int x, int y, int left, int top, int right, int bottom, boolean_t flip, const patch_t * patch);

extern void V_DrawPatchName(screen_t * scr, int x, int y, const char *name);
extern void V_DrawPatchInDirectName(screen_t * scr, int x, int y, const char *name);

//
// now with r_draw.h
//
extern const coltable_t *dc_colourmap;
extern int dc_x;
extern int dc_yl;
extern int dc_yh;
extern int dc_width, dc_height;
extern fixed_t dc_yfrac;
extern fixed_t dc_iscale;
extern fixed_t dc_texturemid;   // !!! FIXME: redundant
extern fixed_t dc_translucency;

#ifndef NOSMOOTHING
extern boolean_t dc_usesmoothing;
extern boolean_t ds_usesmoothing;
#endif

// first pixel in a column
extern const byte *dc_source;

#ifndef NOSMOOTHING
extern fixed_t dc_xfrac;
extern const byte *dc_source2;
#endif

extern byte **ylookup;
extern int *columnofs;

extern void (*resinit_r_draw_c) (void);
extern void (*R_DrawColumn) (void);
extern void (*R_DrawFuzzColumn) (void);
extern void (*R_DrawTranslucentColumn) (void);
extern void (*R_DrawTranslatedColumn) (void);
extern void (*R_DrawTranslucentTranslatedColumn) (void);
extern void (*R_VideoErase) (unsigned ofs, int count); 

extern int ds_y;
extern int ds_x1;
extern int ds_x2;

extern const coltable_t *ds_colourmap;

extern fixed_t ds_xfrac, ds_yfrac;
extern fixed_t ds_xstep, ds_ystep;
extern int ds_width, ds_height;

// start of a 64*64 tile image
extern const byte *ds_source;

extern const byte *dc_translation;

extern void (*R_DrawSpan) (void);
extern void (*R_DrawTranslucentSpan) (void);
extern void (*R_FillBackScreen) (void);
extern void (*R_DrawViewBorder) (void);
extern void V_ClearPageBackground(screen_t * scr);

// Screen Modes
extern screenmode_t *scrmode;
extern int numscrmodes;

// FIXME:
#define V_MarkRect(x,y,w,h)  do {} while (0)

#endif // __MULTIRES_H__
