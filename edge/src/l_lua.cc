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

#include "e_main.h"
#include "l_lua.h"
#include "version.h"

#include "w_wad.h"
#include "z_zone.h"


static lua_State *HUD_ST;

static bool has_loaded = false;


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
	int pnum = luaL_checkint(L, 1);

	//... FIXME

	return 0;
}


// hud.text_font(name)
//
static int HD_text_font(lua_State *L)
{
	int pnum = luaL_checkint(L, 1);

	//... FIXME

	return 0;
}


// hud.text_color(name)
//
static int HD_text_color(lua_State *L)
{
	int pnum = luaL_checkint(L, 1);

	//... FIXME

	return 0;
}


// hud.draw_image(x, y, name)
//
static int HD_draw_image(lua_State *L)
{
	int pnum = luaL_checkint(L, 1);

	//... FIXME

	return 0;
}


// hud.draw_text(x, y, str)
//
static int HD_draw_text(lua_State *L)
{
	int pnum = luaL_checkint(L, 1);

	//... FIXME

	return 0;
}


// hud.number(x, y, len, num)
//
static int HD_draw_number(lua_State *L)
{
	int pnum = luaL_checkint(L, 1);

	//... FIXME

	return 0;
}


// hud.render_world(x, y, w, h)
//
static int HD_render_world(lua_State *L)
{
	int pnum = luaL_checkint(L, 1);

	//... FIXME

	return 0;
}


// hud.render_automap(x, y, w, h)
//
static int HD_render_automap(lua_State *L)
{
	int pnum = luaL_checkint(L, 1);

	//... FIXME

	return 0;
}


static const luaL_Reg hud_module[] =
{
	{ "raw_debug_print", HD_raw_debug_print },

    { "scaling",         HD_scaling     },
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
	int pnum = luaL_checkint(L, 1);

	//... FIXME

	return 0;
}


// player.armor(type)
//
static int PL_armor(lua_State *L)
{
	int pnum = luaL_checkint(L, 1);

	//... FIXME

	return 0;
}


// player.frags()
//
static int PL_frags(lua_State *L)
{
	int pnum = luaL_checkint(L, 1);

	//... FIXME

	return 0;
}


// player.has_key(key)
//
static int PL_has_key(lua_State *L)
{
	int pnum = luaL_checkint(L, 1);

	//... FIXME

	return 0;
}


// player.has_weapon_slot(keynum)
//
static int PL_has_weapon_slot(lua_State *L)
{
	int pnum = luaL_checkint(L, 1);

	//... FIXME

	return 0;
}


// player.ammo(type)
//
static int PL_ammo(lua_State *L)
{
	int pnum = luaL_checkint(L, 1);

	//... FIXME

	return 0;
}


// player.ammomax(type)
//
static int PL_ammomax(lua_State *L)
{
	int pnum = luaL_checkint(L, 1);

	//... FIXME

	return 0;
}


// player.cur_ammo(clip)
//
static int PL_cur_ammo(lua_State *L)
{
	int pnum = luaL_checkint(L, 1);

	//... FIXME

	return 0;
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
	lua_getglobal(HUD_ST, "player");

	// TODO: set some fields (who etc) in 'player' table

	lua_pop(HUD_ST, 1);


	lua_getglobal(HUD_ST, "hud");

	// TODO: set some fields (width and height) in 'hud' table


	lua_getglobal(HUD_ST, "stack_trace");

	if (lua_type(HUD_ST, -1) == LUA_TNIL)
		I_Error("LUA scripts: missing function '%s'\n", "stack_trace");

	lua_getfield(HUD_ST, -2, "draw_all");

	if (lua_type(HUD_ST, -1) == LUA_TNIL)
		I_Error("LUA scripts: missing function 'hud.%s'\n", "draw_all");

	int nargs = 0;
	int nresult = 0;

	int status = lua_pcall(HUD_ST, nargs, nresult, -2-nargs);
	if (status != 0)
	{
		const char *msg = lua_tolstring(HUD_ST, -1, NULL);

		I_Error("LUA script error:\n%s", msg);
	}

	// remove the traceback function and 'hud' table
	lua_remove(HUD_ST, -1-nresult);
	lua_remove(HUD_ST, -1-nresult);
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
