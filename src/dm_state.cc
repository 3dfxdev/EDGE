//----------------------------------------------------------------------------
//  EDGE Global State Variables
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2004  The EDGE Team.
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
//  Based on the DOOM source code, released by Id Software under the
//  following copyright:
//
//    Copyright (C) 1993-1996 by id Software, Inc.
//
//----------------------------------------------------------------------------

#include "i_defs.h"
#include "dm_state.h"

/*
   // Game Mode - identify IWAD as shareware, retail etc.
   GameMode_t gamemode = indetermined;
   GameMission_t gamemission = doom;

   // Language.
   Language_t   language = english;
 */
// Set if homebrew PWAD stuff has been added.
bool modifiedgame;
