//------------------------------------------------------------------------
//  LUA HUD code
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

#include "ddf/main.h"
#include "ddf/font.h"

#include "e_main.h"
#include "l_lua.h"
#include "version.h"

#include "e_player.h"
#include "hu_font.h"
#include "w_wad.h"
#include "z_zone.h"

#include "r_misc.h"     //  R_Render
#include "r_automap.h"  // AM_Drawer


static lua_State *HUD_ST;

static bool has_loaded = false;

static player_t *cur_player = NULL;

static font_c *cur_font = NULL;
static colourmap_c *cur_colmap = NULL;


static void FrameSetup(void)
{
	fontdef_c *DEF = fontdefs.Lookup("DOOM");  // FIXME allow other default
	SYS_ASSERT(DEF);

	cur_font = hu_fonts.Lookup(DEF);
	SYS_ASSERT(cur_font);

	cur_colmap = NULL;

	cur_player = players[displayplayer];

	
	// setup some fields in 'player' module

	lua_getglobal(HUD_ST, "player");

	// TODO

	lua_pop(HUD_ST, 1);


	// TODO: setup some fields in 'hud' module
}


static void DoWriteText(int x, int y, const char *str)
{
	float scale = 1.0f;

	float cx = x;
	float cy = y;

	for (; *str; str++)
	{
		char ch = *str;

		if (ch == '\n')
		{
			cx = x;
			cy += 12.0f * scale;  // FIXME: use font's height
			continue;
		}

		if (cx >= 320.0f)
			continue;

		cur_font->DrawChar(cx, cy, ch, scale,1.0f, cur_colmap, 1.0f);

		cx += cur_font->CharWidth(ch) * scale;
	}
}


//------------------------------------------------------------------------
//  HUD MODULE
//------------------------------------------------------------------------


// hud.raw_debug_print(str)
//
static int HD_raw_debug_print(lua_State *L)
{
	int nargs = lua_gettop(L);

	if (nargs >= 1)
	{
		const char *res = luaL_checkstring(L,1);
		SYS_ASSERT(res);

		I_Debugf("%s", res);
	}

	return 0;
}


// hud.scaling(w, h)
//
static int HD_scaling(lua_State *L)
{
	int w = luaL_checkint(L, 1);
	int h = luaL_checkint(L, 2);

	if (w <= 80 || h <= 80)
		I_Error("Bad hud.scaling size: %dx%d\n", w, h);

	//... FIXME

	return 0;
}


// hud.game_mode()
//
static int HD_game_mode(lua_State *L)
{
	//!!!! FIXME
	
	lua_pushstring(L, "sp");
	return 1;
}


// hud.text_font(name)
//
static int HD_text_font(lua_State *L)
{
	const char *font_name = luaL_checkstring(L, 1);

	fontdef_c *DEF = fontdefs.Lookup(font_name);
	SYS_ASSERT(DEF);

	cur_font = hu_fonts.Lookup(DEF);
	SYS_ASSERT(cur_font);

	return 0;
}


// hud.text_color(name)
//
static int HD_text_color(lua_State *L)
{
	//... FIXME

	return 0;
}


// hud.draw_image(x, y, name)
//
static int HD_draw_image(lua_State *L)
{
	//... FIXME

	return 0;
}


// hud.draw_text(x, y, str)
//
static int HD_draw_text(lua_State *L)
{
	int x = luaL_checkint(L, 1);
	int y = luaL_checkint(L, 2);

	const char *str = luaL_checkstring(L, 3);

	DoWriteText(x, y, str);

	return 0;
}


// hud.number(x, y, len, num)
//
static int HD_draw_number(lua_State *L)
{
	int x   = luaL_checkint(L, 1);
	int y   = luaL_checkint(L, 2);
	int len = luaL_checkint(L, 3);
	int num = luaL_checkint(L, 4);

	if (len < 1 || len > 20)
		I_Error("hud.draw_number: bad field length: %d\n", len);

	char buffer[200];

	sprintf(buffer, "%*d", len, num);

	DoWriteText(x, y, buffer);

	return 0;
}


