//------------------------------------------------------------------------
//  COAL Play Simulation Interface
//------------------------------------------------------------------------
//
//  Copyright (c) 2006-2009  The EDGE Team.
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
//------------------------------------------------------------------------

#include "i_defs.h"

#include "coal/coal.h"

#include "ddf/types.h"

#include "vm_coal.h"
#include "p_local.h"  
#include "p_mobj.h"
#include "r_state.h"

#include "g_state.h"
#include "e_main.h"
#include "g_game.h"

#include "e_player.h"
#include "hu_font.h"
#include "hu_draw.h"
#include "r_modes.h"
#include "r_image.h"
#include "r_sky.h"


extern coal::vm_c *ui_vm;

extern void VM_SetFloat(coal::vm_c *vm, const char *name, double value);
extern void VM_CallFunction(coal::vm_c *vm, const char *name);


player_t * ui_player_who = NULL;


//------------------------------------------------------------------------
//  PLAYER MODULE
//------------------------------------------------------------------------


// player.num_players()
//
static int PL_num_players(lua_State *L)
{
	lua_pushinteger(L, numplayers);
	return 1;
}


// player.set_who(index)
//
static int PL_set_who(lua_State *L)
{
	int index = luaL_checkint(L, 1);

	if (index < 0 || index >= numplayers)
		I_Error("player.set_who: bad index value: %d (numplayers=%d)\n", index, numplayers);

	if (index == 0)
	{
		cur_player = players[consoleplayer];
		return 0;
	}

	int who = displayplayer;

	for (; index > 1; index--)
	{
		do
		{
			who = (who + 1) % MAXPLAYERS;
		}
		while (players[who] == NULL);
	}

	cur_player = players[who];

	return 0;
}


// player.is_bot()
//
static int PL_is_bot(lua_State *L)
{
	lua_pushboolean(L, (cur_player->playerflags & PFL_Bot) ? 1 : 0);
	return 1;
}


// player.get_name()
//
static int PL_get_name(lua_State *L)
{
	lua_pushstring(L, cur_player->playername);
	return 1;
}


// player.health()
//
static int PL_health(lua_State *L)
{
	float h = cur_player->health * 100 / cur_player->mo->info->spawnhealth;

	lua_pushinteger(L, (int)floor(h + 0.99));
	return 1;
}


// player.armor(type)
//
static int PL_armor(lua_State *L)
{
	int kind = luaL_checkint(L, 1);

	if (kind < 1 || kind > NUMARMOUR)
		I_Error("player.armor: bad armor index: %d\n", kind);
	
	kind--;

	lua_pushinteger(L, (int)floor(cur_player->armours[kind] + 0.99));
	return 1;
}


// player.total_armor(type)
//
static int PL_total_armor(lua_State *L)
{
	lua_pushinteger(L, (int)floor(cur_player->totalarmour + 0.99));
	return 1;
}


// player.frags()
//
static int PL_frags(lua_State *L)
{
	lua_pushinteger(L, cur_player->frags);
	return 1;
}


// player.under_water()
//
static int PL_under_water(lua_State *L)
{
	lua_pushboolean(L, cur_player->underwater ? 1 : 0);
	return 1;
}


// player.on_ground()
//
static int PL_on_ground(lua_State *L)
{
	lua_pushboolean(L, (cur_player->mo->z <= cur_player->mo->floorz) ? 1 : 0);
	return 1;
}


// player.is_swimming()
//
static int PL_is_swimming(lua_State *L)
{
	lua_pushboolean(L, cur_player->swimming ? 1 : 0);
	return 1;
}


// player.is_jumping()
//
static int PL_is_jumping(lua_State *L)
{
	lua_pushboolean(L, (cur_player->jumpwait > 0) ? 1 : 0);
	return 1;
}


// player.is_crouching()
//
static int PL_is_crouching(lua_State *L)
{
	lua_pushboolean(L, (cur_player->mo->extendedflags & EF_CROUCHING) ? 1 : 0);
	return 1;
}


// player.is_attacking()
//
static int PL_is_attacking(lua_State *L)
{
	lua_pushboolean(L, (cur_player->attackdown[0] ||
	                    cur_player->attackdown[1]) ? 1 : 0);
	return 1;
}


// player.is_rampaging()
//
static int PL_is_rampaging(lua_State *L)
{
	lua_pushboolean(L, (cur_player->attackdown_count >= 70) ? 1 : 0);
	return 1;
}


// player.is_grinning()
//
static int PL_is_grinning(lua_State *L)
{
	lua_pushboolean(L, (cur_player->grin_count > 0) ? 1 : 0);
	return 1;
}


