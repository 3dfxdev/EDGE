//----------------------------------------------------------------------------
//  EDGE Video Code  
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
#include "v_res.h"

#include "am_map.h"
#include "con_cvar.h"
#include "con_defs.h" // BCC Needs to know what funclist_s is.
#include "e_event.h"
#include "dm_defs.h"
#include "dm_state.h"
#include "m_argv.h"
#include "m_fixed.h"
#include "r_draw1.h"
#include "r_draw2.h"
#include "r_plane.h"
#include "r_state.h"
#include "r_things.h"
#include "r_vbinit.h"
#include "st_stuff.h"
#include "v_screen.h"
#include "v_video1.h"
#include "v_video2.h"
#include "z_zone.h"

//
//v_video.c stuff
//
screen_t *main_scr;
screen_t *back_scr;

int SCREENWIDTH;
int SCREENHEIGHT;
int SCREENPITCH;
int SCREENBITS;
bool SCREENWINDOW;
bool graphicsmode = false;

float DX, DY, DXI, DYI, DY2, DYI2;
int SCALEDWIDTH, SCALEDHEIGHT, X_OFFSET, Y_OFFSET;

float BASEYCENTER;
float BASEXCENTER;

//
//r_draw.c stuff
//

//
// For R_DrawColumn functions...
// Source is the top of the column to scale.
//
const coltable_t *dc_colourmap;
int dc_x;
int dc_yl;
int dc_yh;
int dc_width, dc_height;
fixed_t dc_yfrac;
fixed_t dc_ystep;

// -KM- 1998/11/25 Added translucency parameter.
fixed_t dc_translucency;

#ifndef NOSMOOTHING
// -ES- 1999/05/16 True if the drawing primitive uses smoothing.
bool dc_usesmoothing;
bool ds_usesmoothing;

#endif

// first pixel in a column
const byte *dc_source;

// translation table (remaps the 256 colours)
const byte *dc_translation;

// r_drawspan:
int ds_y;
int ds_x1;
int ds_x2;

const coltable_t *ds_colourmap;

fixed_t ds_xfrac, ds_yfrac;
fixed_t ds_xstep, ds_ystep;
int ds_width, ds_height;

// start of a 64*64 tile image 
const byte *ds_source;

// just for profiling
int dscount;

// -ES- 1998/08/20 Explicit initialisation to NULL
byte **ylookup = NULL;
int *columnofs = NULL;

// Screen Modes
screenmode_t *scrmode;
int numscrmodes;

static void SetRes(void)
{
  // -ES- 1999/03/04 Removed weird aspect ratio warning - bad ratios don't look awful anymore :-)
  SCALEDWIDTH = (SCREENWIDTH - (SCREENWIDTH % 320));
  SCALEDHEIGHT = 200 * (SCALEDWIDTH / 320);

  // -KM- 1999/01/31 Add specific check for this: resolutions such as 640x350
  //  used to fail.
  if (SCALEDHEIGHT > SCREENHEIGHT)
  {
    SCALEDHEIGHT = (SCREENHEIGHT - (SCREENHEIGHT % 200));
    SCALEDWIDTH = 320 * (SCALEDHEIGHT / 200);
  }

  // -ES- 1999/03/29 Allow very low resolutions
  if (SCALEDWIDTH < 320 || SCALEDHEIGHT < 200)
  {
    SCALEDWIDTH = SCREENWIDTH;
    SCALEDHEIGHT = SCREENHEIGHT;
    X_OFFSET = Y_OFFSET = 0;
  }
  else
  {
    X_OFFSET = (SCREENWIDTH - SCALEDWIDTH) / 2;
    Y_OFFSET = (SCREENHEIGHT - SCALEDHEIGHT) / 2;
  }

  //
  // Weapon Centering
  // Calculates the weapon height, relative to the aspect ratio.
  //
  // Moved here from a #define in r_things.c  -ACB- 1998/08/04
  //
  // -ES- 1999/03/04 Better psprite scaling
  BASEYCENTER = 100;
  BASEXCENTER = 160;

#if 0  // -AJA- This message meaningless at the moment
  // -KM- 1998/07/31 Cosmetic indenting
  I_Printf("  Scaled Resolution: %d x %d\n", SCALEDWIDTH, SCALEDHEIGHT);
#endif

  DX = SCALEDWIDTH / 320.0f;
  DXI = 320.0f / SCALEDWIDTH;
  DY = SCALEDHEIGHT / 200.0f;
  DYI = 200.0f / SCALEDHEIGHT;
  DY2 = DY / 2;
  DYI2 = DYI * 2;

  if (back_scr)
    V_DestroyScreen(back_scr);

  back_scr = V_CreateScreen(SCREENWIDTH, SCREENHEIGHT, BPP);

  ST_ReInit();

  AM_InitResolution();
}

