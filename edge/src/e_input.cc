//----------------------------------------------------------------------------
//  EDGE Input handling
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2004  The EDGE Team.
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
//
// -MH- 1998/07/02 Added key_flyup and key_flydown variables (no logic yet)
// -MH- 1998/08/18 Flyup and flydown logic
//

#include "i_defs.h"
#include "g_game.h"

#include "am_map.h"
#include "con_main.h"
#include "dm_defs.h"
#include "dm_state.h"
#include "dstrings.h"
#include "e_main.h"
#include "f_finale.h"
#include "m_argv.h"
#include "m_cheat.h"
#include "m_inline.h"
#include "m_misc.h"
#include "m_menu.h"
#include "m_random.h"
#include "hu_stuff.h"
#include "p_bot.h"
#include "p_local.h"
#include "p_setup.h"
#include "p_tick.h"
#include "r_data.h"
#include "r_layers.h"
#include "r_sky.h"
#include "r_view.h"
#include "rad_trig.h"
#include "s_sound.h"
#include "sv_chunk.h"
#include "sv_main.h"
#include "st_stuff.h"
#include "version.h"
#include "v_res.h"
#include "w_image.h"
#include "w_textur.h"
#include "w_wad.h"
#include "wi_stuff.h"
#include "z_zone.h"

//
// controls (have defaults) 
// 
int key_right;
int key_left;
int key_lookup;
int key_lookdown;
int key_lookcenter;

// -ES- 1999/03/28 Zoom Key
int key_zoom;

int key_up;
int key_down;
int key_strafeleft;
int key_straferight;
int key_fire;
int key_use;
int key_strafe;
int key_speed;
int key_autorun;
int key_nextweapon;
int key_prevweapon;
int key_jump;
int key_map;
int key_180;
int key_talk;
int key_mlook;
int key_secondatk;
int key_reload;

// -MH- 1998/07/10 Flying keys
int key_flyup;
int key_flydown;

#define MAXPLMOVE  (forwardmove[1])

static int forwardmove[2] = {0x19, 0x32};
static int upwardmove[2]  = {0x19, 0x32};  // -MH- 1998/08/18 Up/Down movement
static int sidemove[2]    = {0x18, 0x28};
static int angleturn[3]   = {640, 1280, 320};  // + slow turn 

#define ZOOM_ANGLE_DIV  3

#define SLOWTURNTICS    6

#define NUMKEYS         512

#define GK_DOWN  0x01
#define GK_UP    0x02

static byte gamekeydown[NUMKEYS];

int turnheld;   // for accelerative turning 
int mlookheld;  // for accelerative mlooking 

//-------------------------------------------
// -KM-  1998/09/01 Analogue binding
// -ACB- 1998/09/06 Two-stage turning switch
//
int mouse_xaxis = AXIS_TURN;  // joystick values are used once
int mouse_yaxis = AXIS_FORWARD;
int joy_xaxis = AXIS_TURN;  // joystick values are repeated
int joy_yaxis = AXIS_FORWARD;

// The last one is ignored (AXIS_DISABLE)
static int analogue[6] = {0, 0, 0, 0, 0, 0};

bool stageturn;  // Stage Turn Control

int forwardmovespeed;  // Speed controls

int angleturnspeed;
int sidemovespeed;

int mlookspeed = 1000 / 64;

// -ACB- 1999/09/30 Has to be true or false - bool-ified
bool invertmouse = false;


bool E_InputCheckKey(int keynum)
{
#ifdef DEVELOPERS
	if ((keynum >> 16) > NUMKEYS)
		I_Error("Invalid key!");
	else if ((keynum & 0xffff) > NUMKEYS)
		I_Error("Invalid key!");
#endif

	if (gamekeydown[keynum >> 16] & GK_DOWN)
		return true;
	else if (gamekeydown[keynum & 0xffff] & GK_DOWN)
		return true;
	else
		return false;
}

#if 0  // UNUSED ???
static int CmdChecksum(ticcmd_t * cmd)
{
	int i;
	int sum = 0;

	for (i = 0; i < (int)sizeof(ticcmd_t) / 4 - 1; i++)
		sum += ((int *)cmd)[i];

	return sum;
}
#endif

//
// E_BuildTiccmd
//
// Builds a ticcmd from all of the available inputs
//
// -ACB- 1998/07/02 Added Vertical angle checking for mlook.
// -ACB- 1998/07/10 Reformatted: I can read the code! :)
// -ACB- 1998/09/06 Apply speed controls to -KM-'s analogue controls
// -AJA- 1999/08/10: Reworked the GetSpeedDivisor macro.
//
#define GetSpeedDivisor(speed) \
	(((speed) == 8) ? 6 : ((8 - (speed)) << 4))

static bool allow180 = true;
static bool allowzoom = true;
static bool allowautorun = true;

