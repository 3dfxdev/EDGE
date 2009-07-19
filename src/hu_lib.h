//----------------------------------------------------------------------------
//  EDGE Heads-up-display library Code (header)
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2008  The EDGE Team.
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

#ifndef __HULIB__
#define __HULIB__

// We are referring to patches.
#include "r_defs.h"
#include "r_image.h"
#include "hu_font.h"
#include "hu_style.h"

//
//  Font stuff
//


#define HU_MAXLINES		4
#define HU_MAXLINELENGTH	80


// initialises heads-up widget library
void HL_Init(void);


void HL_WriteText(style_c *style, int text_type, int x, int y,
		const char *str, float scale = 1.0f, float alpha = 1.0f);

void HL_WriteTextTrans(style_c *style, int text_type, int x, int y,
		rgbcol_t col, const char *str, float scale = 1.0f, float alpha = 1.0f);

#endif  // __HULIB__

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
