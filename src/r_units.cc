//----------------------------------------------------------------------------
//  EDGE2 OpenGL Rendering (Unit batching)
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2009  The EDGE2 Team.
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

#include "../epi/image_data.h"

#include "m_argv.h"
#include "r_gldefs.h"
#include "r_units.h"
#include "z_zone.h"

#include "r_misc.h"
#include "r_image.h"
#include "r_texgl.h"
#include "r_shader.h"


cvar_c r_colorlighting;
cvar_c r_colormaterial;

cvar_c r_dumbsky;
cvar_c r_dumbmulti;
cvar_c r_dumbcombine;
cvar_c r_dumbclamp;


#define MAX_L_VERT  4096
#define MAX_L_UNIT  (MAX_L_VERT / 4)

#define DUMMY_CLAMP  789


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

static bool batch_sort;


//
// RGL_InitUnits
//
// Initialise the unit system.  Once-only call.
//
void RGL_InitUnits(void)
{
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


//
// RGL_StartUnits
//
// Starts a fresh batch of units.
//
// When 'sort_em' is true, the units will be sorted to keep
// texture changes to a minimum.  Otherwise, the batch is
// drawn in the same order as given.
//
void RGL_StartUnits(bool sort_em)
{
	cur_vert = cur_unit = 0;

	batch_sort = sort_em;

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

static inline void myActiveTexture(GLuint id)
{
#ifndef DREAMCAST
	if (GLEW_VERSION_1_3)
		glActiveTexture(id);
	else /* GLEW_ARB_multitexture */
		glActiveTextureARB(id);
#endif
}

static inline void myMultiTexCoord2f(GLuint id, GLfloat s, GLfloat t)
{
#ifndef DREAMCAST
	if (GLEW_VERSION_1_3)
		glMultiTexCoord2f(id, s, t);
	else /* GLEW_ARB_multitexture */
		glMultiTexCoord2fARB(id, s, t);
#endif
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

	SYS_ASSERT((blending & BL_CULL_BOTH) != BL_CULL_BOTH);

	// check we have enough space left
	if (cur_vert + max_vert > MAX_L_VERT || cur_unit >= MAX_L_UNIT)
	{
		RGL_DrawUnits();
	}

	unit = local_units + cur_unit;

	if (env1 == ENV_NONE) tex1 = 0;
	if (env2 == ENV_NONE) tex2 = 0;

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

	// adjust colors (for special effects)
	for (int i = 0; i < actual_vert; i++)
	{
		local_gl_vert_t *v = &local_verts[cur_vert + i];

		v->rgba[0] *= ren_red_mul;
		v->rgba[1] *= ren_grn_mul;
		v->rgba[2] *= ren_blu_mul;
	}

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

static void EnableCustomEnv(GLuint env, bool enable)
{
#ifndef DREAMCAST
	switch (env)
	{
		case ENV_SKIP_RGB:
			if (enable)
			{
				glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
				glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_REPLACE);
				glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_PREVIOUS);
			}
			else
			{
				/* no need to modify TEXTURE_ENV_MODE */
				glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
				glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE);
			}
			break;

		default:
			I_Error("INTERNAL ERROR: no such custom env: %08x\n", env);
	}
#endif
}

static inline void RGL_SendRawVector(const local_gl_vert_t *V)
{
#ifndef DREAMCAST
	if (r_colormaterial.d || ! r_colorlighting.d)
		glColor4fv(V->rgba);
	else
	{
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, V->rgba);
		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, V->rgba);
	}
#else
	glColor4fv(V->rgba);
#endif
	myMultiTexCoord2f(GL_TEXTURE0, V->texc[0].x, V->texc[0].y);
	myMultiTexCoord2f(GL_TEXTURE1, V->texc[1].x, V->texc[1].y);

	glNormal3f(V->normal.x, V->normal.y, V->normal.z);
#ifndef NO_EDGEFLAG
	glEdgeFlag(V->EDGE2);
#endif

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

	if (batch_sort)
	{
		std::sort(local_unit_map.begin(),
				  local_unit_map.begin() + cur_unit,
				  Compare_Unit_pred());
	}

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_BLEND);

	glAlphaFunc(GL_GREATER, 0);

	glPolygonOffset(0, 0);

#if 0
	float fog_col[4] = { 0, 0, 0.1, 0};
	glEnable(GL_FOG);
	glFogi(GL_FOG_MODE, GL_EXP);
	glFogf(GL_FOG_DENSITY, 0.001f);
	glFogfv(GL_FOG_COLOR, fog_col);
