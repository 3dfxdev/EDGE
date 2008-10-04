//------------------------------------------------------------------------
//  LUA Play Simulation Interface
//------------------------------------------------------------------------
//
//  Copyright (c) 2006-2008  The EDGE Team.
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
#include "i_luainc.h"

#include "l_lua.h"
#include "p_vm.h"

#include "ddf/types.h"
#include "p_local.h"  
#include "p_mobj.h"
#include "r_state.h"

#include "dm_state.h"
#include "e_main.h"
#include "g_game.h"

#include "e_player.h"
#include "hu_font.h"
#include "r_draw.h"
#include "r_modes.h"
#include "r_image.h"
#include "r_sky.h"


static player_t *cur_player = NULL;


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
			if (DDF_CompareName(name, pw->info->ddf.name.c_str()) == 0)
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

	lua_pushstring(L, info->ddf.name.c_str());
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

	if (cur_player->attacker)
	{
		lua_pushstring(L, "enemy");
		return 1;
	}

	// getting hurt because of your own damn stupidity
	lua_pushstring(L, "self");
	return 1;
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


const luaL_Reg player_module[] =
{
	{ "num_players", PL_num_players },
	{ "set_who",     PL_set_who  },
    { "is_bot",      PL_is_bot   },
    { "get_name",    PL_get_name },

    { "health",      PL_health      },
    { "armor",       PL_armor       },
    { "total_armor", PL_total_armor },
    { "frags",       PL_frags       },
    { "ammo",        PL_ammo        },
    { "ammomax",     PL_ammomax     },

    { "is_swimming",     PL_is_swimming   },
    { "is_jumping",      PL_is_jumping    },
    { "is_crouching",    PL_is_crouching  },
    { "is_using",        PL_is_using      },
    { "is_attacking",    PL_is_attacking  },
    { "is_rampaging",    PL_is_rampaging  },
    { "is_grinning",     PL_is_grinning   },

    { "under_water",     PL_under_water   },
    { "on_ground",       PL_on_ground     },
    { "move_speed",      PL_move_speed    },
    { "air_in_lungs",    PL_air_in_lungs  },

    { "has_key",         PL_has_key   },
    { "has_power",       PL_has_power },
    { "power_left",      PL_power_left },
    { "has_weapon",      PL_has_weapon      },
    { "has_weapon_slot", PL_has_weapon_slot },
    { "cur_weapon",      PL_cur_weapon      },
    { "cur_weapon_slot", PL_cur_weapon_slot },

    { "main_ammo",       PL_main_ammo  },
    { "ammo_type",       PL_ammo_type  },
    { "ammo_pershot",    PL_ammo_pershot },
    { "clip_ammo",       PL_clip_ammo  },
    { "clip_size",       PL_clip_size  },
    { "clip_is_shared",  PL_clip_is_shared },

    { "hurt_by",         PL_hurt_by    },
    { "hurt_pain",       PL_hurt_pain  },
    { "hurt_dir",        PL_hurt_dir   },
    { "hurt_angle",      PL_hurt_angle },

	{ NULL, NULL } // the end
};


//------------------------------------------------------------------------
//  WORLD MODULE
//------------------------------------------------------------------------

// world.activate_line_type(type, tag)
//
static int PS_activate_line_type(lua_State *L)
{
    if (lua_gettop(L) != 2)
    {
        return luaL_error(L, 
                          "\nRequired parameters: %s", 
                          "<linedef-action-type> <tag>");
    }

    int typenum = luaL_checkint(L, 1);
    int tag = luaL_checkint(L, 2);

	P_RemoteActivation(NULL, typenum, tag, 0, line_Any);

    return 0;
}

// ---- Sidedefs ----
typedef enum
{
    PS_SDPROP_UPPERTEX,
    PS_SDPROP_MIDDLETEX,
    PS_SDPROP_LOWERTEX,
    PS_SDPROP_NUMTYPES
}
ps_sidedef_prop_e;

static const char *ps_sidedef_properties[PS_SDPROP_NUMTYPES] = 
{ 
    "upper_tex", 
    "middle_tex",
    "lower_tex"
};

static void SetSidedefTexture(int sd_num, 
                              int prop_type, 
                              const image_c* image)
{
    if (sd_num < 0 || sd_num >= numsides)
        return;

    side_t *side = sides + sd_num;

    if (prop_type == PS_SDPROP_UPPERTEX)
        side->top.image = image;
    else if (prop_type == PS_SDPROP_MIDDLETEX)
        side->middle.image = image;
    else /* if (prop_type == PS_SDPROP_LOWERTEX) */
        side->bottom.image = image;
}


