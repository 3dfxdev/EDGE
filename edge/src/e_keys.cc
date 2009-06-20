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

#include "i_defs.h"

#include "e_event.h"
#include "e_keys.h"
#include "e_input.h"
#include "e_main.h"


extern byte gamekeydown[];


typedef struct
{
	int keycode; char name[24];
}
specialkey_t;

static specialkey_t keynames[] =
{
    { KEYD_RIGHTARROW, "RightArrow" },
    { KEYD_LEFTARROW,  "LeftArrow" },
    { KEYD_UPARROW,    "UpArrow" },
    { KEYD_DOWNARROW,  "DownArrow" },
    { KEYD_BACKSPACE,  "Backspace" },
    { KEYD_ESCAPE,     "Escape" },
    { KEYD_ENTER,      "Enter" },

    { KEYD_EQUALS, "Equals" },
    { KEYD_MINUS,  "Minus" },
    { KEYD_INSERT, "Insert" },
    { KEYD_DELETE, "Delete" },
    { KEYD_PGDN,   "PageDown" },
    { KEYD_PGUP,   "PageUp" },
    { KEYD_HOME,   "Home" },
    { KEYD_END,    "End" },
    { KEYD_SPACE,  "Space" },
    { KEYD_TILDE,  "Tilde" },

    { KEYD_TAB, "Tab" },
    { KEYD_F1,  "F1" },
    { KEYD_F2,  "F2" },
    { KEYD_F3,  "F3" },
    { KEYD_F4,  "F4" },
    { KEYD_F5,  "F5" },
    { KEYD_F6,  "F6" },
    { KEYD_F7,  "F7" },
    { KEYD_F8,  "F8" },
    { KEYD_F9,  "F9" },
    { KEYD_F10, "F10" },
    { KEYD_F11, "F11" },
    { KEYD_F12, "F12" },

    { KEYD_SHIFT,  "Shift" },
    { KEYD_CTRL,   "Ctrl" },
    { KEYD_ALT,    "Alt" },
    { KEYD_SCRLOCK,  "ScrollLock" },
    { KEYD_NUMLOCK,  "NumLock" },
    { KEYD_CAPSLOCK, "CapsLock" },

	// mouse and joystick buttons
    { KEYD_MOUSE1, "Mouse1" },
    { KEYD_MOUSE2, "Mouse2" },
    { KEYD_MOUSE3, "Mouse3" },
    { KEYD_MOUSE4, "Mouse4" },
    { KEYD_MWHEEL_UP, "Wheel Up" },
    { KEYD_MWHEEL_DN, "Wheel Down" },

	// the end
    { 0, "" }
};


void key_binding_c::Clear()
{
	keys[0] = keys[1] = keys[2] = keys[3];
}

bool key_binding_c::Add(short keyd)
{
	SYS_ASSERT(0 < keyd && keyd < NUMKEYS);

	if (HasKey(keyd))
		return true;

	for (int i = 0; i < 4; i++)
		if (keys[i] == 0)
		{
			keys[i] = keyd;
			return true;
		}

	return false; // failed
}

bool key_binding_c::Remove(short keyd)
{
	SYS_ASSERT(0 < keyd && keyd < NUMKEYS);

	if (! HasKey(keyd))
		return true;

	for (int i = 0; i < 4; i++)
		if (keys[i] == keyd)
		{
			keys[i] = 0;
			return true;
		}

	return false; // failed
}

bool key_binding_c::HasKey(short keyd) const
{
	return (keys[0] == keyd) || (keys[1] == keyd) ||
	       (keys[2] == keyd) || (keys[3] == keyd);
}

bool key_binding_c::HasKey(const event_t *ev) const
{
	if (ev->value.key.sym <= 0) return false;
	if (ev->value.key.sym >= NUMKEYS) return false;

	return HasKey(ev->value.key.sym);
}

bool key_binding_c::IsPressed() const
{
	for (int i = 0; i < 4; i++)
		if (keys[i] > 0)
			if (gamekeydown[keys[i]])
				return true;

	return false;
}


//----------------------------------------------------------------------------


key_link_t  all_binds[] =
{
    { "forward",       &key_forward,     KEYD_UPARROW,   'W' },
    { "back",          &key_backward,    KEYD_DOWNARROW, 'S' },
    { "strafeleft",    &key_strafeleft,  ',', 'A' },
    { "straferight",   &key_straferight, '.', 'D' },
    { "up",            &key_flyup,       KEYD_INSERT, '/' },
    { "down",          &key_flydown,     KEYD_DELETE, 'V' },

    { "left",          &key_left,        KEYD_LEFTARROW,  0 },
    { "right",         &key_right,       KEYD_RIGHTARROW, 0 },
    { "lookup",        &key_lookup,      KEYD_PGUP, 0 },
    { "lookdown",      &key_lookdown,    KEYD_PGDN, 0 },
    { "lookcenter",    &key_lookcenter,  KEYD_HOME, KEYD_END },

    { "fire",          &key_fire,        KEYD_CTRL, KEYD_MOUSE1 },
    { "secondatk",     &key_secondatk,   'E', KEYD_MOUSE2 },
    { "use",           &key_use,         KEYD_SPACE, 0 },
    { "strafe",        &key_strafe,      KEYD_ALT,   0 },
    { "speed",         &key_speed,       KEYD_SHIFT, 0 },
    { "autorun",       &key_autorun,     KEYD_CAPSLOCK,  0 },
    { "nextweapon",    &key_nextweapon,  KEYD_MWHEEL_UP, 0 },
    { "prevweapon",    &key_prevweapon,  KEYD_MWHEEL_DN, 0 },

    { "map",           &key_map,         KEYD_TAB, 0 },
    { "zoom",          &key_zoom,        '\\', 0 },
    { "turn180",       &key_180,         0, 0 },
    { "reload",        &key_reload,      'R', 0 },
    { "talk",          &key_talk,        'T', 0 },
    { "console",       &key_console,     KEYD_TILDE, 0 },
    { "mlook",         &key_mlook,       'M', 0 },

	// the end
	{ NULL, NULL, 0, 0 }
};


void E_ResetAllBinds(void)
{
	for (int i = 0; all_binds[i].name; i++)
	{
		key_link_t *link = &all_binds[i];

		link->bind->Clear();

		if (link->def_key1) link->bind->Add(link->def_key1);
		if (link->def_key2) link->bind->Add(link->def_key2);
	}

	key_fire.Add(KEYD_JOYBASE);
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