#endif

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

		if ((active_blending ^ unit->blending) & (BL_Masked | BL_Less))
		{
			if (unit->blending & BL_Less)
			{
				// glAlphaFunc is updated below, because the alpha
				// value can change from unit to unit while the
				// BL_Less flag remains set.
				glEnable(GL_ALPHA_TEST);
			}
			else if (unit->blending & BL_Masked)
			{
				glEnable(GL_ALPHA_TEST);
				glAlphaFunc(GL_GREATER, 0);
			}
			else
				glDisable(GL_ALPHA_TEST);
		}

		if ((active_blending ^ unit->blending) & (BL_Alpha | BL_Add))
		{
			if (unit->blending & BL_Add)
			{
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE);
			}
			else if (unit->blending & BL_Alpha)
			{
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			}
			else
				glDisable(GL_BLEND);
		}

		if ((active_blending ^ unit->blending) & BL_CULL_BOTH)
		{
			if (unit->blending & BL_CULL_BOTH)
			{
				glEnable(GL_CULL_FACE);
				glCullFace((unit->blending & BL_CullFront) ? GL_FRONT : GL_BACK);
			}
			else
				glDisable(GL_CULL_FACE);
		}

		if ((active_blending ^ unit->blending) & BL_NoZBuf)
		{
			glDepthMask((unit->blending & BL_NoZBuf) ? GL_FALSE : GL_TRUE);
		}

		active_blending = unit->blending;

		if (active_blending & BL_Less)
		{
			// NOTE: assumes alpha is constant over whole polygon
			float a = local_verts[unit->first].rgba[3];

			glAlphaFunc(GL_GREATER, a * 0.66f);
		}

		for (int t=1; t >= 0; t--)
		{
			myActiveTexture(GL_TEXTURE0 + t);

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
				if (active_env[t] >= CUSTOM_ENV_BEGIN &&
					active_env[t] <= CUSTOM_ENV_END)
				{
					EnableCustomEnv(active_env[t], false);
				}

				if (unit->env[t] >= CUSTOM_ENV_BEGIN &&
					unit->env[t] <= CUSTOM_ENV_END)
				{
					EnableCustomEnv(unit->env[t], true);
				}
				else if (unit->env[t] != ENV_NONE)
					glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, unit->env[t]);

				active_env[t] = unit->env[t];
			}
		}

		GLint old_clamp = DUMMY_CLAMP;

		if ((active_blending & BL_ClampY) && active_tex[0] != 0)
		{
			glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, &old_clamp);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
				r_dumbclamp.d ? GL_CLAMP : GL_CLAMP_TO_EDGE);
		}

		glBegin(unit->shape);

		for (int v_idx=0; v_idx < unit->count; v_idx++)
		{
			RGL_SendRawVector(local_verts + unit->first + v_idx);
		}

		glEnd();

		// restore the clamping mode
		if (old_clamp != DUMMY_CLAMP)
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, old_clamp);
	}

	// all done
	cur_vert = cur_unit = 0;

	glPolygonOffset(0, 0);

	for (int t=1; t >=0; t--)
	{
		myActiveTexture(GL_TEXTURE0 + t);

		if (active_env[t] >= CUSTOM_ENV_BEGIN &&
			active_env[t] <= CUSTOM_ENV_END)
		{
			EnableCustomEnv(active_env[t], false);
		}
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glDisable(GL_TEXTURE_2D);
	}

	glDepthMask(GL_TRUE);
	glCullFace(GL_BACK);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glAlphaFunc(GL_GREATER, 0);

	glDisable(GL_ALPHA_TEST);
	glDisable(GL_BLEND);
	glDisable(GL_CULL_FACE);
}

#ifdef polyQ
//POLYQUAD from early EDGE Versions (to test Shadows prototype?)

//----------------------------------------------------------------------------
//
//  RAW POLYQUAD CODE
//
//  Note: quads are always vertical rectangles, and currently the
//  bottom and top lines are always horizontal (i.e. never sloped).
//
//  Note 2: polygons are always horizontal.
//  

