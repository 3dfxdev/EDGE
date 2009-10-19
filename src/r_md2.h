//----------------------------------------------------------------------------
//  MD2 Models
//----------------------------------------------------------------------------
//
//  Copyright (c) 2002-2009  The EDGE Team.
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

#ifndef __R_MD2_H__
#define __R_MD2_H__

#include "epi/file.h"

#include "r_defs.h"
#include "p_mobj.h"


// opaque handle for rest of the engine
class md2_model_c;


md2_model_c *MD2_LoadModel(epi::file_c *f); 

short MD2_FindFrame(md2_model_c *md, const char *name);

void MD2_RenderModel(md2_model_c *md, const image_c * skin_img, bool is_weapon,
		             int frame1, int frame2, float lerp,
		             float x, float y, float z, mobj_t *mo,
					 region_properties_t *props,
					 float scale, float aspect, float bias);

void MD2_RenderModel_2D(md2_model_c *md, const image_c * skin_img, int frame,
		                float x, float y, float xscale, float yscale,
		                const mobjtype_c *info);

#endif /* __R_MD2_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
