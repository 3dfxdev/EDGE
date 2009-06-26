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

#include "ddf/font.h"

#include "l_lua.h"
#include "hu_vm.h"
#include "p_vm.h"

#include "g_state.h"
#include "e_main.h"
#include "g_game.h"
#include "version.h"

#include "e_player.h"
#include "hu_font.h"
#include "r_draw.h"
#include "r_modes.h"
#include "w_wad.h"
#include "z_zone.h"


lua_vm_c *vm;  // the instance


static bool has_loaded = false;


//
// Constructor
//
lua_vm_c::lua_vm_c() : state(NULL)
{
	/* nothing needed */
}

//
// Destructor
//
lua_vm_c::~lua_vm_c()
{
	lua_close(state);
	state = NULL;
}


void lua_vm_c::Open()
{
    state = lua_open();
    if (! state)
        I_Error("LUA Init failed: cannot create new state");

    int status = lua_cpcall(state, &init_lua, NULL);
    if (status != 0)
        I_Error("LUA Init failed (status %d)", status);
}


static const luaL_Reg lua_somelibs[] =
{
	{"", luaopen_base},
	{LUA_TABLIBNAME, luaopen_table},
	{LUA_STRLIBNAME, luaopen_string},
	{LUA_MATHLIBNAME, luaopen_math},
	{LUA_DBLIBNAME,  luaopen_debug},  // TODO: remove this too
	{NULL, NULL}
};

void lua_vm_c::open_basic_libs(lua_State *L)
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

void lua_vm_c::remove_loaders(lua_State *L)
{
	// -AJA- sandboxing: remove the dofile() and loadfile() functions
	//       which are supplied by the Lua baselib.

	static const char * bad_funcs[] =
	{
		"dofile", "loadfile",
		NULL // the end
	};

	for (int i = 0; bad_funcs[i]; i++)
	{
		lua_pushnil(L);
		lua_setfield(L, LUA_GLOBALSINDEX, bad_funcs[i]);
	}
}

int lua_vm_c::init_lua(lua_State *L)
{
	/* stop collector during initialization */
	lua_gc(L, LUA_GCSTOP, 0);
	{
		/* open libraries */
		open_basic_libs(L);

		remove_loaders(L);
	}
	lua_gc(L, LUA_GCRESTART, 0);

	return 0;
}


void lua_vm_c::LoadModule(const char *name, const luaL_Reg *funcs)
{
    luaL_register(state, name, funcs);
    lua_pop(state, 1);
}

void lua_vm_c::LoadBuffer(const char *name, char *buffer, int length)
{
    int status = luaL_loadbuffer(state, buffer, length, name);

    if (status == 0)
        status = lua_pcall(this->state, 0, 0, 0);

    if (status != 0)
    {
        const char *msg = lua_tolstring(this->state, -1, NULL);

        I_Error("LUA scripts: failure loading '%s' (%d)\n%s",
                name, status, msg);
    }
}


void lua_vm_c::SetIntVar(const char *parent, const char *name, int value)
{
	int stack_idx = LUA_GLOBALSINDEX;

	if (parent)
	{
		lua_getglobal(state, parent);
		if (lua_isnil(state, -1))
			I_Error("VM Error: no such global table '%s'\n", parent);
		stack_idx = -2;
	}

	lua_pushnumber(state, value);
	lua_setfield(state, stack_idx, name);

	if (parent)
		lua_pop(state, 1);
}

void lua_vm_c::SetBoolVar(const char *parent, const char *name, bool value)
{
	int stack_idx = LUA_GLOBALSINDEX;

	if (parent)
	{
		lua_getglobal(state, parent);
		if (lua_isnil(state, -1))
			I_Error("VM Error: no such global table '%s'\n", parent);
		stack_idx = -2;
	}

	lua_pushboolean(state, value ? 1 : 0);
	lua_setfield(state, stack_idx, name);

	if (parent)
		lua_pop(state, 1);
}

void lua_vm_c::SetStringVar(const char *parent, const char *name, const char *value)
{
	int stack_idx = LUA_GLOBALSINDEX;

	if (parent)
	{
		lua_getglobal(state, parent);
		if (lua_isnil(state, -1))
			I_Error("VM Error: no such global table '%s'\n", parent);
		stack_idx = -2;
	}

	lua_pushstring(state, value);
	lua_setfield(state, stack_idx, name);

	if (parent)
		lua_pop(state, 1);
}


void lua_vm_c::CallFunction(const char *parent, const char *name)
{
	lua_getglobal(state, "stack_trace");

	if (lua_isnil(state, -1))
		I_Error("LUA scripts: missing function '%s'\n", "stack_trace");

	if (parent)
	{
		lua_getglobal(state, parent);
		if (lua_isnil(state, -1))
			I_Error("VM Error: no such global table '%s'\n", parent);
		lua_getfield(state, -1, name);
	}
	else
	{
		lua_pushnil(state);
		lua_getglobal(state, name);
	}

	if (lua_isnil(state, -1))
	{
		if (parent)
			I_Error("LUA scripts: missing function '%s.%s'\n", parent, name);
		else
			I_Error("LUA scripts: missing function '%s'\n", name);
	}

	int nargs = 0;

	int status = lua_pcall(state, nargs, 0, -3-nargs);
	if (status != 0)
	{
		const char *msg = lua_tolstring(state, -1, NULL);

		I_Error("LUA script error:\n%s", msg);
	}

	// remove the traceback function and parent table
	lua_pop(state, 2);
}


//------------------------------------------------------------------------

void LU_Init()
{
	vm = new lua_vm_c();

	vm->Open();

	HU_RegisterModules();
	 P_RegisterModules();
}


void LU_Close()
{
	if (vm)
	{
		delete vm;
		vm = NULL;
	}
}


void LU_LoadScripts()
{
	static const char *script_lumps[] =
	{
		"LUAUTIL0",  // edge.wad
		"LUAUTIL1",  // user - in a TC
		"LUAUTIL2",  // user - for an add-on

		"LUAHUD0",  // edge.wad
		"LUAHUD1",  // user - in a TC
		"LUAHUD2",  // user - for an add-on

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

		vm->LoadBuffer(script_lumps[i], buffer, length);

		Z_Free(buffer);

		has_loaded = true;
	}

	if (! has_loaded)
		I_Error("Missing required LUAHUDx lumps!\n");
}



//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
