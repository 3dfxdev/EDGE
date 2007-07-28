//----------------------------------------------------------------------------
//  EDGE OpenGL Rendering (Unit batching)
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2005  The EDGE Team.
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

#include "con_cvar.h"
#include "e_search.h"
#include "m_argv.h"
#include "r2_defs.h"
#include "rgl_defs.h"
#include "rgl_unit.h"
#include "z_zone.h"

#include <math.h>
#include <string.h>

bool use_lighting = true;
bool use_color_material = true;
bool use_vertex_array = true;

bool dumb_sky = false;

#define USE_GLXTNS  1


#define MAX_L_VERT  4096
#define MAX_L_UNIT  (MAX_L_VERT / 4)

// -AJA- FIXME
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE  0x812F
#endif

// a single unit (polygon, quad, etc) to pass to the GL
typedef struct local_gl_unit_s
{
	// unit mode (e.g. GL_POLYGON)
	GLuint mode;

	// range of local vertices
	int first, count;

	// texture(s) used
	GLuint tex_id[2];

	// pass number (when multiple passes_
	int pass;

	// blending modes
	int blending;
}
local_gl_unit_t;

static local_gl_vert_t local_verts[MAX_L_VERT];
static local_gl_unit_t local_units[MAX_L_UNIT];

static GLuint local_unit_map[MAX_L_UNIT];

static int cur_vert;
static int cur_unit;

static bool solid_mode;

bool rgl_1d_debug = false;


//
// RGL_InitUnits
//
// Initialise the unit system.  Once-only call.
//
void RGL_InitUnits(void)
{
	M_CheckBooleanParm("lighting",      &use_lighting, false);
	M_CheckBooleanParm("vertexarray",   &use_vertex_array, false);
	M_CheckBooleanParm("colormaterial", &use_color_material, false);
	M_CheckBooleanParm("dumbsky",       &dumb_sky, false);

	CON_CreateCVarBool("lighting",      cf_normal, &use_lighting);
	CON_CreateCVarBool("vertexarray",   cf_normal, &use_vertex_array);
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
	if (true)  /// XXX
	{
		// setup pointers to client state
		glVertexPointer(3, GL_FLOAT, sizeof(local_gl_vert_t), &local_verts[0].x);
		glColorPointer (4, GL_FLOAT, sizeof(local_gl_vert_t), &local_verts[0].col);
		glTexCoordPointer(2, GL_FLOAT, sizeof(local_gl_vert_t), &local_verts[0].t_x);
		glNormalPointer(GL_FLOAT, sizeof(local_gl_vert_t), &local_verts[0].n_x);
		glEdgeFlagPointer(sizeof(local_gl_vert_t), &local_verts[0].edge);

		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_COLOR_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glEnableClientState(GL_NORMAL_ARRAY);
		glEnableClientState(GL_EDGE_FLAG_ARRAY);

#if 0  // REQUIRES OPENGL 1.3
		glClientActiveTexture(GL_TEXTURE0);
#endif
	}
}

void ComputeMiddle(vec3_t *mid, vec3_t *verts, int count)
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
local_gl_vert_t *RGL_BeginUnit(GLuint mode, int max_vert,
							   GLuint tex_id, GLuint tex2_id,
							   int pass, int blending)
{
	local_gl_unit_t *unit;

	SYS_ASSERT(max_vert > 0);
	SYS_ASSERT(tex_id != 0);
	SYS_ASSERT(pass >= 0);

	// check for out-of-space
	if (cur_vert + max_vert > MAX_L_VERT || cur_unit >= MAX_L_UNIT)
	{
		RGL_DrawUnits();
	}

	unit = local_units + cur_unit;

	unit->mode      = mode;
	unit->tex_id[0] = tex_id;
	unit->tex_id[1] = tex2_id;
	unit->pass      = pass;
	unit->first     = cur_vert;  // count set later
	unit->blending  = blending;

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

void RGL_SendRawVector(const local_gl_vert_t *V)
{
	if (use_color_material || ! use_lighting)
		glColor4fv(V->col);
	else
	{
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, V->col);
		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, V->col);
	}

	glTexCoord2f(V->t_x, V->t_y);

#ifdef USE_GLXTNS
	glMultiTexCoord2f(GL_TEXTURE1, V->t2_x, V->t2_y);
#endif

	glNormal3f(V->n_x, V->n_y, V->n_z);
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
	int i, j;
	GLuint cur_tex  = 0x76543210;
#ifdef USE_GLXTNS
	GLuint cur_tex2 = 0x76543210;
#endif

	GLint old_tmode = 0;

	int cur_pass = -1;
	int cur_blending = -1;

	if (cur_unit == 0)
		return;

	for (i=0; i < cur_unit; i++)
		local_unit_map[i] = i;

	// need to sort ?
	if (solid_mode)
	{
#define CMP(a,b)  \
	(local_units[a].pass < local_units[b].pass ||   \
	(local_units[a].pass == local_units[b].pass &&   \
	(local_units[a].tex_id[0] < local_units[b].tex_id[0] ||   \
	(local_units[a].tex_id[0] == local_units[b].tex_id[0] &&   \
	(local_units[a].blending < local_units[b].blending)))))
		QSORT(GLuint, local_unit_map, cur_unit, CUTOFF);
#undef CMP
	}

	glEnable(GL_TEXTURE_2D);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_BLEND);

//	glEnable(GL_LIGHT_0);


#if 0
	float fog_col[4] = { 1, 1, 0.3, 0};
	glDisable(GL_FOG);
	glFogi(GL_FOG_MODE, GL_EXP);
	glFogf(GL_FOG_DENSITY, 0.002f);
	glFogfv(GL_FOG_COLOR, fog_col);
