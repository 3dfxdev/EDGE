//------------------------------------------------------------------------
//  TEXTURE LOADING
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2009 Andrew Apted
//  Copyright (C) 1997-2003 André Majorel et al
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
//
//  Based on Yadex which incorporated code from DEU 5.21 that was put
//  in the public domain in 1994 by Raphaël Quinet and Brendon Wyber.
//
//------------------------------------------------------------------------

#ifndef __W_TEXTURE_H__
#define __W_TEXTURE_H__

#include "im_img.h"


void W_LoadTextures();

Img * W_GetTexture(const char *name);

// TODO  Img * W_PlaceholderTex()


// ick, remove
void GetWallTextureSize (s16_t *, s16_t *, const char *);


#endif  /* __W_TEXTURE_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
