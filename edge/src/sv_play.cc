//----------------------------------------------------------------------------
//  EDGE New SaveGame Handling (Players)
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2003  The EDGE Team.
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

#include "dm_state.h"
#include "e_player.h"
#include "p_local.h"
#include "sv_chunk.h"
#include "sv_main.h"
#include "r_state.h"
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
  SF(playername[0], "playername", 1, SVT_STRING, 
      SR_PlayerGetName, SR_PlayerPutName),
	SF(mo, "mo", 1, SVT_INDEX("mobjs"), SR_MobjGetMobj, SR_MobjPutMobj),
	SF(viewz, "viewz", 1, SVT_FLOAT, SR_GetFloat, SR_PutFloat),
	SF(viewheight, "viewheight", 1, SVT_FLOAT, SR_GetFloat, SR_PutFloat),
  SF(deltaviewheight, "deltaviewheight", 1, SVT_FLOAT, 
      SR_GetFloat, SR_PutFloat),
  SF(std_viewheight, "std_viewheight", 1, SVT_FLOAT, 
      SR_GetFloat, SR_PutFloat),
	SF(health, "health", 1, SVT_FLOAT, SR_GetFloat, SR_PutFloat),
	SF(armours[0], "armours", NUMARMOUR, SVT_FLOAT, SR_GetFloat, SR_PutFloat),
	SF(powers[0],  "powers",  NUMPOWERS, SVT_FLOAT, SR_GetFloat, SR_PutFloat),
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
	SF(air_in_lungs, "air_in_lungs", 1, SVT_INT, SR_GetInt, SR_PutInt),
	SF(underwater, "underwater", 1, SVT_BOOLEAN, SR_GetBoolean, SR_PutBoolean),
	SF(flash, "flash_b", 1, SVT_BOOLEAN, SR_GetBoolean, SR_PutBoolean),
	SF(psprites[0], "psprites", NUMPSPRITES, SVT_STRUCT("psprite_t"), 
	SR_PlayerGetPSprite, SR_PlayerPutPSprite),

	// NOT HERE:
	//   in_game: only in-game players are saved.
	//   key_choices: depends on DDF too much, and not important.
	//   remember_atk1/2: ditto.
	//   next,prev: links are regenerated.
	//   avail_weapons: regenerated.
	
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
	SF(clip_size, "clip_size", 1, SVT_INT, SR_GetInt, SR_PutInt),
	SF(sa_clip_size, "sa_clip_size", 1, SVT_INT, SR_GetInt, SR_PutInt),
  
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

//
// SV_PlayerCountElems
//
int SV_PlayerCountElems(void)
{
	player_t *p;
	int count=0;

	for (p=players; p; p=p->next)
		count++;

	return count;
}

//
// SV_PlayerGetElem
//
void *SV_PlayerGetElem(int index)
{
	player_t *p;

	for (p=players; p && index > 0; p=p->next)
		index--;

	if (!p)
		I_Error("LOADGAME: Invalid Player: %d\n", index);

	DEV_ASSERT2(index == 0);

	return p;
}

//
// SV_PlayerFindElem
//
int SV_PlayerFindElem(player_t *elem)
{
	player_t *p;
	int index;

	for (p=players, index=0; p && p != elem; p=p->next)
		index++;

	if (!p)
		I_Error("LOADGAME: No such PlayerPtr: %p\n", elem);

	return index;
}

//
// SV_PlayerCreateElems
//
void SV_PlayerCreateElems(int num_elems)
{
	int j;

	// free existing players
	P_RemoveAllPlayers();

	players = NULL;

	for (; num_elems > 0; num_elems--)
	{
		player_t *p = Z_ClearNew(player_t, 1);

		p->pnum = num_elems-1;
		sprintf(p->playername, "Player%d", 1 + p->pnum);

		p->next = players;
		p->prev = NULL;

		if (players)
			players->prev = p;

		players = p;

		// initialise defaults
		p->in_game = true;
		p->remember_atk1 = p->remember_atk2 = -1;

		for (j=0; j < NUMPSPRITES; j++)
		{
			p->psprites[j].sx = 1.0f;
			p->psprites[j].sy = WEAPONTOP;
		}
	}
}

