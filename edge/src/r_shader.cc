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

#include "i_defs.h"
#include "i_defs_gl.h"

#include "ddf/main.h"

#include "p_mobj.h"
#include "r_defs.h"
#include "r_gldefs.h"
#include "r_image.h"   // W_ImageCache
#include "r_misc.h"
#include "r_shader.h"
#include "r_state.h"


//----------------------------------------------------------------------------
//  LIGHT IMAGES
//----------------------------------------------------------------------------

#define LIM_CURVE_SIZE  32

class light_image_c
{
public:
	std::string name;

	const image_c *image;

	GLuint tex_id;

	rgbcol_t curve[LIM_CURVE_SIZE];

public:
	light_image_c(const char * _name) : name(_name), tex_id(0)
	{ }

	~light_image_c()
	{ }

	void MakeStdCurve() // TEMP CRUD
	{
		for (int i = 0; i < LIM_CURVE_SIZE-1; i++)
		{
			float d = i / (float)(LIM_CURVE_SIZE - 1);

			float sq = exp(-5.44 * d * d);

			int r1 = (int)(255 * sq);
			int g1 = (int)(255 * sq);
			int b1 = (int)(255 * sq);

			curve[i] = RGB_MAKE(r1, g1, b1);
		}

		curve[LIM_CURVE_SIZE-1] = RGB_MAKE(0, 0, 0);
	}

	rgbcol_t CurvePoint(float d, rgbcol_t tint)
	{
		// d is distance away from centre, between 0.0 and 1.0

		d *= (float)LIM_CURVE_SIZE;

		if (d >= LIM_CURVE_SIZE-1)
			return curve[LIM_CURVE_SIZE-1];

		// linearly interpolate between curve points

		int p1 = (int)floor(d);

		SYS_ASSERT(p1+1 < LIM_CURVE_SIZE);

		int dd = (int)(256 * (d - p1));

		SYS_ASSERT(0 <= dd && dd <= 256);

		int r1 = RGB_RED(curve[p1]);
		int g1 = RGB_GRN(curve[p1]);
		int b1 = RGB_BLU(curve[p1]);

		int r2 = RGB_RED(curve[p1+1]);
		int g2 = RGB_GRN(curve[p1+1]);
		int b2 = RGB_BLU(curve[p1+1]);

		r1 = (r1 * (256-dd) + r2 * dd) >> 8;
		g1 = (g1 * (256-dd) + g2 * dd) >> 8;
		b1 = (b1 * (256-dd) + b2 * dd) >> 8;

		r1 = r1 * RGB_RED(tint) / 255;
		g1 = g1 * RGB_GRN(tint) / 255;
		b1 = b1 * RGB_BLU(tint) / 255;

		return RGB_MAKE(r1, g1, b1);
	}
};

static light_image_c *GetLightImage(const mobjtype_c *info, int DL)
{
	// Intentional Const Overrides
	dlight_info_c *D_info = (dlight_info_c *) &info->dlight[DL];
	
	if (! D_info->cache_data)
	{
		// FIXME !!!! share light_image_c instances

		const char *shape = D_info->shape.GetString();

		light_image_c *lim = new light_image_c(shape);

		// FIXME !!!! we need the EPI::BASIC_IMAGE in order to compute the curve
		lim->MakeStdCurve();

		lim->image = W_ImageLookup(shape, INS_Graphic, ILF_Null);

		lim->tex_id = W_ImageCache(lim->image);

		D_info->cache_data = lim;
	}

	return (light_image_c *) D_info->cache_data;
}



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

class colormap_shader_c : public abstract_shader_c
{
private:
	// FIXME colormap_c 

	int light_lev;

	GLuint fade_tex;

	bool simple_cmap;

public:
	colormap_shader_c(int _light, GLuint _tex) :
		light_lev(_light), fade_tex(_tex), simple_cmap(true)
	{
	}

	virtual ~colormap_shader_c()
	{ /* nothing to do */ }

private:
	inline float DistFromViewplane(float x, float y, float z)
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

