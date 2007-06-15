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

	SYS_ASSERT(cur->num_verts > 0);

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

	SYS_ASSERT(extras >= 1);

	// Note: doesn't handle already split quads (i.e. num_verts > 4).
	SYS_ASSERT(quad->num_verts == 4);

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

	SYS_ASSERT(extras >= 1);

	// Note: doesn't handle already split quads (i.e. num_verts > 4).
	SYS_ASSERT(quad->num_verts == 4);

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

	SYS_ASSERT(extras >= 1);

	// Note: doesn't handle already split quads (i.e. num_verts > 4).
	SYS_ASSERT(quad->num_verts == 4);

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
	SYS_ASSERT(poly);
	SYS_ASSERT(poly->num_verts <= poly->max_verts);

	if (poly->num_verts == poly->max_verts)
	{
		poly->max_verts += 8;
		Z_Resize(poly->verts, vec3_t, poly->max_verts);
	}

	SYS_ASSERT(poly->num_verts < poly->max_verts);

	PQ_ADD_VERT(poly, x, y ,z);
}

static INLINE void AddPolyVertIntercept(raw_polyquad_t *poly,
										vec3_t *P, vec3_t *S, float y)
{
	float frac;

	SYS_ASSERT(P->y != S->y);
	SYS_ASSERT(MIN(P->y, S->y)-1 <= y && y <= MAX(P->y, S->y)+1);

	frac = (y - P->y) / (S->y - P->y);

	AddPolyDynPoint(poly, P->x + (S->x - P->x) * frac, y,
		P->z + (S->z - P->z) * frac);
}

static INLINE void AddPolyHorizIntercept(raw_polyquad_t *poly,
										 vec3_t *P, vec3_t *S, float x)
{
	float frac;

	SYS_ASSERT(P->x != S->x);
	SYS_ASSERT(MIN(P->x, S->x)-1 <= x && x <= MAX(P->x, S->x)+1);

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
	SYS_ASSERT(orig_num >= 3);

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
	SYS_ASSERT(orig_num >= 3);

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
	SYS_ASSERT(orig_num >= 3);

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
	SYS_ASSERT(orig_num >= 3);

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

	SYS_ASSERT(division > 0);

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

		SYS_ASSERT(tail);
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

		SYS_ASSERT(cur->num_verts > 0);
		SYS_ASSERT(cur->num_verts <= cur->max_verts);

		vert = RGL_BeginUnit(cur->quad ? GL_QUAD_STRIP : GL_POLYGON,
			cur->num_verts, tex_id, tex2_id, pass, blending);

		for (j=0; j < cur->num_verts; j++)
		{
			(* CoordFunc)(cur->verts + j, vert + j, data);
		}

		RGL_EndUnit(j);
	}
}


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

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
