//----------------------------------------------------------------------------
//  EDGE Player Handling
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

#include "i_defs.h"
#include "g_game.h"

#include "am_map.h"
#include "con_cvar.h"
#include "con_main.h"
#include "dm_defs.h"
#include "dm_state.h"
#include "dstrings.h"
#include "e_demo.h"
#include "e_input.h"
#include "e_main.h"
#include "e_player.h"
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

#include "epi/epiendian.h"

#ifdef USE_HAWKNL
#include "n_network.h"
#endif

#define SAVEGAMESIZE    0x50000
#define SAVESTRINGSIZE  24

static void G_DoReborn(player_t *p);

static void G_DoNewGame(void);
static void G_DoLoadGame(void);
static void G_DoCompleted(void);
static void G_DoSaveGame(void);

gameaction_e gameaction = ga_nothing;
gamestate_e gamestate = GS_NOTHING;
skill_t gameskill = sk_invalid;

bool paused = false;

// ok to save / end game 
bool usergame;

// for comparative timing purposes 
bool nodrawers;
bool noblit;

int starttime;

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

int gametic;

long random_seed;

// if true, load all graphics at start 
bool precache = true;

// parms for world map / intermission 
wbstartstruct_t wminfo;

// -ACB- 2004/05/25 We need to store our current gamedef
const gamedef_c* currgamedef = NULL;

// -ACB- 2004/05/25 We need to store our current/next mapdefs
const mapdef_c* currmap = NULL;
const mapdef_c* nextmap = NULL;

//--------------------------------------------

static int loadgame_slot;
static int savegame_slot;
static char savedescription[32];

// deferred stuff...
static const mapdef_c *d_newmap = NULL;
static const gamedef_c *d_gamedef = NULL;
static skill_t d_newskill;
static bool d_newwarp;

#define TURBOTHRESHOLD  0x32