// player.is_using()
//
static int PL_is_using(lua_State *L)
{
	lua_pushboolean(L, cur_player->usedown ? 1 : 0);
	return 1;
}


// player.move_speed()
//
static int PL_move_speed(lua_State *L)
{
	lua_pushnumber(L, cur_player->actual_speed);
	return 1;
}


// player.air_in_lungs()
//
static int PL_air_in_lungs(lua_State *L)
{
	if (cur_player->air_in_lungs <= 0)
	{
		lua_pushnumber(L, 0.0f);
		return 1;
	}

	float value = cur_player->air_in_lungs * 100.0f /
	              cur_player->mo->info->lung_capacity;

	value = CLAMP(0.0f, value, 100.0f);

	lua_pushnumber(L, value);
	return 1;
}


// player.has_key(key)
//
static int PL_has_key(lua_State *L)
{
	int key = luaL_checkint(L, 1);

	if (key < 1 || key > 16)
		I_Error("player.has_key: bad key number: %d\n", key);

	key--;

	int value = (cur_player->cards & (1 << key)) ? 1 : 0;

	lua_pushboolean(L, value);
	return 1;
}


// player.has_power(power)
//
static int PL_has_power(lua_State *L)
{
	int power = luaL_checkint(L, 1);

	if (power < 1 || power > NUMPOWERS)
		I_Error("player.has_power: bad powerup number: %d\n", power);

	power--;

	int value = (cur_player->powers[power] > 0) ? 1 : 0;

	// special check for GOD mode
	if (power == PW_Invulnerable && (cur_player->cheats & CF_GODMODE))
		value = 1;

	lua_pushboolean(L, value);
	return 1;
}


// player.power_left(power)
//
static int PL_power_left(lua_State *L)
{
	int power = luaL_checkint(L, 1);

	if (power < 1 || power > NUMPOWERS)
		I_Error("player.power_left: bad powerup number: %d\n", power);

	power--;

	float value = cur_player->powers[power];

	if (value > 0)
		value /= TICRATE;

	lua_pushnumber(L, value);
	return 1;
}


// player.has_weapon_slot(slot)
//
static int PL_has_weapon_slot(lua_State *L)
{
	int slot = luaL_checkint(L, 1);

	if (slot < 0 || slot > 9)
		I_Error("player.has_weapon_slot: bad slot number: %d\n", slot);

	int value = cur_player->avail_weapons[slot] ? 1 : 0;

	lua_pushboolean(L, value);
	return 1;
}


static int PL_cur_weapon_slot(lua_State *L)
{
	int slot;

	if (cur_player->ready_wp < 0)
		slot = -1;
	else
		slot = cur_player->weapons[cur_player->ready_wp].info->bind_key;

	lua_pushinteger(L, slot);
	return 1;
}


// player.has_weapon(name)
//
static int PL_has_weapon(lua_State *L)
{
	const char * name = luaL_checkstring(L, 1);
	SYS_ASSERT(name);

	for (int j = 0; j < MAXWEAPONS; j++)
	{
		playerweapon_t *pw = &cur_player->weapons[j];

		if (pw->owned && ! (pw->flags & PLWEP_Removing))
		{
			if (DDF_CompareName(name, pw->info->name.c_str()) == 0)
			{
				lua_pushboolean(L, 1);
				return 1;
			}
		}
	}

	lua_pushboolean(L, 0);
	return 1;
}


static int PL_cur_weapon(lua_State *L)
{
	if (cur_player->pending_wp >= 0)
	{
		lua_pushstring(L, "change");
		return 1;
	}

	if (cur_player->ready_wp < 0)
	{
		lua_pushstring(L, "none");
		return 1;
	}

	weapondef_c *info = cur_player->weapons[cur_player->ready_wp].info;

	lua_pushstring(L, info->name.c_str());
	return 1;
}


// player.ammo(type)
//
static int PL_ammo(lua_State *L)
{
	int ammo = luaL_checkint(L, 1);

	if (ammo < 1 || ammo > NUMAMMO)
		I_Error("player.ammo: bad ammo number: %d\n", ammo);

	ammo--;

	lua_pushinteger(L, cur_player->ammo[ammo].num);
	return 1;
}


// player.ammomax(type)
//
static int PL_ammomax(lua_State *L)
{
	int ammo = luaL_checkint(L, 1);

	if (ammo < 1 || ammo > NUMAMMO)
		I_Error("player.ammomax: bad ammo number: %d\n", ammo);

	ammo--;

	lua_pushinteger(L, cur_player->ammo[ammo].max);
	return 1;
}