typedef struct video_func_s
{
  void (*V_CopyRect) (screen_t * dest, screen_t * src, int srcx, int srcy, int width, int height, int destx, int desty);
  void (*V_CopyScreen) (screen_t * dest, screen_t * src);

  void (*V_DrawPixel) (screen_t * scr, int x, int y, int col);
  void (*V_DrawLine) (screen_t * scr, int x1, int y1, int x2, int y2, int col);
  void (*V_DrawBox) (screen_t * scr, int x, int y, int w, int h, int col);
  void (*V_DrawBoxAlpha) (screen_t * scr, int x, int y, int w, int h, int col, fixed_t alpha);

  void (*resinit_r_draw_c) (void);
  void (*R_DrawColumn) (void);
  void (*R_DrawColumn_MIP) (void);
  void (*R_DrawFuzzColumn) (void);
  void (*R_DrawTranslatedColumn) (void);
  void (*R_DrawTranslucentTranslatedColumn) (void);
  void (*R_VideoErase) (unsigned ofs, int count);
  void (*R_DrawSpan) (void);
  void (*R_DrawSpan_MIP) (void);
  void (*R_DrawTranslucentSpan) (void);
  void (*R_DrawTranslucentSpan_MIP) (void);
  void (*R_DrawHoleySpan_MIP) (void);
  void (*R_FillBackScreen) (void);
  void (*R_DrawViewBorder) (void);
  void (*R_DrawTranslucentColumn) (void);
  void (*R_DrawTranslucentColumn_MIP) (void);

  funclist_t *colfuncs;
  funclist_t *spanfuncs;
  funclist_t *enlarge_2_2_funcs;
}
video_func_t;

video_func_t video_func[] =
{
    {
        V_CopyRect8,
        V_CopyScreen8,

    // -AJA- 1999/07/05: Added these three.
        V_DrawPixel8,
        V_DrawLine8,
        V_DrawBox8,
        V_DrawBoxAlpha8,

        resinit_r_draw_c8,
        R_DrawColumn8_CVersion,
        R_DrawColumn8_MIP,
        R_DrawFuzzColumn8,
        R_DrawTranslatedColumn8,
        R_DrawTranslucentTranslatedColumn8,
        R_VideoErase8,
        R_DrawSpan8_CVersion,
        R_DrawSpan8_MIP,
        R_DrawTranslucentSpan8,
        R_DrawTranslucentSpan8_MIP,
        R_DrawHoleySpan8_MIP,
        R_FillBackScreen8,
        R_DrawViewBorder8,
        R_DrawTranslucentColumn8,
        R_DrawTranslucentColumn8_MIP,

        &drawcol8_funcs,
        &drawspan8_funcs,
        &enlarge8_2_2_funcs
    }
#ifndef NOHICOLOUR
    ,
    {
        V_CopyRect16,
        V_CopyScreen16,

    // -AJA- 1999/07/05: Added these three.
        V_DrawPixel16,
        V_DrawLine16,
        V_DrawBox16,
        V_DrawBoxAlpha16,

        resinit_r_draw_c16,
        R_DrawColumn16_CVersion,
        R_DrawColumn16_MIP,
        R_DrawFuzzColumn16,
        R_DrawTranslatedColumn16,
        R_DrawTranslucentTranslatedColumn16,
        R_VideoErase16,
        R_DrawSpan16_CVersion,
        R_DrawSpan16_MIP,
        R_DrawTranslucentSpan16,
        R_DrawTranslucentSpan16_MIP,
        R_DrawHoleySpan16_MIP,
        R_FillBackScreen16,
        R_DrawViewBorder16,
        R_DrawTranslucentColumn16,
        R_DrawTranslucentColumn16_MIP,

        &drawcol16_funcs,
        &drawspan16_funcs,
        &enlarge16_2_2_funcs
    }
#endif // NOHICOLOUR
};

