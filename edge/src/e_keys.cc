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
    { KEYD_SPACE,   "SPACE" },
    { KEYD_TILDE,   "TILDE" },
    { KEYD_INSERT,  "INSERT" },
    { KEYD_DELETE,  "DELETE" },
    { KEYD_PGDN,    "PAGEDOWN" },
    { KEYD_PGUP,    "PAGEUP" },
    { KEYD_HOME,    "HOME" },
    { KEYD_END,     "END" },
    { KEYD_SHIFT,   "SHIFT" },
    { KEYD_CTRL,    "CTRL" },
    { KEYD_ALT,     "ALT" },
    { KEYD_TAB,     "TAB" },

	// function keys
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

	// keypad
	{ KEYD_KP0, "KP_0" },
	{ KEYD_KP1, "KP_1" },
	{ KEYD_KP2, "KP_2" },
	{ KEYD_KP3, "KP_3" },
	{ KEYD_KP4, "KP_4" },
	{ KEYD_KP5, "KP_5" },
	{ KEYD_KP6, "KP_6" },
	{ KEYD_KP7, "KP_7" },
	{ KEYD_KP8, "KP_8" },
	{ KEYD_KP9, "KP_9" },

	{ KEYD_KP_DOT,   "KP_DOT" },
	{ KEYD_KP_PLUS,  "KP_PLUS" },
	{ KEYD_KP_MINUS, "KP_MINUS" },
	{ KEYD_KP_STAR,  "KP_STAR" },
	{ KEYD_KP_SLASH, "KP_SLASH" },
	{ KEYD_KP_EQUAL, "KP_EQUALS" },
	{ KEYD_KP_ENTER, "KP_ENTER" },

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

    { KEYD_JOY1,  "JOY1"  },
    { KEYD_JOY2,  "JOY2"  },
    { KEYD_JOY3,  "JOY3"  },
    { KEYD_JOY4,  "JOY4"  },
    { KEYD_JOY5,  "JOY5"  },
    { KEYD_JOY6,  "JOY6"  },
    { KEYD_JOY7,  "JOY7"  },
    { KEYD_JOY8,  "JOY8"  },
    { KEYD_JOY9,  "JOY9"  },
    { KEYD_JOY10, "JOY10" },
    { KEYD_JOY11, "JOY11" },
    { KEYD_JOY12, "JOY12" },
    { KEYD_JOY13, "JOY13" },
    { KEYD_JOY14, "JOY14" },
    { KEYD_JOY15, "JOY15" },

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
	keys[0] = keys[1] = keys[2] = keys[3] = 0;
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

void key_binding_c::Toggle(short keyd)
{
	if (HasKey(keyd))
		Remove(keyd);
	else
		Add(keyd);
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

extern key_binding_c k_automap;
extern key_binding_c k_am_zoomin, k_am_zoomout;
extern key_binding_c k_am_follow, k_am_grid;
extern key_binding_c k_am_mark, k_am_clear;

extern key_binding_c k_console, k_talk;


key_link_t  all_binds[] =
{
	// Automap
    { "k_automap",       &k_automap,     KEYD_TAB, 0 },
	{ "k_am_zoomin",     &k_am_zoomin,   '=', 0 },
	{ "k_am_zoomout",    &k_am_zoomout,  '-', 0 },

    { "k_am_follow",     &k_am_follow,   'F', 0 },
    { "k_am_grid",       &k_am_grid,     'G', 0 },
    { "k_am_mark",       &k_am_mark,     'M', 0 },
    { "k_am_clear",      &k_am_clear,    'C', 0 },

	// Weapons
    { "k_weapon1",       &k_weapons[1],  '1', 0 },
    { "k_weapon2",       &k_weapons[2],  '2', 0 },
    { "k_weapon3",       &k_weapons[3],  '3', 0 },
    { "k_weapon4",       &k_weapons[4],  '4', 0 },
    { "k_weapon5",       &k_weapons[5],  '5', 0 },
    { "k_weapon6",       &k_weapons[6],  '6', 0 },
    { "k_weapon7",       &k_weapons[7],  '7', 0 },
    { "k_weapon8",       &k_weapons[8],  '8', 0 },
    { "k_weapon9",       &k_weapons[9],  '9', 0 },
    { "k_weapon0",       &k_weapons[0],  '0', 0 },
    { "k_nextweapon",    &k_nextweapon,  KEYD_MWHEEL_UP, 0 },
    { "k_prevweapon",    &k_prevweapon,  KEYD_MWHEEL_DN, 0 },

	// Movement
    { "k_forward",       &k_forward,     KEYD_UPARROW,   'W' },
    { "k_back",          &k_back,        KEYD_DOWNARROW, 'S' },
    { "k_left",          &k_left,        ',', 'A' },
    { "k_right",         &k_right,       '.', 'D' },
    { "k_up",            &k_up,          KEYD_INSERT, '/' },
    { "k_down",          &k_down,        KEYD_DELETE, 'V' },

    { "k_turnleft",      &k_turnleft,    KEYD_LEFTARROW,  0 },
    { "k_turnright",     &k_turnright,   KEYD_RIGHTARROW, 0 },
    { "k_turn180",       &k_turn180,     0, 0 },
    { "k_lookup",        &k_lookup,      KEYD_PGUP, 0 },
    { "k_lookdown",      &k_lookdown,    KEYD_PGDN, 0 },
    { "k_lookcenter",    &k_lookcenter,  KEYD_HOME, KEYD_END },

	// Firing etc
    { "k_fire",          &k_fire,        KEYD_CTRL, KEYD_MOUSE1 },
    { "k_secondatk",     &k_secondatk,   'E', 0 },
    { "k_use",           &k_use,         KEYD_SPACE, 0 },
    { "k_strafe",        &k_strafe,      KEYD_ALT,   KEYD_MOUSE3 },
    { "k_speed",         &k_speed,       KEYD_SHIFT, 0 },
    { "k_autorun",       &k_autorun,     KEYD_CAPSLOCK,  0 },

	// Miscellaneous
    { "k_zoom",          &k_zoom,        '\\', 0 },
    { "k_reload",        &k_reload,      'R', 0 },
    { "k_console",       &k_console,     KEYD_TILDE, 0 },
    { "k_mlook",         &k_mlook,       'M', 0 },
    { "k_talk",          &k_talk,        'T', 0 },

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

	k_fire.Add(KEYD_JOY1);
}


key_link_t *E_FindKeyBinding(const char *func_name)
{
	for (int i = 0; all_binds[i].name; i++)
	{
		key_link_t *link = &all_binds[i];

		if (stricmp(link->name, func_name) == 0)
			return link;
	}

	// user convenience: allow k_jump as an alias
	if (stricmp(func_name, "k_jump") == 0)
		return E_FindKeyBinding("k_up");

	return NULL; // not found
}


int E_MatchAllKeys(std::vector<const char *>& list, const char *pattern)
{
	list.clear();

	for (int i = 0; all_binds[i].name; i++)
	{
		if (CON_MatchPattern(all_binds[i].name, pattern))
			list.push_back(all_binds[i].name);
	}

	return (int)list.size();
}


std::string E_FormatConfig(key_link_t *link)
{
	std::string result = link->name;

	result += " -c";

	for (int i = 0; i < 4; i++)
		if (link->bind->keys[i] > 0)
		{
			result += " ";
			result += E_GetKeyName(link->bind->keys[i]);
		}

	result += "\n";

	return result;
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