raw_polyquad_t *RGL_NewPolyQuad(int maxvert, bool quad)
{
	raw_polyquad_t *poly;

	poly = Z_ClearNew(raw_polyquad_t, 1);

	poly->verts = Z_New(vec3_t, maxvert);
	poly->num_verts = 0;
	poly->max_verts = maxvert;

	poly->quad = quad;

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
	int j;

	raw_polyquad_t *cur = poly;
	///    poly = poly->sisters;

	DEV_ASSERT2(cur->num_verts > 0);

	cur->min = cur->verts[0];
	cur->max = cur->verts[0];

	for (j=1; j < cur->num_verts; j++)
	{
		vec3_t *src = cur->verts + j;

		if (src->x < cur->min.x) cur->min.x = src->x;
		else if (src->x > cur->max.x) cur->max.x = src->x;

		if (src->y < cur->min.y) cur->min.y = src->y;
		else if (src->y > cur->max.y) cur->max.y = src->y;

		if (src->z < cur->min.z) cur->min.z = src->z;
		else if (src->z > cur->max.z) cur->max.z = src->z;
	}
}

static void RGL_DoSplitQuadVertSep(raw_polyquad_t *quad, int extras)
{
	int j;
	float h1, h2;
	float span_z;

	DEV_ASSERT2(extras >= 1);

	// Note: doesn't handle already split quads (i.e. num_verts > 4).
	DEV_ASSERT2(quad->num_verts == 4);

	// the original QUAD will end up being the top-most part, and we
	// create the newbies from the bottom upwards.  Hence final order is
	// top piece to bottom piece.
	//
	h1 = quad->min.z;
	span_z = quad->max.z - quad->min.z;

	for (j=0; j < extras; j++, h1 = h2)
	{
		raw_polyquad_t *N = RGL_NewPolyQuad(4, true);
		N->num_verts = 4;

		Z_MoveData(N->verts, quad->verts, vec3_t, 4);

		h2 = quad->min.z + span_z * (j+1) / (float)(extras + 1);

		N->verts[0].z = h1;  N->verts[1].z = h2;
		N->verts[2].z = h1;  N->verts[3].z = h2;

		// link it in
		RGL_BoundPolyQuad(N);

		N->sisters = quad->sisters;
		quad->sisters = N;
	}

	quad->verts[0].z = h1;
	quad->verts[2].z = h1;

	RGL_BoundPolyQuad(quad);
}

static void RGL_DoSplitQuadHorizSep(raw_polyquad_t *quad, int extras)
{
	int j;
	vec2_t p1, p2;
	vec2_t span;

	DEV_ASSERT2(extras >= 1);

	// Note: doesn't handle already split quads (i.e. num_verts > 4).
	DEV_ASSERT2(quad->num_verts == 4);

	// the original QUAD will end up being the right-most part, and we
	// create the newbies from the left.  Hence final order is right
	// piece to left piece.
	//
	p1.x = quad->verts[0].x;
	p1.y = quad->verts[0].y;

	span.x = quad->verts[2].x - p1.x;
	span.y = quad->verts[2].y - p1.y;

	for (j=0; j < extras; j++, p1 = p2)
	{
		raw_polyquad_t *N = RGL_NewPolyQuad(4, true);
		N->num_verts = 4;

		Z_MoveData(N->verts, quad->verts, vec3_t, 4);

		p2.x = quad->verts[0].x + span.x * (j+1) / (float)(extras + 1);
		p2.y = quad->verts[0].y + span.y * (j+1) / (float)(extras + 1);

		N->verts[0].x = p1.x;  N->verts[0].y = p1.y;
		N->verts[1].x = p1.x;  N->verts[1].y = p1.y;

		N->verts[2].x = p2.x;  N->verts[2].y = p2.y;
		N->verts[3].x = p2.x;  N->verts[3].y = p2.y;

		// link it in
		RGL_BoundPolyQuad(N);

		N->sisters = quad->sisters;
		quad->sisters = N;
	}

	quad->verts[0].x = p1.x; quad->verts[0].y = p1.y;
	quad->verts[1].x = p1.x; quad->verts[1].y = p1.y;

	RGL_BoundPolyQuad(quad);
}