// world.get_sidedef_prop(prop_type, sidedef_num)
//
static int PS_get_sidedef_prop(lua_State *L)
{
    if (lua_gettop(L) < 2)
    {
        return luaL_error(L, 
                   "\nRequired parameters: %s", 
                   "<property>, <sidedef_no>");
    }

    // Get property type
    const char *prop = lua_tolstring(L, 1, NULL);

    int prop_type = 0; 
    while(prop_type < PS_SDPROP_NUMTYPES && 
          strcmp(ps_sidedef_properties[prop_type],prop))
        prop_type++;

    if (prop_type == PS_SDPROP_NUMTYPES)
        return luaL_error(L, "Invalid property");

    // Get property from sector
    int sd_num = luaL_checkint(L, 2);
    if (sd_num >= 0 && sd_num < numlines)
    {
        side_t *side = sides + sd_num;
        const image_c *image = NULL;

        switch (prop_type)
        {
        case PS_SDPROP_UPPERTEX:
        case PS_SDPROP_MIDDLETEX:
        case PS_SDPROP_LOWERTEX:
            if (prop_type == PS_SDPROP_UPPERTEX)
                image = side->top.image;
            else if (prop_type == PS_SDPROP_MIDDLETEX)
                image = side->middle.image;
            else /* if (prop_type == PS_SDPROP_LOWERTEX) */
                image = side->bottom.image;

            if (image)
                lua_pushstring(L, image->name);
            else
                lua_pushnil(L);
            break;

        default:
            lua_pushnil(L);
            break;
        }

    }
    else
    {
        lua_pushnil(L);
    }

    return 1;
}

// world.set_sidedef_prop(prop_type, sidedef_num, attribute_value)
//
static int PS_set_sidedef_prop(lua_State *L)
{
    if (lua_gettop(L) < 3)
    {
        return luaL_error(L, 
                          "\nRequired parameters: %s", 
                          "<property>, <sidedef_num>, <attribute_value>");
    }

    // Get property type
    const char *prop = lua_tolstring(L, 1, NULL);

    int prop_type = 0; 
    while(prop_type < PS_SDPROP_NUMTYPES && 
          strcmp(ps_sidedef_properties[prop_type],prop))
        prop_type++;

    if (prop_type == PS_SDPROP_NUMTYPES)
        return luaL_error(L, "Invalid property");

    // Set sidedef(s)
    if (prop_type >= PS_SDPROP_UPPERTEX &&
        prop_type <= PS_SDPROP_LOWERTEX)
    {
        const image_c *image = W_ImageLookup(luaL_checkstring(L, 3),
                                             INS_Texture);
        if (image)
        {
            int type = lua_type(L, 2);
            if (type == LUA_TTABLE)
            {
                lua_pushnil(L);  // Get first key
                while (lua_next(L, 2) != 0) {
                    if (lua_type(L, -1) == LUA_TNUMBER)
                        SetSidedefTexture(lua_tointeger(L, -1),
                                          prop_type,
                                          image);

                    lua_pop(L, 1); // Pop value, leaving the key for next func
                }
            }
            else if (type == LUA_TNUMBER)
            {
                SetSidedefTexture(luaL_checkint(L, 2),
                                  prop_type,
                                  image);
            }
            else 
            {
                return luaL_error(L, "Expected numeric or table.");
            }
        }
    }
    return 0;
}


// ---- Lines ----
typedef enum
{
    PS_LINEPROP_SIDEDEFS,
    PS_LINEPROP_NUMTYPES
}
ps_line_prop_e;

static const char *ps_line_properties[PS_LINEPROP_NUMTYPES] = 
{ 
    "sidedefs"
};

// world.get_line_prop(prop_type, line_num)
//
static int PS_get_line_prop(lua_State *L)
{
    if (lua_gettop(L) < 2)
    {
        return luaL_error(L, 
                          "\nRequired parameters: %s", 
                          "<property>, <line_num>");
    }

    // Get property type
    const char *prop = lua_tolstring(L, 1, NULL);

    int prop_type = 0; 
    while(prop_type < PS_LINEPROP_NUMTYPES && 
          strcmp(ps_line_properties[prop_type],prop))
        prop_type++;

    if (prop_type == PS_LINEPROP_NUMTYPES)
        return luaL_error(L, "Invalid property");

    // Get property from sector
    int line_num = luaL_checkint(L, 2);
    if (line_num >= 0 && line_num < numlines)
    {
        line_t *line = lines + line_num;

        switch(prop_type)
        {

        case PS_LINEPROP_SIDEDEFS:
        {
            lua_createtable(L, 0, 2);
            int table_index = lua_gettop(L);

            side_t* side;
            for (int i=0; i<2; i++)
            {
                side = line->side[i];
                if (side)
                    lua_pushinteger(L, side - sides);
                else
                    lua_pushnil(L);

                lua_rawseti(L, table_index, i+1);
            }
            break;
        }

        default:
            lua_pushnil(L);
            break;
        }
    }
    else
    {
        lua_pushnil(L);
    }

    return 1;
}