	inline void TexCoord(local_gl_vert_t *v, int t, const vec3_t *lit_pos)
	{
		float dist = DistFromViewplane(lit_pos->x, lit_pos->y, lit_pos->z);
				
		v->texc[t].x = dist / 1600.0;
		v->texc[t].y = ((light_lev / 4) + 0.5) / 64.0;
	}

public:
	virtual void Sample(multi_color_c *col, float x, float y, float z)
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

	virtual void Corner(multi_color_c *col, float nx, float ny, float nz,
			            struct mobj_s *mod_pos, bool pl_weap)
	{
		// TODO: improve this

		Sample(col, mod_pos->x, mod_pos->y, mod_pos->z);
	}

	virtual void WorldMix(GLuint shape, int num_vert,
		GLuint tex, float alpha, int pass, int blending,
		void *data, shader_coord_func_t func)
	{
		local_gl_vert_t * glvert = RGL_BeginUnit(shape, num_vert,
				GL_MODULATE, tex,
				(simple_cmap || dumb_multi) ? GL_MODULATE : GL_DECAL,
				fade_tex, pass, blending);

		for (int v_idx=0; v_idx < num_vert; v_idx++)
		{
			local_gl_vert_t *dest = glvert + v_idx;

			dest->rgba[3] = alpha;

			vec3_t lit_pos;

			(*func)(data, v_idx, &dest->pos, dest->rgba,
					&dest->texc[0], &dest->normal, &lit_pos);

			TexCoord(dest, 1, &lit_pos);
		}

		RGL_EndUnit(num_vert);
	}

};

extern GLuint MakeColormapTexture( int mode );

extern abstract_shader_c *MakeColormapShader(void)
{
	GLuint tex = MakeColormapTexture(0);

	return new colormap_shader_c(144, tex);
}



//----------------------------------------------------------------------------
//  DYNAMIC LIGHTS
//----------------------------------------------------------------------------

class dynlight_shader_c : public abstract_shader_c
{
private:
	mobj_t *mo;

	light_image_c *lim[2];

public:
	dynlight_shader_c(mobj_t *object) : mo(object)
	{
		lim[0] = GetLightImage(mo->info, 0);
		lim[1] = GetLightImage(mo->info, 1);
	}

	virtual ~dynlight_shader_c()
	{ /* nothing to do */ }

private:
	inline float TexCoord(const mobj_t *mo, float r,
		const vec3_t *lit_pos, const vec3_t *normal,
		GLfloat *rgb, vec2_t *texc)
	{
		float dx = lit_pos->x - mo->x;
		float dy = lit_pos->y - mo->y;
		float dz = lit_pos->z - mo->z;

		float nx = normal->x;
		float ny = normal->y;
		float nz = normal->z;

		if (fabs(nz) > 50*(fabs(nx)+fabs(ny)))
		{
			/* horizontal plane */
			texc->x = (1 + dx / r) / 2.0;
			texc->y = (1 + dy / r) / 2.0;

			return fabs(dz) / r;
		}
		else
		{
			float n_len = sqrt(nx*nx + ny*ny + nz*nz);

			nx /= n_len;
			ny /= n_len;
			nz /= n_len;

			float dxy = nx * dy - ny * dx;

			r /= sqrt(nx*nx + ny*ny);  // correct ??

			texc->y = (1 + dz  / r) / 2.0;
			texc->x = (1 + dxy / r) / 2.0;

			return fabs(nx*dx + ny*dy + nz*dz) / r;
		}
	}

	inline float WhatRadius(int DL)
	{
		if (DL == 0)
			return mo->dlight.r;

		return mo->info->dlight[1].radius * mo->dlight.r /
			   mo->info->dlight[0].radius;
	}

	inline rgbcol_t WhatColor(int DL)
	{
		return (DL == 0) ? mo->dlight.color : mo->info->dlight[1].colour;
	}

