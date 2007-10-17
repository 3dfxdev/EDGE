//----------------------------------------------------------------------------
//  EDGE New SaveGame Handling (Players)
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2007  The EDGE Team.
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
// See the file "docs/save_sys.txt" for a complete description of the
// new savegame system.
//
// This file handles:
//   player_t        [PLAY]
//   playerweapon_t  [WEAP]
//   playerammo_t    [AMMO]
//   psprite_t       [PSPR]
//

#include "i_defs.h"

#include "epi/strings.h"

#include "ddf/main.h"

#include "p_bot.h"
#include "sv_chunk.h"
#include "sv_main.h"
#include "z_zone.h"


#undef SF
#define SF  SVFIELD


// forward decls.
int SV_PlayerCountElems(void);
int SV_PlayerFindElem(player_t *elem);
void * SV_PlayerGetElem(int index);
void SV_PlayerCreateElems(int num_elems);
void SV_PlayerFinaliseElems(void);

bool SR_PlayerGetAmmo(void *storage, int index, void *extra);
bool SR_PlayerGetWeapon(void *storage, int index, void *extra);
bool SR_PlayerGetPSprite(void *storage, int index, void *extra);
bool SR_PlayerGetName(void *storage, int index, void *extra);
bool SR_PlayerGetState(void *storage, int index, void *extra);
bool SR_WeaponGetInfo(void *storage, int index, void *extra);

void SR_PlayerPutAmmo(void *storage, int index, void *extra);
void SR_PlayerPutWeapon(void *storage, int index, void *extra);
void SR_PlayerPutPSprite(void *storage, int index, void *extra);
void SR_PlayerPutName(void *storage, int index, void *extra);
void SR_PlayerPutState(void *storage, int index, void *extra);
void SR_WeaponPutInfo(void *storage, int index, void *extra);

extern bool SR_MobjGetType(void *storage, int index, void *extra);
extern void SR_MobjPutType(void *storage, int index, void *extra);


//----------------------------------------------------------------------------
//
//  PLAYER STRUCTURE AND ARRAY
//
static player_t sv_dummy_player;

#define SV_F_BASE  sv_dummy_player

static savefield_t sv_fields_player[] =
{
	SF(pnum, "pnum", 1, SVT_INT, SR_GetInt, SR_PutInt),
	SF(playerstate, "playerstate", 1, SVT_INT, SR_GetInt, SR_PutInt),
	SF(playerflags, "playerflags", 1, SVT_INT, SR_GetInt, SR_PutInt),
	SF(playername[0], "playername", 1, SVT_STRING, 
        SR_PlayerGetName, SR_PlayerPutName),
	SF(mo, "mo", 1, SVT_INDEX("mobjs"), SR_MobjGetMobj, SR_MobjPutMobj),
	SF(viewz, "viewz", 1, SVT_FLOAT, SR_GetFloat, SR_PutFloat),
	SF(viewheight, "viewheight", 1, SVT_FLOAT, SR_GetFloat, SR_PutFloat),
	SF(deltaviewheight, "deltaviewheight", 1, SVT_FLOAT, SR_GetFloat, SR_PutFloat),
	SF(std_viewheight, "std_viewheight", 1, SVT_FLOAT, SR_GetFloat, SR_PutFloat),
	SF(actual_speed, "actual_speed", 1, SVT_FLOAT, SR_GetFloat, SR_PutFloat),
	SF(health, "health", 1, SVT_FLOAT, SR_GetFloat, SR_PutFloat),
	SF(armours[0], "armours", NUMARMOUR, SVT_FLOAT, SR_GetFloat, SR_PutFloat),
	SF(armour_types[0], "armour_types", NUMARMOUR, SVT_STRING, SR_MobjGetType, SR_MobjPutType),
	SF(powers[0],  "powers",  NUMPOWERS, SVT_FLOAT, SR_GetFloat, SR_PutFloat),
	SF(keep_powers,  "keep_powers", 1, SVT_INT, SR_GetInt, SR_PutInt),
	SF(cards, "cards_ke", 1, SVT_ENUM, SR_GetEnum, SR_PutEnum),
	SF(frags, "frags", 1, SVT_INT, SR_GetInt, SR_PutInt),
	SF(totalfrags, "totalfrags", 1, SVT_INT, SR_GetInt, SR_PutInt),
	SF(ready_wp, "ready_wp", 1, SVT_INT, SR_GetInt, SR_PutInt),
	SF(pending_wp, "pending_wp", 1, SVT_INT, SR_GetInt, SR_PutInt),
	SF(weapons[0], "weapons", MAXWEAPONS, SVT_STRUCT("playerweapon_t"), 
        SR_PlayerGetWeapon, SR_PlayerPutWeapon),
	SF(ammo[0], "ammo", NUMAMMO, SVT_STRUCT("playerammo_t"), 
	SR_PlayerGetAmmo, SR_PlayerPutAmmo),
	SF(cheats, "cheats", 1, SVT_INT, SR_GetInt, SR_PutInt),
	SF(killcount, "killcount", 1, SVT_INT, SR_GetInt, SR_PutInt),
	SF(itemcount, "itemcount", 1, SVT_INT, SR_GetInt, SR_PutInt),
	SF(secretcount, "secretcount", 1, SVT_INT, SR_GetInt, SR_PutInt),
	SF(jumpwait, "jumpwait", 1, SVT_INT, SR_GetInt, SR_PutInt),
	SF(idlewait, "idlewait", 1, SVT_INT, SR_GetInt, SR_PutInt),
	SF(air_in_lungs, "air_in_lungs", 1, SVT_INT, SR_GetInt, SR_PutInt),
	SF(underwater, "underwater", 1, SVT_BOOLEAN, SR_GetBoolean, SR_PutBoolean),
	SF(flash, "flash_b", 1, SVT_BOOLEAN, SR_GetBoolean, SR_PutBoolean),
	SF(psprites[0], "psprites", NUMPSPRITES, SVT_STRUCT("psprite_t"), 
	SR_PlayerGetPSprite, SR_PlayerPutPSprite),

	// NOT HERE:
	//   in_game: only in-game players are saved.
	//   key_choices: depends on DDF too much, and not important.
	//   remember_atk[]: ditto.
	//   next,prev: links are regenerated.
	//   avail_weapons, totalarmour: regenerated.

	SVFIELD_END
};

