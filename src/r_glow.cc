//----------------------------------------------------------------------------
//  EDGE Glow Lighting
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2007  The EDGE Team.
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

#include "i_defs.h"
#include "i_defs_gl.h"

#include "ddf/main.h"

#include "p_mobj.h"
#include "r_glow.h"


//----------------------------------------------------------------------------
//  COLORMAP
//----------------------------------------------------------------------------



//----------------------------------------------------------------------------
//  DYNAMIC LIGHTS
//----------------------------------------------------------------------------

class dynlight_glow_c : public glow_source_c
{
public:
	mobj_t *mo;

public:
	dynlight_glow_c(mobj_t *object) : mo(object)
	{ }
	
	virtual ~dynlight_glow_c()
	{ /* nothing to do */ }

	virtual void Sample(glow_color_c *col, float x, float y, float z)
	{
		// FIXME: assumes standard DLIGHT image

		float dx = x - mo->x;
		float dy = y - mo->y;
		float dz = z - mo->z;

		float d_squared = dx*dx + dy*dy + dz*dz;

		d_squared /= (mo->dlight[0].r * mo->dlight[0].r);

		float L = exp(-5.44 * d_squared);

		L = L * mo->state->bright / 255.0;

		if (L < 1/256.0)
			return;

		if (L > 4.0)
			L = 4.0;

		if (mo->info->dlight0.type == DLITE_Modulate)
			col->mod_Give(mo->dlight[0].color, L); 
		else
			col->add_Give(mo->dlight[0].color, L); 
	}
};


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
