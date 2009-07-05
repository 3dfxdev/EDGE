//----------------------------------------------------------------------------
//  EDGE Input handling
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

#ifndef __E_INPUT_H__
#define __E_INPUT_H__

#include "e_keys.h"
#include "e_ticcmd.h"

// Input event types.

typedef enum
{
	ev_keydown,
	ev_keyup,
	ev_mouse,
}
evtype_t;

// Event structure.
typedef struct event_s
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
			int dx;
			int dy;
		} 
		mouse;
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
#define AXIS_FLY         5  // includes JUMP/CROUCH and SWIM

#define JAXIS_HISTORY  10

class jaxis_group_c
{
public:
	cvar_c axis;
	cvar_c sens;

	cvar_c dead;  // values below this become zero
	cvar_c peak;  // values above this become one
	cvar_c tune;  // apply a power function

	cvar_c filter;  // tics to filter 0..4

	// current value, possibly filtered, adjusted for deadzone / peak
	// and with tuning applied.  Sensitivity is _NOT_ applied.
	float value;

	int history[JAXIS_HISTORY];

public:
	 jaxis_group_c();
	~jaxis_group_c();

	void NewTic(int v);

private:
	void SetFromRaw(int raw);
};

void E_ClearInput(void);
void E_BuildTiccmd(ticcmd_t * cmd);
bool E_InputCheckKey(int keynum);
void E_ReleaseAllKeys(void);
void E_SetTurboScale(int scale);
void E_UpdateKeyState(void);

void E_ProcessEvents(void);
void E_PostEvent(event_t * ev);

bool INP_Responder(event_t * ev);

extern cvar_c in_autorun;
extern cvar_c in_stageturn;

/* keyboard stuff */

extern key_binding_c k_forward;
extern key_binding_c k_back;
extern key_binding_c k_left;
extern key_binding_c k_right;

extern key_binding_c k_turnright;
extern key_binding_c k_turnleft;
extern key_binding_c k_lookup;
extern key_binding_c k_lookdown;
extern key_binding_c k_lookcenter;

// -ES- 1999/03/28 Zoom Key
extern key_binding_c k_zoom;

// -ACB- for -MH- 1998/07/19 Flying keys
extern key_binding_c k_up;
extern key_binding_c k_down;

extern key_binding_c k_fire;
extern key_binding_c k_secondatk;
extern key_binding_c k_use;
extern key_binding_c k_strafe;
extern key_binding_c k_speed;
extern key_binding_c k_autorun;
extern key_binding_c k_turn180;
extern key_binding_c k_mlook;  // -AJA- 1999/07/27.
extern key_binding_c k_reload;  // -AJA- 2004/11/10.

extern key_binding_c k_weapons[10]; // -AJA- 2009/06/20.
extern key_binding_c k_nextweapon;
extern key_binding_c k_prevweapon;

#endif  /* __E_INPUT_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
