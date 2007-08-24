//----------------------------------------------------------------------------
//  MD2 Models
//----------------------------------------------------------------------------
//
//  Copyright (c) 2002-2007  The EDGE Team.
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

void MD2_RenderModel(md2_model_c *md, GLuint skin_tex, bool is_weapon,
		             mobj_t *mo, region_properties_t *props);


#endif /* __R_MD2_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
