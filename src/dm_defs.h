//----------------------------------------------------------------------------
//  EDGE2 Basic Definitions File
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

#ifndef __DEFINITIONS__
#define __DEFINITIONS__


//
// Global parameters/defines.
//

// The current state of the game: whether we are
// playing, gazing at the intermission screen,
// the game final animation, or a demo. 
typedef enum
{
	GS_NOTHING = 0,
	GS_TITLESCREEN,
	GS_LEVEL,
	GS_INTERMISSION,
	GS_FINALE,
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

// -AJA- 2008/03/08: Extrafloor placement
#define MTF_EXFLOOR_MASK    0x3C00
#define MTF_EXFLOOR_SHIFT   10

//HEXEN/LEGACY STUFF

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

	int g_agression;

	bool edge_compat;
	bool kicking;
	bool weapon_switch;
	bool pass_missile;
	bool team_damage;
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
#define KEYD_TAB        9
#define KEYD_ENTER      13
#define KEYD_ESCAPE     27
#define KEYD_SPACE      32
#define KEYD_BACKSPACE  127

#define KEYD_TILDE      ('`')
#define KEYD_EQUALS     ('=')
#define KEYD_MINUS      ('-')
#define KEYD_RIGHTARROW	174
#define KEYD_LEFTARROW	172
#define KEYD_UPARROW	173
#define KEYD_DOWNARROW	175

#define KEYD_SHIFT		182
#define KEYD_RCTRL		157
#define KEYD_RSHIFT		182
#define KEYD_RALT		184
#define KEYD_LALT		184
#define KEYD_HOME		199
#define KEYD_PGUP		201
#define KEYD_END		207
#define KEYD_PGDN		209
#define KEYD_INSERT		210
#define KEYD_DELETE		211

#define KEYD_F1			187
#define KEYD_F2			188
#define KEYD_F3			189
#define KEYD_F4			190
#define KEYD_F5			191
#define KEYD_F6			192
#define KEYD_F7			193
#define KEYD_F8			194
#define KEYD_F9			195
#define KEYD_F10		196
#define KEYD_F11		215
#define KEYD_F12		216

#define KEYD_KP0		224
#define KEYD_KP1		225
#define KEYD_KP2		226
#define KEYD_KP3		227
#define KEYD_KP4		228
#define KEYD_KP5		229
#define KEYD_KP6		230
#define KEYD_KP7		231
#define KEYD_KP8		232
#define KEYD_KP9		233
#define KEYD_KP_DOT		234
#define KEYD_KP_PLUS	235
#define KEYD_KP_MINUS	236
#define KEYD_KP_STAR	237
#define KEYD_KP_SLASH	238
#define KEYD_KP_EQUAL	239
#define KEYD_KP_ENTER	240

#define KEYD_PRTSCR		212
#define KEYD_NUMLOCK	197
#define KEYD_SCRLOCK	198
#define KEYD_CAPSLOCK	254
#define KEYD_PAUSE		255

// Values from here on aren't actually keyboard keys, but buttons
// on joystick or mice.

#define KEYD_MOUSE1		256
#define KEYD_MOUSE2		257
#define KEYD_MOUSE3		258
#define KEYD_MOUSE4		259
#define KEYD_MOUSE5		260
#define KEYD_MOUSE6		261
#define KEYD_WHEEL_UP	270
#define KEYD_WHEEL_DN	271

//#define KEYD_MWHEELUP       235
//#define KEYD_MWHEELDOWN     236

#define KEYD_JOY1		273
#define KEYD_JOY2		274
#define KEYD_JOY3		275
#define KEYD_JOY4		276
#define KEYD_JOY5		277
#define KEYD_JOY6		278
#define KEYD_JOY7		279
#define KEYD_JOY8		280
#define KEYD_JOY9		281
#define KEYD_JOY10		282
#define KEYD_JOY11		283
#define KEYD_JOY12		284
#define KEYD_JOY13		285
#define KEYD_JOY14		286
#define KEYD_JOY15		287

#define KEYD_AXIS1      384
#define KEYD_AXIS2      385
#define KEYD_AXIS3      386
#define KEYD_AXIS4      387
#define KEYD_AXIS5      388
#define KEYD_AXIS6      389
#define KEYD_AXIS7      390
#define KEYD_AXIS8      391
#define KEYD_AXIS9      392
#define KEYD_AXIS10     393
#define KEYD_AXIS11     394
#define KEYD_AXIS12     395
#define KEYD_AXIS13     396
#define KEYD_AXIS14     397
#define KEYD_AXIS15     398
#define KEYD_AXIS16     399
#define KEYD_AXIS_FLAG_NEGATIVE 16
// exclusive!
#define KEYD_AXIS_MAX   416

// -KM- 1998/09/27 Analogue binding, added a fly axis
#define AXIS_DISABLE     0
#define AXIS_TURN        1
#define AXIS_MLOOK       2
#define AXIS_FORWARD     3
#define AXIS_STRAFE      4
#define AXIS_FLY         5  // includes SWIM up/down

// SPLITSCREEN
#define SPLITSCREEN
#define MAXSPLITSCREENPLAYERS	2

#define NUM_KEYS				0x19A

#endif // __DEFINITIONS__


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