static void RGL_DoSplitQuadHoriz(raw_polyquad_t *quad, int extras)
{
	int j;
	vec3_t p1, p2;
	vec2_t span;

	DEV_ASSERT2(extras >= 1);

	// Note: doesn't handle already split quads (i.e. num_verts > 4).
	DEV_ASSERT2(quad->num_verts == 4);

	p1 = quad->verts[0];
	p2 = quad->verts[3];

	// resize vertex array in the polyquad
	if (extras*2 + 4 > quad->max_verts)
	{
		Z_Resize(quad->verts, vec3_t, extras*2 + 4);
		quad->max_verts = extras*2 + 4;
	}

	quad->num_verts = extras*2 + 4;

	quad->verts[0] = p1;
	quad->verts[1] = p1;
	quad->verts[1].z = p2.z;
	quad->verts[extras*2 + 2] = p2;
	quad->verts[extras*2 + 3] = p2;
	quad->verts[extras*2 + 2].z = p1.z;

	span.x = p2.x - p1.x;
	span.y = p2.y - p1.y;

	for (j=0; j < extras; j++)
	{
		vec3_t *pair = quad->verts + (j+1) * 2;

		pair[0].x = p1.x + span.x * (j+1) / (float)(extras + 1);
		pair[0].y = p1.y + span.y * (j+1) / (float)(extras + 1);
		pair[0].z = p1.z;

		pair[1].x = pair[0].x;
		pair[1].y = pair[0].y;
		pair[1].z = p2.z;
	}

	// no need to recompute the bounds, they are still valid
}

static void RGL_DoSplitQuad(raw_polyquad_t *quad, int division,
							bool separate)
{
	float span_xy, span_z;

	raw_polyquad_t *orig = quad;

	// first pass: split vertically
	for (quad = orig; quad; )
	{
		raw_polyquad_t *cur = quad;
		quad = quad->sisters;

		span_z = cur->max.z - cur->min.z;

		if (span_z > division)
		{
			RGL_DoSplitQuadVertSep(cur, (int)(span_z / division));
		}
	}

	// second pass: split horizontally
	for (quad = orig; quad; )
	{
		raw_polyquad_t *cur = quad;
		quad = quad->sisters;

		span_xy = MAX(cur->max.x - cur->min.x, cur->max.y - cur->min.y);

		if (span_xy > division)
		{
			if (separate)
				RGL_DoSplitQuadHorizSep(cur, (int)(span_xy / division));
			else
				RGL_DoSplitQuadHoriz(cur, (int)(span_xy / division));
		}
	}
}


//----------------------------------------------------------------------------


static INLINE void AddPolyDynPoint(raw_polyquad_t *poly,
								   float x, float y, float z)
{
	DEV_ASSERT2(poly);
	DEV_ASSERT2(poly->num_verts <= poly->max_verts);

	if (poly->num_verts == poly->max_verts)
	{
		poly->max_verts += 8;
		Z_Resize(poly->verts, vec3_t, poly->max_verts);
	}

	DEV_ASSERT2(poly->num_verts < poly->max_verts);

	PQ_ADD_VERT(poly, x, y ,z);
}

static INLINE void AddPolyVertIntercept(raw_polyquad_t *poly,
										vec3_t *P, vec3_t *S, float y)
{
	float frac;

	DEV_ASSERT2(P->y != S->y);
	DEV_ASSERT2(MIN(P->y, S->y)-1 <= y && y <= MAX(P->y, S->y)+1);

	frac = (y - P->y) / (S->y - P->y);

	AddPolyDynPoint(poly, P->x + (S->x - P->x) * frac, y,
		P->z + (S->z - P->z) * frac);
}

static INLINE void AddPolyHorizIntercept(raw_polyquad_t *poly,
										 vec3_t *P, vec3_t *S, float x)
{
	float frac;

	DEV_ASSERT2(P->x != S->x);
	DEV_ASSERT2(MIN(P->x, S->x)-1 <= x && x <= MAX(P->x, S->x)+1);

	frac = (x - P->x) / (S->x - P->x);

	AddPolyDynPoint(poly, x, P->y + (S->y - P->y) * frac,
		P->z + (S->z - P->z) * frac);
}