savestruct_t sv_struct_player =
{
	NULL,          // link in list
	"player_t",    // structure name
	"play",        // start marker
	sv_fields_player,  // field descriptions
	SVDUMMY,       // dummy base
	true,          // define_me
	NULL           // pointer to known struct
};

#undef SV_F_BASE

savearray_t sv_array_player =
{
	NULL,               // link in list
	"players",          // array name
	&sv_struct_player,  // array type
	true,               // define_me

	SV_PlayerCountElems,     // count routine
	SV_PlayerGetElem,        // index routine
	SV_PlayerCreateElems,    // creation routine
	SV_PlayerFinaliseElems,  // finalisation routine

	NULL,     // pointer to known array
	0         // loaded size
};


//----------------------------------------------------------------------------
//
//  WEAPON STRUCTURE
//
static playerweapon_t sv_dummy_playerweapon;

#define SV_F_BASE  sv_dummy_playerweapon

static savefield_t sv_fields_playerweapon[] =
{
	SF(info, "info", 1, SVT_STRING, SR_WeaponGetInfo, SR_WeaponPutInfo),
	SF(owned, "owned", 1, SVT_BOOLEAN, SR_GetBoolean, SR_PutBoolean),
	SF(flags, "flags", 1, SVT_INT, SR_GetInt, SR_PutInt),
	SF(clip_size[0], "clip_size",    1, SVT_INT, SR_GetInt, SR_PutInt),
	SF(clip_size[1], "sa_clip_size", 1, SVT_INT, SR_GetInt, SR_PutInt),
	SF(model_skin, "model_skin", 1, SVT_INT, SR_GetInt, SR_PutInt),

	SVFIELD_END
};

savestruct_t sv_struct_playerweapon =
{
	NULL,          // link in list
	"playerweapon_t",    // structure name
	"weap",        // start marker
	sv_fields_playerweapon,  // field descriptions
	SVDUMMY,       // dummy base
	true,          // define_me
	NULL           // pointer to known struct
};

#undef SV_F_BASE


//----------------------------------------------------------------------------
//
//  AMMO STRUCTURE
//
static playerammo_t sv_dummy_playerammo;

#define SV_F_BASE  sv_dummy_playerammo

static savefield_t sv_fields_playerammo[] =
{
	SF(num, "num", 1, SVT_INT, SR_GetInt, SR_PutInt),
	SF(max, "max", 1, SVT_INT, SR_GetInt, SR_PutInt),

	SVFIELD_END
};

savestruct_t sv_struct_playerammo =
{
	NULL,          // link in list
	"playerammo_t",    // structure name
	"ammo",        // start marker
	sv_fields_playerammo,  // field descriptions
	SVDUMMY,       // dummy base
	true,          // define_me
	NULL           // pointer to known struct
};