void E_BuildTiccmd(ticcmd_t * cmd)
{
	Z_Clear(cmd, ticcmd_t, 1);

	bool strafe = E_InputCheckKey(key_strafe);
	int speed = E_InputCheckKey(key_speed) ? 1 : 0;

	if (autorunning)
		speed = !speed;

	int forward = 0;
	int upward = 0;  // -MH- 1998/08/18 Fly Up/Down movement
	int side = 0;

	//
	// -KM- 1998/09/01 use two stage accelerative turning on all devices
	//
	// -ACB- 1998/09/06 Allow stage turning to be switched off for
	//                  analogue devices...
	//
	int t_speed = speed;

	if (E_InputCheckKey(key_right) || E_InputCheckKey(key_left) ||
		(analogue[AXIS_TURN] && stageturn))
		turnheld++;
	else
		turnheld = 0;

	// slow turn ?
	if (turnheld < SLOWTURNTICS)
		t_speed = 2;

	int m_speed = speed;

	if (E_InputCheckKey(key_lookup) || E_InputCheckKey(key_lookdown))
		mlookheld++;
	else
		mlookheld = 0;

	// slow turn ?
	if (mlookheld < SLOWTURNTICS)
		m_speed = 2;

	// -ES- 1999/03/28 Zoom Key
	if (E_InputCheckKey(key_zoom))
	{
		if (allowzoom)
		{
			cmd->extbuttons |= EBT_ZOOM;
			allowzoom = false;
		}
	}
	else
		allowzoom = true;

	// -AJA- 2000/04/14: Autorun toggle
	if (E_InputCheckKey(key_autorun))
	{
		if (allowautorun)
		{
			autorunning  = !autorunning;
			allowautorun = false;
		}
	}
	else
		allowautorun = true;

	// You have to release the 180 deg turn key before you can press it again
	if (E_InputCheckKey(key_180))
	{
		if (allow180)
			cmd->angleturn = ANG180 >> 16;
		allow180 = false;
	}
	else
	{
		allow180 = true;
		cmd->angleturn = 0;
	}

	//let movement keys cancel each other out
	if (strafe)
	{
		if (E_InputCheckKey(key_right))
			side += sidemove[speed];

		if (E_InputCheckKey(key_left))
			side -= sidemove[speed];

		// -KM- 1998/09/01 Analogue binding
		// -ACB- 1998/09/06 Side Move Speed Control
		int i = GetSpeedDivisor(sidemovespeed);
		side += analogue[AXIS_TURN] * sidemove[speed] / i;
	}
	else
	{
		int angle_rate = angleturn[t_speed];

		if (viewiszoomed)
			angle_rate /= ZOOM_ANGLE_DIV;

		if (E_InputCheckKey(key_right))
			cmd->angleturn -= angle_rate;

		if (E_InputCheckKey(key_left))
			cmd->angleturn += angle_rate;

		// -KM- 1998/09/01 Analogue binding
		// -ACB- 1998/09/06 Angle Turn Speed Control
		int i = GetSpeedDivisor(angleturnspeed);
		cmd->angleturn -= analogue[AXIS_TURN] * angle_rate / i;
	}

	cmd->mlookturn = 0;

	if (level_flags.mlook)
	{
		int mlook_rate = angleturn[m_speed] / 2;

		if (viewiszoomed)
			mlook_rate /= ZOOM_ANGLE_DIV;

		// -ACB- 1998/07/02 Use VertAngle for Look/up down.
		if (E_InputCheckKey(key_lookup))
			cmd->mlookturn += mlook_rate;

		// -ACB- 1998/07/02 Use VertAngle for Look/up down.
		if (E_InputCheckKey(key_lookdown))
			cmd->mlookturn -= mlook_rate;

		// -KM- 1998/09/01 More analogue binding
		cmd->mlookturn += analogue[AXIS_MLOOK] * mlook_rate /
			((21 - mlookspeed) << 3);

		// -ACB- 1998/07/02 Use CENTER flag to center the vertical look.
		if (E_InputCheckKey(key_lookcenter))
			cmd->extbuttons |= EBT_CENTER;
	}

	// -MH- 1998/08/18 Fly up
	if (level_flags.true3dgameplay)
	{
		if ((E_InputCheckKey(key_flyup)))
			upward += upwardmove[speed];

		// -MH- 1998/08/18 Fly down
		if ((E_InputCheckKey(key_flydown)))
			upward -= upwardmove[speed];

		int i = GetSpeedDivisor(forwardmovespeed);
		upward += analogue[AXIS_FLY] * upwardmove[speed] / i;
	}

	if (E_InputCheckKey(key_up))
		forward += forwardmove[speed];

	if (E_InputCheckKey(key_down))
		forward -= forwardmove[speed];

	// -KM- 1998/09/01 Analogue binding
	// -ACB- 1998/09/06 Forward Move Speed Control
	int i = GetSpeedDivisor(forwardmovespeed);
	forward -= analogue[AXIS_FORWARD] * forwardmove[speed] / i;

	// -ACB- 1998/09/06 Side Move Speed Control
	int j = GetSpeedDivisor(sidemovespeed);
	side += analogue[AXIS_STRAFE] * sidemove[speed] / j;

	if (E_InputCheckKey(key_straferight))
		side += sidemove[speed];

	if (E_InputCheckKey(key_strafeleft))
		side -= sidemove[speed];

	// buttons
	cmd->chatchar = HU_DequeueChatChar();

	if (E_InputCheckKey(key_fire))
		cmd->buttons |= BT_ATTACK;

	if (E_InputCheckKey(key_use))
		cmd->buttons |= BT_USE;

	if (E_InputCheckKey(key_jump))
		cmd->extbuttons |= EBT_JUMP;

	if (E_InputCheckKey(key_secondatk))
		cmd->extbuttons |= EBT_SECONDATK;

	if (E_InputCheckKey(key_reload))
		cmd->extbuttons |= EBT_RELOAD;

	// -KM- 1998/11/25 Weapon change key
	for (int w = 0; w < WEAPON_KEYS; w++)
	{
		if (E_InputCheckKey('0' + w))
		{
			cmd->buttons |= BT_CHANGE;
			cmd->buttons |= w << BT_WEAPONSHIFT;
			break;
		}
	}

	if (E_InputCheckKey(key_nextweapon))
	{
		cmd->buttons |= BT_CHANGE;
		cmd->buttons |= (BT_NEXT_WEAPON << BT_WEAPONSHIFT);
	}
	else if (E_InputCheckKey(key_prevweapon))
	{
		cmd->buttons |= BT_CHANGE;
		cmd->buttons |= (BT_PREV_WEAPON << BT_WEAPONSHIFT);
	}

	// -MH- 1998/08/18 Yep. More flying controls...
	if (upward > MAXPLMOVE)
		upward = MAXPLMOVE;
	else if (upward < -MAXPLMOVE)
		upward = -MAXPLMOVE;

	if (forward > MAXPLMOVE)
		forward = MAXPLMOVE;
	else if (forward < -MAXPLMOVE)
		forward = -MAXPLMOVE;

	if (side > MAXPLMOVE)
		side = MAXPLMOVE;
	else if (side < -MAXPLMOVE)
		side = -MAXPLMOVE;

	cmd->upwardmove  += upward;
	cmd->forwardmove += forward;
	cmd->sidemove    += side;

	// -KM- 1998/09/01 Analogue binding
	Z_Clear(analogue, int, 5);
}

