//------------------------------------------------------------------------
//  LUA General Stuff
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

#include "dm_state.h"
#include "e_main.h"
#include "g_game.h"
#include "l_lua.h"
#include "version.h"

#include "e_player.h"
#include "hu_font.h"
#include "r_draw.h"
#include "r_modes.h"
#include "w_wad.h"
#include "z_zone.h"

#include "r_misc.h"     //  R_Render
#include "r_automap.h"  // AM_Drawer


lua_State *HUD_ST;

static bool has_loaded = false;

static int hud_last_time = -1;


static void FrameSetup(void)
{
	fontdef_c *DEF = fontdefs.Lookup("DOOM");  // FIXME allow other default
	SYS_ASSERT(DEF);

	cur_font = hu_fonts.Lookup(DEF);
	SYS_ASSERT(cur_font);

	cur_colmap = NULL;
	cur_scale  = 1.0f;
	cur_alpha  = 1.0f;

	cur_player = players[displayplayer];

	cur_coord_W = 320;
	cur_coord_H = 200;


	int now_time = I_GetTime();
	int passed_time = 0;

	if (hud_last_time > 0 && hud_last_time <= now_time)
	{
		passed_time = MIN(now_time - hud_last_time, TICRATE);
	}

	hud_last_time = now_time;


	// setup some fields in 'hud' module

	lua_getglobal(HUD_ST, "hud");

	lua_pushinteger(HUD_ST, screen_hud);
	lua_setfield(HUD_ST, -2, "which");

	lua_pushboolean(HUD_ST, automapactive ? 1 : 0);
	lua_setfield(HUD_ST, -2, "automap");

	lua_pushinteger(HUD_ST, now_time);
	lua_setfield(HUD_ST, -2, "now_time");

	lua_pushinteger(HUD_ST, passed_time);
	lua_setfield(HUD_ST, -2, "passed_time");

	lua_pop(HUD_ST, 1);
}


//------------------------------------------------------------------------

static const luaL_Reg lua_somelibs[] =
{
	{"", luaopen_base},
	{LUA_TABLIBNAME, luaopen_table},
	{LUA_STRLIBNAME, luaopen_string},
	{LUA_MATHLIBNAME, luaopen_math},
	{LUA_DBLIBNAME,  luaopen_debug},  // TODO: remove this too
	{NULL, NULL}
};

static void open_somelibs(lua_State *L)
{
	// -AJA- modified 'lua_openlibs' from Lua source code, which
	//       removes these libraries: package, io, os.

	const luaL_Reg *lib = lua_somelibs;

	for (; lib->func; lib++)
	{
		lua_pushcfunction(L, lib->func);
		lua_pushstring(L, lib->name);
		lua_call(L, 1, 0);
	}
}

static void remove_loaders(lua_State *L)
{
	// -AJA- sandboxing: remove the dofile() and loadfile() functions
	//       which are supplied by the Lua baselib.

	static const char * bad_funcs[] =
	{
		"dofile", "loadfile",

		NULL  // end of list
	};

	for (int i = 0; bad_funcs[i]; i++)
	{
		lua_pushnil(L);
		lua_setfield(L, LUA_GLOBALSINDEX, bad_funcs[i]);
	}
}

static int p_init_lua(lua_State *L)
{
	/* stop collector during initialization */
	lua_gc(L, LUA_GCSTOP, 0);
	{
		/* open libraries */

		open_somelibs(L);

		remove_loaders(L);

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
	static const char *script_lumps[] =
	{
		"LUAUTIL0",  // edge.wad
		"LUAUTIL1",  // user - in a TC
		"LUAUTIL2",  // user - for a add-on

		"LUAHUD0",  // edge.wad
		"LUAHUD1",  // user - in a TC
		"LUAHUD2",  // user - for a add-on

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

		has_loaded = true;

		Z_Free(buffer);
	}

	if (! has_loaded)
		I_Error("Missing required LUAHUDx lumps!\n");
}


void LU_BeginLevel(void)
{
	hud_last_time = -1;
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
