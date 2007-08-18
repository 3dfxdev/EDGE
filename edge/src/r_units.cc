//----------------------------------------------------------------------------
//  EDGE OpenGL Rendering (Unit batching)
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
//
// -AJA- 2000/10/09: Began work on this new unit system.
//

#include "i_defs.h"
#include "i_defs_gl.h"

#include <vector>
#include <algorithm>

#include "con_cvar.h"
#include "m_argv.h"
#include "r_gldefs.h"
#include "r_units.h"
#include "z_zone.h"

#include "r_misc.h"
#include "r_image.h" //!!!

bool use_lighting = true;
bool use_color_material = true;

bool dumb_sky   = false;
bool dumb_multi = false;

#define USE_GLXTNS  1  // REMOVE !!!

// -AJA- FIXME
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE  0x812F
#endif


#define MAX_L_VERT  4096
#define MAX_L_UNIT  (MAX_L_VERT / 4)


// a single unit (polygon, quad, etc) to pass to the GL
typedef struct local_gl_unit_s
{
	// unit mode (e.g. GL_POLYGON)
	GLuint shape;

	// environment modes (GL_REPLACE, GL_MODULATE, GL_DECAL, GL_ADD)
	GLuint env[2];

	// texture(s) used
	GLuint tex[2];

	// pass number (multiple pass rendering)
	int pass;

	// blending flags
	int blending;

	// range of local vertices
	int first, count;
}
local_gl_unit_t;


static local_gl_vert_t local_verts[MAX_L_VERT];
static local_gl_unit_t local_units[MAX_L_UNIT];

static std::vector<local_gl_unit_t *> local_unit_map;

static int cur_vert;
static int cur_unit;

static bool solid_mode;


//
// RGL_InitUnits
//
// Initialise the unit system.  Once-only call.
//
void RGL_InitUnits(void)
{
	M_CheckBooleanParm("lighting",      &use_lighting, false);
	M_CheckBooleanParm("colormaterial", &use_color_material, false);
	M_CheckBooleanParm("dumbsky",       &dumb_sky, false);

	CON_CreateCVarBool("lighting",      cf_normal, &use_lighting);
	CON_CreateCVarBool("colormaterial", cf_normal, &use_color_material);
	CON_CreateCVarBool("dumbsky",       cf_normal, &dumb_sky);

	// Run the soft init code
	RGL_SoftInitUnits();
}

//
// RGL_SoftInitUnits
//
// -ACB- 2004/02/15 Quickly-hacked routine to reinit stuff lost on res change
//
void RGL_SoftInitUnits()
{
}

static void ComputeMiddle(vec3_t *mid, vec3_t *verts, int count)
{
	mid->x = verts[0].x;
	mid->y = verts[0].y;
	mid->z = verts[0].z;

	for (int i=1; i < count; i++)
	{
		mid->x += verts[i].x;
		mid->y += verts[i].y;
		mid->z += verts[i].z;
	}

	mid->x /= float(count);
	mid->y /= float(count);
	mid->z /= float(count);
}



//
// RGL_StartUnits
//
// Starts a fresh batch of units.  There should be two batches of
// units, the first with solid == true (handling all solid
// walls/floors), and the second with solid == false (handling
// everything else: sprites, masked textures, translucent planes).
//
// The solid batch will be sorted to keep texture changes to a
// minimum.  The non-solid batch is drawn in-order (and should be
// processed from furthest to closest).
//
void RGL_StartUnits(bool solid)
{
	cur_vert = cur_unit = 0;

	solid_mode = solid;

	local_unit_map.resize(MAX_L_UNIT);
}

//
// RGL_FinishUnits
//
// Finishes a batch of units, drawing any that haven't been drawn yet.
//
void RGL_FinishUnits(void)
{
	RGL_DrawUnits();
}

