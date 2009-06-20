//----------------------------------------------------------------------------
//  EDGE Key handling
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2009  The EDGE Team.
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

#ifndef __E_KEYS_H__
#define __E_KEYS_H__

#include "e_event.h"

class key_binding_c
{
public:
	/* this is very primitive right now */

	short keys[4];

public:
	key_binding_c()
	{
		keys[0] = keys[1] = keys[2] = keys[3];
	}

	~key_binding_c()
	{ }

	void Clear();

	bool Add(short keyd);
	bool Remove(short keyd);

	bool HasKey(short keyd) const;
	bool HasKey(const event_t *ev) const;

	bool IsPressed() const;

	std::string FormatKeyList() const;
};


typedef struct
{
	const char *name;

	key_binding_c *bind;

	short def_key1;
	short def_key2;
}
key_link_t;


extern key_link_t  all_binds[];

void E_ResetAllBinds(void);
key_link_t *E_FindKeyBinding(const char *func_name);
std::string E_FormatConfig(key_link_t *link);

const char *E_GetKeyName(int keyd);
int E_KeyFromName(const char *name);


//----------------------------------------------------------------------------
//
// Keyboard definition.
// Most key data are simple ascii (uppercased).
//
#define KEYD_IGNORE     0

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
#define KEYD_CTRL       (0x80+0x1d)
#define KEYD_SHIFT      (0x80+0x36)
#define KEYD_ALT        (0x80+0x38)
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

#define NUMKEYS  0x140

#endif  /* __E_KEYS_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