//
//
// RGL_DoSplitPolyVert
// 
// ALGORITHM:
//
//   (a) Traverse the points of the polygon in normal (clockwise)
//       order.  Let P be the current point, and S be the successor
//       point (or the first point if P is the last).
// 
//   (b) Add point P to new polygon.
//
//   (c) If P is lower than S, traverse the set of extra lines
//       upwards.  If P is higher than S, traverse the set of extra
//       lines downwards.  If P same height as S, do nothing (continue
//       main loop).
// 
//   (d) Check each extra line (height Y) for intercept, and if it
//       does intercept P->S then add the new point to the new
//       polygon.  Ignore exact matches (P.y == Y or S.y == Y).
// 
static void RGL_DoSplitPolyVert(raw_polyquad_t *poly, int extras)
{
	int j, k;
	float y;
	float span_y = poly->max.y - poly->min.y;
	float min_y = poly->min.y;

	vec3_t *orig_verts;
	int orig_num;

	int start, end, step;

	// copy original vertices
	orig_num = poly->num_verts;
	DEV_ASSERT2(orig_num >= 3);

	orig_verts = Z_New(vec3_t, orig_num);
	Z_MoveData(orig_verts, poly->verts, vec3_t, orig_num);

	// clear current polygon
	poly->num_verts = 0;

	for (k=0; k < orig_num; k++)
	{
		vec3_t P, S;
		bool down;

		P = orig_verts[k];
		S = orig_verts[(k+1) % orig_num];

		down = (P.y > S.y);

		// always add current point
		AddPolyDynPoint(poly, P.x, P.y, P.z);

		// handle same Y coords
		if (fabs(P.y - S.y) < 0.01)
			continue;

		if (down)
			start = extras-1, end = -1, step = -1;
		else
			start = 0, end = extras, step = +1;

		for (j=start; j != end; j += step)
		{
			y = min_y + span_y * (j+1) / (float)(extras+1);

			// no intercept ?
			if (y <= (down ? S.y : P.y) + 0.01f || 
				y >= (down ? P.y : S.y) - 0.01f)
				continue;

			AddPolyVertIntercept(poly, &P, &S, y);
		}
	}

	// no need to recompute bbox -- still valid

	Z_Free(orig_verts);
}

static void RGL_DoSplitPolyHoriz(raw_polyquad_t *poly, int extras)
{
	int j, k;
	float x;
	float span_x= poly->max.x - poly->min.x;
	float min_x= poly->min.x;

	vec3_t *orig_verts;
	int orig_num;

	int start, end, step;

	// copy original vertices
	orig_num = poly->num_verts;
	DEV_ASSERT2(orig_num >= 3);

	orig_verts = Z_New(vec3_t, orig_num);
	Z_MoveData(orig_verts, poly->verts, vec3_t, orig_num);

	// clear current polygon
	poly->num_verts = 0;

	for (k=0; k < orig_num; k++)
	{
		vec3_t P, S;
		bool left;

		P = orig_verts[k];
		S = orig_verts[(k+1) % orig_num];

		left = (P.x > S.x);

		// always add current point
		AddPolyDynPoint(poly, P.x, P.y, P.z);

		// handle same X coords
		if (fabs(P.x - S.x) < 0.01)
			continue;

		if (left)
			start = extras-1, end = -1, step = -1;
		else
			start = 0, end = extras, step = +1;

		for (j=start; j != end; j += step)
		{
			x = min_x + span_x * (j+1) / (float)(extras+1);

			// no intercept ?
			if (x <= (left ? S.x : P.x) + 0.01f || 
				x >= (left ? P.x : S.x) - 0.01f)
				continue;

			AddPolyHorizIntercept(poly, &P, &S, x);
		}
	}

	// no need to recompute bbox -- still valid

	Z_Free(orig_verts);
}