//
// RGL_BeginUnit
//
// Begin a new unit, with the given parameters (mode and texture ID).
// `max_vert' is the maximum expected vertices of the quad/poly (the
// actual number can be less, but never more).  Returns a pointer to
// the first vertex structure.  `masked' should be true if the texture
// contains "holes" (like sprites).  `blended' should be true if the
// texture should be blended (like for translucent water or sprites).
//
local_gl_vert_t *RGL_BeginUnit(GLuint shape, int max_vert, 
		                       GLuint env1, GLuint tex1,
							   GLuint env2, GLuint tex2,
							   int pass, int blending)
{
	local_gl_unit_t *unit;

	SYS_ASSERT(max_vert > 0);
	SYS_ASSERT(pass >= 0);

	// check we have enough space left
	if (cur_vert + max_vert > MAX_L_VERT || cur_unit >= MAX_L_UNIT)
	{
		RGL_DrawUnits();
	}

	unit = local_units + cur_unit;

	unit->shape  = shape;
	unit->env[0] = env1;
	unit->env[1] = env2;
	unit->tex[0] = tex1;
	unit->tex[1] = tex2;

	unit->pass     = pass;
	unit->blending = blending;
	unit->first    = cur_vert;  // count set later

	return local_verts + cur_vert;
}

//
// RGL_EndUnit
//
void RGL_EndUnit(int actual_vert)
{
	local_gl_unit_t *unit;

	SYS_ASSERT(actual_vert > 0);

	unit = local_units + cur_unit;

	unit->count = actual_vert;

	cur_vert += actual_vert;
	cur_unit++;

	SYS_ASSERT(cur_vert <= MAX_L_VERT);
	SYS_ASSERT(cur_unit <= MAX_L_UNIT);
}


struct Compare_Unit_pred
{
	inline bool operator() (const local_gl_unit_t *A, const local_gl_unit_t *B) const
	{
		if (A->pass != B->pass)
			return A->pass < B->pass;

		if (A->tex[0] != B->tex[0])
			return A->tex[0] < B->tex[0];

		if (A->tex[1] != B->tex[1])
			return A->tex[1] < B->tex[1];

		if (A->env[0] != B->env[0])
			return A->env[0] < B->env[0];

		if (A->env[1] != B->env[1])
			return A->env[1] < B->env[1];

		return A->blending < B->blending;
	}
};

static inline void RGL_SendRawVector(const local_gl_vert_t *V)
{
	if (use_color_material || ! use_lighting)
		glColor4fv(V->rgba);
	else
	{
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, V->rgba);
		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, V->rgba);
	}

	glMultiTexCoord2f(GL_TEXTURE0, V->texc[0].x, V->texc[0].y);
	glMultiTexCoord2f(GL_TEXTURE1, V->texc[1].x, V->texc[1].y);

	glNormal3f(V->normal.x, V->normal.y, V->normal.z);
	glEdgeFlag(V->edge);

	// vertex must be last
	glVertex3f(V->pos.x, V->pos.y, V->pos.z);
}