#undef SV_F_BASE


//----------------------------------------------------------------------------
//
//  PSPRITE STRUCTURE
//
static pspdef_t sv_dummy_psprite;

#define SV_F_BASE  sv_dummy_psprite

static savefield_t sv_fields_psprite[] =
{
	SF(state, "state", 1, SVT_STRING, SR_PlayerGetState, SR_PlayerPutState),
	SF(next_state, "next_state", 1, SVT_STRING, 
	SR_PlayerGetState, SR_PlayerPutState),
	SF(tics, "tics", 1, SVT_INT, SR_GetInt, SR_PutInt),
	SF(visibility, "visibility", 1, SVT_FLOAT, SR_GetFloat, SR_PutFloat),
	SF(vis_target, "vis_target", 1, SVT_FLOAT, SR_GetFloat, SR_PutFloat),

	// NOT HERE:
	//   sx, sy: they can be regenerated.

	SVFIELD_END
};

savestruct_t sv_struct_psprite =
{
	NULL,          // link in list
	"pspdef_t",    // structure name
	"pspr",        // start marker
	sv_fields_psprite,  // field descriptions
    SVDUMMY,       // dummy base
	true,          // define_me
	NULL           // pointer to known struct
};

#undef SV_F_BASE


//----------------------------------------------------------------------------

int SV_PlayerCountElems(void)
{
	return numplayers;
}


void *SV_PlayerGetElem(int index)
{
	if (index >= numplayers)
		I_Error("LOADGAME: Invalid player index: %d\n", index);

	for (int pnum = 0; pnum < MAXPLAYERS; pnum++)
	{
		player_t *p = players[pnum];
		if (! p) continue;

		if (index == 0)
			return p;

		index--;
	}

	I_Error("Internal error in SV_PlayerGetElem: index not found.\n");
	return NULL;
}


int SV_PlayerFindElem(player_t *elem)
{
	int index = 0;

	for (int pnum = 0; pnum < MAXPLAYERS; pnum++)
	{
		player_t *p = players[pnum];
		if (! p) continue;

		if (p == elem)
			return index;

		index++;  // only count non-NULL pointers
	}

	I_Error("Internal error in SV_PlayerFindElem: No such PlayerPtr: %p\n", elem);
	return 0;
}


void SV_PlayerCreateElems(int num_elems)
{
	// free existing players (sets all pointers to NULL)
	P_DestroyAllPlayers();

	if (num_elems > MAXPLAYERS)
		I_Error("LOADGAME: too many players (%d)\n", num_elems);

	numplayers = num_elems;

	for (int pnum = 0; pnum < num_elems; pnum++)
	{
		player_t *p = new player_t;

		Z_Clear(p, player_t, 1);

		// Note: while loading, we don't follow the normal principle
		//       where players[p->pnum] == p.  This is fixed in the
		//       finalisation function.
		players[pnum] = p;

		// initialise defaults

		p->pnum = -1;  // checked during finalisation.
		sprintf(p->playername, "Player%d", 1 + p->pnum);

		p->remember_atk[0] = p->remember_atk[1] = -1;

		for (int j=0; j < NUMPSPRITES; j++)
		{
			p->psprites[j].sx = 0;
			p->psprites[j].sy = 0;
		}

		for (int k=0; k < WEAPON_KEYS; k++)
			p->key_choices[k] = WPSEL_None;

		for (int w=0; w < MAXWEAPONS; w++)
			p->weapons[w].model_skin = 1;
	}
}


