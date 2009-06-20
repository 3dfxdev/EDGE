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
    { KEYD_RIGHTARROW, "RIGHTARROW" },
    { KEYD_LEFTARROW,  "LEFTARROW" },
    { KEYD_UPARROW,    "UPARROW" },
    { KEYD_DOWNARROW,  "DOWNARROW" },
    { KEYD_BACKSPACE,  "BACKSPACE" },
    { KEYD_SCRLOCK,    "SCROLLLOCK" },
    { KEYD_NUMLOCK,    "NUMLOCK" },
    { KEYD_CAPSLOCK,   "CAPSLOCK" },

    { KEYD_ESCAPE,  "ESCAPE" },
    { KEYD_ENTER,   "ENTER" },
    { KEYD_INSERT,  "INSERT" },
    { KEYD_DELETE,  "DELETE" },
    { KEYD_PGDN,    "PAGEDOWN" },
    { KEYD_PGUP,    "PAGEUP" },
    { KEYD_HOME,    "HOME" },
    { KEYD_END,     "END" },
    { KEYD_SPACE,   "SPACE" },
    { KEYD_TILDE,   "TILDE" },
    { KEYD_SHIFT,   "SHIFT" },
    { KEYD_CTRL,    "CTRL" },
    { KEYD_ALT,     "ALT" },
    { KEYD_TAB,     "TAB" },

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

	// mouse and joystick buttons
    { KEYD_MOUSE1, "MOUSE1" },
    { KEYD_MOUSE2, "MOUSE2" },
    { KEYD_MOUSE3, "MOUSE3" },
    { KEYD_MOUSE4, "MOUSE4" },
    { KEYD_MOUSE5, "MOUSE5" },
    { KEYD_MOUSE6, "MOUSE6" },
    { KEYD_MOUSE7, "MOUSE7" },

    { KEYD_MWHEEL_UP, "WHEELUP"   },
    { KEYD_MWHEEL_DN, "WHEELDOWN" },

    { KEYD_JOYBUT1, "JOYBUT1" },
    { KEYD_JOYBUT2, "JOYBUT2" },
    { KEYD_JOYBUT3, "JOYBUT3" },
    { KEYD_JOYBUT4, "JOYBUT4" },
    { KEYD_JOYBUT5, "JOYBUT5" },
    { KEYD_JOYBUT6, "JOYBUT6" },
    { KEYD_JOYBUT7, "JOYBUT7" },
    { KEYD_JOYBUT8, "JOYBUT8" },
    { KEYD_JOYBUT9, "JOYBUT9" },

	// the end
    { 0, "" }
};

const char *E_GetKeyName(int keyd)
{
	if (keyd == 0)
		return "NONE";

	for (int i = 0; keynames[i].keycode > 0; i++)
		if (keynames[i].keycode == keyd)
			return keynames[i].name;

	static char buffer[16];

	if (33 <= keyd && keyd <= 126)
	{
		buffer[0] = keyd;
		buffer[1] = 0;

		return buffer;
	}

	sprintf(buffer, "0x%03x", keyd);

	return buffer;
}

int E_KeyFromName(const char *name)
{
	if (strlen(name) == 1 && 33 <= name[0] && name[0] <= 126)
		return (int)toupper(name[0]);

	for (int i = 0; keynames[i].keycode > 0; i++)
		if (DDF_CompareName(keynames[i].name, name) == 0)
			return keynames[i].keycode;

	if (name[0] == '0' && tolower(name[1]) == 'x')
		return strtol(name+2, NULL, 16);

	//??? not sure whether to handle "NONE" or not

	// unknown!
	return 0;
}


//----------------------------------------------------------------------------

void key_binding_c::Clear()
{
	keys[0] = keys[1] = keys[2] = keys[3];
}

bool key_binding_c::Add(short keyd)
{
	SYS_ASSERT(0 < keyd && keyd < NUMKEYS);

	if (HasKey(keyd))
		return false;  // no change

	for (int i = 0; i < 4; i++)
		if (keys[i] == 0)
		{
			keys[i] = keyd;
			return true;
		}

#if 1
	// all slots are full, so drop one of them
	keys[0] = keys[1];
	keys[1] = keys[2];
	keys[2] = keys[3];

	keys[3] = keyd;
	return true;
#else
	return false; // failed (no change)
#endif
}