// Returns the video_func struct that uses the given BPP
static video_func_t *BPPToFuncTable(unsigned int bpp)
{
  if (bpp > sizeof(video_func) / sizeof(video_func[0]) || bpp < 1)
    I_Error("Invalid BPP: %d!", bpp);

  return &video_func[bpp - 1];
}

static void SetBPP(void)
{
  static video_func_t *functable = NULL;

  if (functable)
  {
    // functable is the previous resolution's table. Undo the
    // R_Draw* function dependencies that were set for that BPP.
    CON_SetFunclistDest(functable->colfuncs, NULL);
    CON_SetFunclistDest(functable->spanfuncs, NULL);
    CON_SetFunclistDest(functable->enlarge_2_2_funcs, NULL);
  }

  // -ES- 1998/08/20 Moved away BPP autodetect to V_MultiResInit
  functable = BPPToFuncTable(BPP);

  //okay, set all the function pointers

  V_CopyRect = functable->V_CopyRect;
  V_CopyScreen = functable->V_CopyScreen,

  V_DrawPixel = functable->V_DrawPixel;
  V_DrawLine = functable->V_DrawLine;
  V_DrawBox = functable->V_DrawBox;
  V_DrawBoxAlpha = functable->V_DrawBoxAlpha;

  resinit_r_draw_c = functable->resinit_r_draw_c;

  R_DrawFuzzColumn = functable->R_DrawFuzzColumn;
  R_DrawTranslatedColumn = functable->R_DrawTranslatedColumn;
  R_DrawTranslucentTranslatedColumn = functable->R_DrawTranslucentTranslatedColumn;
  R_VideoErase = functable->R_VideoErase;
  R_DrawSpan = functable->R_DrawSpan;
  R_DrawSpan_MIP = functable->R_DrawSpan_MIP;
  R_DrawTranslucentSpan = functable->R_DrawTranslucentSpan;
  R_DrawTranslucentSpan_MIP = functable->R_DrawTranslucentSpan_MIP;
  R_DrawHoleySpan_MIP = functable->R_DrawHoleySpan_MIP;
  R_DrawTranslucentColumn = functable->R_DrawTranslucentColumn;
  R_DrawTranslucentColumn_MIP = functable->R_DrawTranslucentColumn_MIP;
  R_DrawColumn_MIP = functable->R_DrawColumn_MIP;

  CON_SetFunclistDest(functable->colfuncs, &R_DrawColumn);
  CON_SetFunclistDest(functable->spanfuncs, &R_DrawSpan);
  CON_SetFunclistDest(functable->enlarge_2_2_funcs, &R_DoEnlargeView_2_2);
}

//
// V_InitResolution
// Inits everything resolution-dependent to SCREENWIDTH x SCREENHEIGHT x BPP
//
// -ES- 1998/08/20 Added this
//
void V_InitResolution(void)
{
  SetBPP();
  SetRes();
}

//
// V_MultiResInit
//
// Called once at startup to initialise first V_InitResolution
//
// -ACB- 1999/09/19 Removed forcevga reference
//
bool V_MultiResInit(void)
{
  I_Printf("Resolution: %d x %d x %dc\n", SCREENWIDTH, SCREENHEIGHT, 
      1 << SCREENBITS);
  return true;
}