void SV_PlayerFinaliseElems(void)
{
	int first = -1;

	consoleplayer = -1;
	displayplayer = -1;

	player_t *temp[MAXPLAYERS];

	for (int i = 0; i < MAXPLAYERS; i++)
	{
		temp[i] = players[i];
		players[i] = NULL;
	}

	for (int pnum = 0; pnum < MAXPLAYERS; pnum++)
	{
		player_t *p = temp[pnum];
		if (! p) continue;

		if (p->pnum < 0)
			I_Error("LOADGAME: player did not load (index %d) !\n", pnum);

		if (p->pnum >= MAXPLAYERS)
			I_Error("LOADGAME: player with bad index (%d) !\n", p->pnum);

		if (! p->mo)
			I_Error("LOADGAME: Player %d has no mobj !\n", p->pnum);

		if (players[p->pnum])
			I_Error("LOADGAME: Two players with same number !\n");

		players[p->pnum] = p;

		if (first < 0)
			first = p->pnum;

		if (p->playerflags & PFL_Console)
			consoleplayer = p->pnum;

		if (p->playerflags & PFL_Display)
			displayplayer = p->pnum;

		if (p->playerflags & PFL_Bot)
			P_BotCreate(p, true);
		else
			p->builder = P_ConsolePlayerBuilder;

		// -AJA- 2007/08/22: new PURPLE armor was squeezed into middle
		if (savegame_version < 0x13100)
		{
			p->armours[ARMOUR_Red]    = p->armours[3];
			p->armours[ARMOUR_Yellow] = p->armours[2];
			p->armours[ARMOUR_Purple] = 0;
		}

		P_UpdateAvailWeapons(p);
		P_UpdateTotalArmour(p);
	}

	if (first < 0)
		I_Error("LOADGAME: No players !!\n");

	if (consoleplayer < 0)
		G_SetConsolePlayer(first);

	if (displayplayer < 0)
		G_SetDisplayPlayer(consoleplayer);
}


//----------------------------------------------------------------------------

//
// SR_PlayerGetAmmo
//
bool SR_PlayerGetAmmo(void *storage, int index, void *extra)
{
	playerammo_t *dest = (playerammo_t *)storage + index;

	if (sv_struct_playerammo.counterpart)
		return SV_LoadStruct(dest, sv_struct_playerammo.counterpart);

	return true;  // presumably
}

//
// SR_PlayerPutAmmo
//
void SR_PlayerPutAmmo(void *storage, int index, void *extra)
{
	playerammo_t *src = (playerammo_t *)storage + index;

	SV_SaveStruct(src, &sv_struct_playerammo);
}

//
// SR_PlayerGetWeapon
//
bool SR_PlayerGetWeapon(void *storage, int index, void *extra)
{
	playerweapon_t *dest = (playerweapon_t *)storage + index;

	if (sv_struct_playerweapon.counterpart)
		return SV_LoadStruct(dest, sv_struct_playerweapon.counterpart);

	return true;  // presumably
}

//
// SR_PlayerPutWeapon
//
void SR_PlayerPutWeapon(void *storage, int index, void *extra)
{
	playerweapon_t *src = (playerweapon_t *)storage + index;

	SV_SaveStruct(src, &sv_struct_playerweapon);
}

//
// SR_PlayerGetPSprite
//
bool SR_PlayerGetPSprite(void *storage, int index, void *extra)
{
	pspdef_t *dest = (pspdef_t *)storage + index;

	//!!! FIXME: should skip if no counterpart
	if (sv_struct_psprite.counterpart)
		return SV_LoadStruct(dest, sv_struct_psprite.counterpart);

	return true;  // presumably
}

//
// SR_PlayerPutPSprite
//
void SR_PlayerPutPSprite(void *storage, int index, void *extra)
{
	pspdef_t *src = (pspdef_t *)storage + index;

	SV_SaveStruct(src, &sv_struct_psprite);
}

//
// SR_PlayerGetName
//
bool SR_PlayerGetName(void *storage, int index, void *extra)
{
	char *dest = (char *)storage;
	const char *str;

	SYS_ASSERT(index == 0);

	str = SV_GetString();
	Z_StrNCpy(dest, str, MAX_PLAYNAME-1);
	SV_FreeString(str);

	return true;
}

//
// SR_PlayerPutName
//
void SR_PlayerPutName(void *storage, int index, void *extra)
{
	char *src = (char *)storage;

	SYS_ASSERT(index == 0);

	SV_PutString(src);
}

//
// SR_WeaponGetInfo
//
bool SR_WeaponGetInfo(void *storage, int index, void *extra)
{
	weapondef_c ** dest = (weapondef_c **)storage + index;
	const char *name;

	name = SV_GetString();

	*dest = name ? weapondefs.Lookup(name) : NULL;
	SV_FreeString(name);

	return true;
}

//
// SR_WeaponPutInfo
//
void SR_WeaponPutInfo(void *storage, int index, void *extra)
{
	weapondef_c *info = ((weapondef_c **)storage)[index];

	SV_PutString(info ? info->ddf.name.GetString() : NULL);
}


//----------------------------------------------------------------------------

