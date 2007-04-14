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
//
// -MH- 1998/07/02 Added key_flyup and key_flydown variables (no logic yet)
// -MH- 1998/08/18 Flyup and flydown logic
//

#include "i_defs.h"

#include "con_cvar.h"
#include "g_game.h"
#include "hu_stuff.h"
#include "m_argv.h"
#include "m_random.h"
#include "p_bot.h"
#include "p_local.h"
#include "st_stuff.h"

#include "epi/endianess.h"

#include <math.h>
#include <stdio.h>
#include <string.h>


//
// PLAYER ARRAY
//
// Main rule is that players[p->num] == p (for all players p).
// The array only holds players "in game", the remaining fields
// are NULL.  There may be NULL entries in-between valid entries
// (e.g. player #2 left the game, so players[2] becomes NULL).
// This means that numplayers is NOT an index to last entry + 1.
//
// The consoleplayer and displayplayer variables must be valid
// indices at all times.
//
player_t *players[MAXPLAYERS];
int numplayers;

int consoleplayer = -1; // player taking events
int displayplayer = -1; // view being displayed 

// for intermission
int totalkills, totalitems, totalsecret;

#define BODYQUESIZE     32

mobj_t *bodyque[BODYQUESIZE];
int bodyqueslot;


//
// G_PlayerFinishLevel
//
// Called when a player completes a level.
//
void G_PlayerFinishLevel(player_t *p)
{
	int i;

	for (i = 0; i < NUMPOWERS; i++)
		p->powers[i] = 0;

	p->keep_powers = 0;

	p->cards = KF_NONE;

	p->mo->flags &= ~MF_FUZZY;  // cancel invisibility 

	p->extralight = 0;  // cancel gun flashes 

	// cancel colourmap effects
	p->effect_colourmap = NULL;

	// no palette changes 
	p->damagecount = 0;
	p->bonuscount  = 0;
	p->grin_count  = 0;
}

//
// player_s::Reborn
//
// Called after a player dies. 
// Almost everything is cleared and initialised.
//
void player_s::Reborn()
{
	playerstate = PST_LIVE;

	mo = NULL;
	health = 0;

	memset(armours, 0, sizeof(armours));
	memset(powers,  0, sizeof(powers));

	keep_powers = 0;
	totalarmour = 0;
	cards = KF_NONE;

	ready_wp = WPSEL_None;
	pending_wp = WPSEL_NoChange;

	memset(weapons, 0, sizeof(weapons));
	memset(key_choices, 0, sizeof(key_choices));
	memset(avail_weapons, 0, sizeof(avail_weapons));
	memset(ammo, 0, sizeof(ammo));

	cheats = 0;
	refire = 0;
	bob = 0;
	kick_offset = 0;
	damagecount = bonuscount = 0;
	extralight = 0;
	flash = false;

	attacker = NULL;

	effect_colourmap = NULL;
	effect_left = 0;

	memset(psprites, 0, sizeof(psprites));
	
	jumpwait = 0;
	idlewait = 0;
	air_in_lungs = 0;
	underwater = false;
	swimming = false;
	grin_count = 0;
	old_health = 0;
	attackdown_count = 0;
	face_index = 0;
	face_count = 0;

	remember_atk[0] = remember_atk[1] = -1;
}