//
// G_DoLoadLevel 
//
void G_DoLoadLevel(void)
{
	if (currmap == NULL)
		I_Error("G_DoLoadLevel: No Current Map selected");

	// Set the sky map.
	//
	// First thing, we have a dummy sky texture name, a flat. The data is
	// in the WAD only because we look for an actual index, instead of simply
	// setting one.
	//
	// -ACB- 1998/08/09 Reference current map for sky name.

	sky_image = W_ImageLookup(currmap->sky, INS_Texture);

	if (wipegamestate == GS_LEVEL)
		wipegamestate = GS_NOTHING;  // force a wipe

	// -AJA- need this for GLBSP plugin
	gamestate = GS_NOTHING;

	// -AJA- FIXME: this background camera stuff is a mess
	background_camera_mo = NULL;
	R_ExecuteSetViewSize();

	for (int pnum = 0; pnum < MAXPLAYERS; pnum++)
	{
		player_t *p = players[pnum];
		if (! p) continue;

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
	HANDLE_FLAG(level_flags.weapon_switch, MPF_WeaponSwitch);

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
	RAD_FinishMenu(0);

	P_SetupLevel(gameskill, currmap->autotag);

	RAD_SpawnTriggers(currmap->ddf.name.GetString());

	G_SetDisplayPlayer(consoleplayer); // view the guy you are playing

	starttime = I_GetTime();
	exittime = 0x7fffffff;
	exit_skipall = false;

	gamestate = GS_LEVEL;
	gameaction = ga_nothing;

	CON_SetVisible( /* !!! showMessages?vs_minimal: */ vs_notvisible);
	CON_Printf("%s\n", currmap->ddf.name.GetString());

	// clear cmd building stuff
	E_ClearInput();

	paused = false;
	viewactive = true;
}



//
// G_Responder  
//
// Get info needed to make ticcmd_ts for the players.
// 
bool G_Responder(event_t * ev)
{
	// any other key pops up menu if in demos
	if (gameaction == ga_nothing && !singledemo &&
		(demoplayback || gamestate == GS_DEMOSCREEN))
	{
		if (ev->type == ev_keydown)
		{
			M_StartControlPanel();
			S_StartSound(NULL, sfx_swtchn);
			return true;
		}

		return false;
	}

	if (ev->type == ev_keydown && ev->value.key == KEYD_F12)
	{
		// 25-6-98 KM Allow spy mode for demos even in deathmatch
		if (gamestate == GS_LEVEL && (demoplayback || true || !deathmatch)) //!!!! DEBUGGING
		{
			G_ToggleDisplayPlayer();
			return true;
		}
	}

	if (ev->type == ev_keydown && ev->value.key == KEYD_PAUSE)
	{
		if (!netgame)
		{
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
			return true;
		}
	}

	if (gamestate == GS_LEVEL)
	{
		if (RAD_Responder(ev))
			return true;  // RTS system ate it

		if (ST_Responder(ev))
			return true;  // status window ate it 

		if (AM_Responder(ev))
			return true;  // automap ate it 

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

	return INP_Responder(ev);
}

//
// G_TiccmdTicker
//
static void G_TiccmdTicker(void)
{
	int buf = gametic % BACKUPTICS;

	for (int pnum = 0; pnum < MAXPLAYERS; pnum++)
	{
		player_t *p = players[pnum];
		if (! p) continue;

		ticcmd_t *cmd = &p->cmd;

		// -ES- FIXME: Change format of player_t->cmd?
		*cmd = p->in_cmds[buf];

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

		if (netgame && !netdemo)
		{
			if (gametic > BACKUPTICS && p->consistency[buf] != cmd->consistency)
			{
/* !!!! DEBUG */	I_Warning("Consistency failure on player %d (%i should be %i)",
					p->pnum + 1, cmd->consistency, p->consistency[buf]);
			}
			if (p->mo)
				p->consistency[buf] = (int)p->mo->x;
			else
				p->consistency[buf] = P_ReadRandomState() & 0xff;
		}
	}
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
	// do player reborns if needed
	int pnum;
	for (pnum = 0; pnum < MAXPLAYERS; pnum++)
	{
		player_t *p = players[pnum];

		if (p && p->playerstate == PST_REBORN)
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
	// and build new consistency check.
	G_TiccmdTicker();

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
// G_DoReborn
// 
static void G_DoReborn(player_t *p)
{
	// single player ?
	if (!netgame && !deathmatch)
	{
		// -AJA- 2003/10/09: we should only get here after the player
		//       died, i.e. not through other uses of PST_REBORN that
		//       appear in this file.  FIXME: this is confusing !

		if (gamestate == GS_LEVEL)
			gameaction = ga_loadlevel;
		return;
	}

	// first disassociate the corpse 
	p->mo->player = NULL;

	// spawn at random spot if in death match 
	if (deathmatch)
	{
		G_DeathMatchSpawnPlayer(p);
		return;
	}

	// respawn at the start
	G_CoopSpawnPlayer(p);
}


void G_DeferredScreenShot(void)
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
static void G_DoCompleted(void)
{
	gameaction = ga_nothing;

	for (int pnum = 0; pnum < MAXPLAYERS; pnum++)
	{
		player_t *p = players[pnum];
		if (! p) continue;

		p->leveltime = leveltime;

		// take away cards and stuff
		G_PlayerFinishLevel(p);
	}

	if (automapactive)
		AM_Stop();
	
	if (rts_menuactive)
		RAD_FinishMenu(0);

	// handle "no stat" levels
	if (currmap->wistyle == WISTYLE_None || exit_skipall)
	{
		viewactive = false;
		automapactive = false;

		G_WorldDone();
		return;
	}

	wminfo.level = currmap->ddf.name.GetString();
	wminfo.last = currmap;
	wminfo.next = nextmap;
	wminfo.maxkills = totalkills;
	wminfo.maxitems = totalitems;
	wminfo.maxsecret = totalsecret;
	wminfo.maxfrags = 0;
	wminfo.partime = currmap->partime;

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
// G_DeferredLoadGame
//
// Can be called by the startup code or the menu task. 
//
void G_DeferredLoadGame(int slot)
{
	loadgame_slot = slot;
	gameaction = ga_loadgame;
}

static void G_DoLoadGame(void)
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
		globs->mapsector.crc != mapsector_CRC.crc ||
		globs->mapline.count != numlines ||
		globs->mapline.crc != mapline_CRC.crc ||
		globs->mapthing.count != mapthing_NUM ||
		globs->mapthing.crc != mapthing_CRC.crc)
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
	// has changed (e.g. CM_BOOM -> CM_EDGE).
	DDF_BoomClearGenTypes();

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
// G_DeferredSaveGame
//
// Called by the menu task.
// Description is a 24 byte text string 
//
void G_DeferredSaveGame(int slot, const char *description)
{
	savegame_slot = slot;
	strcpy(savedescription, description);
	gameaction = ga_savegame;
}

static void G_DoSaveGame(void)
{
	epi::string_c fn;
	saveglobals_t *globs;
	time_t cur_time;
	char timebuf[100];

 	G_FileNameFromSlot(fn, savegame_slot);
	
	if (! SV_OpenWriteFile(fn.GetString(), (EDGEVER << 8) | EDGEPATCH))
	{
		I_Error("Unable to create savegame file: %s\n", fn.GetString());
		return; /* NOT REACHED */
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

	globs->console_player = 0; // !!!FIXME CHECK: consoleplayer->pnum;
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
	globs->mapsector.crc = mapsector_CRC.crc;
	globs->mapline.count = numlines;
	globs->mapline.crc = mapline_CRC.crc;
	globs->mapthing.count = mapthing_NUM;
	globs->mapthing.crc = mapthing_CRC.crc;

	// FIXME: store DDF CRC values too...

	SV_BeginSave();

	SV_SaveGLOB(globs);
	SV_SaveEverything();

	SV_FreeGLOB(globs);

	SV_FinishSave();
	SV_CloseWriteFile();

	savedescription[0] = 0;

	CON_Printf("%s", language["GameSaved"]);

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

static void G_DoNewGame(void)
{
	demoplayback = false;

	if (d_newwarp)
	{
		if (netdemo)
		{
			deathmatch = netdemo = netgame = false;
			consoleplayer = 0; //???

			// !!! FIXME: this is wrong
#if 0
			for (p = players->next; p; p = p->next)
				p->in_game = false;
#endif
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
	int pnum;

	for (pnum = 0; pnum < MAXPLAYERS; pnum++)
	{
		player_t *p = players[pnum];
		if (! p) continue;

		memset(p->consistency, -1, sizeof(p->consistency));
	}

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
	for (pnum = 0; pnum < MAXPLAYERS; pnum++)
	{
		player_t *p = players[pnum];
		if (! p) continue;

		p->playerstate = PST_REBORN;
	}

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