//
// SV_PlayerFinaliseElems
//
void SV_PlayerFinaliseElems(void)
{
	player_t *p;

	// create playerlookup[] array
	if (! playerlookup)
		playerlookup = Z_ClearNew(player_t *, MAXPLAYERS);

	for (p=players; p; p=p->next)
	{
		if (p->pnum >= MAXPLAYERS)
		{
			// Ouch!!
			continue;
		}

		if (playerlookup[p->pnum])
			I_Error("LOADGAME: Two players with same number !\n");

		if (! p->mo)
			I_Error("LOADGAME: Player %d has no mobj !\n", p->pnum);

		playerlookup[p->pnum] = p;

		p->thinker = P_ConsolePlayerThinker;  //!!!!! FIXME

		P_UpdateAvailWeapons(p);
	}

	consoleplayer = displayplayer = players;  //!!!! FIXME
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

	DEV_ASSERT(index == 0, ("SR_PlayerGetName: index != 0"));

	str = SV_GetString();
	Z_StrNCpy(dest, str, MAX_PLAYNAME-1);
	Z_Free((char *)str);

	return true;
}

//
// SR_PlayerPutName
//
void SR_PlayerPutName(void *storage, int index, void *extra)
{
	char *src = (char *)storage;

	DEV_ASSERT(index == 0, ("SR_PlayerGetName: index != 0"));

	SV_PutString(src);
}

//
// SR_WeaponGetInfo
//
bool SR_WeaponGetInfo(void *storage, int index, void *extra)
{
	weaponinfo_t ** dest = (weaponinfo_t **)storage + index;
	const char *name;
	int num;

	name = SV_GetString();

	num = name ? DDF_WeaponLookup(name) : -1;
	Z_Free((char *)name);

	*dest = (num < 0) ? NULL : weaponinfo[num];
	return true;
}

//
// SR_WeaponPutInfo
//
void SR_WeaponPutInfo(void *storage, int index, void *extra)
{
	weaponinfo_t *info = ((weaponinfo_t **)storage)[index];

	SV_PutString(info ? info->ddf.name : NULL);
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
	int i, base, offset;

	const char *swizzle;
	const weaponinfo_t *actual;

	swizzle = SV_GetString();

	if (! swizzle)
	{
		*dest = NULL;
		return true;
	}

	Z_StrNCpy(buffer, swizzle, 256-1);
	Z_Free((char *)swizzle);

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

	for (i=numweapons-1; i >= 0; i--)
	{
		actual = weaponinfo[i];

		if (! actual->ddf.name)
			continue;

		if (DDF_CompareName(buffer, actual->ddf.name) == 0)
			break;
	}

	if (i < 0)
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

	char swizzle[64];
	int i, s_num, base;

	const weaponinfo_t *actual;

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
		s_num = weaponinfo[0]->first_state;
	}

	// find the weapon that this state belongs to.
	// Traverses backwards in case #CLEARALL was used.
	actual = NULL;

	for (i=numweapons-1; i >= 0; i--)
	{
		actual = weaponinfo[i];

		if (actual->last_state <= 0 ||
			actual->last_state < actual->first_state)
			continue;

		if (actual->first_state <= s_num && s_num <= actual->last_state)
			break;
	}

	if (i < 0)
	{
		I_Warning("SAVEGAME: weapon state %d cannot be found !!\n", s_num);
		actual = weaponinfo[0];
		s_num = actual->first_state;
	}

	// find the nearest base state

	for (base = s_num; 
		base > actual->first_state && states[base].label == NULL;
		base--)
	{ /* nothing */ }

	sprintf(swizzle, "%s:%s:%d", actual->ddf.name,
		states[base].label ? states[base].label : "*",
		1 + s_num - base);

#if 0
	L_WriteDebug("Swizzled state of weapon %d -> `%s'\n", s_num, swizzle);
#endif

	SV_PutString(swizzle);
}

