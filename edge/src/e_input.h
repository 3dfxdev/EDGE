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

#include "e_event.h"
#include "e_keys.h"
#include "e_ticcmd.h"

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

extern key_binding_c key_forward;
extern key_binding_c key_backward;
extern key_binding_c key_strafeleft;
extern key_binding_c key_straferight;

extern key_binding_c key_right;
extern key_binding_c key_left;
extern key_binding_c key_lookup;
extern key_binding_c key_lookdown;
extern key_binding_c key_lookcenter;

// -ES- 1999/03/28 Zoom Key
extern key_binding_c key_zoom;

// -ACB- for -MH- 1998/07/19 Flying keys
extern key_binding_c key_flyup;
extern key_binding_c key_flydown;

extern key_binding_c key_fire;
extern key_binding_c key_secondatk;
extern key_binding_c key_use;
extern key_binding_c key_strafe;
extern key_binding_c key_speed;
extern key_binding_c key_autorun;
extern key_binding_c key_nextweapon;
extern key_binding_c key_prevweapon;
extern key_binding_c key_map;
extern key_binding_c key_180;
extern key_binding_c key_talk;
extern key_binding_c key_console;
extern key_binding_c key_mlook;  // -AJA- 1999/07/27.
extern key_binding_c key_reload;  // -AJA- 2004/11/10.

#endif  /* __E_INPUT_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
