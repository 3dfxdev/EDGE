//------------------------------------------------------------------------
//  Global Vars
//------------------------------------------------------------------------
//
//  Copyright (c) 2008  Andrew J Apted
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

#ifndef __QPAKMAN_MAIN_H__
#define __QPAKMAN_MAIN_H__


typedef enum
{
  GAME_Quake1   = 0,
  GAME_Quake2   = 1,
  GAME_Hexen2   = 2,
  GAME_Haktoria = 3,
}
game_kind_e;

extern game_kind_e game_type;

extern std::string color_name;


extern bool opt_force;
extern bool opt_picture;
extern bool opt_raw;
extern bool opt_dither;


void FatalError(const char *str, ...);


#endif // __QPAKMAN_MAIN_H__

//--- editor settings ---
// vi:ts=2:sw=2:expandtab
