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
/// Renamed the KEYD_KP stuff to simply KEYD_
#define KEYD_TAB        9
//#define KEYD_ENTER      13
#define KEYD_ENTER	13
#define KEYD_ESCAPE     27
#define KEYD_SPACE      32
#define KEYD_BACKSPACE  127

#define KEYD_TILDE      ('`')
#define KEYD_EQUALS     ('=')
#define KEYD_MINUS      ('-')
#define KEYD_RIGHTARROW (0x80+0x2e)
#define KEYD_LEFTARROW  (0x80+0x2c)
#define KEYD_UPARROW    (0x80+0x2d)
#define KEYD_DOWNARROW  (0x80+0x2f)

#define KEYD_RCTRL      (0x80+0x1d)
#define KEYD_RSHIFT     (0x80+0x36)
#define KEYD_RALT       (0x80+0x38)
#define KEYD_LALT       KEYD_RALT
#define KEYD_HOME       (0x80+0x47)
#define KEYD_PGUP       (0x80+0x49)
#define KEYD_END        (0x80+0x4f)
#define KEYD_PGDN       (0x80+0x51)
#define KEYD_INSERT     (0x80+0x52)
#define KEYD_DELETE     (0x80+0x53)

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

/// Renamed all KEYD_KP -> KEYP_
/* #define KEYP_0          0
#define KEYP_1          KEY_END
#define KEYP_2          KEY_DOWNARROW
#define KEYP_3          KEY_PGDN
#define KEYP_4          KEY_LEFTARROW
#define KEYP_5          '5'
#define KEYP_6          KEY_RIGHTARROW
#define KEYP_7          KEY_HOME
#define KEYP_8          KEY_UPARROW
#define KEYP_9          KEY_PGUP
#define KEYP_DIVIDE     '/'
#define KEYP_PLUS       '+'
#define KEYP_MINUS      '-'
#define KEYP_MULTIPLY   '*'
#define KEYP_PERIOD     0
#define KEYP_EQUALS     KEY_EQUALS
#define KEYP_ENTER      (0x80+0x70)//KEY_ENTER */
#define KEYP_0        (0x80+0x60)
#define KEYP_1        (0x80+0x61)
#define KEYP_2        (0x80+0x62)
#define KEYP_3        (0x80+0x63)
#define KEYP_4        (0x80+0x64)
#define KEYP_5        (0x80+0x65)
#define KEYP_6        (0x80+0x66)
#define KEYP_7        (0x80+0x67)
#define KEYP_8        (0x80+0x68)
#define KEYP_9       (0x80+0x69)
#define KEYP_PERIOD     (0x80+0x6a)
#define KEYP_PLUS    (0x80+0x6b)
#define KEYP_MINUS   (0x80+0x6c)
#define KEYP_MULTIPLY    (0x80+0x6d)
#define KEYP_DIVIDE   (0x80+0x6e) ///KEYP_DIVIDE
#define KEYP_EQUALS   (0x80+0x6f)
#define KEYP_ENTER    (0x80+0x70)

#define KEYD_PRTSCR     (0x80+0x54)
#define KEYD_NUMLOCK    (0x80+0x45)
#define KEYD_SCRLOCK    (0x80+0x46)
#define KEYD_CAPSLOCK   (0x80+0x7e)
#define KEYD_PAUSE      (0x80+0x7f)

// Values from here on aren't actually keyboard keys, but buttons
// on joystick or mice.

#define KEYD_MOUSE1     (0x100)
#define KEYD_MOUSE2     (0x101)
#define KEYD_MOUSE3     (0x102)
#define KEYD_MOUSE4     (0x103)
#define KEYD_MOUSE5     (0x104)
#define KEYD_MOUSE6     (0x105)
#define KEYD_WHEEL_UP   (0x10e)
#define KEYD_WHEEL_DN   (0x10f)

#define KEYD_JOY1       (0x110+1)
#define KEYD_JOY2       (0x110+2)
#define KEYD_JOY3       (0x110+3)
#define KEYD_JOY4       (0x110+4)
#define KEYD_JOY5       (0x110+5)
#define KEYD_JOY6       (0x110+6)
#define KEYD_JOY7       (0x110+7)
#define KEYD_JOY8       (0x110+8)
#define KEYD_JOY9       (0x110+9)
#define KEYD_JOY10      (0x110+10)
#define KEYD_JOY11      (0x110+11)
#define KEYD_JOY12      (0x110+12)
#define KEYD_JOY13      (0x110+13)
#define KEYD_JOY14      (0x110+14)
#define KEYD_JOY15      (0x110+15)

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

#define SCANCODE_TO_KEYS_ARRAY {                                            \
    0,   0,   0,   0,   'a',                                  /* 0-9 */     \
    'b', 'c', 'd', 'e', 'f',                                                \
    'g', 'h', 'i', 'j', 'k',                                  /* 10-19 */   \
    'l', 'm', 'n', 'o', 'p',                                                \
    'q', 'r', 's', 't', 'u',                                  /* 20-29 */   \
    'v', 'w', 'x', 'y', 'z',                                                \
    '1', '2', '3', '4', '5',                                  /* 30-39 */   \
    '6', '7', '8', '9', '0',                                                \
    KEYD_ENTER, KEYD_ESCAPE, KEYD_BACKSPACE, KEYD_TAB, ' ',       /* 40-49 */   \
    KEYD_MINUS, KEYD_EQUALS, '[', ']', '\\',                                  \
    0,   ';', '\'', '`', ',',                                 /* 50-59 */   \
    '.', '/', KEYD_CAPSLOCK, KEYD_F1, KEYD_F2,                                 \
    KEYD_F3, KEYD_F4, KEYD_F5, KEYD_F6, KEYD_F7,                   /* 60-69 */   \
    KEYD_F8, KEYD_F9, KEYD_F10, KEYD_F11, KEYD_F12,                              \
    KEYD_PRTSCR, KEYD_SCRLOCK, KEYD_PAUSE, KEYD_INSERT, KEYD_HOME,     /* 70-79 */   \
    KEYD_PGUP, KEYD_DELETE, KEYD_END, KEYD_PGDN, KEYD_RIGHTARROW,                   \
    KEYD_LEFTARROW, KEYD_DOWNARROW, KEYD_UPARROW,                /* 80-89 */   \
    KEYD_NUMLOCK, KEYP_DIVIDE,                                               \
    KEYP_MULTIPLY, KEYP_MINUS, KEYP_PLUS, KEYP_ENTER, KEYP_1,               \
    KEYP_2, KEYP_3, KEYP_4, KEYP_5, KEYP_6,                   /* 90-99 */   \
    KEYP_7, KEYP_8, KEYP_9, KEYP_0, KEYP_PERIOD,                            \
    0, 0, 0, KEYP_EQUALS,                                     /* 100-103 */ \
}

#define arrlen(array) (sizeof(array) / sizeof(*array))

#endif // __DEFINITIONS__


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
