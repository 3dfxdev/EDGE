//----------------------------------------------------------------------------
//  EDGE Game Handling Code
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
#include "m_swap.h"
#include "m_random.h"
#include "hu_stuff.h"
#include "p_bot.h"
#include "p_local.h"
#include "p_saveg.h"
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

#define SAVEGAMESIZE    0x50000
#define SAVESTRINGSIZE  24

bool G_CheckDemoStatus(void);
void G_ReadDemoTiccmd(ticcmd_t * cmd);
void G_WriteDemoTiccmd(ticcmd_t * cmd);

static void G_DoReborn(player_t *p);

void G_DoLoadLevel(void);
void G_DoNewGame(void);
void G_DoLoadGame(void);
void G_DoPlayDemo(void);
void G_DoCompleted(void);
void G_DoVictory(void);
void G_DoSaveGame(void);

gameaction_e gameaction = ga_nothing;
gamestate_e gamestate = GS_NOTHING;
skill_t gameskill = sk_invalid;

bool paused = false;

// ok to save / end game 
bool usergame;

// send a pause event next tic 
static bool sendpause = false;

// send a save event next tic 
static bool sendsave = false;

// if true, exit with report on completion 
static bool timingdemo;

// for comparative timing purposes 
bool nodrawers;
bool noblit;
static int starttime;

// -KM- 1998/11/25 Exit time is the time when the level will actually finish
// after hitting the exit switch/killing the boss.  So that you see the
// switch change or the boss die.

int exittime = 0x7fffffff;
bool exit_skipall = false;  // -AJA- temporary (maybe become "exit_mode")

bool viewactive = false;

// GAMEPLAY MODES:
//
//   netgame  deathmatch   mode
//   --------------------------------------
//     N         0         single player
//     Y         0         coop
//     -         1         deathmatch
//     -         2         altdeath

// only if started as net death
int deathmatch;

// only true if packets are broadcast 
bool netgame;

player_t *players = NULL;
player_t **playerlookup = NULL;

// player taking events and displaying 
player_t *consoleplayer;

// view being displayed 
player_t *displayplayer;

int gametic;

// for intermission
int totalkills, totalitems, totalsecret;

static long random_seed;
bool demorecording;
bool demoplayback;
bool netdemo;
bool newdemo;

// 98-7-10 KM Remove maxdemo limit
static const byte *playdemobuffer = NULL;
static FILE *demofile = NULL;
static int demo_p;
// -ES- 2000/01/28 Added.
static int demo_length;
int maxdemo;

// quit after playing a demo from cmdline 
bool singledemo;

// if true, load all graphics at start 
bool precache = true;

// parms for world map / intermission 
wbstartstruct_t wminfo;

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
int key_jump;
int key_map;
int key_180;
int key_talk;
int key_mlook;
int key_secondatk;

// -MH- 1998/07/10 Flying keys
int key_flyup;
int key_flydown;

#define MAXPLMOVE  (forwardmove[1])

#define TURBOTHRESHOLD  0x32

static int forwardmove[2] = {0x19, 0x32};
static int upwardmove[2]  = {0x19, 0x32};  // -MH- 1998/08/18 Up/Down movement
static int sidemove[2]    = {0x18, 0x28};
static int angleturn[3]   = {640, 1280, 320};  // + slow turn 

#define ZOOM_ANGLE_DIV  3

#define SLOWTURNTICS    6

#define NUMKEYS         512

static bool gamekeydown[NUMKEYS];
int turnheld;  // for accelerative turning 

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

fixed_t mlookspeed = 1000 / 64;

// -ACB- 1999/09/30 Has to be true or false - bool-ified
bool invertmouse = false;

// -ACB- 2004/05/25 We need to store our current gamedef
const gamedef_c* currgamedef = NULL;

// -ACB- 2004/05/25 We need to store our current/next mapdefs
const mapdef_c* currmap = NULL;
const mapdef_c* nextmap = NULL;

//--------------------------------------------

static int loadgame_slot;
static int savegame_slot;
static char savedescription[32];

#define BODYQUESIZE     32

mobj_t *bodyque[BODYQUESIZE];
int bodyqueslot;

static const mapdef_c *d_newmap = NULL;
static const gamedef_c *d_gamedef = NULL;
static skill_t d_newskill;
static bool d_newwarp;

// -KM- 1998/09/01 Made static.
static int CheckKey(int keynum)
{
#ifdef DEVELOPERS
	if ((keynum >> 16) > NUMKEYS)
		I_Error("Invalid key!");
	else if ((keynum & 0xffff) > NUMKEYS)
		I_Error("Invalid key!");
#endif

	if (gamekeydown[keynum >> 16])
		return true;
	else if (gamekeydown[keynum & 0xffff])
		return true;
	else
		return false;
}

//
// Writes data to the demo
//
// -ES- 1999/10/17 Added.
//
static void WriteToDemo(const void *src, int length)
{
	fwrite(src,1,length,demofile);
	// This will make sure that also the last bytes are written at a crash
	fflush(demofile);
}

