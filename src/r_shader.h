//----------------------------------------------------------------------------
//  EDGE Lighting Shaders
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

#ifndef __R_SHADER_H__
#define __R_SHADER_H__

#include "ddf/types.h"

/// #include "r_units.h"

class multi_color_c
{
public:
	int mod_R, mod_G, mod_B;
	int add_R, add_G, add_B;

public:
	 multi_color_c() { }
	~multi_color_c() { }

	void Clear()
	{
		mod_R = mod_G = mod_B = 0;
		add_R = add_G = add_B = 0;
	}

	void operator+= (const multi_color_c& rhs)
	{
		mod_R += rhs.mod_R;
		mod_G += rhs.mod_G;
		mod_B += rhs.mod_B;

		add_R += rhs.add_R;
		add_G += rhs.add_G;
		add_B += rhs.add_B;
	}

	int mod_MAX() const
	{
		return MAX(mod_R, MAX(mod_G, mod_B));
	}

	int add_MAX() const
	{
		return MAX(add_R, MAX(add_G, add_B));
	}

	void mod_Give(rgbcol_t rgb, float qty)
	{
		if (qty > 100) qty = 100;

		mod_R += (int)(RGB_RED(rgb) * qty);
		mod_G += (int)(RGB_GRN(rgb) * qty);
		mod_B += (int)(RGB_BLU(rgb) * qty);
	}

	void add_Give(rgbcol_t rgb, float qty)
	{
		if (qty > 100) qty = 100;

		add_R += (int)(RGB_RED(rgb) * qty);
		add_G += (int)(RGB_GRN(rgb) * qty);
		add_B += (int)(RGB_BLU(rgb) * qty);
	}
};


typedef void (* shader_coord_func_t)(void *data, int v_idx,
	vec3_t *pos, float *rgb, vec2_t *texc,
	vec3_t *normal, vec3_t *lit_pos);


/* abstract base class */
class abstract_shader_c
{
public:
	abstract_shader_c() { }
	virtual ~abstract_shader_c() { }

	virtual void Sample(multi_color_c *col, float x, float y, float z) = 0;
	// used for arbitrary points in the world (sprites)

	virtual void Corner(multi_color_c *col, float nx, float ny, float nz,
			            struct mobj_s *mod_pos, bool is_weapon = false) = 0;
	// used for normal-based lighting (MD2 models)

	virtual void WorldMix(GLuint shape, int num_vert,
		GLuint tex, float alpha, int *pass_var, int blending,
		void *data, shader_coord_func_t func) = 0;
	// used to render overlay textures (world polygons)
};


/* FUNCTIONS */


#endif /* __R_SHADER_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
