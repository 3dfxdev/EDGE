//----------------------------------------------------------------------------
//  EDGE Game Handling Code
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2005  The EDGE Team.
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

#ifndef __G_GAME__
#define __G_GAME__

#include "ddf_main.h"
#include "dm_defs.h"
#include "e_event.h"
#include "e_player.h"

#include "epi/strings.h"

extern int random_seed;  // for demo code
extern int starttime;     //

// -KM- 1998/11/25 Added support for finales before levels
typedef enum
{
	ga_nothing = 0,
	ga_newgame,
	ga_loadlevel,
	ga_loadgame,
	ga_savegame,
	ga_playdemo,
	ga_completed,
	ga_briefing,
	ga_endgame
}
gameaction_e;

extern gameaction_e gameaction;

class newgame_params_c
{
public:
	newgame_params_c();
	newgame_params_c(const newgame_params_c& src);
	~newgame_params_c();

	skill_t skill;
	int deathmatch;

	const mapdef_c *map;
	const gamedef_c *game;

	int random_seed;
	int total_players;
	playerflag_e players[MAXPLAYERS];

public:
	/* methods */

	void SinglePlayer(int num_bots = 0);
	// setup for single player (no netgame) and possibly some bots.
};

//
// GAME
//
void G_InitNew(newgame_params_c& params);

//
// Called by the Startup code & M_Responder; A normal game
// is started by calling the beginning map. The level jump
// cheat can get us anywhere.
//
// -ACB- 1998/08/10 New DDF Structure, Use map reference name.
//
bool G_DeferredInitNew(newgame_params_c& params, bool compat_check = false);

// Can be called by the startup code or M_Responder,
// calls P_SetupLevel or W_EnterWorld.
void G_DeferredLoadGame(int slot);
void G_DeferredSaveGame(int slot, const char *description);
void G_DeferredScreenShot(void);
void G_DeferredEndGame(void);

// -KM- 1998/11/25 Added Time param
void G_ExitLevel(int time);
void G_SecretExitLevel(int time);
void G_ExitToLevel(char *name, int time, bool skip_all);

void G_WorldDone(void);

void G_Ticker(bool fresh_game_tic);
bool G_Responder(event_t * ev);

bool G_CheckWhenAppear(when_appear_e appear);
void G_FileNameFromSlot(epi::string_c& fn, int slot);

extern const gamedef_c* currgamedef;
extern const mapdef_c* currmap;
extern const mapdef_c* nextmap;

mapdef_c* G_LookupMap(const char *refname);

void G_DoLoadLevel(void);         // for demo code
void G_SpawnInitialPlayers(void); //

#endif  /* __G_GAME__ */