//
// INP_Responder  
//
// Get info needed to make ticcmd_ts for the players.
// 
bool INP_Responder(event_t * ev)
{
	switch (ev->type)
	{
		case ev_keydown:

			if (ev->value.key < NUMKEYS)
				gamekeydown[ev->value.key] |= GK_DOWN;

			// eat key down events 
			return true;

		case ev_keyup:
			if (ev->value.key < NUMKEYS)
				gamekeydown[ev->value.key] |= GK_UP;

			// always let key up events filter down 
			return false;

			// -KM- 1998/09/01 Change mouse/joystick to analogue
		case ev_analogue:
			{
				// -AJA- 1999/07/27: Mlook key like quake's.
				if (level_flags.mlook && E_InputCheckKey(key_mlook))
				{
					if (ev->value.analogue.axis == mouse_xaxis)
					{
						analogue[AXIS_TURN] += ev->value.analogue.amount;
						return true;
					}
					if (ev->value.analogue.axis == mouse_yaxis)
					{
						analogue[AXIS_MLOOK] += ev->value.analogue.amount;
						return true;
					}
				}

				analogue[ev->value.analogue.axis] += ev->value.analogue.amount;
				return true;  // eat events
			}

		default:
			break;
	}

	return false;
}

//
// E_SetTurboScale
//
// Sets the turbo scale (100 is normal)
//
void E_SetTurboScale(int scale)
{
	const int origforwardmove[2] = {0x19, 0x32};
	const int origupwardmove[2] = {0x19, 0x32};
	const int origsidemove[2] = {0x18, 0x28};

	upwardmove[0]  = origupwardmove[0] * scale / 100;
	upwardmove[1]  = origupwardmove[1] * scale / 100;
	forwardmove[0] = origforwardmove[0] * scale / 100;
	forwardmove[1] = origforwardmove[1] * scale / 100;
	sidemove[0]    = origsidemove[0] * scale / 100;
	sidemove[1]    = origsidemove[1] * scale / 100;
}

//
// E_ClearInput
//
void E_ClearInput(void)
{
	Z_Clear(gamekeydown, byte, NUMKEYS);
	Z_Clear(analogue, int, 5);
}

//
// E_UpdateKeyState
//
// Finds all keys in the gamekeydown[] array which have been released
// and clears them.  The value is NOT cleared by INP_Responder() since
// that prevents very fast presses (also the mousewheel) from being
// down long enough to be noticed by E_BuildTiccmd().
//
// -AJA- 2005/02/17: added this.
//
void E_UpdateKeyState(void)
{
	for (int k = 0; k < NUMKEYS; k++)
		if (gamekeydown[k] & GK_UP)
			gamekeydown[k] = 0;
}
