//----------------------------------------------------------------------------
//  EDGE2 Event handling Header
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2009  The EDGE2 Team.
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
typedef enum 
{
    ev_keydown,
    ev_keyup,
    ev_mouse,
    ev_mousedown,
    ev_mouseup,
    ev_gamepad
} evtype_t;

// Event structure.
// Event structure.
typedef struct 
{
    evtype_t    type;
    int         data1;        // keys / mouse/joystick buttons
    int         data2;        // mouse/joystick x move (data2)
    int         data3;        // mouse/joystick y move (data3)
    int         data4;        // misc data

} event_t;

/* typedef struct
{
	evtype_t type;

	union
	{
		struct
		{
			int sym;
		}
		key;
	
		struct
		{
			int dx;
			int dy;
		}
		mouse;
	}
	value;
}
event_t; */

#endif // __E_EVENT_H__

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
