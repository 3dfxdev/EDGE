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

#ifndef __L_LUA_H__
#define __L_LUA_H__

void LU_Init(void);
void LU_Close(void);

void LU_LoadScripts(void);

void LU_BeginLevel(void);
void LU_RunHud(void);

#endif // __L_LUA_H__

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
