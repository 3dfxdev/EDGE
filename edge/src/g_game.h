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

#ifndef __G_GAME__
#define __G_GAME__

#include "dm_defs.h"
#include "e_event.h"
#include "ddf_main.h"

#include "epi/epistring.h"

extern long random_seed;  //
extern int starttime;     // for demo code
extern bool netdemo;      //

//
// GAME
//
void G_InitNew(skill_t skill, const mapdef_c *map, const gamedef_c *gamedef, long seed);

//
// Called by the Startup code & M_Responder; A normal game
// is started by calling the beginning map. The level jump
// cheat can get us anywhere.
//
// -ACB- 1998/08/10 New DDF Structure, Use map reference name.
//
bool G_DeferredInitNew(skill_t skill, 
					   const char *mapname,
					   bool warpopt);

// Can be called by the startup code or M_Responder,
// calls P_SetupLevel or W_EnterWorld.
void G_DeferredLoadGame(int slot);

// Called by M_Responder.
void G_DeferredSaveGame(int slot, const char *description);

// -KM- 1998/11/25 Added Time param
void G_ExitLevel(int time);
void G_SecretExitLevel(int time);
void G_ExitToLevel(char *name, int time, bool skip_all);

void G_WorldDone(void);

void G_Ticker(void);
bool G_Responder(event_t * ev);

void G_DeferredScreenShot(void);

bool G_CheckWhenAppear(when_appear_e appear);
void G_FileNameFromSlot(epi::string_c& fn, int slot);

extern const gamedef_c* currgamedef;
extern const mapdef_c* currmap;
extern const mapdef_c* nextmap;

namespace game
{
	mapdef_c* LookupMap(const char *refname);
};

void G_DoLoadLevel(void);  // for demo code

#endif  /* __G_GAME__ */