//
// RGL_DoSplitPolyVertSep
//
// ALGORITHM:
//
//   (a) Let the vertical range be Y1..Y2 which will contain the
//       new piece of the original polygon.
//       
//   (b) Traverse the points of the polygon in normal (clockwise)
//       order.  Let P be the current point, and S be the successor
//       point (or the first point if P is the last).
// 
//   (c) If P is lower than S, let CY = Y1 and DY = Y2, otherwise
//       let CY = Y2 and DY = Y1.
//
//   (d) Detect whether points P and S are: above the range (U),
//       on the top border of range (Y2), inside the range (M), on the
//       bottom border (Y1), or below the range (L).
//
//   (e) The following table shows what to do:
//
//          P    S   |  Action
//       ------------+------------------------------------------
//          L   L,Y1 |  nothing
//          L   M,Y2 |  add intercept (P->S on CY)
//          L    U   |  double intercept (P->S on CY then DY)
//       ------------+------------------------------------------
//          Y1   L   |  add P
//         M,Y2  L   |  add P then intercept (P->S on DY)
//       ------------+------------------------------------------
//          M*   M*  |  add P
//       ------------+------------------------------------------
//         Y1,M  U   |  add P then intercept (P->S on DY)
//          Y2   U   |  add P
//       ------------+------------------------------------------
//          U    L   |  double intercept (P->S on CY then DY)
//          U   Y1,M |  add intercept (P->S on CY)
//          U   Y2,U |  nothing
//       ------------+------------------------------------------
//
//       Where "M*" means M or Y1 or Y2.
//       
static void RGL_DoSplitPolyVertSep(raw_polyquad_t *poly, int extras)
{
	int j, k;
	float y1, y2;
	float span_y = poly->max.y - poly->min.y;
	float min_y = poly->min.y;

	raw_polyquad_t *N;
	vec3_t *orig_verts;
	int orig_num;

	// copy original vertices
	orig_num = poly->num_verts;
	DEV_ASSERT2(orig_num >= 3);

	orig_verts = Z_New(vec3_t, orig_num);
	Z_MoveData(orig_verts, poly->verts, vec3_t, orig_num);

	// clear current polygon
	poly->num_verts = 0;

	y1 = poly->min.y;

	for (j=0; j < extras+1; j++, y1 = y2)
	{
		y2 = min_y + span_y * (j+1) / (float)(extras+1);

		if (j == 0)
		{
			N = poly;
		}
		else
		{
			N = RGL_NewPolyQuad(8, false);

			// link it in
			N->sisters = poly->sisters;
			poly->sisters = N;
		}

		for (k=0; k < orig_num; k++)
		{
			vec3_t P, S;
			int Ppos, Spos;
			int Pedg, Sedg;
			float cy, dy;

			P = orig_verts[k];
			S = orig_verts[(k+1) % orig_num];

			Ppos = (P.y < y1) ? -1 : (P.y > y2) ? +1 : 0;
			Spos = (S.y < y1) ? -1 : (S.y > y2) ? +1 : 0;

			// handle boundary conditions
			Pedg = (fabs(P.y-y1)<0.01) ? -1 : (fabs(P.y-y2)<0.01) ? +1 : 0;
			Sedg = (fabs(S.y-y1)<0.01) ? -1 : (fabs(S.y-y2)<0.01) ? +1 : 0;

			if (Pedg != 0)
				Ppos = 0;

			if (Sedg != 0)
				Spos = 0;

			if (P.y < S.y)
				cy = y1, dy = y2;
			else
				cy = y2, dy = y1;

			// always add current point if inside the range
			if (Ppos == 0)
				AddPolyDynPoint(N, P.x, P.y, P.z);

			// handle the do nothing cases
			if (Ppos == Spos)
				continue;

			if ((Ppos < 0 && Sedg < 0) || (Ppos > 0 && Sedg > 0))
				continue;

			if ((Spos < 0 && Pedg < 0) || (Spos > 0 && Pedg > 0))
				continue;

			// handle the "branching inside->outside" case
			if (Ppos == 0)
			{
				AddPolyVertIntercept(N, &P, &S, dy);
				continue;
			}

			// OK, we know the P->S line must cross CY
			AddPolyVertIntercept(N, &P, &S, cy);

			// check for double intercept
			if (Ppos * Spos < 0)
			{
				AddPolyVertIntercept(N, &P, &S, dy);
			}
		}

		RGL_BoundPolyQuad(N);
	}

	Z_Free(orig_verts);
}

