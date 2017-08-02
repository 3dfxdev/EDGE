//----------------------------------------------------------------------------
//  MD5 Animation Managment
//----------------------------------------------------------------------------
//
//  Copyright (c) 2002-2009  The EDGE2 Team.
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

#ifndef __R_MD5ANIM_H__
#define __R_MD5ANIM_H__

#include "md5_conv/md5_anim.h"
#include "../epi/file.h"
#include "../epi/types.h"

#include "r_defs.h"
#include "p_mobj.h"
#include "w_model.h"

#define R_MAX_MD5_ANIMATIONS	2000

short R_LoadMD5AnimationName(const char * lumpname);
MD5animation * R_GetMD5Animation(short animation_num);
void MD5_RenderModel(modeldef_c *md, int last_anim, int last_frame,
	int current_anim, int current_frame, float lerp, epi::vec3_c pos,
	epi::vec3_c scale,epi::vec3_c bias, mobj_t *mo);

#endif /* __R_MD2_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