//
// RGL_DrawUnits
//
// Forces the set of current units to be drawn.  This call is
// optional (it never _needs_ to be called by client code).
//
void RGL_DrawUnits(void)
{
	if (cur_unit == 0)
		return;

	GLuint active_tex[2] = { 0, 0 };
	GLuint active_env[2] = { 0, 0 };

	int active_pass = 0;
	int active_blending = 0;

	for (int i=0; i < cur_unit; i++)
		local_unit_map[i] = & local_units[i];

	// need to sort ?
	if (solid_mode)
	{
		std::sort(local_unit_map.begin(),
				  local_unit_map.begin() + cur_unit,
				  Compare_Unit_pred());
	}

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_BLEND);

	glPolygonOffset(0, 0);


	for (int j=0; j < cur_unit; j++)
	{
		local_gl_unit_t *unit = local_unit_map[j];

		SYS_ASSERT(unit->count > 0);

		// detect changes in texture/alpha/blending state

		if (active_pass != unit->pass)
		{
			active_pass = unit->pass;

			glPolygonOffset(0, -active_pass);
		}

		if ((active_blending ^ unit->blending) & BL_Masked)
		{
			if (unit->blending & BL_Masked)
				glEnable(GL_ALPHA_TEST);
			else
				glDisable(GL_ALPHA_TEST);
		}

		if ((active_blending ^ unit->blending) & (BL_Alpha | BL_Add))
		{
			if (unit->blending & BL_Alpha)
			{
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			}
			else if (unit->blending & BL_Add)
			{
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE);
			}
			else
				glDisable(GL_BLEND);
		}

		if ((active_blending ^ unit->blending) & BL_CullBack)
		{
			if (unit->blending & BL_CullBack)
				glEnable(GL_CULL_FACE);
			else
				glDisable(GL_CULL_FACE);
		}

		if ((active_blending ^ unit->blending) & BL_NoZBuf)
		{
			glDepthMask((unit->blending & BL_NoZBuf) ? GL_FALSE : GL_TRUE);
		}

		active_blending = unit->blending;

		for (int t=1; t >= 0; t--)
		{
			glActiveTexture(GL_TEXTURE0 + t);

			if (active_tex[t] != unit->tex[t])
			{
				if (unit->tex[t] == 0)
					glDisable(GL_TEXTURE_2D);
				else if (active_tex[t] == 0)
					glEnable(GL_TEXTURE_2D);

				if (unit->tex[t] != 0)
					glBindTexture(GL_TEXTURE_2D, unit->tex[t]);

				active_tex[t] = unit->tex[t];
			}

			if (active_env[t] != unit->env[t])
			{
				if (unit->env[t] != 0)
					glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, unit->env[t]);

				active_env[t] = unit->env[t];
			}
		}

		GLint old_clamp;

		if (active_blending & BL_ClampY)
		{
			glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, &old_clamp);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
				glcap_edgeclamp ? GL_CLAMP_TO_EDGE : GL_CLAMP);
		}

		glBegin(unit->shape);

		for (int v_idx=0; v_idx < unit->count; v_idx++)
		{
			RGL_SendRawVector(local_verts + unit->first + v_idx);
		}

		glEnd();

		// restore the clamping mode
		if (active_blending & BL_ClampY)
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, old_clamp);
	}

	// all done
	cur_vert = cur_unit = 0;

	glPolygonOffset(0, 0);

	for (int t=1; t >=0; t--)
	{
		glActiveTexture(GL_TEXTURE0 + t);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glDisable(GL_TEXTURE_2D);
	}

	glDepthMask(GL_TRUE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glDisable(GL_ALPHA_TEST);
	glDisable(GL_BLEND);
	glDisable(GL_CULL_FACE);
}


//============================================================================

extern image_c *fading_image;

///---static inline void Color_Rainbow(local_gl_vert_t *v, int R, int G, int B, float alpha)
///---{
///---	v->rgba[0] = MIN(1.0, R * ren_red_mul / 255.0);
///---	v->rgba[1] = MIN(1.0, G * ren_grn_mul / 255.0);
///---	v->rgba[2] = MIN(1.0, B * ren_blu_mul / 255.0);
///---	v->rgba[3] = alpha;
///---}
///---
///---static inline void Vertex_Std(local_gl_vert_t *v, const vec3_t *src, GLboolean edge)
///---{
///---	v->x = src->x;
///---	v->y = src->y;
///---	v->z = src->z;
///---
///---	v->edge = edge;
///---}