// hud.render_world(x, y, w, h)
//
static int HD_render_world(lua_State *L)
{
	int x = luaL_checkint(L, 1);
	int y = luaL_checkint(L, 2);
	int w = luaL_checkint(L, 3);
	int h = luaL_checkint(L, 4);

	// FIXME: set view window

 	R_Render();

	return 0;
}


// hud.render_automap(x, y, w, h)
//
static int HD_render_automap(lua_State *L)
{
	int x = luaL_checkint(L, 1);
	int y = luaL_checkint(L, 2);
	int w = luaL_checkint(L, 3);
	int h = luaL_checkint(L, 4);

	// FIXME: set view window

	AM_Drawer();

	return 0;
}


static const luaL_Reg hud_module[] =
{
	{ "raw_debug_print", HD_raw_debug_print },

    { "scaling",         HD_scaling     },
    { "game_mode",       HD_game_mode     },
    { "text_font",       HD_text_font   },
    { "text_color",      HD_text_color  },
    { "draw_image",      HD_draw_image  },
    { "draw_text",       HD_draw_text   },
    { "draw_number",     HD_draw_number },

    { "render_world",    HD_render_world   },
    { "render_automap",  HD_render_automap },

	{ NULL, NULL } // the end
};


//------------------------------------------------------------------------
//  PLAYER MODULE
//------------------------------------------------------------------------


// player.set_who(pnum)
//
static int PL_set_who(lua_State *L)
{
	int pnum = luaL_checkint(L, 1);

	//... FIXME

	return 0;
}


// player.health()
//
static int PL_health(lua_State *L)
{
	//... FIXME

	lua_pushinteger(L, (int)floor(cur_player->health+0.99));
	return 1;
}


// player.armor(type)
//
static int PL_armor(lua_State *L)
{
	//!!! FIXME

	lua_pushinteger(L, 0);
	return 1;
}


// player.frags()
//
static int PL_frags(lua_State *L)
{
	lua_pushinteger(L, cur_player->frags);
	return 1;
}