	inline dlight_type_e WhatType(int DL)
	{
		return mo->info->dlight[DL].type;
	}

public:
	virtual void Sample(multi_color_c *col, float x, float y, float z)
	{
		float dx = x - mo->x;
		float dy = y - mo->y;
		float dz = z - mo->z;

		float dist = sqrt(dx*dx + dy*dy + dz*dz);

		for (int DL = 0; DL < 2; DL++)
		{
			if (WhatType(DL) == DLITE_None)
				break;

			rgbcol_t new_col = lim[DL]->CurvePoint(dist / WhatRadius(DL),
					WhatColor(DL));

			float L = mo->state->bright / 255.0;

			if (new_col != RGB_MAKE(0,0,0) && L > 1/256.0)
			{
				if (WhatType(DL) == DLITE_Add)
					col->add_Give(new_col, L); 
				else
					col->mod_Give(new_col, L); 
			}
		}
	}

	virtual void Corner(multi_color_c *col, float nx, float ny, float nz,
			            struct mobj_s *mod_pos, bool pl_weap)
	{
		float dx = mod_pos->x - mo->x;
		float dy = mod_pos->y - mo->y;
		float dz = MO_MIDZ(mod_pos) - MO_MIDZ(mo->z);

		float dist = sqrt(dx*dx + dy*dy + dz*dz);

		dx /= dist;
		dy /= dist;
		dz /= dist;

		dist = MAX(1.0, dist - mod_pos->radius);

		for (int DL = 0; DL < 2; DL++)
		{
			if (WhatType(DL) == DLITE_None)
				break;

			rgbcol_t new_col = lim[DL]->CurvePoint(dist / WhatRadius(DL),
					WhatColor(DL));

			float L = 0.5 + 0.5 * (dx*nx + dy*ny + dz*nz);

			L *= mo->state->bright / 255.0;

			if (new_col != RGB_MAKE(0,0,0) && L > 1/256.0)
			{
				if (WhatType(DL) == DLITE_Add)
					col->add_Give(new_col, L); 
				else
					col->mod_Give(new_col, L); 
			}
		}
	}

	virtual void WorldMix(GLuint shape, int num_vert,
		GLuint tex, float alpha, int pass, int blending,
		void *data, shader_coord_func_t func)
	{
		//!!!!! FIXME: if (dist_to_light > DL->info->radius) continue;

		for (int DL = 0; DL < 2; DL++)
		{
			if (WhatType(DL) == DLITE_None)
				break;

			bool is_additive = (WhatType(DL) == DLITE_Add);

			rgbcol_t col = WhatColor(DL);

			float R = RGB_RED(col) / 255.0;
			float G = RGB_GRN(col) / 255.0;
			float B = RGB_BLU(col) / 255.0;

			local_gl_vert_t *glvert = RGL_BeginUnit(shape, num_vert,
						is_additive ? ENV_NONE : GL_MODULATE,
						is_additive ? 0 : tex,
						GL_MODULATE, lim[DL]->tex_id, pass, blending);

			for (int v_idx=0; v_idx < num_vert; v_idx++)
			{
				local_gl_vert_t *dest = glvert + v_idx;

				vec3_t lit_pos;

				(*func)(data, v_idx, &dest->pos, dest->rgba,
						&dest->texc[0], &dest->normal, &lit_pos);

				float dist = TexCoord(mo, WhatRadius(DL),
						 &lit_pos, &dest->normal,
						 dest->rgba, &dest->texc[1]);

				float ity = exp(-5.44 * dist * dist);

				dest->rgba[0] = R * ity;
				dest->rgba[1] = G * ity;
				dest->rgba[2] = B * ity;
				dest->rgba[3] = alpha;
			}

			RGL_EndUnit(num_vert);
		}
	}

};

abstract_shader_c *MakeDLightShader(mobj_t *mo)
{
	return new dynlight_shader_c(mo);
}



//----------------------------------------------------------------------------
//  SECTOR GLOWS
//----------------------------------------------------------------------------

class plane_glow_c : public abstract_shader_c
{
private:
	float h;

	mobj_t *mo;

public:
	plane_glow_c(float _height, mobj_t *_glower) :
		h(_height), mo(_glower)
	{ }
	
	virtual ~plane_glow_c()
	{ /* nothing to do */ }