static void RGL_DoSplitPolyHorizSep(raw_polyquad_t *poly, int extras)
{
	int j, k;
	float x1, x2;
	float span_x = poly->max.x - poly->min.x;
	float min_x = poly->min.x;

	raw_polyquad_t *N;
	vec3_t *orig_verts;
	int orig_num;

	// copy original vertices
	orig_num = poly->num_verts;
	DEV_ASSERT2(orig_num >= 3);

	orig_verts = Z_New(vec3_t, orig_num);
	Z_MoveData(orig_verts, poly->verts, vec3_t, orig_num);

	// clear current polygon
	poly->num_verts = 0;

	x1 = poly->min.x;

	for (j=0; j < extras+1; j++, x1 = x2)
	{
		x2 = min_x + span_x * (j+1) / (float)(extras+1);

		if (j == 0)
		{
			N = poly;
		}
		else
		{
			N = RGL_NewPolyQuad(8, false);

			// link it in
			N->sisters = poly->sisters;
			poly->sisters = N;
		}

		for (k=0; k < orig_num; k++)
		{
			vec3_t P, S;
			int Ppos, Spos;
			int Pedg, Sedg;
			float cx, dx;

			P = orig_verts[k];
			S = orig_verts[(k+1) % orig_num];

			Ppos = (P.x < x1) ? -1 : (P.x > x2) ? +1 : 0;
			Spos = (S.x < x1) ? -1 : (S.x > x2) ? +1 : 0;

			// handle boundary conditions
			Pedg = (fabs(P.x-x1)<0.01) ? -1 : (fabs(P.x-x2)<0.01) ? +1 : 0;
			Sedg = (fabs(S.x-x1)<0.01) ? -1 : (fabs(S.x-x2)<0.01) ? +1 : 0;

			if (Pedg != 0)
				Ppos = 0;

			if (Sedg != 0)
				Spos = 0;

			if (P.x < S.x)
				cx = x1, dx = x2;
			else
				cx = x2, dx = x1;

			// always add current point if inside the range
			if (Ppos == 0)
				AddPolyDynPoint(N, P.x, P.y, P.z);

			// handle the do nothing cases
			if (Ppos == Spos)
				continue;

			if ((Ppos < 0 && Sedg < 0) || (Ppos > 0 && Sedg > 0))
				continue;

			if ((Spos < 0 && Pedg < 0) || (Spos > 0 && Pedg > 0))
				continue;

			// handle the "branching inside->outside" case
			if (Ppos == 0)
			{
				AddPolyHorizIntercept(N, &P, &S, dx);
				continue;
			}

			// OK, we know the P->S line must cross CX
			AddPolyHorizIntercept(N, &P, &S, cx);

			// check for double intercept
			if (Ppos * Spos < 0)
			{
				AddPolyHorizIntercept(N, &P, &S, dx);
			}
		}

		RGL_BoundPolyQuad(N);
	}

	Z_Free(orig_verts);
}

static void RGL_DoSplitPolyListVert(raw_polyquad_t *poly,
									int extras, bool separate)
{
	while (poly)
	{
		raw_polyquad_t *cur = poly;
		poly = poly->sisters;

		if (separate)
			RGL_DoSplitPolyVertSep(cur, extras);
		else
			RGL_DoSplitPolyVert(cur, extras);
	}
}

static void RGL_DoSplitPolyListHoriz(raw_polyquad_t *poly,
									 int extras, bool separate)
{
	while (poly)
	{
		raw_polyquad_t *cur = poly;
		poly = poly->sisters;

		if (separate)
			RGL_DoSplitPolyHorizSep(cur, extras);
		else
			RGL_DoSplitPolyHoriz(cur, extras);
	}
}

static void RGL_DoSplitPolygon(raw_polyquad_t *poly, int division,
							   bool separate)
{
	float span_x = poly->max.x - poly->min.x;
	float span_y = poly->max.y - poly->min.y;

	DEV_ASSERT2(division > 0);

	if (span_x > division && span_y > division)
	{
		// split the shortest axis before longest one
		if (span_x < span_y)
		{
			RGL_DoSplitPolyListHoriz(poly, (int)(span_x / division), true);
			span_x = 0;
		}
		else
		{
			RGL_DoSplitPolyListVert(poly, (int)(span_y / division), true);
			span_y = 0;
		}
	}

	if (span_x > division)
	{
		RGL_DoSplitPolyListHoriz(poly, (int)(span_x / division), separate);
	}

	if (span_y > division)
	{
		RGL_DoSplitPolyListVert(poly, (int)(span_y / division), separate);
	}
}


//----------------------------------------------------------------------------


void RGL_SplitPolyQuad(raw_polyquad_t *poly, int division,
					   bool separate)
{
	if (poly->quad)
		RGL_DoSplitQuad(poly, division, separate);
	else
		RGL_DoSplitPolygon(poly, division, separate);
}

void RGL_SplitPolyQuadLOD(raw_polyquad_t *poly, int max_lod, int base_div)
{
	raw_polyquad_t *trav, *tail;
	int lod;

	// first step: make sure nothing is larger than 1024

	RGL_SplitPolyQuad(poly, 1024, true);

	// second step: compute LOD of each bit

	for (trav = poly; trav; )
	{
		raw_polyquad_t *cur = trav;
		trav = trav->sisters;

		lod = R2_GetBBoxLOD(cur->min.x, cur->min.y, cur->min.z,
			cur->max.x, cur->max.y, cur->max.z);

		lod = MAX(lod, max_lod) * base_div;

		if (lod > (1024 * 3/4))
			continue;

		// unlink remaining pieces, putting them back in after splitting
		// this piece.
		//
		cur->sisters = NULL;

		RGL_SplitPolyQuad(cur, lod, (cur->quad) ? false : true);

		for (tail = cur; tail->sisters; tail = tail->sisters)
		{ /* nothing here */ }

		DEV_ASSERT2(tail);
		tail->sisters = trav;
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

		DEV_ASSERT2(cur->num_verts > 0);
		DEV_ASSERT2(cur->num_verts <= cur->max_verts);

		vert = RGL_BeginUnit(cur->quad ? GL_QUAD_STRIP : GL_POLYGON,
			cur->num_verts, tex_id, tex2_id, pass, blending);

		for (j=0; j < cur->num_verts; j++)
		{
			(* CoordFunc)(cur->verts + j, vert + j, data);
		}

		RGL_EndUnit(j);
	}
}