//
// Stores a single byte to the demo.
//
static void WriteByteToDemo(byte c)
{
	WriteToDemo(&c, 1);
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
// G_BuildTiccmd
//
// Builds a ticcmd from all of the available inputs
//
// -ACB- 1998/07/02 Added Vertical angle checking for mlook.
// -ACB- 1998/07/10 Reformatted: I can read the code! :)
// -ACB- 1998/09/06 Apply speed controls to -KM-'s analogue controls
// -AJA- 1999/08/10: Reworked the GetSpeedDivisor macro.
//
#define G_DefineGetSpeedDivisor(speed) \
	(((speed) == 8) ? 1 : (8 - (speed)) << 4)

void G_BuildTiccmd(ticcmd_t * cmd)
{
	int i;
	bool strafe;
	float vertslope;  // -ACB- 1998/07/02 Look angle
#ifdef MOUSE_ACC
	// Define MOUSE_ACC to get smoother movements
	// These values are first added to the movements, and then
	// 66% of the total movement is subtracted from the actual movement,
	// & added to these.
	static float slope_acc = 0;
	static angle_t angle_acc = 0;
#endif

	int speed;
	int tspeed;
	int forward;
	int upward;  // -MH- 1998/08/18 Fly Up/Down movement

	int side;
	// -ACB- 1999/09/20 Removed. base tic is zero-ed out ticcmd.
	//  ticcmd_t *base;
	static bool allow180 = true;
	static bool allowzoom = true;
	static bool allowautorun = true;

	// -ACB- 1999/09/20 Removed. base tic is zero-ed out ticcmd.
	//base = I_BaseTiccmd();  // empty, or external driver
	Z_Clear(cmd, ticcmd_t, 1);

	// -KM- 1998/12/21 If a drone player, do not accept input.
	if (drone)
		return;

	vertslope = 0;

	strafe = CheckKey(key_strafe)?true:false;
	speed = CheckKey(key_speed);

	if (autorunning)
		speed = !speed;

	upward = forward = side = 0;

	//
	// -KM- 1998/09/01 use two stage accelerative turning on all devices
	//
	// -ACB- 1998/09/06 Allow stage turning to be switched off for
	//                  analogue devices...
	//
	if (CheckKey(key_right) || CheckKey(key_left) || (analogue[AXIS_TURN] && stageturn))
		turnheld += ticdup;
	else
		turnheld = 0;

	// slow turn ?
	if (turnheld < SLOWTURNTICS)
		tspeed = 2;
	else
		tspeed = speed;

	// -ES- 1999/03/28 Zoom Key
	if (CheckKey(key_zoom))
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
	if (CheckKey(key_autorun))
	{
		if (allowautorun)
		{
			autorunning  = !autorunning;
			allowautorun = false;
		}
	}
	else
		allowautorun = true;

	if (level_flags.mlook)
	{
		fixed_t mlook_rate = mlookspeed;

		// -ACB- 1998/07/02 Use VertAngle for Look/up down.
		if (CheckKey(key_lookup))
			vertslope += (float)mlook_rate / 1024.0f;

		// -ACB- 1998/07/02 Use VertAngle for Look/up down.
		if (CheckKey(key_lookdown))
			vertslope -= (float)mlook_rate / 1024.0f;

		if (viewiszoomed)
			mlook_rate /= ZOOM_ANGLE_DIV;

		// -ACB- 1998/07/02 Use CENTER flag to center the vertical look.
		if (CheckKey(key_lookcenter))
			cmd->extbuttons |= EBT_CENTER;

		// -KM- 1998/09/01 More analogue binding
		vertslope += M_FixedToFloat(analogue[AXIS_MLOOK] * mlook_rate);
	}

	// You have to release the 180 deg turn key before you can press it again
	if (CheckKey(key_180))
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
		if (CheckKey(key_right))
			side += sidemove[speed];

		if (CheckKey(key_left))
			side -= sidemove[speed];

		// -KM- 1998/09/01 Analogue binding
		// -ACB- 1998/09/06 Side Move Speed Control
		i = G_DefineGetSpeedDivisor(sidemovespeed);
		side += analogue[AXIS_TURN] * sidemove[speed] / i;
	}
	else
	{
		int angle_rate = angleturn[tspeed];

		if (viewiszoomed)
			angle_rate /= ZOOM_ANGLE_DIV;

		if (CheckKey(key_right))
			cmd->angleturn -= angle_rate;

		if (CheckKey(key_left))
			cmd->angleturn += angle_rate;

		// -KM- 1998/09/01 Analogue binding
		// -ACB- 1998/09/06 Angle Turn Speed Control
		i = G_DefineGetSpeedDivisor(angleturnspeed);
		cmd->angleturn -= analogue[AXIS_TURN] * angle_rate / i;
	}

	// -MH- 1998/08/18 Fly up
	if (level_flags.true3dgameplay)
	{
		if ((CheckKey(key_flyup)))
			upward += upwardmove[speed];

		// -MH- 1998/08/18 Fly down
		if ((CheckKey(key_flydown)))
			upward -= upwardmove[speed];

		i = G_DefineGetSpeedDivisor(forwardmovespeed);
		upward += analogue[AXIS_FLY] * upwardmove[speed] / i;
	}

	if (CheckKey(key_up))
		forward += forwardmove[speed];

	if (CheckKey(key_down))
		forward -= forwardmove[speed];

	// -KM- 1998/09/01 Analogue binding
	// -ACB- 1998/09/06 Forward Move Speed Control
	i = G_DefineGetSpeedDivisor(forwardmovespeed);
	forward -= analogue[AXIS_FORWARD] * forwardmove[speed] / i;

	// -ACB- 1998/09/06 Side Move Speed Control
	i = G_DefineGetSpeedDivisor(sidemovespeed);
	side += analogue[AXIS_STRAFE] * sidemove[speed] / i;

	if (CheckKey(key_straferight))
		side += sidemove[speed];

	if (CheckKey(key_strafeleft))
		side -= sidemove[speed];

	// buttons
	cmd->chatchar = HU_DequeueChatChar();

	if (CheckKey(key_fire))
		cmd->buttons |= BT_ATTACK;

	if (CheckKey(key_use))
		cmd->buttons |= BT_USE;

	if (CheckKey(key_jump))
		cmd->extbuttons |= EBT_JUMP;

	if (CheckKey(key_secondatk))
		cmd->extbuttons |= EBT_SECONDATK;

	// -KM- 1998/11/25 Weapon change key
	for (i = 0; i < 10; i++)
	{
		if (CheckKey('0' + i))
		{
			cmd->buttons |= BT_CHANGE;
			cmd->buttons |= i << BT_WEAPONSHIFT;
			break;
		}
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

	cmd->upwardmove += upward;
	cmd->forwardmove += forward;
	cmd->sidemove += side;

#ifdef MOUSE_ACC
	cmd->angleturn += angle_acc;
	angle_acc = cmd->angleturn * 2/3;
	if (angle_acc < 64)
		// disable acc at very small angles (0.35 deg)
		angle_acc = 0;
	cmd->angleturn -= angle_acc;

	vertslope += slope_acc;
	slope_acc = vertslope * 2/3;
	if (slope_acc < 64 * 2 * M_PI / 65536.0f)
		// disable acc at very small angles (0.35 deg)
		slope_acc = 0;
	vertslope -= slope_acc;
#endif

	if (vertslope != 0)
	{
		cmd->extbuttons |= EBT_MLOOK;

		if (vertslope > 0.5f)
			vertslope = 0.5f;
		else if (vertslope < -0.5f)
			vertslope = -0.5f;

		cmd->vertslope = (signed char)(vertslope * 254);
	}

	// special buttons
	if (sendpause)
	{
		sendpause = false;
		cmd->buttons = BT_SPECIAL | BTS_PAUSE;
	}

	if (sendsave)
	{
		sendsave = false;

#if 0  // -AJA- FIXME: doesn't handle save_pages
		if (netgame)
			cmd->buttons = BT_SPECIAL | BTS_SAVEGAME | (savegame_slot << BTS_SAVESHIFT);
		else
#endif
			gameaction = ga_savegame;
	}

	// -KM- 1998/09/01 Analogue binding
	Z_Clear(analogue, int, 5);
}

//
// G_DoLoadLevel 
//
void G_DoLoadLevel(void)
{
	player_t *p;

	if (currmap == NULL)
		I_Error("G_DoLoadLevel: No Current Map selected");

	// Set the sky map.
	//
	// First thing, we have a dummy sky texture name, a flat. The data is
	// in the WAD only because we look for an actual index, instead of simply
	// setting one.
	//
	// -ACB- 1998/08/09 Reference current map for sky name.

	sky_image = W_ImageFromTexture(currmap->sky);

	if (wipegamestate == GS_LEVEL)
		wipegamestate = GS_NOTHING;  // force a wipe

	// -AJA- need this for GLBSP plugin
	gamestate = GS_NOTHING;

	// -AJA- FIXME: this background camera stuff is a mess
	background_camera_mo = NULL;
	R_ExecuteSetViewSize();

	for (p = players; p; p = p->next)
	{
		if (p->playerstate == PST_DEAD ||
			(currmap->force_on & MPF_ResetPlayer))
		{
			p->playerstate = PST_REBORN;
		}

		p->frags = 0;
	}

	// -KM- 1998/12/16 Make map flags actually do stuff.
	// -AJA- 2000/02/02: Made it more generic.

#define HANDLE_FLAG(var, specflag)  \
	if (currmap->force_on & (specflag))  \
	(var) = true;  \
	else if (currmap->force_off & (specflag))  \
	(var) = false;

	HANDLE_FLAG(level_flags.jump, MPF_Jumping);
	HANDLE_FLAG(level_flags.crouch, MPF_Crouching);
	HANDLE_FLAG(level_flags.mlook, MPF_Mlook);
	HANDLE_FLAG(level_flags.itemrespawn, MPF_ItemRespawn);
	HANDLE_FLAG(level_flags.fastparm, MPF_FastParm);
	HANDLE_FLAG(level_flags.true3dgameplay, MPF_True3D);
	HANDLE_FLAG(level_flags.more_blood, MPF_MoreBlood);
	HANDLE_FLAG(level_flags.cheats, MPF_Cheats);
	HANDLE_FLAG(level_flags.trans, MPF_Translucency);
	HANDLE_FLAG(level_flags.respawn, MPF_Respawn);
	HANDLE_FLAG(level_flags.res_respawn, MPF_ResRespawn);
	HANDLE_FLAG(level_flags.have_extra, MPF_Extras);
	HANDLE_FLAG(level_flags.limit_zoom, MPF_LimitZoom);
	HANDLE_FLAG(level_flags.shadows, MPF_Shadows);
	HANDLE_FLAG(level_flags.halos, MPF_Halos);
	HANDLE_FLAG(level_flags.kicking, MPF_Kicking);

#undef HANDLE_FLAG

	if (currmap->force_on & MPF_BoomCompat)
		level_flags.compat_mode = CM_BOOM;
	else if (currmap->force_off & MPF_BoomCompat)
		level_flags.compat_mode = CM_EDGE;

	if (currmap->force_on & MPF_AutoAim)
	{
		if (currmap->force_on & MPF_AutoAimMlook)
			level_flags.autoaim = AA_MLOOK;
		else
			level_flags.autoaim = AA_ON;
	}
	else if (currmap->force_off & MPF_AutoAim)
		level_flags.autoaim = AA_OFF;

	//
	// Note: It should be noted that only the gameskill is
	// passed as the level is already defined in currmap,
	// The method for changing currmap, is using by
	// G_DeferredInitNew.
	//
	// -ACB- 1998/08/09 New P_SetupLevel
	// -KM- 1998/11/25 P_SetupLevel accepts the autotag
	//
	RAD_ClearTriggers();

	P_SetupLevel(gameskill, currmap->autotag);

	RAD_SpawnTriggers(currmap->ddf.name);

	// -KM- 1998/12/21 If a drone player, the display player is already
	//   set up.
	if (!drone)
		displayplayer = consoleplayer;  // view the guy you are playing

	starttime = I_GetTime();
	exittime = 0x7fffffff;
	exit_skipall = false;

	gamestate = GS_LEVEL;
	gameaction = ga_nothing;

	CON_SetVisible( /* !!! showMessages?vs_minimal: */ vs_notvisible);
	CON_Printf("%s\n", currmap->ddf.name);

	// clear cmd building stuff
	Z_Clear(gamekeydown, bool, NUMKEYS);
	Z_Clear(analogue, int, 5);
	sendpause = sendsave = paused = false;

	viewactive = true;
}

//
// G_Responder  
//
// Get info needed to make ticcmd_ts for the players.
// 
bool G_Responder(event_t * ev)
{
	// 25-6-98 KM Allow spy mode for demos even in deathmatch
	if ((gamestate == GS_LEVEL) && (ev->type == ev_keydown) && 
		(ev->value.key == KEYD_F12) && (demoplayback || !deathmatch))
	{
		// spy mode 
		do
		{
			displayplayer = displayplayer->next;
			if (!displayplayer)
				displayplayer = players;
		}
		while (!displayplayer->in_game && displayplayer != consoleplayer);
		return true;
	}

	// any other key pops up menu if in demos
	if (gameaction == ga_nothing && !singledemo && (demoplayback || gamestate == GS_DEMOSCREEN))
	{
		if (ev->type == ev_keydown)
		{
			M_StartControlPanel();
			S_StartSound(NULL, sfx_swtchn);
			return true;
		}
		return false;
	}

	if (gamestate == GS_LEVEL)
	{
		if (ST_Responder(ev))
			return true;  // status window ate it 

		if (AM_Responder(ev))
			return true;  // automap ate it 
	}

	if (gamestate == GS_LEVEL)
	{
		if (HU_Responder(ev))
			return true;  // chat ate the event

		if (M_CheatResponder(ev))
			return true;  // cheat code at it
	}

	if (gamestate == GS_FINALE)
	{
		if (F_Responder(ev))
			return true;  // finale ate the event 
	}

	// -ES- Fixme: Clean up globals gamekeydown and analogue.
	switch (ev->type)
	{
		case ev_keydown:
			if (ev->value.key == KEYD_PAUSE)
			{
				sendpause = true;
				return true;
			}

			if (ev->value.key < NUMKEYS)
				gamekeydown[ev->value.key] = true;

			// eat key down events 
			return true;

		case ev_keyup:
			if (ev->value.key < NUMKEYS)
				gamekeydown[ev->value.key] = false;

			// always let key up events filter down 
			return false;

			// -KM- 1998/09/01 Change mouse/joystick to analogue
		case ev_analogue:
			{
				// -AJA- 1999/07/27: Mlook key like quake's.
				if (level_flags.mlook && CheckKey(key_mlook))
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
// G_Ticker
//
// Make ticcmd_ts for the players.
//
// -ACB- 1998/08/10 Use language object for language specifics.
//
void G_Ticker(void)
{
	int buf;
	ticcmd_t *cmd;
	player_t *p;

	// do player reborns if needed
	for (p = players; p; p = p->next)
	{
		if (p->playerstate == PST_REBORN)
			G_DoReborn(p);
	}

	if (exittime == leveltime)
	{
		gameaction = ga_completed;
		exittime = 0x7fffffff;
	}

	// do things to change the game state
	while (gameaction != ga_nothing)
	{
		switch (gameaction)
		{
			case ga_loadlevel:
				G_DoLoadLevel();
				break;
				
			case ga_newgame:
				G_DoNewGame();
				break;
				
			case ga_loadgame:
				G_DoLoadGame();
				break;
				
			case ga_savegame:
				G_DoSaveGame();
				break;
				
			case ga_playdemo:
				G_DoPlayDemo();
				break;
				
			case ga_completed:
				G_DoCompleted();
				break;
				
			case ga_briefing:
				F_StartFinale(&nextmap->f_pre, ga_loadnext);
				break;
				
			case ga_loadnext:
				currmap = nextmap;
				G_DoLoadLevel();
				break;
				
			case ga_screenshot:
				m_screenshot_required = true;
				gameaction = ga_nothing;
				break;
				
			case ga_nothing:
				break;
				
			default:
				I_Error("G_Ticker: Unknown gameaction %d", gameaction);
				break;
		}
	}

	// get commands, check consistency,
	// and build new consistency check
	buf = (gametic / ticdup) % BACKUPTICS;

	for (p = players; p; p = p->next)
	{
		cmd = &p->cmd;

		// -ES- FIXME: Change format of player_t->cmd?
		*cmd = p->netcmds[buf];

		if (demoplayback)
			G_ReadDemoTiccmd(cmd);

		if (demorecording)
			G_WriteDemoTiccmd(cmd);

		// check for turbo cheats
		if (cmd->forwardmove > TURBOTHRESHOLD
			&& !(gametic & 31) && ((gametic >> 5) & 3) == p->pnum)
		{
			CON_Printf(language["IsTurbo"], p->playername);
		}

		if (netgame && !netdemo && !(gametic % ticdup))
		{
			if (gametic > BACKUPTICS
				&& p->consistency[buf] != cmd->consistency)
			{
				I_Error("Consistency failure on player %d (%i should be %i)",
					p->pnum + 1, cmd->consistency, p->consistency[buf]);
			}
			if (p->mo)
				p->consistency[buf] = (int)p->mo->x;
			else
				p->consistency[buf] = P_ReadRandomState() & 0xff;
		}
	}
	// check for special buttons
	for (p = players; p; p = p->next)
	{
		if (!(p->cmd.buttons & BT_SPECIAL))
			continue;

		switch (p->cmd.buttons & BT_SPECIALMASK)
		{
			case BTS_PAUSE:
				paused = !paused;
				if (paused)
				{
					S_PauseMusic();
					S_PauseSounds();
				}
				else
				{
					S_ResumeMusic();
					S_ResumeSounds();
				}
				// explicit as probably killed the initial effect
				S_StartSound(NULL, sfx_swtchn);
				break;

#if 0  // -AJA- disabled for now
			case BTS_SAVEGAME:
				if (!savedescription[0])
					strcpy(savedescription, "NET GAME");
				savegame_slot =
					(p->cmd.buttons & BTS_SAVEMASK) >> BTS_SAVESHIFT;
				gameaction = ga_savegame;
				break;
#endif
		}
	}

	// do main actions
	switch (gamestate)
	{
		case GS_LEVEL:
			P_Ticker();
			ST_Ticker();
			AM_Ticker();
			HU_Ticker();
			RAD_Ticker();
			break;

		case GS_INTERMISSION:
			WI_Ticker();
			break;

		case GS_FINALE:
			F_Ticker();
			break;

		case GS_DEMOSCREEN:
			E_PageTicker();
			break;

		case GS_NOTHING:
			break;
	}
}

//
// PLAYER STRUCTURE FUNCTIONS
// also see P_SpawnPlayer in P_Things
//


//
// G_PlayerFinishLevel
//
// Called when a player completes a level.
//
static void G_PlayerFinishLevel(player_t *p)
{
	int i;

	for (i = 0; i < NUMPOWERS; i++)
		p->powers[i] = 0;

	p->cards = KF_NONE;

	p->mo->flags &= ~MF_FUZZY;  // cancel invisibility 

	p->extralight = 0;  // cancel gun flashes 

	// cancel colourmap effects
	p->effect_colourmap = NULL;
	p->effect_infrared = false;

	// no palette changes 
	p->damagecount = 0;
	p->bonuscount  = 0;
	p->grin_count  = 0;
}

//
// G_PlayerReborn
//
// Called after a player dies. 
// almost everything is cleared and initialised.
//
void G_PlayerReborn(player_t *p, const mobjtype_c *info)
{
	bool in_game;
	player_t *next, *prev;
	void *data;
	void (*thinker)(const player_t *, void *, ticcmd_t *);

	int frags;
	int totalfrags;
	int killcount;
	int itemcount;
	int secretcount;
	int pnum;

	frags = p->frags;
	totalfrags = p->totalfrags;
	killcount = p->killcount;
	itemcount = p->itemcount;
	secretcount = p->secretcount;
	thinker = p->thinker;
	data = p->data;
	prev = p->prev;
	next = p->next;

#if 0  // -AJA- 2004/04/14: use DDF entry from level thing
	info = DDF_MobjLookupPlayer(p->pnum+1);
#endif

	in_game = p->in_game;
	pnum = p->pnum;

	Z_Clear(p, player_t, 1);

	p->pnum = pnum;
	p->in_game = in_game;

	p->frags = frags;
	p->totalfrags = totalfrags;
	p->killcount = killcount;
	p->itemcount = itemcount;
	p->secretcount = secretcount;
	p->thinker = thinker;
	p->data = data;
	p->prev = prev;
	p->next = next;

	// don't do anything immediately 
	p->usedown = p->attackdown = false;

	p->playerstate = PST_LIVE;

	P_GiveInitialBenefits(p, info);
}

//
// G_CheckSpot  
//
// Returns false if the player cannot be respawned at the given spot
// because something is occupying it 
//
static bool G_CheckSpot(player_t *player, const spawnpoint_t *point)
{
	float x, y, z;
	player_t *p;

	if (!player->mo)
	{
		// first spawn of level, before corpses
		for (p = players; p != player; p = p->next)
		{
			if (p->mo->x == point->x && p->mo->y == point->y)
				return false;
		}
		return true;
	}

	x = point->x;
	y = point->y;
	z = point->z;

	if (!P_CheckAbsPosition(player->mo, x, y, z))
		return false;

	// flush an old corpse if needed 
	if (bodyqueslot >= BODYQUESIZE)
		P_RemoveMobj(bodyque[bodyqueslot % BODYQUESIZE]);
	bodyque[bodyqueslot % BODYQUESIZE] = player->mo;
	bodyqueslot++;

	// spawn a teleport fog 
	// (temp fix for teleport effect)
	x += 20 * M_Cos(point->angle);
	y += 20 * M_Sin(point->angle);
	P_MobjCreateObject(x, y, z, mobjtypes.Lookup("TELEPORT FLASH"));

	return true;
}

//
// G_DeathMatchSpawnPlayer 
//
// Spawns a player at one of the random death match spots
// called at level load and each death 
//
void G_DeathMatchSpawnPlayer(player_t *p)
{
	epi::array_iterator_c it;
	spawnpoint_t *sp;
	
	int i, j;
	int begin;

	if (p->pnum >= dm_starts.GetSize())
		I_Warning("Few deathmatch spots, %d recommended.\n", p->pnum + 1);

	if (dm_starts.GetSize())
	{
		begin = P_Random() % dm_starts.GetSize();

		for (j = 0; j < dm_starts.GetSize(); j++)
		{
			i = (begin + it.GetPos()) % dm_starts.GetSize();

			sp = dm_starts[i];
			if (G_CheckSpot(p, sp))
			{
				P_SpawnPlayer(p, sp);
				return;
			}
		}

		// no good spot, so the player will probably get stuck
		if (playerstarts[p->pnum].info)
			P_SpawnPlayer(p, &playerstarts[p->pnum]);
		else
			P_SpawnPlayer(p, dm_starts[begin]);
	}
	else
	{
		// No deathmatch spawn exists.
		if (playerstarts[p->pnum].info)
		{
			P_SpawnPlayer(p, &playerstarts[p->pnum]);
		}
		else
		{
			for (i = 0; i < MAXPLAYERS; i++)
			{
				if (playerstarts[i].info)
				{
					P_SpawnPlayer(p, &playerstarts[i]);
					break;
				}
			}

			// no player or deathmatch starts could be found.
			if (i == MAXPLAYERS)
				I_Error("No player starts found!");
		}
	}
}

//
// G_DoReborn
// 
static void G_DoReborn(player_t *p)
{
	int i;

	// single player ?
	if (!(netgame || deathmatch))
	{
		// -AJA- 2003/10/09: we should only get here after the player
		//       died, i.e. not through other uses of PST_REBORN that
		//       appear in this file.  FIXME: this is confusing !

		if (gamestate == GS_LEVEL)
			gameaction = ga_loadlevel;
	}
	else
	{
		// respawn at the start

		// first dissasociate the corpse 
		p->mo->player = NULL;

		// spawn at random spot if in death match 
		if (deathmatch)
		{
			G_DeathMatchSpawnPlayer(p);
			return;
		}

		if (playerstarts[p->pnum].info == NULL)
			I_Error("Missing player %d start !\n", p->pnum+1);

		if (G_CheckSpot(p, &playerstarts[p->pnum]))
		{
			P_SpawnPlayer(p, &playerstarts[p->pnum]);
			return;
		}

		I_Warning("Player %d start is invalid.\n", p->pnum+1);

		// try to spawn at one of the other players spots
		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (playerstarts[i].info == NULL)
				continue;

			if (G_CheckSpot(p, &playerstarts[i]))
			{
				P_SpawnPlayer(p, &playerstarts[i]);
				return;
			}
		}

		// he's going to be inside something.  Too bad.
		P_SpawnPlayer(p, &playerstarts[p->pnum]);
	}
}

void G_ScreenShot(void)
{
	gameaction = ga_screenshot;
}

// -KM- 1998/11/25 Added time param which is the time to wait before
//  actually exiting level.
void G_ExitLevel(int time)
{
	nextmap = game::LookupMap(currmap->nextmapname);
	exittime = leveltime + time;
	exit_skipall = false;
}

// -ACB- 1998/08/08 We don't have support for the german edition
//                  removed the check for map31.
void G_SecretExitLevel(int time)
{
	nextmap = game::LookupMap(currmap->secretmapname);
	exittime = leveltime + time;
	exit_skipall = false;
}

void G_ExitToLevel(char *name, int time, bool skip_all)
{
	nextmap = game::LookupMap(name);
	exittime = leveltime + time;
	exit_skipall = skip_all;
}

//
// G_DoCompleted 
//
void G_DoCompleted(void)
{
	player_t *p;

	gameaction = ga_nothing;

	for (p = players; p; p = p->next)
	{
		// take away cards and stuff
		G_PlayerFinishLevel(p);
	}

	if (automapactive)
		AM_Stop();

	// handle "no stat" levels
	if (currmap->wistyle == WISTYLE_None || exit_skipall)
	{
		viewactive = false;
		automapactive = false;

		G_WorldDone();
		return;
	}

	wminfo.level = currmap->ddf.name;
	wminfo.last = currmap;
	wminfo.next = nextmap;
	wminfo.maxkills = totalkills;
	wminfo.maxitems = totalitems;
	wminfo.maxsecret = totalsecret;
	wminfo.maxfrags = 0;
	wminfo.partime = currmap->partime;
	wminfo.pnum = consoleplayer->pnum;

	if (!wminfo.plyr)
		wminfo.plyr = Z_New(wbplayerstruct_t, MAXPLAYERS);

	// FIXME: can overflow ???
	for (p = players; p; p = p->next)
	{
		wminfo.plyr[p->pnum].in = p->in_game;
		wminfo.plyr[p->pnum].skills = p->killcount;
		wminfo.plyr[p->pnum].sitems = p->itemcount;
		wminfo.plyr[p->pnum].ssecret = p->secretcount;
		wminfo.plyr[p->pnum].stime = leveltime;
		wminfo.plyr[p->pnum].frags = p->frags;
		wminfo.plyr[p->pnum].totalfrags = p->totalfrags;
	}

	gamestate = GS_INTERMISSION;
	viewactive = false;
	automapactive = false;

	WI_Start(&wminfo);
}

//
// G_WorldDone 
//
void G_WorldDone(void)
{
	if (exit_skipall && nextmap)
	{
		gameaction = ga_loadnext;
		return;
	}

	F_StartFinale(&currmap->f_end, nextmap ? ga_briefing : ga_nothing);
}

//
// G_FileNameFromSlot
//
// Creates a savegame file name.
//
void G_FileNameFromSlot(epi::string_c& fn, int slot)
{
	fn.Format("%s%c%s%04d.%s", 
				savedir, DIRSEPARATOR, SAVEGAMEBASE, 
				slot + 1, SAVEGAMEEXT);

	return;
}

//
// G_InitFromSavegame
//
// Can be called by the startup code or the menu task. 
//
void G_LoadGame(int slot)
{
	loadgame_slot = slot;
	gameaction = ga_loadgame;
}

void G_DoLoadGame(void)
{
	const mapdef_c *tempmap;
	const gamedef_c *tempgamedef;
	epi::string_c fn;
	saveglobals_t *globs;
	int version;

	gameaction = ga_nothing;

#if 0  // DEBUGGING CODE
	SV_DumpSaveGame(loadgame_slot);
	return;
#endif

	// Try to open		
	G_FileNameFromSlot(fn, loadgame_slot);
	
	if (! SV_OpenReadFile(fn.GetString()))
	{
		I_Printf("LOAD-GAME: cannot open %s\n", fn.GetString());
		return;
	}
	
	if (! SV_VerifyHeader(&version) || ! SV_VerifyContents())
	{
		I_Printf("LOAD-GAME: Savegame is corrupt !\n");
		SV_CloseReadFile();
		return;
	}

	SV_BeginLoad();

	globs = SV_LoadGLOB();

	if (!globs)
		I_Error("LOAD-GAME: Bad savegame file (no GLOB)\n");

	// --- pull info from global structure ---

	tempmap = game::LookupMap(globs->level);
	if (! tempmap)
		I_Error("LOAD-GAME: No such map %s !  Check WADS\n", globs->level);

	tempgamedef = gamedefs.Lookup(tempmap->episode_name);
	if (!tempgamedef)
		I_Error("LOAD-GAME: No such episode/mod %s !  Check WADS\n", 
				tempmap->episode_name.GetString());

	gameskill = (skill_t) globs->skill;
	random_seed = globs->p_random;

	G_InitNew(gameskill, tempmap, tempgamedef, random_seed);

	G_DoLoadLevel();

	// -- Check LEVEL consistency (crc) --
	//
	// FIXME: ideally we shouldn't bomb out, just display an error box

	if (globs->mapsector.count != numsectors ||
		globs->mapsector.crc != mapsector_CRC ||
		globs->mapline.count != numlines ||
		globs->mapline.crc != mapline_CRC ||
		globs->mapthing.count != mapthing_NUM ||
		globs->mapthing.crc != mapthing_CRC)
	{
		I_Error("LOAD-GAME: Level data does not match !  Check WADs\n");
	}

	//!!! FIXME: Check DDF/RTS consistency (crc), warning only

	level_flags = globs->flags;
	level_flags.menu_grav = globs->gravity;

	leveltime   = globs->level_time;
	totalkills  = globs->total_kills;
	totalitems  = globs->total_items;
	totalsecret = globs->total_secrets;

	// con_player = globs->console_player;
	gameskill = (skill_t) globs->skill;
	netgame = globs->netgame?true:false;  /// FIXME: deathmatch var
	
	if (globs->sky_image)  // backwards compat (sky_image added 2003/12/19)
		sky_image = globs->sky_image;

	// clear line/sector lookup caches, in case level_flags.compat_mode
	// changed (e.g. CM_BOOM -> CM_EDGE).
	genlinetypes.Reset();
	gensectortypes.Reset();

	if (SV_LoadEverything() && SV_GetError() == 0)
	{
		/* all went well */ 
	}
	else
	{
		// something went horribly wrong...
		// FIXME (oneday) : show message & go back to title screen

		I_Error("Bad Save Game !\n");
	}

	SV_FreeGLOB(globs);

	SV_FinishLoad();
	SV_CloseReadFile();

	ST_Start();
	HU_Start();
}

//
// G_SaveGame
//
// Called by the menu task.
// Description is a 24 byte text string 
//
void G_SaveGame(int slot, const char *description)
{
	savegame_slot = slot;
	strcpy(savedescription, description);
	sendsave = true;
}

void G_DoSaveGame(void)
{
	epi::string_c fn;
	saveglobals_t *globs;
	time_t cur_time;
	char timebuf[100];

 	G_FileNameFromSlot(fn, savegame_slot);
	
	if (! SV_OpenWriteFile(fn.GetString(), (EDGEVER << 8) | EDGEPATCH))
	{
		//!!! do something
		return;
	}

	globs = SV_NewGLOB();

	// --- fill in global structure ---

	globs->game = Z_StrDup(currmap->episode_name);
	globs->level = Z_StrDup(currmap->ddf.name);
	globs->flags = level_flags;
	globs->gravity = level_flags.menu_grav;

	globs->level_time = leveltime;
	globs->p_random = P_ReadRandomState();
	globs->total_kills   = totalkills;
	globs->total_items   = totalitems;
	globs->total_secrets = totalsecret;

	globs->console_player = consoleplayer->pnum;
	globs->skill = gameskill;
	globs->netgame = netgame ? (1+deathmatch) : 0;
	globs->sky_image = sky_image;

	time(&cur_time);
	strftime(timebuf, 99, "%I:%M %p  %d/%b/%Y", localtime(&cur_time));

	if (timebuf[0] == '0' && isdigit(timebuf[1]))
		timebuf[0] = ' ';

	globs->description = Z_StrDup(savedescription);
	globs->desc_date   = Z_StrDup(timebuf);

	globs->mapsector.count = numsectors;
	globs->mapsector.crc = mapsector_CRC;
	globs->mapline.count = numlines;
	globs->mapline.crc = mapline_CRC;
	globs->mapthing.count = mapthing_NUM;
	globs->mapthing.crc = mapthing_CRC;

	// FIXME: store DDF CRC values too...

	SV_BeginSave();

	SV_SaveGLOB(globs);
	SV_SaveEverything();

	SV_FreeGLOB(globs);

	SV_FinishSave();
	SV_CloseWriteFile();

	savedescription[0] = 0;

	CON_Printf("%s", language["GameSaved"]);

	// draw the pattern into the back screen
	//   R_FillBackScreen();

	gameaction = ga_nothing;
}

//
// G_InitNew
//
// Can be called by the startup code or the menu task.
// consoleplayer, displayplayer, playeringame[] should
// be set.
//

//
// G_DeferredInitNew
//
// This is the procedure that changes the currmap
// at the start of the game and outside the normal
// progression of the game. All thats needed is the
// skill and the name (The name in the DDF File itself).
//
// Returns true if OK, or false if no such map exists.
//
bool G_DeferredInitNew(skill_t skill, const char *mapname, bool warpopt)
{
	d_newmap = game::LookupMap(mapname);

	if (!d_newmap)
		return false;

	// -ACB- 2004/07/01 Added to handle the current game definition changes
	d_gamedef = gamedefs.Lookup(d_newmap->episode_name);
	if (!d_gamedef)
		return false;

	d_newskill = skill;
	d_newwarp = warpopt;			// this is true only when called by -warp option

	gameaction = ga_newgame;
	return true;
}

void G_DoNewGame(void)
{
	player_t *p;

	demoplayback = false;

	if (d_newwarp)
	{
		if (netdemo)
		{
			deathmatch = netdemo = netgame = false;
			consoleplayer = players;

			// !!! FIXME: this is wrong
			for (p = players->next; p; p = p->next)
				p->in_game = false;
		}

		level_flags.fastparm = false;
		level_flags.nomonsters = false;
	}

	quickSaveSlot = -1;

	G_InitNew(d_newskill, d_newmap, d_gamedef, I_PureRandom());
	gameaction = ga_nothing;

	// -AJA- 2003/10/09: support for pre-level briefing screen on first map.
	//       FIXME: kludgy. All this game logic desperately needs rethinking.

	F_StartFinale(&currmap->f_pre, ga_loadlevel);
}

//
// G_InitNew
//
// -ACB- 1998/07/12 Removed Lost Soul/Spectre Ability stuff
// -ACB- 1998/08/10 Inits new game without the need for gamemap or episode.
// -ACB- 1998/09/06 Removed remarked code.
// -KM- 1998/12/21 Added mapdef param so no need for defered init new
//   which was conflicting with net games.
//
void G_InitNew(skill_t skill, const mapdef_c *map, const gamedef_c *gamedef, long seed)
{
	player_t *p;

	for (p = players; p; p = p->next)
		memset(p->consistency, -1, sizeof(p->consistency));

	if (paused)
	{
		paused = false;
		S_ResumeMusic(); // -ACB- 1999/10/07 New Music API
		S_ResumeSounds();  // -ACB- 1999/10/17 New Sound API
	}

	currgamedef = gamedef;
	currmap = map;

	if (skill > sk_nightmare)
		skill = sk_nightmare;

	random_seed = seed;
	P_WriteRandomState(seed);

	// we can not call this until we have the random seed.
	if (demorecording)
		G_BeginRecording();

	// force players to be initialised upon first level load         
	// -AJA- 2003/10/09: except that this doesn't do anything, as the
	//                   playerstate is set to PST_LIVE In P_SpawnPlayer !!
	for (p = players; p; p = p->next)
		p->playerstate = PST_REBORN;

	usergame = true;  // will be set false if a demo 

	demoplayback = false;
	automapactive = false;
	gameskill = skill;

	// copy global flags into the level-specific flags
	level_flags = global_flags;

	if (skill == sk_nightmare)
	{
		level_flags.fastparm = true;
		level_flags.respawn = true;
#ifdef NO_NIGHTMARE_CHEATS
		level_flags.cheats = false;
#endif
	}
}

//
// DEMO RECORDING 
// 
#define DEMOMARKER              0x80

//
// G_ReadDemoTiccmd
//
// A demo file is essentially a stream of ticcmds: every tic,
// the ticcmd holds all the info for movement for a player on
// that tic. This means that a demo merely replays the movements
// and actions of the player.
//
// This function gets the actions from the recdemobuffer and gives
// them to ticcmd to be played out. Its worth a note that this
// is the reason demos desync when played on two different
// versions, since any alteration to the gameplay could give
// a different reaction to a player action and therefore the
// game is different to the original.
//  
void G_ReadDemoTiccmd(ticcmd_t * cmd)
{
	// 98-7-10 KM Demolimit removed
	if (demo_p >= demo_length)
	{
		// end of demo data stream
		G_CheckDemoStatus();
		return;
	}

	// -ACB- 1998/07/11 Added additional ticcmd stuff to demo
	// -MH-  1998/08/18 Added same for fly up/down
	//                  Keep all upward stuff before all forward stuff, to
	//                  keep consistent. Will break existing demos. Damn.
	*cmd = *(ticcmd_t *)&playdemobuffer[demo_p];
	demo_p += sizeof(ticcmd_t);
}

//
// G_WriteDemoTiccmd
//
// A demo file is essentially a stream of ticcmds: every tic,
// the ticcmd holds all the info for movement for a player on
// that tic. This means that a demo merely replays the movements
// and actions of the player.
//
// This function writes the ticcmd to the recdemobuffer and
// then get G_ReadDemoTiccmd to read it, so that whatever is
// recorded is played out. 
//
void G_WriteDemoTiccmd(ticcmd_t * cmd)
{
	// press q to end demo recording
	if (gamekeydown[(int)('q')])
		G_CheckDemoStatus();

	// -ACB- 1998/07/11 Added additional ticcmd stuff to demo
	// -MH-  1998/08/18 Added same for fly up/down
	//                  Keep all upward stuff before all forward stuff, to
	//                  keep consistent. Will break existing demos. Damn.
	WriteToDemo(cmd, sizeof(ticcmd_t));
}

//
// G_RecordDemo 
// 
// 98-7-10 KM Demolimit removed
//
void G_RecordDemo(const char *name)
{
	// assume demo name is less than 256 chars
	epi::string_c demoname;
		
	M_ComposeFileName(demoname, gamedir, name);
	demoname += ".lmp";	// FIXME!! Check extension has not been given
	
	usergame = false;
	maxdemo = 0x20000;

	// Write directly to file. Possibly a bit slower without disk cache, but
	// uses less memory, and the demo can record EDGE crashes.
	demofile = fopen(demoname.GetString(), "wb");	
	demorecording = true;
}

//
// G_BeginRecording
//
// -ACB- 1998/07/02 Changed the code to record as version 0.65 (065),
//                  All of the additional EDGE features are stored in
//                  the demo.
//
// -KM-  1998/07/10 Removed the demo limit.
//
// -ACB- 1998/07/12 Removed Lost Soul/Spectre Ability Check
//
void G_BeginRecording(void)
{
	int i;
	///  player_t *p;

	demo_p = 0;

	WriteByteToDemo(DEMOVERSION);

	if ((int)gameskill == -1)
		gameskill = startskill;

	//---------------------------------------------------------
	// -ACB- 1998/09/03 Record Level Name In Demo
	i = (int)strlen(currmap->ddf.name);
	WriteByteToDemo(i);
	WriteToDemo(currmap->ddf.name, i);
	L_WriteDebug("G_BeginRecording: %s\n", currmap->ddf.name);
	//---------------------------------------------------------

	WriteByteToDemo(gameskill);
	WriteByteToDemo(deathmatch);
	WriteByteToDemo(consoleplayer->pnum);
	WriteToDemo(&level_flags, sizeof(level_flags));

	///  for (p = players; p; p = p->next)
	///    WriteByteToDemo(p->in_game);

	i = LONG(random_seed);
	WriteToDemo(&i, 4);
}

//
// G_SetTurboScale
//
// Sets the turbo scale (100 is normal)
void G_SetTurboScale(int scale)
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
// G_PlayDemo 
//
epi::strent_c defdemoname;

void G_DeferredPlayDemo(const char *name)
{
	defdemoname.Set(name);
	gameaction = ga_playdemo;
}

//
// G_DoPlayDemo
// Sets up the system to play a demo.
//
// -ACB- 1998/07/02 Change the code only to play version 0.65 demos.
// -KM-  1998/07/10 Displayed error message on screen and make demos limitless
// -ACB- 1998/07/12 Removed Lost Soul/Spectre Ability Check
// -ACB- 1998/07/12 Removed error message (became bloody annoying...)
//
void G_DoPlayDemo(void)
{
	skill_t skill;
	int i,j;
	int demversion;
	char mapname[30];
	const gamedef_c *newgamedef;
	const mapdef_c *newmap;
	long random_seed;
	///  player_t *p;

	gameaction = ga_nothing;
	playdemobuffer = (const byte*)W_CacheLumpName(defdemoname);
	demo_p = 0;
	demversion = playdemobuffer[demo_p++];

	// -ES- 1999/10/17 Allow cut off demos: Add a demo marker if it doesn't exist.
	demo_length = W_LumpLength(W_GetNumForName(defdemoname));
	if (demo_length < 16)
		// no real demo could be smaller than 16 bytes
		I_Error("Demo '%s' is too small!", defdemoname.GetString());
	if (playdemobuffer[demo_length-1] != DEMOMARKER)
		I_Warning("Warning: Demo has no end marker! It might be corrupt.\n");
	else
		demo_length--;

	if (demversion != DEMOVERSION)
	{
		gameaction = ga_nothing;
		return;
	}
	else
	{
		//------------------------------------------------------
		// -ACB- 1998/09/03 Read the Level Name from the demo.
		i = playdemobuffer[demo_p++];

		for (j = 0; j < i; j++)
			mapname[j] = playdemobuffer[demo_p + j];
		mapname[i] = 0;

		demo_p += i;
		//------------------------------------------------------

		skill = (skill_t) playdemobuffer[demo_p++];
		deathmatch = playdemobuffer[demo_p++];
		consoleplayer = playerlookup[playdemobuffer[demo_p++]];

		level_flags = *(gameflags_t *)&playdemobuffer[demo_p];
		demo_p += sizeof(level_flags);

		///    for (p = players; p; p = p->next)
		///      p->in_game = playdemobuffer[demo_p++];

		// -ES- 2000/02/04 Random seed
		random_seed = LONG(*(long*)&playdemobuffer[demo_p]);
		demo_p += 4;
	}

	//----------------------------------------------------------------
	// -ACB- 1998/09/03 Setup the given mapname; fail if map does not
	// exist.
	newmap = game::LookupMap(mapname);
	if (newmap == NULL)
	{
		gameaction = ga_nothing;
		return;
	}

	newgamedef = gamedefs.Lookup(newmap->episode_name);
	if (newgamedef == NULL)
	{
		gameaction = ga_nothing;
		return;
	}


	//----------------------------------------------------------------

	if (players->next && players->next->in_game)
	{
		netgame = true;
		netdemo = true;
	}

	// don't spend a lot of time in loadlevel
	precache = false;
	
	G_InitNew(skill, newmap, newgamedef, random_seed);
	G_DoLoadLevel();
	
	precache = true;
	usergame = false;
	demoplayback = true;
}

//
// G_TimeDemo 
//
void G_TimeDemo(const char *name)
{
	nodrawers = M_CheckParm("-nodraw")?true:false;
	noblit = M_CheckParm("-noblit")?true:false;
	timingdemo = true;
	singletics = true;

	defdemoname.Set(name);
	gameaction = ga_playdemo;
}

// 
// G_CheckDemoStatus 
//
//Called after a death or level completion to allow demos to be cleaned up, 
//Returns true if a new demo loop action will take place 
// 
//-KM- 1998/07/10 Reformed code for limitless demo
//
bool G_CheckDemoStatus(void)
{
	int endtime;
	player_t *p;

	if (timingdemo)
	{
		float fps;

		endtime = I_GetTime();
		fps = ((float)(gametic * TICRATE)) / (endtime - starttime);
		I_Error("timed %i gametics in %i realtics, which equals %f fps", gametic,
			endtime - starttime, fps);
	}

	if (demoplayback)
	{
		if (singledemo)
		{
			// -ACB- 1999/09/20 New code order, shutdown system then close program.
			I_SystemShutdown();
			I_CloseProgram(0);
		}

		W_DoneWithLump(playdemobuffer);
		demoplayback = false;
		netdemo = false;
		netgame = false;
		deathmatch = false;

		//!!! FIXME: this is wrong
		for (p = players; p; p = p->next)
			p->in_game = false;

		level_flags.fastparm = false;
		level_flags.nomonsters = false;
		consoleplayer = playerlookup[0];
		E_AdvanceDemo();
		return true;
	}

	if (demorecording)
	{
		// Finish it
		WriteByteToDemo(DEMOMARKER);
		// Finish the demo file:
		fclose(demofile);
		demofile = NULL;
		I_Error("Demo recorded");

		demorecording = false;
	}

	return false;
}

//
// G_CheckWhenAppear
//
bool G_CheckWhenAppear(when_appear_e appear)
{
	if (! (appear & (1 << gameskill)))
		return false;

	if (!netgame && !deathmatch && !(appear & WNAP_Single))
		return false;

	if (netgame && !deathmatch && !(appear & WNAP_Coop))
		return false;

	if (deathmatch && !(appear & WNAP_DeathMatch))
		return false;

	return true;
}

//
// G_CheckConditions
//
bool G_CheckConditions(mobj_t *mo, condition_check_t *cond)
{
	player_t *p = mo->player;
	bool temp;

	for (; cond; cond = cond->next)
	{
		int i_amount = (int)(cond->amount + 0.5f);

		switch (cond->cond_type)
		{
			case COND_Health:
				temp = (mo->health >= cond->amount);

				if ((!cond->negate && !temp) || (cond->negate && temp))
					return false;

				break;

			case COND_Armour:
				if (!p)
					return false;

				if (cond->subtype == ARMOUR_Total)
					temp = (p->totalarmour >= i_amount);
				else
					temp = (p->armours[cond->subtype] >= i_amount);

				if ((!cond->negate && !temp) || (cond->negate && temp))
					return false;

				break;

			case COND_Key:
				if (!p)
					return false;

				temp = ((p->cards & cond->subtype) != 0);

				if ((!cond->negate && !temp) || (cond->negate && temp))
					return false;

				break;

			case COND_Weapon:
				if (!p)
					return false;

				temp = false;

				{
					int i;
					for (i=0; i < MAXWEAPONS; i++)
					{
						if (p->weapons[i].owned &&
							p->weapons[i].info == weapondefs[i])
						{
							temp = true;
							break;
						}
					}
				}

				if ((!cond->negate && !temp) || (cond->negate && temp))
					return false;

				break;

			case COND_Powerup:
				if (!p)
					return false;

				temp = (p->powers[cond->subtype] > cond->amount);

				if ((!cond->negate && !temp) || (cond->negate && temp))
					return false;

				break;

			case COND_Ammo:
				if (!p)
					return false;

				temp = (p->ammo[cond->subtype].num >= i_amount);

				if ((!cond->negate && !temp) || (cond->negate && temp))
					return false;

				break;

			case COND_Jumping:
				if (!p)
					return false;

				temp = (p->jumpwait > 0);

				if ((!cond->negate && !temp) || (cond->negate && temp))
					return false;

				break;

			case COND_Crouching:
				if (!p)
					return false;

				temp = (mo->extendedflags & EF_CROUCHING) ? true : false;

				if ((!cond->negate && !temp) || (cond->negate && temp))
					return false;

				break;

			case COND_Swimming:
				if (!p)
					return false;

				temp = p->swimming;

				if ((!cond->negate && !temp) || (cond->negate && temp))
					return false;

				break;

			case COND_Attacking:
				if (!p)
					return false;

				temp = p->attackdown || p->secondatk_down;

				if ((!cond->negate && !temp) || (cond->negate && temp))
					return false;

				break;

			case COND_Rampaging:
				if (!p)
					return false;

				temp = (p->attackdown_count >= 70);

				if ((!cond->negate && !temp) || (cond->negate && temp))
					return false;

				break;

			case COND_Using:
				if (!p)
					return false;

				temp = p->usedown;

				if ((!cond->negate && !temp) || (cond->negate && temp))
					return false;

				break;

			case COND_Walking:
				if (!p)
					return false;

				temp = (p->bob > 2) && (p->mo->z <= p->mo->floorz);

				if ((!cond->negate && !temp) || (cond->negate && temp))
					return false;

				break;

			case COND_NONE:
			default:
				// unknown condition -- play it safe and succeed
				break;
		}
	}

	// all conditions succeeded
	return true;
}

namespace game
{
	//
	// mapdef_c* LookupMap()
	//
	mapdef_c* LookupMap(const char *refname)
	{
		mapdef_c* m;
		
		m = mapdefs.Lookup(refname);
		if (m)
		{
			if (W_CheckNumForName(m->lump) != -1)
				return m;
		}
		
		return NULL;
	}
}
