//----------------------------------------------------------------------------
//  EDGE Lighting Shaders
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

#ifndef __R_SHADER_H__
#define __R_SHADER_H__

#include "ddf/types.h"

#include "r_units.h"

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
		return MAX(mod_B, MAX(mod_G, mod_B));
	}

	int add_MAX() const
	{
		return MAX(add_B, MAX(add_G, add_B));
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


/* abstract base class */
class abstract_shader_c
{
public:
	abstract_shader_c
	virtual ~abstract_shader_c

	virtual void Sample(multi_color_c *col, float x, float y, float z) = 0;
};


#endif /* __R_SHADER_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