// player.has_key(key)
//
static int PL_has_key(lua_State *L)
{
	int key = luaL_checkint(L, 1);

	if (key < 1 || key > 16)
		I_Error("player.has_key: bad key number: %d\n", key);

	int value = (cur_player->cards & (1 << (key-1))) ? 1 : 0;

	lua_pushboolean(L, value);
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


// player.ammo(type)
//
static int PL_ammo(lua_State *L)
{
	int ammo = luaL_checkint(L, 1);

	if (ammo < 1 || ammo > NUMAMMO)
		I_Error("player.ammo: bad ammo number: %d\n", ammo);

	lua_pushinteger(L, cur_player->ammo[ammo-1].num);
	return 1;
}


// player.ammomax(type)
//
static int PL_ammomax(lua_State *L)
{
	int ammo = luaL_checkint(L, 1);

	if (ammo < 1 || ammo > NUMAMMO)
		I_Error("player.ammomax: bad ammo number: %d\n", ammo);

	lua_pushinteger(L, cur_player->ammo[ammo-1].num);
	return 1;
}


// player.cur_ammo(clip)
//
static int PL_cur_ammo(lua_State *L)
{
	int clip = luaL_checkint(L, 1);

	if (clip < 1 || clip > 2)
		I_Error("player.cur_ammo: bad clip number: %d\n", clip);

	clip--;

	int value = 0;

	if (cur_player->ready_wp >= 0)
	{
		playerweapon_t *pw = &cur_player->weapons[cur_player->ready_wp];

		if (pw->info->ammo[clip] != AM_NoAmmo)
		{
			if (pw->info->show_clip)
			{
				SYS_ASSERT(pw->info->ammopershot[clip] > 0);

				value = pw->clip_size[clip] / pw->info->ammopershot[clip];
			}
			else
			{
				value = cur_player->ammo[pw->info->ammo[clip]].num;

				if (pw->info->clip_size[clip] > 0)
					value += pw->clip_size[clip];
			}
		}
	}

	lua_pushinteger(L, value);
	return 1;
}


static const luaL_Reg player_module[] =
{
	{ "set_who",   PL_set_who },
    { "health",    PL_health  },
    { "armor",     PL_armor   },
    { "frags",     PL_frags   },

    { "has_key",      PL_has_key  },
    { "has_weapon_slot", PL_has_weapon_slot },
    { "ammo",         PL_ammo     },
    { "ammomax",      PL_ammomax  },
    { "cur_ammo",     PL_cur_ammo },

	{ NULL, NULL } // the end
};


//------------------------------------------------------------------------


static int p_init_lua(lua_State *L)
{
	/* stop collector during initialization */
	lua_gc(L, LUA_GCSTOP, 0);
	{
		/* open libraries */

		luaL_openlibs(L);

		/* register our own modules */

		luaL_register(HUD_ST, "hud",    hud_module);
		luaL_register(HUD_ST, "player", player_module);

		// remove the tables which luaL_register created
		lua_pop(L, 2);
	}
	lua_gc(L, LUA_GCRESTART, 0);

	return 0;
}


void LU_Init(void)
{
	HUD_ST = lua_open();

	if (! HUD_ST)
		I_Error("LUA Init failed: cannot create new state");

	int status = lua_cpcall(HUD_ST, &p_init_lua, NULL);

	if (status != 0)
		I_Error("LUA Init failed (status %d)", status);

	// FIXME !!!! make sure script cannot load any DLLs or other scripts
}


void LU_Close(void)
{
	if (HUD_ST)
	{
		lua_close(HUD_ST);
		HUD_ST = NULL;
	}
}


void LU_LoadScripts(void)
{
	// absolutely need this
	W_GetNumForName("LUAHUD");

	static const char *script_lumps[] =
	{
		"LUAUTIL0",  // edge.wad
		"LUAUTIL1",  // user add-on
		"LUAUTIL2",  // user add-on

		"LUAHUD",

		NULL  // end of list
	};

	for (int i = 0; script_lumps[i]; i++)
	{
		int lump = W_CheckNumForName(script_lumps[i]);

		if (lump < 0)
			continue;
	
		I_Printf("Loading LUA lump: %s...\n", script_lumps[i]);

		char *buffer = (char *)W_LoadLumpNum(lump);
		int length   = W_LumpLength(lump);

		int status = luaL_loadbuffer(HUD_ST, buffer, length, script_lumps[i]);

		if (status == 0)
			status = lua_pcall(HUD_ST, 0, 0, 0);

		if (status != 0)
		{
			const char *msg = lua_tolstring(HUD_ST, -1, NULL);

			I_Error("LUA scripts: failure loading '%s' lump (%d)\n%s",
			        script_lumps[i], status, msg);
		}

		Z_Free(buffer);
	}

	has_loaded = true;
}


void LU_RunHud(void)
{
	FrameSetup();


	lua_getglobal(HUD_ST, "stack_trace");

	if (lua_type(HUD_ST, -1) == LUA_TNIL)
		I_Error("LUA scripts: missing function '%s'\n", "stack_trace");

	lua_getglobal(HUD_ST, "hud");
	lua_getfield(HUD_ST, -1, "draw_all");

	if (lua_type(HUD_ST, -1) == LUA_TNIL)
		I_Error("LUA scripts: missing function 'hud.%s'\n", "draw_all");

	int nargs = 0;

	int status = lua_pcall(HUD_ST, nargs, 0, -3-nargs);
	if (status != 0)
	{
		const char *msg = lua_tolstring(HUD_ST, -1, NULL);

		I_Error("LUA script error:\n%s", msg);
	}

	// remove the traceback function and 'hud' table
	lua_pop(HUD_ST, 2);
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