//END QUAD_POLY??

#if 0  // DEBUG ONLY
static void RGL_DumpPolyQuad(raw_polyquad_t *poly, bool single)
{
	int j;

	L_WriteDebug("DUMP POLY %p quad=%d num=%d max=%d\n", poly, 
		poly->quad, poly->num_verts, poly->max_verts);

	while (poly)
	{
		raw_polyquad_t *cur = poly;
		poly = single ? NULL : poly->sisters;

		if (! single)
			L_WriteDebug("--CUR SISTER: %p\n", cur);

#if 0
		L_WriteDebug("  BBOX: (%1.0f,%1.0f,%1.0f) -> (%1.0f,%1.0f,%1.0f)\n",
			cur->min.x, cur->min.y, cur->min.z, 
			cur->max.x, cur->max.y, cur->max.z);
#endif

		if (cur->quad)
		{
			for (j=0; j < cur->num_verts; j += 2)
			{
				L_WriteDebug("  SIDE: (%1.0f,%1.0f,%1.0f) -> (%1.0f,%1.0f,%1.0f)\n",
					cur->verts[j].x, cur->verts[j].y, cur->verts[j].z,
					cur->verts[j+1].x, cur->verts[j+1].y, cur->verts[j+1].z);
			}
		}
		else
		{
			for (j=0; j < cur->num_verts; j += 1)
			{
				L_WriteDebug("  POINT: (%1.0f,%1.0f,%1.0f)\n",
					cur->verts[j].x, cur->verts[j].y, cur->verts[j].z);
			}
		}
	}

	L_WriteDebug("\n");
}

static raw_polyquad_t * CreateTestQuad(float x1, float y1,
									   float z1, float x2, float y2, float z2)
{
	raw_polyquad_t *poly = RGL_NewPolyQuad(4, true);

	PQ_ADD_VERT(poly, x1, y1, z1);
	PQ_ADD_VERT(poly, x1, y1, z2);
	PQ_ADD_VERT(poly, x2, y2, z1);
	PQ_ADD_VERT(poly, x2, y2, z2);

	RGL_BoundPolyQuad(poly);

	return poly;
}

static raw_polyquad_t * CreateTestPolygon1(void)
{
	raw_polyquad_t *poly = RGL_NewPolyQuad(10, false);

	PQ_ADD_VERT(poly, 200, 200,  88);
	PQ_ADD_VERT(poly, 500, 2600, 88);
	PQ_ADD_VERT(poly, 550, 2600, 88);
	PQ_ADD_VERT(poly, 750, 1400, 88);
	//  PQ_ADD_VERT(poly, 800, 1000, 88);
	PQ_ADD_VERT(poly, 800, 800,  88);

	RGL_BoundPolyQuad(poly);

	return poly;
}

void RGL_TestPolyQuads(void)
{
	raw_polyquad_t *test;

	L_WriteDebug("=== QUAD TEST ===\n");
	test = CreateTestQuad(300, 400, 150, 3700, 2200, 1750);
	RGL_DumpPolyQuad(test, false);

	L_WriteDebug("Splitting to 1000 division...\n");
	RGL_SplitPolyQuad(test, 1000, true);
	RGL_DumpPolyQuad(test, false);

	L_WriteDebug("Further splitting to 128 division...\n");
	RGL_SplitPolyQuad(test, 128, false);
	RGL_DumpPolyQuad(test, false);

	L_WriteDebug("=== POLYGON TEST ===\n");
	test = CreateTestPolygon1();
	RGL_DumpPolyQuad(test, false);

	L_WriteDebug("Splitting to 1000 division...\n");
	RGL_SplitPolyQuad(test, 1000, false);
	RGL_DumpPolyQuad(test, false);
}
#endif  // DEBUG
#endif

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