//
// SR_PlayerGetState
//
bool SR_PlayerGetState(void *storage, int index, void *extra)
{
	state_t ** dest = (state_t **)storage + index;

	char buffer[256];
	char *base_p, *off_p;
	int base, offset;

	const char *swizzle;
	const weapondef_c *actual;

	swizzle = SV_GetString();

	if (! swizzle)
	{
		*dest = NULL;
		return true;
	}

	Z_StrNCpy(buffer, swizzle, 256-1);
	SV_FreeString(swizzle);

	// separate string at `:' characters

	base_p = strchr(buffer, ':');

	if (base_p == NULL || base_p[0] == 0)
		I_Error("Corrupt savegame: bad weapon state 1: `%s'\n", buffer);

	*base_p++ = 0;

	off_p = strchr(base_p, ':');

	if (off_p == NULL || off_p[0] == 0)
		I_Error("Corrupt savegame: bad weapon state 2: `%s'\n", base_p);

	*off_p++ = 0;

	// find weapon that contains the state
	// Traverses backwards in case #CLEARALL was used.
	actual = NULL;

/*	for (i=numweapons-1; i >= 0; i--)
	{
		actual = weaponinfo[i];

		if (! actual->ddf.name)
			continue;

		if (DDF_CompareName(buffer, actual->ddf.name) == 0)
			break;
	}
*/
	actual = weapondefs.Lookup(buffer);
	if (!actual)
		I_Error("LOADGAME: no such weapon %s for state %s:%s\n",
		buffer, base_p, off_p);

	// find base state
	offset = strtol(off_p, NULL, 0) - 1;

	for (base=actual->first_state; base <= actual->last_state; base++)
	{
		if (! states[base].label)
			continue;

		if (DDF_CompareName(base_p, states[base].label) == 0)
			break;
	}

	if (base > actual->last_state)
	{
		I_Warning("LOADGAME: no such label `%s' for weapon state.\n", base_p);

		offset = 0;
		base = actual->ready_state;
	}

#if 0
	L_WriteDebug("Unswizzled weapon state `%s:%s:%s' -> %d\n", 
		buffer, base_p, off_p, base + offset);
#endif

	*dest = states + base + offset;

	return true;
}

//
// SR_PlayerPutState
//
// The format of the string is:
//
//    WEAPON:BASE:OFFSET
//
// where WEAPON refers the ddf weapon containing the state.  BASE is
// the nearest labelled state (e.g. "SPAWN"), or "*" as offset from
// the weapon's first state (unlikely to be needed).  OFFSET is the
// integer offset from the base state, which BTW starts at 1 (like in
// ddf).
//
// Alternatively, the string can be NULL, which means the state
// pointer should be NULL.
//
void SR_PlayerPutState(void *storage, int index, void *extra)
{
	state_t *S = ((state_t **)storage)[index];

	epi::array_iterator_c it;
	epi::string_c buf;
	int s_num, base;

	const weapondef_c *actual;

	if (S == NULL)
	{
		SV_PutString(NULL);
		return;
	}

	// get state number, check if valid
	s_num = S - states;

	if (s_num < 0 || s_num >= num_states)
	{
		I_Warning("SAVEGAME: weapon is in invalid state %d\n", s_num);
		s_num = weapondefs[0]->first_state;
	}

	// find the weapon that this state belongs to.
	// Traverses backwards in case #CLEARALL was used.
	actual = NULL;

/*
	for (i=numweapons-1; i >= 0; i--)
	{
		actual = weaponinfo[i];

		if (actual->last_state <= 0 ||
			actual->last_state < actual->first_state)
			continue;

		if (actual->first_state <= s_num && s_num <= actual->last_state)
			break;
	}
*/
	
	for (it=weapondefs.GetIterator(weapondefs.GetDisabledCount());
			it.IsValid(); it++)
	{
		actual = ITERATOR_TO_TYPE(it, weapondef_c*);

		if (actual->last_state <= 0 ||
			actual->last_state < actual->first_state)
			continue;

		if (actual->first_state <= s_num && s_num <= actual->last_state)
			break;
	}

	if (!it.IsValid())
	{
		I_Warning("SAVEGAME: weapon state %d cannot be found !!\n", s_num);
		actual = weapondefs[0];
		s_num = actual->first_state;
	}

	// find the nearest base state

	for (base = s_num; 
		base > actual->first_state && states[base].label == NULL;
		base--)
	{ /* nothing */ }

	buf.Format("%s:%s:%d", actual->ddf.name.GetString(),
		states[base].label ? states[base].label : "*",
		1 + s_num - base);

#if 0
	L_WriteDebug("Swizzled state of weapon %d -> `%s'\n", s_num, buf.GetString());
#endif

	SV_PutString(buf.GetString());
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
