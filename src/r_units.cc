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

bool use_lighting = true;
bool use_color_material = true;

bool dumb_sky = false;

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
		glColor4fv(V->col);
	else
	{
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, V->col);
		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, V->col);
	}

	glMultiTexCoord2f(GL_TEXTURE0, V->tx[0], V->ty[0]);
	glMultiTexCoord2f(GL_TEXTURE1, V->tx[1], V->ty[1]);

	glNormal3f(V->nx, V->ny, V->nz);
	glEdgeFlag(V->edge);

	// vertex must be last
	glVertex3f(V->x, V->y, V->z);
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

		GLuint old_clamp;

		if (active_blending & BL_ClampY)
		{
			glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, &old_clamp);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
				glcap_edgeclamp ? GL_CLAMP_TO_EDGE : GL_CLAMP);
		}

		glBegin(unit->shape);

		for (int kk=0; kk < unit->count; kk++)
		{
			RGL_SendRawVector(local_verts + unit->first + kk);
		}

		glEnd();

		// restore the clamping mode
		if (active_blending & BL_ClampY)
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, old_clamp);
	}

	// all done
	cur_vert = cur_unit = 0;

	glPolygonOffset(0, 0);

	glActiveTexture(GL_TEXTURE1);
	glDisable(GL_TEXTURE_2D);

	glActiveTexture(GL_TEXTURE0);
	glDisable(GL_TEXTURE_2D);

	glDepthMask(GL_TRUE);

	glDisable(GL_ALPHA_TEST);
	glDisable(GL_BLEND);
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
