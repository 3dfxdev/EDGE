//----------------------------------------------------------------------------
//  EDGE Main Rendering Organisation Code
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
//
// -KM- 1998/09/27 Dynamic Colourmaps.
//

#ifndef __R_MAIN__
#define __R_MAIN__

#include "e_player.h"
#include "r_data.h"
#include "r_defs.h"

//
// POV related.
//
extern float_t viewcos;
extern float_t viewsin;
extern float_t viewvertangle;

extern subsector_t *viewsubsector;
extern region_properties_t *view_props;

extern int viewwidth;
extern int viewheight;
extern int viewwindowx;
extern int viewwindowy;
extern int viewwindowwidth;
extern int viewwindowheight;

// the x and y coords of the focus
// -ES- 1999/03/19 Renamed center to focus
extern float_t focusxfrac;
extern float_t focusyfrac;

extern int validcount;

extern int linecount;

// -ES- 1999/03/29 Added these
extern angle_t normalfov, zoomedfov;
extern boolean_t viewiszoomed;

extern boolean_t setsizeneeded;
extern boolean_t changeresneeded;  // -ES- 1998/08/20
extern int use_3d_mode;

extern int framecount;

//
// Lighting LUT.
// Used for z-depth cuing per column/row,
//  and other lighting effects (sector ambient, flash).
//

// Lighting constants.
#define LIGHTLEVELS	        32
#define LIGHTSEGSHIFT	         3

#define MAXLIGHTZ	       512
#define LIGHTZSHIFT		18
#define FLAT_LIGHTZ             24

#define MAXLIGHTSCALE		48
#define LIGHTSCALESHIFT		12
#define FLAT_LIGHTSCALE         16

extern lighttable_t zlight[LIGHTLEVELS][MAXLIGHTZ];
extern lighttable_t scalelight[LIGHTLEVELS][MAXLIGHTSCALE];

extern int extralight;
extern const colourmap_t *effect_colourmap;
extern float_t effect_strength;
extern boolean_t effect_infrared;
extern boolean_t setresfailed;

//
// Function pointers to switch refresh/drawing functions.
// Used to select shadow mode etc.
//
extern void (*colfunc) (void);
extern void (*basecolfunc) (void);
extern void (*fuzzcolfunc) (void);
extern void (*spanfunc) (void);
extern void (*trans_spanfunc) (void);

//
// Utility functions.
angle_t R_PointToAngle(float_t x1, float_t y1, float_t x2, float_t y2);
float_t R_PointToDist(float_t x1, float_t y1, float_t x2, float_t y2);
float_t R_ScaleFromGlobalAngle(angle_t visangle);
subsector_t *R_PointInSubsector(float_t x, float_t y);
region_properties_t *R_PointGetProps(subsector_t *sub, float_t z);

//
// REFRESH - the actual rendering functions.
//

// Renders the view for the next frame.
extern void (*R_Render) (void);

// Called by startup code.
boolean_t R_Init(void);

// -ES- 1998/09/11 Added these prototypes.
void R_SetViewSize(int blocks);

// Changes Field of View to the specified angle.
void R_SetFOV(angle_t fov);

// Changes the FOV variables that the zoom key toggles between.
void R_SetNormalFOV(angle_t newfov);
void R_SetZoomedFOV(angle_t newfov);

// call this to change the resolution before the next frame.
void R_ChangeResolution(int width, int height, int depth, boolean_t windowed);

void R_StartFading(int start, int range);

// only call these when it really is time to do the actual resolution
// or view size change, i.e. at the start of a frame.
void R_ExecuteChangeResolution(void);
void R_ExecuteSetViewSize(void);

#endif