//
// G_CheckSpot  
//
// Returns false if the player cannot be respawned at the given spot
// because something is occupying it.
//
static bool G_CheckSpot(player_t *player, const spawnpoint_t *point)
{
	float x, y, z;

	if (!player->mo)
	{
		// first spawn of level, before corpses
		for (int pnum = 0; pnum < MAXPLAYERS; pnum++)
		{
			player_t *p = players[pnum];

			if (!p || !p->mo || p == player)
				continue;

			if (fabs(p->mo->x - point->x) < 8.0f &&
				fabs(p->mo->y - point->y) < 8.0f)
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

static void SetPlayerConVars(player_t *p)
{
	mobj_t *mobj = p->mo;

	char buffer[16];

	CON_DeleteCVar("health");
	CON_DeleteCVar("frags");
	CON_DeleteCVar("totalfrags");

	CON_CreateCVarReal("health", (cflag_t)(cf_read | cf_delete), &mobj->health);
	CON_CreateCVarInt("frags", (cflag_t)(cf_read | cf_delete), &p->frags);
	CON_CreateCVarInt("totalfrags", (cflag_t)(cf_read | cf_delete), &p->totalfrags);

	int i;
	for (i = 0; i < NUMAMMO; i++)
	{
		sprintf(buffer, "ammo%d", i);
		CON_DeleteCVar(buffer);
		CON_CreateCVarInt(buffer, (cflag_t)(cf_read | cf_delete), &p->ammo[i].num);

		sprintf(buffer, "maxammo%d", i);
		CON_DeleteCVar(buffer);
		CON_CreateCVarInt(buffer, (cflag_t)(cf_read | cf_delete), &p->ammo[i].max);
	}

#if 0  // FIXME:
	for (i = num_disabled_weapons; i < numweapons; i++)
	{
		sprintf(buffer, "weapon%d", i);
		CON_DeleteCVar(buffer);
		CON_CreateCVarBool(buffer, (cflag_t)(cf_read | cf_delete), &p->weapons[i].owned);
	}
#endif

	for (i = 0; i < NUMARMOUR; i++)
	{
		sprintf(buffer, "armour%d", i);
		CON_DeleteCVar(buffer);
		CON_CreateCVarReal(buffer, (cflag_t)(cf_read | cf_delete), &p->armours[i]);
	}

#if 0  // FIXME:
	for (i = 0; i < NUMCARDS; i++)
	{
		sprintf(buffer, "key%d", i);
		CON_DeleteCVar(buffer);
		CON_CreateCVarBool(buffer, cf_read | cf_delete, &p->cards[i]);
	}
#endif

	for (i = 0; i < NUMPOWERS; i++)
	{
		sprintf(buffer, "power%d", i);
		CON_DeleteCVar(buffer);
		CON_CreateCVarReal(buffer, (cflag_t)(cf_read | cf_delete), &p->powers[i]);
	}
}


//
// G_SetConsolePlayer
//
// Note: we don't rely on current value being valid, hence can use
//       these functions during initialisation.
//
void G_SetConsolePlayer(int pnum)
{
	consoleplayer = pnum;

	SYS_ASSERT(players[consoleplayer]);

	for (int i = 0; i < MAXPLAYERS; i++)
		if (players[i])
			players[i]->playerflags &= ~PFL_Console;
	
	players[pnum]->playerflags |= PFL_Console;

	if (M_CheckParm("-testbot") > 0)
	{
		P_BotCreate(players[pnum], false);
	}
	else
	{
		players[pnum]->builder = P_ConsolePlayerBuilder;
		players[pnum]->build_data = NULL;
	}
}

//
// G_SetDisplayPlayer
//
void G_SetDisplayPlayer(int pnum)
{
	displayplayer = pnum;

	SYS_ASSERT(players[displayplayer]);

	for (int i = 0; i < MAXPLAYERS; i++)
		if (players[i])
			players[i]->playerflags &= ~PFL_Display;
	
	players[pnum]->playerflags |= PFL_Display;
}

void G_ToggleDisplayPlayer(void)
{
	for (int i = 1; i <= MAXPLAYERS; i++)
	{
		int pnum = (displayplayer + i) % MAXPLAYERS;

		if (players[pnum])
		{
			G_SetDisplayPlayer(pnum);
			break;
		}
	}
}

//
// P_SpawnPlayer
//
// Called when a player is spawned on the level.
// Most of the player structure stays unchanged between levels.
//
// -KM- 1998/12/21 Cleaned this up a bit.
// -KM- 1999/01/31 Removed all those nasty cases for doomednum (1/4001)
//
static void P_SpawnPlayer(player_t *p, const spawnpoint_t *point)
{
	// -KM- 1998/11/25 This is in preparation for skins.  The creatures.ddf
	//   will hold player start objects, sprite will be taken for skin.
	// -AJA- 2004/04/14: Use DDF entry from level thing.

	if (point->info == NULL)
		I_Error("P_SpawnPlayer: No such item type!");

	const mobjtype_c *info = point->info;

	L_WriteDebug("* P_SpawnPlayer %d @ %1.0f,%1.0f\n",
			point->info->playernum, point->x, point->y);
		
	if (info->playernum <= 0)
		info = mobjtypes.LookupPlayer(p->pnum + 1);

	if (p->playerstate == PST_REBORN)
	{
		p->Reborn();

		P_GiveInitialBenefits(p, info);
	}

	mobj_t *mobj = P_MobjCreateObject(point->x, point->y, point->z, info);

	mobj->angle = point->angle;
	mobj->vertangle = point->vertangle;
	mobj->player = p;
	mobj->health = p->health;

	p->mo = mobj;
	p->playerstate = PST_LIVE;
	p->refire = 0;
	p->damagecount = 0;
	p->bonuscount = 0;
	p->extralight = 0;
	p->effect_colourmap = NULL;
	p->std_viewheight = mobj->height * PERCENT_2_FLOAT(info->viewheight);
	p->viewheight = p->std_viewheight;
	p->jumpwait = 0;

	// don't do anything immediately 
	p->usedown = p->attackdown[0] = p->attackdown[1] = false;

	// setup gun psprite
	P_SetupPsprites(p);

	// give all cards in death match mode
	if (DEATHMATCH())
		p->cards = KF_MASK;

	// -AJA- in COOP, all players are on the same side
	if (COOP_MATCH())
		mobj->side = ~0;

	// -AJA- FIXME: maybe this belongs elsewhere.
	if (p->pnum == consoleplayer)
	{
		// wake up the status bar and heads up text
		ST_Start();
		HU_Start();

		SetPlayerConVars(p);
	}

	// Don't get stuck spawned in things: telefrag them.
	P_TeleportMove(mobj, mobj->x, mobj->y, mobj->z);
}

//
// P_SpawnVoodooDoll
//
static void P_SpawnVoodooDoll(player_t *p, const spawnpoint_t *point)
{
	const mobjtype_c *info = point->info;

	SYS_ASSERT(info);
	SYS_ASSERT(info->playernum > 0);

	L_WriteDebug("* P_SpawnVoodooDoll %d @ %1.0f,%1.0f\n",
			p->pnum+1, point->x, point->y);
		
	mobj_t *mobj = P_MobjCreateObject(point->x, point->y, point->z, info);

	mobj->angle = point->angle;
	mobj->vertangle = point->vertangle;
	mobj->player = p;
	mobj->health = p->health;

	if (COOP_MATCH())
		mobj->side = ~0;

	// Don't get stuck spawned in things: telefrag them.
	P_TeleportMove(mobj, mobj->x, mobj->y, mobj->z);
}

//
// G_DeathMatchSpawnPlayer 
//
// Spawns a player at one of the random deathmatch spots.
// Called at level load and each death.
//
void G_DeathMatchSpawnPlayer(player_t *p)
{
	if (p->pnum >= dm_starts.GetSize())
		I_Warning("Few deathmatch spots, %d recommended.\n", p->pnum + 1);

	if (dm_starts.GetSize())
	{
		int begin = P_Random() % dm_starts.GetSize();

		for (int j = 0; j < dm_starts.GetSize(); j++)
		{
			int i = (begin + j) % dm_starts.GetSize();

			if (G_CheckSpot(p, dm_starts[i]))
			{
				P_SpawnPlayer(p, dm_starts[i]);
				return;
			}
		}
	}

	// no good spot, so the player will probably get stuck
	if (coop_starts.GetSize())
	{
		int begin = P_Random() % coop_starts.GetSize();

		for (int j = 0; j < coop_starts.GetSize(); j++)
		{
			int i = (begin + j) % coop_starts.GetSize();

			if (G_CheckSpot(p, coop_starts[i]))
			{
				P_SpawnPlayer(p, coop_starts[i]);
				return;
			}
		}
	}

	I_Error("No player starts found!");
}

//
// G_CoopSpawnPlayer 
//
// Spawns a player at one of the random deathmatch spots.
// Called at level load and each death.
//
void G_CoopSpawnPlayer(player_t *p)
{
	spawnpoint_t *sp = coop_starts.FindPlayer(p->pnum+1);

	if (sp == NULL)
		I_Error("Missing player %d start !\n", p->pnum+1);

	if (! G_CheckSpot(p, sp))
	{
		I_Warning("Player %d start is invalid.\n", p->pnum+1);

		int begin = p->pnum;

		// try to spawn at one of the other players spots
		for (int j = 0; j < coop_starts.GetSize(); j++)
		{
			int i = (begin + j) % coop_starts.GetSize();

			spawnpoint_t *new_sp = coop_starts[i];

			if (G_CheckSpot(p, new_sp))
			{
				sp = new_sp;
				break;
			}
		}
	}

	P_SpawnPlayer(p, sp);
}

//
// G_SpawnVoodooDolls
//
void G_SpawnVoodooDolls(player_t *p)
{
	for (int i = 0; i < voodoo_doll_starts.GetSize(); i++)
	{
		spawnpoint_t *sp = voodoo_doll_starts[i];

		if (sp->info->playernum != p->pnum + 1)
			continue;
#if 0
		if (! G_CheckSpot(p, sp))
			continue;
#endif
		P_SpawnVoodooDoll(p, sp);
	}
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

				if (cond->sub.type == ARMOUR_Total)
					temp = (p->totalarmour >= i_amount);
				else
					temp = (p->armours[cond->sub.type] >= i_amount);

				if ((!cond->negate && !temp) || (cond->negate && temp))
					return false;

				break;

			case COND_Key:
				if (!p)
					return false;

				temp = ((p->cards & cond->sub.type) != 0);

				if ((!cond->negate && !temp) || (cond->negate && temp))
					return false;

				break;

			case COND_Weapon:
				if (!p)
					return false;

				temp = false;

				for (int i=0; i < MAXWEAPONS; i++)
				{
					if (p->weapons[i].owned &&
						p->weapons[i].info == cond->sub.weap)
					{
						temp = true;
						break;
					}
				}

				if ((!cond->negate && !temp) || (cond->negate && temp))
					return false;

				break;

			case COND_Powerup:
				if (!p)
					return false;

				temp = (p->powers[cond->sub.type] > cond->amount);

				if ((!cond->negate && !temp) || (cond->negate && temp))
					return false;

				break;

			case COND_Ammo:
				if (!p)
					return false;

				temp = (p->ammo[cond->sub.type].num >= i_amount);

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

				temp = p->attackdown[0] || p->attackdown[1];

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


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
