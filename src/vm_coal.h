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

#ifndef __L_LUA_H__
#define __L_LUA_H__

// forward definitions
struct lua_State;
struct luaL_Reg;

// LUA VM Instance
class lua_vm_c
{
public:
    struct var_s
    {
        const char *name;
        int type;
        int value;
    };
    
private:
    lua_State *state;

public:
     lua_vm_c();
    ~lua_vm_c();

public:
    void Open();

    void CallFunction(const char *parent_name, const char *name);
    void LoadModule(const char *name, const luaL_Reg *funcs);
    void LoadBuffer(const char *name, char *buffer, int length);

/// lua_State* GetState() { return this->state; } 
/// std::string RunCommand(const char *cmd);

    void SetIntVar(const char *table, const char *name, int value);
    void SetBoolVar(const char *table, const char *name, bool value);
    void SetStringVar(const char *table, const char *name, const char * value);

private:
	static int init_lua(lua_State *L);
	static void open_basic_libs(lua_State *L);
	static void remove_loaders(lua_State *L);
};

extern lua_vm_c *vm; // The instance

void LU_Init();
void LU_Close();

void LU_LoadScripts();

#endif // __L_LUA_H__

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