// player.main_ammo(clip)
//
static int PL_main_ammo(lua_State *L)
{
	int value = 0;

	if (cur_player->ready_wp >= 0)
	{
		playerweapon_t *pw = &cur_player->weapons[cur_player->ready_wp];

		if (pw->info->ammo[0] != AM_NoAmmo)
		{
			if (pw->info->show_clip)
			{
				SYS_ASSERT(pw->info->ammopershot[0] > 0);

				value = pw->clip_size[0] / pw->info->ammopershot[0];
			}
			else
			{
				value = cur_player->ammo[pw->info->ammo[0]].num;

				if (pw->info->clip_size[0] > 0)
					value += pw->clip_size[0];
			}
		}
	}

	lua_pushinteger(L, value);
	return 1;
}


// player.ammo_type(ATK)
//
static int PL_ammo_type(lua_State *L)
{
	int ATK = luaL_checkint(L, 1);

	if (ATK < 1 || ATK > 2)
		I_Error("player.ammo_type: bad attack number: %d\n", ATK);

	ATK--;

	int value = 0;

	if (cur_player->ready_wp >= 0)
	{
		playerweapon_t *pw = &cur_player->weapons[cur_player->ready_wp];
		
		value = 1 + (int) pw->info->ammo[ATK];
	}

	lua_pushinteger(L, value);
	return 1;
}


// player.ammo_pershot(ATK)
//
static int PL_ammo_pershot(lua_State *L)
{
	int ATK = luaL_checkint(L, 1);

	if (ATK < 1 || ATK > 2)
		I_Error("player.ammo_pershot: bad attack number: %d\n", ATK);

	ATK--;

	int value = 0;

	if (cur_player->ready_wp >= 0)
	{
		playerweapon_t *pw = &cur_player->weapons[cur_player->ready_wp];
		
		value = pw->info->ammopershot[ATK];
	}

	lua_pushinteger(L, value);
	return 1;
}


// player.clip_ammo(ATK)
//
static int PL_clip_ammo(lua_State *L)
{
	int ATK = luaL_checkint(L, 1);

	if (ATK < 1 || ATK > 2)
		I_Error("player.clip_ammo: bad attack number: %d\n", ATK);

	ATK--;

	int value = 0;

	if (cur_player->ready_wp >= 0)
	{
		playerweapon_t *pw = &cur_player->weapons[cur_player->ready_wp];

		value = pw->clip_size[ATK];
	}

	lua_pushinteger(L, value);
	return 1;
}


// player.clip_size(ATK)
//
static int PL_clip_size(lua_State *L)
{
	int ATK = luaL_checkint(L, 1);

	if (ATK < 1 || ATK > 2)
		I_Error("player.clip_size: bad attack number: %d\n", ATK);

	ATK--;

	int value = 0;

	if (cur_player->ready_wp >= 0)
	{
		playerweapon_t *pw = &cur_player->weapons[cur_player->ready_wp];

		value = pw->info->clip_size[ATK];
	}

	lua_pushinteger(L, value);
	return 1;
}


// player.clip_is_shared()
//
static int PL_clip_is_shared(lua_State *L)
{
	int value = 0;

	if (cur_player->ready_wp >= 0)
	{
		playerweapon_t *pw = &cur_player->weapons[cur_player->ready_wp];

		if (pw->info->shared_clip)
			value = 1;
	}

	lua_pushboolean(L, value);
	return 1;
}


// player.hurt_by()
//
static int PL_hurt_by(lua_State *L)
{
	if (cur_player->damagecount <= 0)
		return 0;  // return NIL

	// getting hurt because of your own damn stupidity
	if (cur_player->attacker == cur_player->mo)
		lua_pushstring(L, "self");
	else if (cur_player->attacker && (cur_player->attacker->side & cur_player->mo->side))
		lua_pushstring(L, "friend");
	else if (cur_player->attacker)
		lua_pushstring(L, "enemy");
	else
		lua_pushstring(L, "other");

	return 1;
}


// player.hurt_mon()
//
static int PL_hurt_mon(lua_State *L)
{
	if (cur_player->damagecount > 0 &&
		cur_player->attacker &&
		cur_player->attacker != cur_player->mo)
	{
		lua_pushstring(L, cur_player->attacker->info->name.c_str());
		return 1;
	}

	return 0;  // return NIL
}


// player.hurt_pain()
//
static int PL_hurt_pain(lua_State *L)
{
	lua_pushnumber(L, cur_player->damage_pain);
	return 1;
}


