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

struct event_s;

class key_binding_c
{
public:
	short keys[4];

	bool was_down;

public:
	key_binding_c() : was_down(false)
	{
		keys[0] = keys[1] = keys[2] = keys[3] = 0;
	}

	~key_binding_c()
	{ }

	void Clear();

	bool Add(short keyd);
	bool Remove(short keyd);
	void Toggle(short keyd);

	bool HasKey(short keyd) const;
	bool HasKey(const struct event_s *ev) const;

	bool IsPressed() const;

	// this only returns true if key was just pressed down
	// (i.e. was not pressed previously).
	bool WasJustPressed();

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

void E_UnbindAll(void);
void E_ResetAllBinds(void);

key_link_t *E_FindKeyBinding(const char *func_name);
int E_MatchAllKeys(std::vector<const char *>& list, const char *pattern);

std::string E_FormatConfig(key_link_t *link);

const char *E_GetKeyName(int keyd);
int E_KeyFromName(const char *name);


//----------------------------------------------------------------------------
//
// Keyboard definition.
// Most key data are simple ascii (uppercased).
//
#define KEYD_IGNORE     0

#define KEYD_TAB        9
#define KEYD_ENTER      13
#define KEYD_ESCAPE     27
#define KEYD_SPACE      32
#define KEYD_BACKSPACE  127

#define KEYD_TILDE      ('`')
#define KEYD_RIGHTARROW (0x80+0x2e)
#define KEYD_LEFTARROW  (0x80+0x2c)
#define KEYD_UPARROW    (0x80+0x2d)
#define KEYD_DOWNARROW  (0x80+0x2f)

#define KEYD_F1         (0x80+0x31)
#define KEYD_F2         (0x80+0x32)
#define KEYD_F3         (0x80+0x33)
#define KEYD_F4         (0x80+0x34)
#define KEYD_F5         (0x80+0x35)
#define KEYD_F6         (0x80+0x36)
#define KEYD_F7         (0x80+0x37)
#define KEYD_F8         (0x80+0x38)
#define KEYD_F9         (0x80+0x39)
#define KEYD_F10        (0x80+0x3a)
#define KEYD_F11        (0x80+0x3b)
#define KEYD_F12        (0x80+0x3c)

#define KEYD_CTRL       (0x80+0x41)
#define KEYD_SHIFT      (0x80+0x42)
#define KEYD_ALT        (0x80+0x43)
#define KEYD_HOME       (0x80+0x46)
#define KEYD_END        (0x80+0x47)
#define KEYD_PGUP       (0x80+0x48)
#define KEYD_PGDN       (0x80+0x49)
#define KEYD_INSERT     (0x80+0x4a)
#define KEYD_DELETE     (0x80+0x4b)

#define KEYD_KP0        (0x80+0x50)
#define KEYD_KP1        (0x80+0x51)
#define KEYD_KP2        (0x80+0x52)
#define KEYD_KP3        (0x80+0x53)
#define KEYD_KP4        (0x80+0x54)
#define KEYD_KP5        (0x80+0x55)
#define KEYD_KP6        (0x80+0x56)
#define KEYD_KP7        (0x80+0x57)
#define KEYD_KP8        (0x80+0x58)
#define KEYD_KP9        (0x80+0x59)

#define KEYD_KP_DOT     (0x80+0x60)
#define KEYD_KP_PLUS    (0x80+0x61)
#define KEYD_KP_MINUS   (0x80+0x62)
#define KEYD_KP_STAR    (0x80+0x63)
#define KEYD_KP_SLASH   (0x80+0x64)
#define KEYD_KP_EQUAL   (0x80+0x65)
#define KEYD_KP_ENTER   (0x80+0x66)

#define KEYD_PRINT      (0x80+0x71)
#define KEYD_CAPSLOCK   (0x80+0x72)
#define KEYD_NUMLOCK    (0x80+0x73)
#define KEYD_SCRLOCK    (0x80+0x74)
#define KEYD_PAUSE      (0x80+0x75)

// All keys greater than this aren't actually keyboard keys,
// but buttons on joystick/mice.
#define KEYD_NONKBKEY   0x100

#define KEYD_MOUSE1     0x101
#define KEYD_MOUSE2     0x102
#define KEYD_MOUSE3     0x103
#define KEYD_MOUSE4     0x104 // -ACB- 1999/09/30 Fourth Mouse Button Added 
#define KEYD_MOUSE5     0x105
#define KEYD_MOUSE6     0x106
#define KEYD_MOUSE7     0x107
#define KEYD_WHEEL_UP   0x108
#define KEYD_WHEEL_DN   0x109

#define KEYD_JOY1       0x111
#define KEYD_JOY2       0x112
#define KEYD_JOY3       0x113
#define KEYD_JOY4       0x114
#define KEYD_JOY5       0x115
#define KEYD_JOY6       0x116
#define KEYD_JOY7       0x117
#define KEYD_JOY8       0x118
#define KEYD_JOY9       0x119
#define KEYD_JOY10      0x11a
#define KEYD_JOY11      0x11b
#define KEYD_JOY12      0x11c
#define KEYD_JOY13      0x11d
#define KEYD_JOY14      0x11e
#define KEYD_JOY15      0x11f

#define NUMKEYS  0x140

#endif  /* __E_KEYS_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
