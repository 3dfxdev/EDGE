//----------------------------------------------------------------------------
//  EDGE Wipe Main
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
// DESCRIPTION:
//   Mission start screen wipe/melt, special effects.
// 

#ifndef __F_WIPE_H__
#define __F_WIPE_H__

#include "v_screen.h"
#include "m_fixed.h"

//
//                       SCREEN WIPE PACKAGE
//

typedef enum
{
  // no wiping
  WIPE_None,
  // weird screen melt
  WIPE_Melt,
  // cross-fading
  WIPE_Crossfade,
  // pixel fading
  WIPE_Pixelfade,

  // new screen simply scrolls in from the given side of the screen
  // (or if reversed, the old one scrolls out to the given side)
  WIPE_Top,
  WIPE_Bottom,
  WIPE_Left,
  WIPE_Right,

  // Scrolls in from all corners
  WIPE_Corners,

  // Opens like doors
  WIPE_Doors,

  WIPE_NUMWIPES
}
wipetype_e;

typedef struct wipeinfo_s wipeinfo_t;
typedef struct wipeparm_s wipeparm_t;
typedef struct wipescr_s wipescr_t;

typedef struct wipetype_s
{
	// allow that start or end are equal to dest
	bool allow_start_dest;
	bool allow_end_dest;
	// these should init/destroy the data field of wp
	void (*init_info) (wipeparm_t * wp);
	void (*destroy_info) (wipeparm_t * wp);
	// returns the default duration of the effect.
	int (*def_duration) (wipeinfo_t * wi);

	// wipe: take in some way (1-progress)*start plus
	// progress*end and put the result in dest.
	void (*do_wipe) (wipeparm_t * wp);
}
wipetype_t;

struct wipescr_s
{
	// the id of the screen to be wiped. Wiping is cancelled if
	// the screen changes before it's finished.
	long id;

	// This is either just a subscreen to the one that was passed to
	// InitWipe, or it's a standalone copy of it.
	screen_t *scr;
	// if this is true, scr is a copy of the actual screen, and should
	// be updated in every call to WIPE_DoWipe. Used when destination
	// intersects with start (or end).
	bool update_scr;
	// used for positioning if update_scr is set.
	int update_x, update_y;
};

// the parameters to the actual wipe routines
struct wipeparm_s
{
	// the screens the wiping affects. Points to the scr elements of the
	// wipeinfo's wipescrs, but may be shuffled if the effect is reversed.
	screen_t *dest;
	screen_t *start;
	screen_t *end;

	// ranges from 0 to 1.0. 0 means 100% start, 1.0 means 100% end.
	float progress;

	// data that is customisable for wipetype
	void *data;
};

struct wipeinfo_s
{
	wipescr_t dest, start, end;

	// the total number of tics this wipe will last.
	int duration;

	// tells whether this wipeinfo is active or done.
	bool active;

	// w & h of the area where wiping will take place
	int width, height;

	// a reversed wipe does the effect backwards, but it still starts with
	// start screen and ends with end screen. E.g. a reversed melt means that
	// the end screen anti-melts (moves upward) until it covers the entire
	// screen. Crossfading is not affected by revesion.
	bool reversed;
	wipetype_t *type;

	// the parameters to the wiper
	wipeparm_t parms;
};

extern wipetype_t wipes[WIPE_NUMWIPES];

extern wipetype_e wipe_method;
extern int wipe_reverse;

// for enum cvars
extern const char WIPE_EnumStr[];

// creates and destroys wipeinfos.
wipeinfo_t *WIPE_CreateWipeInfo(void);
void WIPE_DestroyWipeInfo(wipeinfo_t * wi);

wipeinfo_t *WIPE_InitWipe(screen_t * dest, int destx, int desty,
    screen_t * start, int startx, int starty, int cstart,
    screen_t * end, int endx, int endy, int cend,
    int width, int height, wipeinfo_t * wi,
    int duration, bool reverse, wipetype_e effect);

// you don't need to call this, that's done automatically.
void WIPE_StopWipe(screen_t * dest, screen_t * start, screen_t * end, wipeinfo_t * wi);

// does the progress'th tic of the wipe. Returns true when done. Always keep
// calling this until it returns true.
bool WIPE_DoWipe(screen_t * dest, screen_t * start, screen_t * end, int progress, wipeinfo_t * wi);

#endif