// ---- Sectors ----
typedef enum
{
    PS_SECTPROP_LIGHTING,
    PS_SECTPROP_CEILING,
    PS_SECTPROP_FLOOR,
    PS_SECTPROP_CEILING_TEX,
    PS_SECTPROP_FLOOR_TEX,
    PS_SECTPROP_LINES,
    PS_SECTPROP_NUMTYPES
}
ps_sector_prop_e;

static const char *ps_sector_properties[PS_SECTPROP_NUMTYPES] = 
{ 
    "light", "ceiling", "floor", "ceiling_tex", "floor_tex", "lines"
};

static void SetSectorProperty(lua_State *L, 
                              int sec_num, 
                              int prop_type, 
                              int index)
{
    if (sec_num < 0 || sec_num >= numsectors)
        return;

    sector_t *sec = sectors + sec_num;

    switch (prop_type)
    {
    case PS_SECTPROP_LIGHTING:
        sec->props.lightlevel = luaL_checkint(L, index);
        break;

    case PS_SECTPROP_CEILING:
    case PS_SECTPROP_FLOOR:
    {
        float dh = (float)luaL_checknumber(L, index);
        bool is_ceiling;
        if (prop_type == PS_SECTPROP_CEILING)
        {    
            dh = dh - sec->c_h;
            is_ceiling = true;
        }
        else
        {    
            dh = dh - sec->f_h;
            is_ceiling = false;
        }

        if (P_CheckSolidSectorMove(sec, is_ceiling, dh))
            P_SolidSectorMove(sec, is_ceiling, dh);

        break;
    }
    
    case PS_SECTPROP_CEILING_TEX:
    case PS_SECTPROP_FLOOR_TEX:
    {
        const image_c *image = W_ImageLookup(luaL_checkstring(L, index), 
                                             INS_Flat);
        if (image)
        {
			if (prop_type == PS_SECTPROP_FLOOR_TEX)
				sec->floor.image = image;
			else
				sec->ceil.image = image;

			if (image == skyflatimage)
                R_ComputeSkyHeights();
		}

        break;
    }

    default:
        break;
    }
}

// world.get_sectors(tag)
//
static int PS_get_sectors(lua_State *L) 
{
    if (lua_gettop(L) != 1)
    {
        return luaL_error(L, 
                          "\nRequired parameters: %s", 
                          "<tag>");
    }

    int tag = luaL_checkint(L, 1); 

	// OPTIMISE !
    int table_index = 0;
    int table_pos = 1;
	for (int i=0; i < numsectors; i++)
	{
		if (sectors[i].tag == tag)
        {
            if (!table_index)
            {
                lua_createtable(L, 0, 1);
                table_index = lua_gettop(L);
            }

            lua_pushinteger(L, i);
            lua_rawseti(L, table_index, table_pos);

            table_pos++;
        }
	}
    
    // No table created on the stack, therefore push on a NIL as 
    // the return value
    if (!table_index)
        lua_pushnil(L);

    return 1;
}

// world.get_sec_prop(prop_type, sector_num)
//
static int PS_get_sec_prop(lua_State *L)
{
    if (lua_gettop(L) != 2)
    {
        return luaL_error(L, 
                          "\nRequired parameters: %s", 
                          "<property>, <sector_num>");
    }

    // Get property type
    const char *prop = lua_tolstring(L, 1, NULL);

    int prop_type = 0; 
    while(prop_type < PS_SECTPROP_NUMTYPES && 
          strcmp(ps_sector_properties[prop_type],prop))
        prop_type++;

    if (prop_type == PS_SECTPROP_NUMTYPES)
        return luaL_error(L, "Invalid property");

    // Get property from sector
    int sec_num = luaL_checkint(L, 2);
    if (sec_num >= 0 && sec_num < numsectors)
    {
        sector_t *sec = sectors + sec_num;

        switch (prop_type)
        {
        case PS_SECTPROP_LIGHTING:
            lua_pushinteger(L, sec->props.lightlevel);
            break;

        case PS_SECTPROP_CEILING:
            lua_pushnumber(L, sec->c_h);
            break;

        case PS_SECTPROP_FLOOR:
            lua_pushnumber(L, sec->f_h);
            break;

        case PS_SECTPROP_CEILING_TEX:
            lua_pushstring(L, sec->ceil.image->name);
            break;

        case PS_SECTPROP_FLOOR_TEX:
            lua_pushstring(L, sec->floor.image->name);
            break;

        case PS_SECTPROP_LINES:
            if (sec->linecount)
            {
                lua_createtable(L, 0, sec->linecount);
                int table_index = lua_gettop(L);

                for(int i=0; i<sec->linecount; i++)
                {
                    lua_pushinteger(L, sec->lines[i] - lines);
                    lua_rawseti(L, table_index, i+1);
                }
            }
            else
            {
                lua_pushnil(L);
            }

            break;

        default:
            lua_pushnil(L);
            break;
        }
    }
    else
    {
        lua_pushnil(L);
    }

    return 1;
}

