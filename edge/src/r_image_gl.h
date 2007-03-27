//----------------------------------------------------------------------------
//  EDGE Generalised Image Handling (OpenGL Specifics)
//----------------------------------------------------------------------------
// 
//  Copyright (c) 2005  The EDGE Team.
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

#ifndef __R_IMAGE_OGL__
#define __R_IMAGE_OGL__

#include "r_image.h"

void W_LockImagesOGL(void);
void W_UnlockImagesOGL(void);

GLuint W_ImageGetOGL(const cached_image_t *c);

#endif  // __R_IMAGE_OGL__


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
