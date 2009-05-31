//----------------------------------------------------------------------------
//  EDGE Data Definition File Code (Main)
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2008  The EDGE Team.
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

#include <vector>


//-------------------------------------------------------------------------
//-----------------------  THING STATE STUFF   ----------------------------
//-------------------------------------------------------------------------

#define S_NULL    0   // state
#define SPR_NULL  0   // sprite / model

typedef enum
{
	SFF_Weapon   = (1 << 0),
	SFF_Model    = (1 << 1),
	SFF_Unmapped = (1 << 2), // model_frame not yet looked up
}
state_frame_flag_e;

// State Struct
typedef struct state_s
{
	// sprite ref
	short sprite;

    // frame ref (begins at 0)
	short frame;
 
	// brightness (0 to 255)
	short bright;
 
	short flags;

	// duration in tics
	int tics;

	// model frame name like "run2", normally NULL
	const char *model_frame;

	// label for state, or NULL
	const char *label;

	// routine to be performed
	void (* action)(struct mobj_s * mo, void *data);

	// parameter for routine, or NULL
	void *action_par;

	// next state ref.  S_NULL means "remove me"
	int nextstate;

	// jump state ref.  S_NULL not valid
	int jumpstate;
}
state_t;


// -------EXTERNALISATIONS-------

extern const state_t ddf_template_state;

extern std::vector<std::string> ddf_sprite_names;
extern std::vector<std::string> ddf_model_names;


#endif // __DDF_STAT_H__

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