// world.set_sec_prop(prop_type, sector_num(s), attribute_value)
//
static int PS_set_sec_prop(lua_State *L)
{
    if (lua_gettop(L) < 3)
    {
        return luaL_error(L, 
                          "\nRequired parameters: %s", 
                          "<property>, <sector_nums>, <attribute_value>");
    }

    // Get property type
    const char *prop = lua_tolstring(L, 1, NULL);

    int prop_type = 0; 
    while(prop_type < PS_SECTPROP_NUMTYPES && 
          strcmp(ps_sector_properties[prop_type],prop))
        prop_type++;

    if (prop_type == PS_SECTPROP_NUMTYPES)
        return luaL_error(L, "Invalid property");

    if (prop_type == PS_SECTPROP_LINES) // Read-only 
        return luaL_error(L, "Property is read only");

    // Get sector(s)
    const int prop_value_index = 3;
    int type = lua_type(L, 2);
    if (type == LUA_TTABLE)
    {
        lua_pushnil(L);  // Get first key
        while (lua_next(L, 2) != 0) {
            if (lua_type(L, -1) == LUA_TNUMBER)
                SetSectorProperty(L, 
                                  lua_tointeger(L, -1),
                                  prop_type,
                                  prop_value_index);

            lua_pop(L, 1); // Pop value, leaving the key for next func
        }
    }
    else if (type == LUA_TNUMBER)
    {
        SetSectorProperty(L, 
                          luaL_checkint(L, 2),
                          prop_type,
                          prop_value_index);
    }
    else 
    {
        return luaL_error(L, "Expected numeric or table.");
    }

    return 0;
}

// world.spawn_thing(thing_name/thing_num, angle, x, y, z)
//
static int PS_spawn_thing(lua_State *L)
{
    if (lua_gettop(L) != 5)
    {
        return luaL_error(L, 
                   "\nRequired parameters: %s", 
                   "<thing name/num>, <angle>, <x>, <y>, <z>");
    }

	const mobjtype_c *minfo;
    if (lua_isnumber(L, 1))
    {
        int type = lua_tointeger(L,1);
        minfo = mobjtypes.Lookup(type);
        if (minfo == NULL)
            return luaL_error(L, "Unknown thing type '%d'", type);
    }
    else
    {
        const char *name = lua_tolstring(L, 1, NULL);

        const int idx = mobjtypes.FindLast(name);
        if (idx < 0)
            return luaL_error(L, "Unknown thing name '%s'", name);
        
        minfo = mobjtypes[idx];
    }

    int angle = luaL_checkint(L, 2);
    int x = luaL_checkint(L, 3);
    int y = luaL_checkint(L, 4);
    int z = luaL_checkint(L, 5);

	mobj_t *mo = P_MobjCreateObject(x, y, z, minfo);
	if (!P_CheckAbsPosition(mo, x, y, z))
	{
		P_RemoveMobj(mo);
		return 0;
	}

	P_SetMobjDirAndSpeed(mo, angle, 0, 0);

	mo->spawnpoint.x = x;
	mo->spawnpoint.y = y;
	mo->spawnpoint.z = z;
	mo->spawnpoint.angle = angle;
	mo->spawnpoint.vertangle = 0;
	mo->spawnpoint.info = minfo;
	mo->spawnpoint.flags = 0;

    return 0;
}

const luaL_Reg world_module[] =
{
	{ "activate_line_type",  PS_activate_line_type },
	{ "get_line_prop",       PS_get_line_prop },
	{ "get_sidedef_prop",    PS_get_sidedef_prop },
	{ "get_sec_prop",        PS_get_sec_prop },
	{ "get_sectors",         PS_get_sectors },
	{ "set_sidedef_prop",    PS_set_sidedef_prop },
	{ "set_sec_prop",        PS_set_sec_prop },
	{ "spawn_thing",         PS_spawn_thing },
    { NULL,                  NULL }
};


//------------------------------------------------------------------------

void P_RegisterModules()
{
    vm->LoadModule("player", player_module);
    vm->LoadModule("world",  world_module);
}

void P_SetupPlayerModule(int player)
{
    cur_player = players[player];
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
