//----------------------------------------------------------------------------
//  EDGE Data Definition File Code (Main)
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2007  The EDGE Team.
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

#ifndef __DDF_STAT_H__
#define __DDF_STAT_H__

#include "dm_defs.h"


//-------------------------------------------------------------------------
//-----------------------  THING STATE STUFF   ----------------------------
//-------------------------------------------------------------------------

typedef int statenum_t;

#define S_NULL    0   // state
#define SPR_NULL  0   // sprite

// State Struct
typedef struct
{
	// sprite ref
	int sprite;

    // frame ref
	short frame;
 
	// brightness
	short bright;
 
	// duration in tics
	int tics;

	// label for state, or NULL
	const char *label;

	// routine to be performed
	void (* action)(struct mobj_s * object);

	// parameter for routine, or NULL
	void *action_par;

	// next state ref.  S_NULL means "remove me".
	int nextstate;

	// jump state ref.  S_NULL means remove.
	int jumpstate;
}
state_t;


// -------EXTERNALISATIONS-------

extern state_t *states;
extern int num_states;

#endif // __DDF_STAT_H__

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