bool key_binding_c::Remove(short keyd)
{
	SYS_ASSERT(0 < keyd && keyd < NUMKEYS);

	for (int i = 0; i < 4; i++)
		if (keys[i] == keyd)
		{
			keys[i] = 0;
			return true;
		}

	return false; // no change
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

#define GK_DOWN  0x01

bool key_binding_c::IsPressed() const
{
	for (int i = 0; i < 4; i++)
		if (keys[i] > 0)
			if (gamekeydown[keys[i]] & GK_DOWN)
				return true;

	return false;
}

std::string key_binding_c::FormatKeyList() const
{
	std::string result;

	for (int i = 0; i < 4; i++)
		if (keys[i] > 0)
		{
			if (! result.empty())
				result += "  ";

			result += E_GetKeyName(keys[i]);
		}

	return result;
}


//----------------------------------------------------------------------------

extern key_binding_c key_map;
extern key_binding_c key_am_left, key_am_right, key_am_up, key_am_down;
extern key_binding_c key_am_zoomin, key_am_zoomout;
extern key_binding_c key_am_follow, key_am_grid, key_am_mark, key_am_clear;

extern key_binding_c key_console, key_talk;


key_link_t  all_binds[] =
{
	// Automap
	{ "am_left",       &key_am_left,     KEYD_LEFTARROW,  0 },
	{ "am_right",      &key_am_right,    KEYD_RIGHTARROW, 0 },
	{ "am_up",         &key_am_up,       KEYD_UPARROW,    0 },
	{ "am_down",       &key_am_down,     KEYD_DOWNARROW,  0 },

	{ "am_zoomin",     &key_am_zoomin,   '=', 0 },
	{ "am_zoomout",    &key_am_zoomout,  '-', 0 },

    { "am_follow",     &key_am_follow,   'F', 0 },
    { "am_grid",       &key_am_grid,     'G', 0 },
    { "am_mark",       &key_am_mark,     'M', 0 },
    { "am_clear",      &key_am_clear,    'C', 0 },

	// Weapons
    { "weapon1",       &key_weapons[1],  '1', 0 },
    { "weapon2",       &key_weapons[2],  '2', 0 },
    { "weapon3",       &key_weapons[3],  '3', 0 },
    { "weapon4",       &key_weapons[4],  '4', 0 },
    { "weapon5",       &key_weapons[5],  '5', 0 },
    { "weapon6",       &key_weapons[6],  '6', 0 },
    { "weapon7",       &key_weapons[7],  '7', 0 },
    { "weapon8",       &key_weapons[8],  '8', 0 },
    { "weapon9",       &key_weapons[9],  '9', 0 },
    { "weapon0",       &key_weapons[0],  '0', 0 },
    { "nextweapon",    &key_nextweapon,  KEYD_MWHEEL_UP, 0 },
    { "prevweapon",    &key_prevweapon,  KEYD_MWHEEL_DN, 0 },

	// Movement
    { "forward",       &key_forward,     KEYD_UPARROW,   'W' },
    { "back",          &key_back,        KEYD_DOWNARROW, 'S' },
    { "left",          &key_left,        ',', 'A' },
    { "right",         &key_right,       '.', 'D' },
    { "up",            &key_up,          KEYD_INSERT, '/' },
    { "down",          &key_down,        KEYD_DELETE, 'V' },

    { "turnleft",      &key_turnleft,    KEYD_LEFTARROW,  0 },
    { "turnright",     &key_turnright,   KEYD_RIGHTARROW, 0 },
    { "lookup",        &key_lookup,      KEYD_PGUP, 0 },
    { "lookdown",      &key_lookdown,    KEYD_PGDN, 0 },
    { "lookcenter",    &key_lookcenter,  KEYD_HOME, KEYD_END },

	// Firing etc
    { "fire",          &key_fire,        KEYD_CTRL, KEYD_MOUSE1 },
    { "secondatk",     &key_secondatk,   'E', 0 },
    { "use",           &key_use,         KEYD_SPACE, 0 },
    { "strafe",        &key_strafe,      KEYD_ALT,   KEYD_MOUSE3 },
    { "speed",         &key_speed,       KEYD_SHIFT, 0 },
    { "autorun",       &key_autorun,     KEYD_CAPSLOCK,  0 },

	// Miscellaneous
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


void E_UnbindAll(void)
{
	for (int i = 0; all_binds[i].name; i++)
	{
		all_binds[i].bind->Clear();
	}
}

void E_ResetAllBinds(void)
{
	for (int i = 0; all_binds[i].name; i++)
	{
		key_link_t *link = &all_binds[i];

		link->bind->Clear();

		if (link->def_key1) link->bind->Add(link->def_key1);
		if (link->def_key2) link->bind->Add(link->def_key2);
	}

	key_fire.Add(KEYD_JOYBUT1);
}

key_link_t *E_FindKeyBinding(const char *func_name)
{
	for (int i = 0; all_binds[i].name; i++)
	{
		key_link_t *link = &all_binds[i];

		if (DDF_CompareName(link->name, func_name) == 0)
			return link;
	}

	return NULL; // not found
}

std::string E_FormatConfig(key_link_t *link)
{
	std::string result;

	for (int i = 0; i < 4; i++)
		if (link->bind->keys[i] > 0)
		{
			result += "bind ";
			result += E_GetKeyName(link->bind->keys[i]);
			result += " \"";
			result += link->name;
			result += "\"\n";
		}

	return result;
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
