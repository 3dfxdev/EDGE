//----------------------------------------------------------------------------
//  EDGE Rendering Data Handling Code
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2000  The EDGE Team.
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
//
// DESCRIPTION:
//  Refresh module, data I/O, caching, retrieval of graphics
//  by name.
//

#ifndef __W_TEXTURE__
#define __W_TEXTURE__

#include "r_defs.h"
#include "r_state.h"
#include "z_zone.h"

// Texture cache reference structure, returned by the texture cache system.
// The actual structure is private.
typedef byte cached_tex_t;

// Retrieve column data.
const byte *W_GetColumn(int col, const cached_tex_t *t);

// Standard cache functions. Call W_CacheTextureNum when you first need
// the lump, and release it with DoneWithTexture.
const cached_tex_t *W_CacheTextureNum(int texnum);
void W_DoneWithTexture(const cached_tex_t *t);

void W_PreCacheTextureNum(int texnum);

void W_InitTextures(void);

// Called by P_Ticker for switches and animations,
// returns the texture number for the texture name.
int W_TextureNumForName(const char *name);
int W_CheckTextureNumForName(const char *name);

// -AJA- !!!! FIXME: BIG HACK HERE
void *W_TextureDefForName(const char *name);

#endif
