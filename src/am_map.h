//----------------------------------------------------------------------------
//  EDGE Automap Functions
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

#ifndef __AMMAP_H__
#define __AMMAP_H__

#include "dm_defs.h"
#include "e_event.h"

//
// Automap drawing structs
//
typedef struct
{
  int x, y;
}
fpoint_t;

typedef struct
{
  fpoint_t a, b;
}
fline_t;

typedef struct
{
  float_t x, y;
}
mpoint_t;

typedef struct
{
  mpoint_t a, b;
}
mline_t;

typedef struct
{
  float_t slp, islp;
}
islope_t;

void AM_InitResolution(void);

// Called by main loop.
boolean_t AM_Responder(event_t * ev);

// Called by main loop.
void AM_Ticker(void);

// Called by main loop,
// called instead of view drawer if automap active.
void AM_Drawer(void);

// Called to force the automap to quit
// if the level is completed while it is up.
void AM_Stop(void);

#endif