	virtual void Sample(multi_color_c *col, float x, float y, float z)
	{
		// FIXME: assumes standard DLIGHT image

		float dz = (z - h) / mo->dlight.r;

		float L = exp(-5.44 * dz * dz);

		L = L * mo->state->bright / 255.0;

		if (L > 1/256.0)
		{
			if (mo->info->dlight[0].type == DLITE_Add)
				col->add_Give(mo->dlight.color, L); 
			else
				col->mod_Give(mo->dlight.color, L); 
		}
	}

	virtual void WorldMix(GLuint shape, int num_vert,
		GLuint tex, float alpha, int pass, int blending,
		void *data, shader_coord_func_t func)
	{
		/* TODO */
	}
};


class wall_glow_c : public abstract_shader_c
{
private:
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

	virtual void Sample(multi_color_c *col, float x, float y, float z)
	{
		// FIXME: assumes standard DLIGHT image

		float dist = (ld->v1->x - x) * nx + (ld->v1->y - y) * ny;

		float L = exp(-5.44 * dist * dist);

		L = L * mo->state->bright / 255.0;

		if (L > 1/256.0)
		{
			if (mo->info->dlight[0].type == DLITE_Add)
				col->add_Give(mo->dlight.color, L); 
			else
				col->mod_Give(mo->dlight.color, L); 
		}
	}

	virtual void WorldMix(GLuint shape, int num_vert,
		GLuint tex, float alpha, int pass, int blending,
		void *data, shader_coord_func_t func)
	{
		/* TODO */
	}
};



//----------------------------------------------------------------------------
//  LASER GLOWS
//----------------------------------------------------------------------------

class laser_glow_c : public abstract_shader_c
{
private:
	vec3_t s, e;

	float length;
	vec3_t normal;

	const mobjtype_c *info;
	float bright;

	light_image_c *lim[2];

public:
	laser_glow_c(const vec3_t& _v1, const vec3_t& _v2,
				 const mobjtype_c *_info, float _intensity) :
		s(_v1), e(_v2), info(_info), bright(_intensity)
	{
		normal.x = e.x - s.x;
		normal.y = e.y - s.y;
		normal.z = e.z - s.z;

		length = sqrt(normal.x * normal.x + normal.y * normal.y +
				      normal.z * normal.z);

		if (length < 0.1)
			length = 0.1;

		normal.x /= length;
		normal.y /= length;
		normal.z /= length;

		lim[0] = GetLightImage(info, 0);
		lim[1] = GetLightImage(info, 1);
	}

	virtual ~laser_glow_c()
	{ /* nothing to do */ }

private:
	inline float WhatRadius(int DL)
	{
		return info->dlight[DL].radius;
	}

	inline rgbcol_t WhatColor(int DL)
	{
		return info->dlight[DL].colour;
	}

	inline dlight_type_e WhatType(int DL)
	{
		return info->dlight[DL].type;
	}

public:
	virtual void Sample(multi_color_c *col, float x, float y, float z)
	{
		x -= s.x;
		y -= s.y;
		z -= s.z;

		/* get perpendicular and along distances */
		
		// dot product
		float along = x*normal.x + y*normal.y + z*normal.z;

		// cross product
		float cx = y * normal.z - normal.y * z;
		float cy = z * normal.x - normal.z * x;
		float cz = x * normal.y - normal.x * y;

		float dist = (cx*cx + cy*cy + cz*cz);

		for (int DL=0; DL < 2; DL++)
		{
			if (WhatType(DL) == DLITE_None)
				break;

			float d = dist;

			if (along < 0)
				d -= along;
			else if (along > length)
				d += (along - length);

			rgbcol_t new_col = lim[DL]->CurvePoint(d / WhatRadius(DL),
					WhatColor(DL));

			float L = bright / 255.0;

			if (new_col != RGB_MAKE(0,0,0) && L > 1/256.0)
			{
				if (WhatType(DL) == DLITE_Add)
					col->add_Give(new_col, L); 
				else
					col->mod_Give(new_col, L); 
			}
		}
	}

	virtual void WorldMix(GLuint shape, int num_vert,
		GLuint tex, float alpha, int pass, int blending,
		void *data, shader_coord_func_t func)
	{
		/* TODO */
	}
};


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
