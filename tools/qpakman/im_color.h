//------------------------------------------------------------------------
//  Color Management
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

#ifndef __IMAGE_COLOR_H__
#define __IMAGE_COLOR_H__

void COL_SetPalette(int game_type);
void COL_SetFullBright(bool enable);

u32_t COL_ReadPalette(byte pix);

byte COL_FindColor(const byte *palette, u32_t rgb_col);
byte COL_MapColor(u32_t rgb_col);

#endif  /* __IMAGE_COLOR_H__ */

//--- editor settings ---
// vi:ts=2:sw=2:expandtab
