//----------------------------------------------------------------------------
//  EDGE Input handling
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2005  The EDGE Team.
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
#include "e_ticcmd.h"

void E_ClearInput(void);
void E_BuildTiccmd(ticcmd_t * cmd);
bool E_InputCheckKey(int keynum);
void E_ReleaseAllKeys(void);
void E_SetTurboScale(int scale);
void E_UpdateKeyState(void);

bool INP_Responder(event_t * ev);

/* keyboard stuff */

extern int key_right;
extern int key_left;
extern int key_lookup;
extern int key_lookdown;
extern int key_lookcenter;

// -ES- 1999/03/28 Zoom Key
extern int key_zoom;
extern int key_up;
extern int key_down;

extern int key_strafeleft;
extern int key_straferight;

// -ACB- for -MH- 1998/07/19 Flying keys
extern int key_flyup;
extern int key_flydown;

extern int key_fire;
extern int key_use;
extern int key_strafe;
extern int key_speed;
extern int key_autorun;
extern int key_nextweapon;
extern int key_prevweapon;
extern int key_jump;
extern int key_map;
extern int key_180;
extern int key_talk;
extern int key_mlook;  // -AJA- 1999/07/27.
extern int key_secondatk;
extern int key_reload;  // -AJA- 2004/11/10.

#endif  /* __E_INPUT_H__ */