//
// V_ClearPageBackground
//
// -KM- 1998/07/21 This func clears around the edge of a scaled pic
//
void V_ClearPageBackground(screen_t * scr)
{
  int y;
  int leftoffset = BPP * (SCREENWIDTH - SCALEDWIDTH) / 2;
  int topoffset = (SCREENHEIGHT - SCALEDHEIGHT) / 2;
  byte *dest;
  
  dest = scr->data;

  if (!V_ScreenHasCurrentRes(scr))
    I_Error("V_ClearPageBackground: Not supported by the given screen!");

  for (y = 0; y < topoffset; y++, dest += SCREENPITCH)
  {
    Z_Clear(dest, byte, SCREENWIDTH * BPP);
  }

  if (SCALEDWIDTH < SCREENWIDTH)
  {
    for (; y < (SCREENHEIGHT - topoffset); y++, dest += SCREENPITCH)
    {
      Z_Clear(dest, byte, leftoffset);
      Z_Clear(dest + SCREENWIDTH * BPP - leftoffset, byte, leftoffset);
    }
  }

  for (; y < SCREENHEIGHT; y++, dest += SCREENPITCH)
  {
    Z_Clear(dest, byte, SCREENWIDTH * BPP);
  }
}

//
// V_AddAvailableResolution
//
// Adds a resolution to the scrmodes list. This is used so we can
// select it within the video options menu.
//
// -ACB- 1999/10/03 Written
//
void V_AddAvailableResolution(screenmode_t *mode)
{
  int i;

  // Unused depth: do not add.
  if (mode->depth != 8 && mode->depth != 16 && mode->depth != 24)
    return;

  L_WriteDebug("V_AddAvailableResolution: Res %d x %d - %dbpp\n", mode->width,
      mode->height, mode->depth);

  if (!scrmode)
  {
    scrmode = Z_New(screenmode_t, 1);
    scrmode[0] = mode[0];
    numscrmodes = 1;
    return;
  }

  // Go through existing list and check width and height do not already exist
  for(i = 0; i < numscrmodes; i++)
  {
    if (scrmode[i].width == mode->width && scrmode[i].height == mode->height &&
        scrmode[i].depth == mode->depth && scrmode[i].windowed == mode->windowed)
      return;

    if ((scrmode[i].width > mode->width || scrmode[i].height > mode->height) &&
        scrmode[i].depth == mode->depth)
      break;
  }
  
  Z_Resize(scrmode, screenmode_t, numscrmodes+1);

  if (i != numscrmodes)
    Z_MoveData(&scrmode[i+1], &scrmode[i], screenmode_t, numscrmodes - i);

  scrmode[i] = mode[0];
  numscrmodes++;

  return;
}

//
// V_FindClosestResolution
// 
// Finds the closest available resolution to the one specified.
// Returns an index into scrmode[].  The samesize/samedepth will limit
// the search, so -1 is returned if there were no matches.  The search
// only considers modes with the same `windowed' flag.
//
#define DEPTH_MUL  25  // relative important of depth

int V_FindClosestResolution(screenmode_t *mode,
    bool samesize, bool samedepth)
{
  int i;

  int best_idx = -1;
  int best_dist = INT_MAX;

  for (i=0; i < numscrmodes; i++)
  {
    int dw = ABS(scrmode[i].width  - mode->width);
    int dh = ABS(scrmode[i].height - mode->height);
    int dd = ABS(scrmode[i].depth  - mode->depth) * DEPTH_MUL;
    int dist;

    if (scrmode[i].windowed != mode->windowed)
      continue;

    // an exact match is always good...
    if (dw == 0 && dh == 0 && dd == 0)
      return i;

    if (samesize && !(dw == 0 && dh == 0))
      continue;

    if (samedepth && dd != 0)
      continue;

    dist = dw * dw + dh * dh + dd * dd;

    if (dist >= best_dist)
      continue;

    // found a better match
    best_idx = i;
    best_dist = dist;
  }

  return best_idx;
}

//
// V_CompareModes
//
// Returns -1 for less than, 0 if same, or +1 for greater than.
//
int V_CompareModes(screenmode_t *A, screenmode_t *B)
{
  if (A->width < B->width)
    return -1;
  else if (A->width > B->width)
    return +1;

  if (A->height < B->height)
    return -1;
  else if (A->height > B->height)
    return +1;

  if (A->depth < B->depth)
    return -1;
  else if (A->depth > B->depth)
    return +1;

  if (A->windowed < B->windowed)
    return -1;
  else if (A->windowed > B->windowed)
    return +1;

  return 0;
}

