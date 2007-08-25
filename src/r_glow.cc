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
#include "r_misc.h"
#include "r_state.h"


//----------------------------------------------------------------------------
//  COLORMAP
//----------------------------------------------------------------------------

int R_DoomLightingEquation(int L, float dist)
{
	/* L in the range 0 to 63 */

	int index = (59 - L) - int(1280 / MAX(1, dist));
	int min_L = MAX(0, 36 - L);

	/* result is colormap index (0 bright .. 31 dark) */

	return CLAMP(index, min_L, 31);
}

class colormap_glow_c : public glow_source_c
{
public:
	// FIXME colormap_c 

	int light_lev;

public:
	colormap_glow_c(int light) : light_lev(light)
	{ }
	
	virtual ~colormap_glow_c()
	{ /* nothing to do */ }

	virtual void Sample(glow_color_c *col, float x, float y, float z)
	{
		// FIXME: assumes standard COLORMAP

		float dist = DistFromViewplane(x, y, z);

		int cmap_idx = R_DoomLightingEquation(light_lev/4, dist);

		int X = 255 - cmap_idx * 8 - cmap_idx / 5;

		col->mod_R += X;
		col->mod_G += X;
		col->mod_B += X;

		// FIXME: for foggy maps, need to adjust add_R/G/B too
	}

	float DistFromViewplane(float x, float y, float z)
	{
		float lk_cos = M_Cos(viewvertangle);
		float lk_sin = M_Sin(viewvertangle);

		// view vector
		float vx = lk_cos * viewcos;
		float vy = lk_cos * viewsin;
		float vz = lk_sin;

		float dx = (x - viewx) * vx;
		float dy = (y - viewy) * vy;
		float dz = (z - viewz) * vz;

		return dx + dy + dz;
	}
};


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

		if (L > 1/256.0)
		{
			if (mo->info->dlight0.type == DLITE_Add)
				col->add_Give(mo->dlight[0].color, L); 
			else
				col->mod_Give(mo->dlight[0].color, L); 
		}
	}
};


//----------------------------------------------------------------------------
//  SECTOR GLOWS
//----------------------------------------------------------------------------

class plane_glow_c : public glow_source_c
{
public:
	float h;

	mobj_t *mo;

public:
	plane_glow_c(float _height, mobj_t *_glower) :
		h(_height), mo(_glower)
	{ }
	
	virtual ~plane_glow_c()
	{ /* nothing to do */ }

	virtual void Sample(glow_color_c *col, float x, float y, float z)
	{
		// FIXME: assumes standard DLIGHT image

		float dz = (z - h) / mo->dlight[0].r;

		float L = exp(-5.44 * dz * dz);

		L = L * mo->state->bright / 255.0;

		if (L > 1/256.0)
		{
			if (mo->info->dlight0.type == DLITE_Add)
				col->add_Give(mo->dlight[0].color, L); 
			else
				col->mod_Give(mo->dlight[0].color, L); 
		}
	}
};


class wall_glow_c : public glow_source_c
{
public:
	line_t *ld;
	mobj_t *mo;

	float nx, ny; // normal

public:
	wall_glow_c(line_t *_wall, mobj_t *_glower) :
		ld(_wall), mo(_glower)
	{
		nx = (ld->v1->y - ld->v2->y) / ld->length;
		ny = (ld->v2->x - ld->v1->x) / ld->length;
	}

	virtual ~wall_glow_c()
	{ /* nothing to do */ }

	virtual void Sample(glow_color_c *col, float x, float y, float z)
	{
		// FIXME: assumes standard DLIGHT image

		float dist = (ld->v1->x - x) * nx + (ld->v1->y - y) * ny;

		float L = exp(-5.44 * dist * dist);

		L = L * mo->state->bright / 255.0;

		if (L > 1/256.0)
		{
			if (mo->info->dlight0.type == DLITE_Add)
				col->add_Give(mo->dlight[0].color, L); 
			else
				col->mod_Give(mo->dlight[0].color, L); 
		}
	}
};

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