static inline void TexCoord_Fader(local_gl_vert_t *v, int t,
		const vec3_t *lit_pos, int lit_Nom, bool bottom)
{
	// distance from viewplane: (point - camera) . viewvec

	float lk_sin = M_Sin(viewvertangle);
	float lk_cos = M_Cos(viewvertangle);

	vec3_t viewvec;

	viewvec.x = lk_cos * viewcos;
	viewvec.y = lk_cos * viewsin;
	viewvec.z = lk_sin;

	float dx = (lit_pos->x - viewx) * viewvec.x;
	float dy = (lit_pos->y - viewy) * viewvec.y;
	float dz = (lit_pos->z - viewz) * viewvec.z;

	v->texc[t].x = (dx + dy + dz) / 1600.0;

	int lt_ay = lit_Nom / 4;
	if (bottom) lt_ay += 64;

	v->texc[t].y = (lt_ay + 0.5) / 128.0;
	v->texc[t].y = CLAMP(v->texc[t].y, 0.01, 0.99);
}


static inline void Pipeline_Colormap(int& group,
	GLuint shape, int num_vert,
	GLuint tex, float alpha, int blending, int flags,
	void *func_data, pipeline_coord_func_t func)
{
	/* FIRST PASS : draw the colormapped primitive */

	local_gl_vert_t *glvert;

	GLuint fade_tex = W_ImageCache(fading_image);

	/* FIXME: GL_DECAL single-pass mode */

	int lit_Nom = 160; //!!!!! FIXME
	bool simple_cmap = false;

	if (true)
	{
		glvert = RGL_BeginUnit(shape, num_vert, GL_MODULATE, tex,
				(simple_cmap || dumb_multi) ? GL_MODULATE : GL_DECAL, fade_tex,
				group, blending);
		group++;

		for (int v_idx=0; v_idx < num_vert; v_idx++)
		{
			local_gl_vert_t *dest = glvert + v_idx;

			dest->rgba[3] = alpha;

			vec3_t lit_pos;

			(*func)(func_data, v_idx, &dest->pos, dest->rgba,
					&dest->texc[0], &dest->normal, &lit_pos);

			TexCoord_Fader(dest, 1, &lit_pos, lit_Nom, false);
		}

		RGL_EndUnit(num_vert);
	}

///	if (false)  // additive pass (test for old cards)
///	{
///		blending &= ~BL_Alpha;
///		blending |=  BL_Add;
///
///		glvert = RGL_BeginUnit(shape, num_vert,
///				GL_MODULATE, tex, GL_ADD, fade_tex,
///				group, blending);
///		group++;
///
///		for (int v_idx=0; v_idx < num_vert; v_idx++)
///		{
///			local_gl_vert_t *dest = glvert + v_idx;
///
///			vec3_t lit_pos;
///
///			(*func)(func_data, v_idx, &dest->pos, dest->rgba,
///					&dest->texc[0], &dest->normal, &lit_pos);
///
///			dest->rgba[0] = 0.0;
///			dest->rgba[1] = 0.0;
///			dest->rgba[2] = 0.0;
///			dest->rgba[3] = alpha;
///
///			TexCoord_Fader(dest, 1, &lit_pos, lit_Nom, true);
///		}
///
///		RGL_EndUnit(num_vert);
///	}

	/* TODO: flat-shaded maps */
}

static inline void Pipeline_Glows(int& group)
{
	/* SECOND PASS : sector glows */

//	if (flags & PIPEF_NoLight)
//		return;

	if (true)
	{
		/* TODO: floor */
	}

	if (true)
	{
		/* TODO: ceiling */
	}

	if (true)
	{
		/* TODO: wall */
	}
}

static inline void Pipeline_Shadow(int& group)
{
	/* THIRD PASS : shadows */

//	if (! (flags & PIPEF_Shadows))
//		return;
}

static inline void Pipeline_DLights(int& group)
{
	/* FOURTH PASS : dynamic lighting */

//	if (flags & PIPEF_NoLight)
//		return;
}


void R_RunPipeline(GLuint shape, int num_vert,
		           GLuint tex, float alpha, int blending, int flags,
				   void *func_data, pipeline_coord_func_t func)
{
	int group = 0;

	Pipeline_Colormap(group,
		shape, num_vert,
		tex, alpha, blending, flags,
		func_data, func);

	Pipeline_Glows(group  );

	Pipeline_Shadow(group  );

	Pipeline_DLights(group  );
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
