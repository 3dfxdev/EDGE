//----------------------------------------------------------------------------
//  EDGE Status Bar Library Code
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

#ifndef __STLIB__
#define __STLIB__

// We are referring to patches.
#include "r_defs.h"
#include "w_image.h"

//
// Typedefs of widgets
//

// Number widget

typedef struct
{
  // upper right-hand corner
  //  of the number (right-justified)
  int x, y;

  // max # of digits in number
  int width;

  // last number value
  int oldnum;

  // pointer to current value
  int *num;

  // pointer to boolean stating
  //  whether to update number
  boolean_t *on;

  // list of images for 0-9
  const image_t ** digits;

  // minus, or NULL for none.
  const image_t *minus;

  // colourmap
  const colourmap_t *colmap;
}
st_number_t;

// -ES- 1999/11/10 Quick hack to allow floats. FIXME: Rewrite All.
typedef struct
{
  st_number_t num;
  flo_t *f;
}
st_flo_t;


// Percent widget ("child" of number widget,
//  or, more precisely, contains a number widget.)
typedef struct
{
  // number information
  st_flo_t f;

  // percent sign graphic
  const image_t *percsign;
}
st_percent_t;

// Multiple Icon widget
typedef struct
{
  // center-justified location of icons
  int x, y;

  // last icon number
  int oldinum;

  // pointer to current icon
  int *inum;

  // pointer to boolean stating
  //  whether to update icon
  boolean_t *on;

  // list of icons
  const image_t ** icons;
}
st_multicon_t;

// Binary Icon widget

typedef struct
{
  // center-justified location of icon
  int x, y;

  // last icon value
  boolean_t oldval;

  // pointer to current icon status
  boolean_t *val;

  // pointer to boolean
  //  stating whether to update icon
  boolean_t *on;

  // icon
  const image_t *icon;
}
st_binicon_t;

//
// Widget creation, access, and update routines
//

// Initialises widget library.
// More precisely, initialise STMINUS,
//  everything else is done somewhere else.
//
void STLIB_Init(void);

// Number widget routines
void STLIB_InitNum(st_number_t * n, int x, int y, 
    const image_t ** digits, const image_t *minus, int *num, 
    boolean_t * on, int width);

void STLIB_InitFloat(st_flo_t * n, int x, int y, const image_t ** digits, 
    flo_t *num, boolean_t * on, int width);

void STLIB_UpdateNum(st_number_t * n, boolean_t refresh);
void STLIB_UpdateFloat(st_flo_t * n, boolean_t refresh);

// Percent widget routines
void STLIB_InitPercent(st_percent_t * p, int x, int y, 
    const image_t ** digits, const image_t *percsign,
    flo_t *num, boolean_t * on);

void STLIB_UpdatePercent(st_percent_t * per, int refresh);

// Multiple Icon widget routines
void STLIB_InitMultIcon(st_multicon_t * mi, int x, int y, 
    const image_t ** icons, int *inum, boolean_t * on);

void STLIB_UpdateMultIcon(st_multicon_t * mi, boolean_t refresh);

// Binary Icon widget routines

void STLIB_InitBinIcon(st_binicon_t * b, int x, int y, 
    const image_t * icon, boolean_t * val, boolean_t * on);

void STLIB_UpdateBinIcon(st_binicon_t * bi, boolean_t refresh);

#endif
