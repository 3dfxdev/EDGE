//----------------------------------------------------------------------------
//  EDGE Basic Definitions File
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

#ifndef __DEFINITIONS__
#define __DEFINITIONS__

#include "dm_type.h"

//
// Global parameters/defines.
//

// State updates, number of tics / second.
#define TICRATE         35

// The current state of the game: whether we are
// playing, gazing at the intermission screen,
// the game final animation, or a demo. 
typedef enum
{
	GS_NOTHING = -1,
	GS_LEVEL,
	GS_INTERMISSION,
	GS_FINALE,
	GS_DEMOSCREEN
}
gamestate_e;

//
// Difficulty/skill settings/filters.
//

// Skill flags.
#define MTF_EASY         1
#define MTF_NORMAL       2
#define MTF_HARD         4

// Deaf monsters/do not react to sound.
#define MTF_AMBUSH       8

// Multiplayer only.
#define MTF_NOT_SINGLE  16

// -AJA- 1999/09/22: Boom compatibility.
#define MTF_NOT_DM      32
#define MTF_NOT_COOP    64

// -AJA- 2000/07/31: Friend flag, from MBF
#define MTF_FRIEND      128 

// -AJA- 2004/11/04: This bit should be zero (otherwise old WAD).
#define MTF_RESERVED    256

typedef enum
{
	sk_invalid   = -1,
	sk_baby      = 0,
	sk_easy      = 1,
	sk_medium    = 2,
	sk_hard      = 3,
	sk_nightmare = 4,
	sk_numtypes  = 5
}
skill_t;

// -KM- 1998/12/16 Added gameflags typedef here.
typedef enum
{
	AA_OFF,
	AA_ON,
	AA_MLOOK
}
autoaim_t;

typedef struct gameflags_s
{
	// checkparm of -nomonsters
	bool nomonsters;

	// checkparm of -fast
	bool fastparm;

	bool respawn;
	bool res_respawn;
	bool itemrespawn;

	bool true3dgameplay;
	int menu_grav;
	bool more_blood;

	bool jump;
	bool crouch;
	bool mlook;
	autoaim_t autoaim;

	bool cheats;
	bool have_extra;
	bool limit_zoom;
	bool shadows;
	bool halos;

	bool edge_compat;
	bool kicking;
	bool weapon_switch;
	bool pass_missile;
}
gameflags_t;

#define  VISIBLE (1.0f)
#define  VISSTEP (1.0f/256.0f)
#define  INVISIBLE (0.0f)

//
// DOOM keyboard definition.
// This is the stuff configured by Setup.Exe.
// Most key data are simple ascii (uppercased).
//
#define KEYD_TILDE      ('`')
#define KEYD_RIGHTARROW (0x80+0x2e)
#define KEYD_LEFTARROW  (0x80+0x2c)
#define KEYD_UPARROW    (0x80+0x2d)
#define KEYD_DOWNARROW  (0x80+0x2f)
#define KEYD_ESCAPE     27
#define KEYD_ENTER      13
#define KEYD_TAB        9
#define KEYD_SPACE      32
#define KEYD_BACKSPACE  127
#define KEYD_EQUALS     0x3d
#define KEYD_MINUS      0x2d

#define KEYD_F1         (0x80+0x3b)
#define KEYD_F2         (0x80+0x3c)
#define KEYD_F3         (0x80+0x3d)
#define KEYD_F4         (0x80+0x3e)
#define KEYD_F5         (0x80+0x3f)
#define KEYD_F6         (0x80+0x40)
#define KEYD_F7         (0x80+0x41)
#define KEYD_F8         (0x80+0x42)
#define KEYD_F9         (0x80+0x43)
#define KEYD_F10        (0x80+0x44)
#define KEYD_F11        (0x80+0x57)
#define KEYD_F12        (0x80+0x58)
#define KEYD_RCTRL      (0x80+0x1d)
#define KEYD_RSHIFT     (0x80+0x36)
#define KEYD_RALT       (0x80+0x38)
#define KEYD_LALT       KEYD_RALT
#define KEYD_NUMLOCK    (0x80+0x45)
#define KEYD_SCRLOCK    (0x80+0x46)
#define KEYD_HOME       (0x80+0x47)
#define KEYD_PGUP       (0x80+0x49)
#define KEYD_END        (0x80+0x4f)
#define KEYD_PGDN       (0x80+0x51)
#define KEYD_INSERT     (0x80+0x52)
#define KEYD_DELETE     (0x80+0x53)
#define KEYD_PRTSCR     (0x80+0x54)
#define KEYD_CAPSLOCK   0xfe
#define KEYD_PAUSE      0xff

// All keys greater than this aren't actually keyboard keys, but buttons on
// joystick/mice.
#define KEYD_NONKBKEY   0x100

#define KEYD_MOUSE1     0x100
#define KEYD_MOUSE2     0x101
#define KEYD_MOUSE3     0x102
#define KEYD_MOUSE4     0x103 // -ACB- 1999/09/30 Fourth Mouse Button Added 
#define KEYD_MWHEEL_UP  0x104
#define KEYD_MWHEEL_DN  0x105

#define KEYD_JOYBASE    0x110

// -KM- 1998/09/27 Analogue binding, added a fly axis
#define AXIS_DISABLE     5
#define AXIS_TURN        0
#define AXIS_FORWARD     1
#define AXIS_STRAFE      2
#define AXIS_FLY         4  // includes SWIM up/down
#define AXIS_MLOOK       3

#endif // __DEFINITIONS__


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
