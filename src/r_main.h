//----------------------------------------------------------------------------
//  EDGE Main Rendering Organisation Code
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
extern float viewcos;
extern float viewsin;
extern angle_t viewvertangle;

extern subsector_t *viewsubsector;
extern region_properties_t *view_props;

extern int viewwidth;
extern int viewheight;
extern int viewwindowx;
extern int viewwindowy;
extern int viewwindowwidth;
extern int viewwindowheight;

extern int validcount;

extern int linecount;

// -ES- 1999/03/29 Added these
extern angle_t normalfov, zoomedfov;
extern bool viewiszoomed;

extern bool setsizeneeded;
extern bool changeresneeded;  // -ES- 1998/08/20

extern int framecount;

extern int extralight;
extern const colourmap_c *effect_colourmap;
extern int effect_left;
extern bool effect_infrared;
extern bool setresfailed;

//
// Utility functions.
angle_t R_PointToAngle(float x1, float y1, float x2, float y2);
float R_PointToDist(float x1, float y1, float x2, float y2);
float R_ScaleFromGlobalAngle(angle_t visangle);
subsector_t *R_PointInSubsector(float x, float y);
region_properties_t *R_PointGetProps(subsector_t *sub, float z);

//
// REFRESH - the actual rendering functions.
//

// Renders the view for the next frame.
void R_Render(void);

// Called by startup code.
void R_Init(void);

// -ES- 1998/09/11 Added these prototypes.
void R_SetViewSize(int blocks);

// Changes Field of View to the specified angle.
void R_SetFOV(angle_t fov);

// Changes the FOV variables that the zoom key toggles between.
void R_SetNormalFOV(angle_t newfov);
void R_SetZoomedFOV(angle_t newfov);

// call this to change the resolution before the next frame.
void R_ChangeResolution(int width, int height, int depth, bool windowed);

void R_StartFading(int start, int range);

// only call these when it really is time to do the actual resolution
// or view size change, i.e. at the start of a frame.
void R_ExecuteChangeResolution(void);
void R_ExecuteSetViewSize(void);

#endif