// player.hurt_dir()
//
static int PL_hurt_dir(lua_State *L)
{
	int dir = 0;

	if (cur_player->attacker && cur_player->attacker != cur_player->mo)
	{
		mobj_t *badguy = cur_player->attacker;
		mobj_t *pmo    = cur_player->mo;

		angle_t diff = R_PointToAngle(pmo->x, pmo->y, badguy->x, badguy->y) - pmo->angle;

		if (diff >= ANG45 && diff <= ANG135)
		{
			dir = -1;
		}
		else if (diff >= ANG225 && diff <= ANG315)
		{
			dir = +1;
		}
	}

	lua_pushinteger(L, dir);
	return 1;
}


// player.hurt_angle()
//
static int PL_hurt_angle(lua_State *L)
{
	float value = 0;

	if (cur_player->attacker && cur_player->attacker != cur_player->mo)
	{
		mobj_t *badguy = cur_player->attacker;
		mobj_t *pmo    = cur_player->mo;

		angle_t real_a = R_PointToAngle(pmo->x, pmo->y, badguy->x, badguy->y);

		value = ANG_2_FLOAT(real_a);

		if (value > 360.0f)
			value -= 360.0f;

		if (value < 0)
			value += 360.0f;
	}

	lua_pushnumber(L, value);
	return 1;
}


//------------------------------------------------------------------------


void VM_RegisterPlaysim()
{
	ui_vm->AddNativeFunction("player.num_players", PL_num_players);
	ui_vm->AddNativeFunction("player.set_who",     PL_set_who);
    ui_vm->AddNativeFunction("player.is_bot",      PL_is_bot);
    ui_vm->AddNativeFunction("player.get_name",    PL_get_name);

    ui_vm->AddNativeFunction("player.health",      PL_health);
    ui_vm->AddNativeFunction("player.armor",       PL_armor);
    ui_vm->AddNativeFunction("player.total_armor", PL_total_armor);
    ui_vm->AddNativeFunction("player.frags",       PL_frags);
    ui_vm->AddNativeFunction("player.ammo",        PL_ammo);
    ui_vm->AddNativeFunction("player.ammomax",     PL_ammomax);

    ui_vm->AddNativeFunction("player.is_swimming",     PL_is_swimming);
    ui_vm->AddNativeFunction("player.is_jumping",      PL_is_jumping);
    ui_vm->AddNativeFunction("player.is_crouching",    PL_is_crouching);
    ui_vm->AddNativeFunction("player.is_using",        PL_is_using);
    ui_vm->AddNativeFunction("player.is_attacking",    PL_is_attacking);
    ui_vm->AddNativeFunction("player.is_rampaging",    PL_is_rampaging);
    ui_vm->AddNativeFunction("player.is_grinning",     PL_is_grinning);

    ui_vm->AddNativeFunction("player.under_water",     PL_under_water);
    ui_vm->AddNativeFunction("player.on_ground",       PL_on_ground);
    ui_vm->AddNativeFunction("player.move_speed",      PL_move_speed);
    ui_vm->AddNativeFunction("player.air_in_lungs",    PL_air_in_lungs);

    ui_vm->AddNativeFunction("player.has_key",         PL_has_key);
    ui_vm->AddNativeFunction("player.has_power",       PL_has_power);
    ui_vm->AddNativeFunction("player.power_left",      PL_power_left);
    ui_vm->AddNativeFunction("player.has_weapon",      PL_has_weapon);
    ui_vm->AddNativeFunction("player.has_weapon_slot", PL_has_weapon_slot);
    ui_vm->AddNativeFunction("player.cur_weapon",      PL_cur_weapon);
    ui_vm->AddNativeFunction("player.cur_weapon_slot", PL_cur_weapon_slot);

    ui_vm->AddNativeFunction("player.main_ammo",       PL_main_ammo);
    ui_vm->AddNativeFunction("player.ammo_type",       PL_ammo_type);
    ui_vm->AddNativeFunction("player.ammo_pershot",    PL_ammo_pershot);
    ui_vm->AddNativeFunction("player.clip_ammo",       PL_clip_ammo);
    ui_vm->AddNativeFunction("player.clip_size",       PL_clip_size);
    ui_vm->AddNativeFunction("player.clip_is_shared",  PL_clip_is_shared);

    ui_vm->AddNativeFunction("player.hurt_by",         PL_hurt_by);
    ui_vm->AddNativeFunction("player.hurt_mon",        PL_hurt_mon);
    ui_vm->AddNativeFunction("player.hurt_pain",       PL_hurt_pain);
    ui_vm->AddNativeFunction("player.hurt_dir",        PL_hurt_dir);
    ui_vm->AddNativeFunction("player.hurt_angle",      PL_hurt_angle);
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
