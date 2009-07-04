//----------------------------------------------------------------------------
//  EDGE Event handling Header
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
//
//  Based on the DOOM source code, released by Id Software under the
//  following copyright:
//
//    Copyright (C) 1993-1996 by id Software, Inc.
//
//----------------------------------------------------------------------------

#ifndef __E_EVENT_H__
#define __E_EVENT_H__


//
// Event handling.
//

// Input event types.
// -KM- 1998/09/01 Amalgamate joystick/mouse into analogue
typedef enum
{
	ev_keydown,
	ev_keyup,
	ev_analogue
}
evtype_t;

// Event structure.
typedef struct
{
	evtype_t type;

	union
	{
		struct 
	    {
	        int sym;
	    	int unicode;
	    }
	    key;
	
		struct
		{
			int axis;
			float amount;
		} 
		analogue;
	} 
	value;
}
event_t;


// -KM- 1998/09/27 Analogue binding, added a fly axis
#define AXIS_DISABLE     0
#define AXIS_TURN        1
#define AXIS_FORWARD     2
#define AXIS_STRAFE      3
#define AXIS_MLOOK       4
#define AXIS_FLY         5  // includes SWIM up/down

#endif // __E_EVENT_H__

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