#endif

	for (i=0; i < cur_unit; i++)
	{
		local_gl_unit_t *unit = local_units + local_unit_map[i];

		SYS_ASSERT(unit->count > 0);
		SYS_ASSERT(unit->tex_id != 0);

		// detect changes in texture/alpha/blending and change state

		if (cur_pass != unit->pass)
		{
			cur_pass = unit->pass;

#ifdef USE_GLXTNS
			glPolygonOffset(0, -cur_pass);
#endif
		}

		if (cur_blending != unit->blending)
		{
			cur_blending = unit->blending;

			if (cur_blending & BL_Masked)
				glEnable(GL_ALPHA_TEST);
			else
				glDisable(GL_ALPHA_TEST);

			if (cur_blending & BL_Alpha)
			{
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			}
			else if (cur_blending & BL_Add)
			{
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE);
			}
			else
				glDisable(GL_BLEND);

			glDepthMask((cur_blending & BL_NoZBuf) ? GL_FALSE : GL_TRUE);

			if (cur_blending & BL_Multi)
			{
#ifdef USE_GLXTNS
				glActiveTexture(GL_TEXTURE1);
				glEnable(GL_TEXTURE_2D);
				glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
				glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
				glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE);

				if (cur_tex2 != unit->tex_id[1])
				{
					cur_tex2 = unit->tex_id[1];
					glBindTexture(GL_TEXTURE_2D, cur_tex2);
				}

				glActiveTexture(GL_TEXTURE0);
#endif
			}
			else
			{
#ifdef USE_GLXTNS
				glActiveTexture(GL_TEXTURE1);
				glDisable(GL_TEXTURE_2D);
				glActiveTexture(GL_TEXTURE0);
#endif
			}
		}

		if (cur_tex != unit->tex_id[0])
		{
			cur_tex = unit->tex_id[0];
			glBindTexture(GL_TEXTURE_2D, cur_tex);
		}

		if (cur_blending & BL_ClampY)
		{
			glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, &old_tmode);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
				glcap_edgeclamp ? GL_CLAMP_TO_EDGE : GL_CLAMP);
		}


		if (use_vertex_array)
		{
			glDrawArrays(unit->mode, unit->first, unit->count);
		}
		else
		{
			glBegin(unit->mode);

			for (j=0; j < unit->count; j++)
				RGL_SendRawVector(local_verts + unit->first + j);

			glEnd();
		}

		// restore the clamping mode
		if (cur_blending & BL_ClampY)
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, old_tmode);
	}

	// all done
	cur_vert = cur_unit = 0;

#ifdef USE_GLXTNS
	glPolygonOffset(0, 0);

	glActiveTexture(GL_TEXTURE1);
	glDisable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE0);
#endif

	glDepthMask(GL_TRUE);

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_BLEND);
}


//----------------------------------------------------------------------------
//
//  RAW POLYQUAD CODE
//
//  Note: quads are always vertical rectangles, and currently the
//  bottom and top lines are always horizontal (i.e. never sloped).
//
//  Note 2: polygons are always horizontal.
//  

raw_polyquad_t *RGL_NewPolyQuad(int maxvert)
{
	raw_polyquad_t *poly;

	poly = Z_ClearNew(raw_polyquad_t, 1);

	poly->verts = Z_New(vec3_t, maxvert);
	poly->num_verts = 0;
	poly->max_verts = maxvert;

	return poly;
}

void RGL_FreePolyQuad(raw_polyquad_t *poly)
{
	while (poly)
	{
		raw_polyquad_t *cur = poly;
		poly = poly->sisters;

		Z_Free(cur->verts);
		Z_Free(cur);
	}
}

void RGL_BoundPolyQuad(raw_polyquad_t *poly)
{
	SYS_ASSERT(poly->num_verts > 0);

	poly->bb_mid.x = 0;
	poly->bb_mid.y = 0;
	poly->bb_mid.z = 0;

	poly->bb_rad = 0;

	int j;

	for (j=0; j < poly->num_verts; j++)
	{
		poly->bb_mid.x += poly->verts[j].x;
		poly->bb_mid.y += poly->verts[j].y;
		poly->bb_mid.z += poly->verts[j].z;
	}

	poly->bb_mid.x /= poly->num_verts;
	poly->bb_mid.y /= poly->num_verts;
	poly->bb_mid.z /= poly->num_verts;

	for (j=0; j < poly->num_verts; j++)
	{
		float dist = APPROX_DIST3(
						fabs(poly->bb_mid.x - poly->verts[j].x),
						fabs(poly->bb_mid.y - poly->verts[j].y),
						fabs(poly->bb_mid.z - poly->verts[j].z));

		if (poly->bb_rad < dist)
			poly->bb_rad = dist;
	}
}


void RGL_RenderPolyQuad(raw_polyquad_t *poly, void *data,
						void (* CoordFunc)(vec3_t *src, local_gl_vert_t *vert, void *data),
						GLuint tex_id, GLuint tex2_id,
						int pass, int blending)
{
	int j;
	local_gl_vert_t *vert;

	while (poly)
	{
		raw_polyquad_t *cur = poly;
		poly = poly->sisters;

		SYS_ASSERT(cur->num_verts > 0);
		SYS_ASSERT(cur->num_verts <= cur->max_verts);

		vert = RGL_BeginUnit(GL_POLYGON,
			cur->num_verts, tex_id, tex2_id, pass, blending);

		for (j=0; j < cur->num_verts; j++)
		{
			(* CoordFunc)(cur->verts + j, vert + j, data);
		}

		RGL_EndUnit(j);
	}
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
