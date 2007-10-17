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

#include "epi/image_data.h"

#include "con_cvar.h"
#include "m_argv.h"
#include "r_gldefs.h"
#include "r_units.h"
#include "z_zone.h"

#include "r_misc.h"
#include "r_image.h" //!!!
#include "r_texgl.h" //!!!
#include "r_shader.h" //!!!

bool use_lighting = true;
bool use_color_material = true;

bool dumb_sky     = false;
bool dumb_multi   = false;
bool dumb_combine = false;
bool dumb_clamp   = false;


///---// -AJA- FIXME
///---#ifndef GL_CLAMP_TO_EDGE
///---#define GL_CLAMP_TO_EDGE  0x812F
///---#endif


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

static bool batch_sort;


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

#if 0  // NOT CURRENTLY USED
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
#endif



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
	if (GLEW_VERSION_1_3)
		glActiveTexture(id);
	else /* GLEW_ARB_multitexture */
		glActiveTextureARB(id);
}

static inline void myMultiTexCoord2f(GLuint id, GLfloat s, GLfloat t)
{
	if (GLEW_VERSION_1_3)
		glMultiTexCoord2f(id, s, t);
	else /* GLEW_ARB_multitexture */
		glMultiTexCoord2fARB(id, s, t);
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
}

static inline void RGL_SendRawVector(const local_gl_vert_t *V)
{
	if (use_color_material || ! use_lighting)
		glColor4fv(V->rgba);
	else
	{
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, V->rgba);
		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, V->rgba);
	}

	myMultiTexCoord2f(GL_TEXTURE0, V->texc[0].x, V->texc[0].y);
	myMultiTexCoord2f(GL_TEXTURE1, V->texc[1].x, V->texc[1].y);

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

	if (batch_sort)
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

		GLint old_clamp;

		if (active_blending & BL_ClampY)
		{
			glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, &old_clamp);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
				dumb_clamp ? GL_CLAMP_TO_EDGE : GL_CLAMP);
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
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glDisable(GL_ALPHA_TEST);
	glDisable(GL_BLEND);
	glDisable(GL_CULL_FACE);
}


//============================================================================


static GLuint fade_tex[2];

static const region_properties_t *cmap_props;

static int cmap_lit_Nom;


static inline void TexCoord_Fader(local_gl_vert_t *v, int t,
		const vec3_t *lit_pos)
{
	// distance from viewplane: (point - camera) . viewvec

	float dx = (lit_pos->x - viewx) * viewforward.x;
	float dy = (lit_pos->y - viewy) * viewforward.y;
	float dz = (lit_pos->z - viewz) * viewforward.z;

	v->texc[t].x = (dx + dy + dz) / 1600.0;

	int lt_ay = cmap_lit_Nom / 4;

	v->texc[t].y = (lt_ay + 0.5) / 64.0;
}


void R_ColmapPipe_SetProps(const struct region_properties_s *props)
{
	cmap_props = props;

	cmap_lit_Nom = cmap_props->lightlevel + ren_extralight;
	cmap_lit_Nom = CLAMP(cmap_lit_Nom, 0, 255);

	// FIXME !!!!! fade_tex from props->colourmap
}

void R_ColmapPipe_AdjustLight(int adjust)
{
	cmap_lit_Nom = cmap_props->lightlevel + adjust + ren_extralight;
	cmap_lit_Nom = CLAMP(cmap_lit_Nom, 0, 255);
}


static abstract_shader_c *cmap_shader;

extern abstract_shader_c *MakeColormapShader(void);

extern GLuint MakeColormapTexture( int mode );


static inline void Pipeline_Colormap(int& group,
	GLuint shape, int num_vert,
	GLuint tex, float alpha, int blending, int flags,
	void *func_data, pipeline_coord_func_t func)
{
	if (! cmap_shader)
		cmap_shader = MakeColormapShader();

	cmap_shader->WorldMix(shape, num_vert, tex,
			alpha, group, blending, func_data,
			(shader_coord_func_t) func);
	group++;

	return;


	/* FIRST PASS : draw the colormapped primitive */

	local_gl_vert_t *glvert;

	bool simple_cmap = true;

	if (true)
	{
	if (fade_tex[0] == 0)
		fade_tex[0] = MakeColormapTexture(0); // simple_cmap ? 0 : 1);
	
		glvert = RGL_BeginUnit(shape, num_vert, GL_MODULATE, tex,
				(simple_cmap || dumb_multi) ? GL_MODULATE : GL_DECAL,
				fade_tex[0], group, blending);
		group++;

		for (int v_idx=0; v_idx < num_vert; v_idx++)
		{
			local_gl_vert_t *dest = glvert + v_idx;

			dest->rgba[3] = alpha;

			vec3_t lit_pos;

			(*func)(func_data, v_idx, &dest->pos, dest->rgba,
					&dest->texc[0], &dest->normal, &lit_pos);

			TexCoord_Fader(dest, 1, &lit_pos);
		}

		RGL_EndUnit(num_vert);
	}

	// FOR OLD CARDS: additive pass (yuck!)

	if (dumb_multi && ! simple_cmap)
	{
		if (dumb_combine && (blending & (BL_Masked | BL_Alpha | BL_Add)))
		{
			// Ouch : for very old (dumb) cards, anything with holes or 
			//        translucent parts is going to be fucked.
			return;
		}

		if (fade_tex[1] == 0)
			fade_tex[1] = MakeColormapTexture(2);

		blending |= BL_Add;

		glvert = RGL_BeginUnit(shape, num_vert, GL_MODULATE, fade_tex[1],
					0, 0, group, blending);
		group++;

		for (int v_idx=0; v_idx < num_vert; v_idx++)
		{
			local_gl_vert_t *dest = glvert + v_idx;

			vec3_t lit_pos;

			(*func)(func_data, v_idx, &dest->pos, dest->rgba,
					&dest->texc[1], &dest->normal, &lit_pos);

			dest->rgba[0] = 1.0;
			dest->rgba[1] = 1.0;
			dest->rgba[2] = 1.0;
			dest->rgba[3] = alpha;

			TexCoord_Fader(dest, 0, &lit_pos);
		}

		RGL_EndUnit(num_vert);
	}
}


//------------------------------------------------------------------------

static const mobj_t *glow_floor;

static GLuint glow_tex[3];

static float glow_h[2] = { 0, 128 };


void R_GlowPipe_SetFloor(const struct mobj_s *glow, float h)
{
	glow_floor = glow;
}

void R_GlowPipe_SetCeiling(const struct mobj_s *glow)
{
}

void R_GlowPipe_SetWall(const struct mobj_s *glow)
{
}

static GLuint MakeGlowTexture(void) // test
{
	epi::image_data_c img(16, 64, 3);

	for (int x=0; x < img.width;  x++)
	for (int y=0; y < img.height; y++)
	{
		u8_t *pix = img.PixelAt(x, y);

		float ity = pow(1.0 - y/63.0, 2.5);

		pix[0] = int(255 * ity);
		pix[1] = pix[0];
		pix[2] = pix[1];
	}

	return R_UploadTexture(&img, NULL, UPL_Clamp | UPL_Smooth);
}

#if 0
static inline void TexCoord_WallGlow(local_gl_vert_t *v, int t,
		float x1, float y1, float nx, float ny)
{
	float dist = (v->x - x1) * nx + (v->y - y1) * ny;

	v->s[t] = 0.5;
	v->t[t] = dist / 192.0;
}
#endif


static inline void Pipeline_Glows(int& group,
	GLuint shape, int num_vert,
	GLuint tex, float alpha, int blending, int flags,
	void *func_data, pipeline_coord_func_t func)
{
	/* SECOND PASS : sector glows */

  	if (flags & PIPEF_NoLight)
  		return;

return; //!!!!!!
	local_gl_vert_t *glvert;

	blending &= ~BL_Alpha;
	blending |=  BL_Add;

	for (int p=0; p < 2; p++)
	{
		/// if (! glow_obj[p]) continue;

		//!!!!! FIXME: check if light too far away

		if (glow_tex[p] == 0)
			glow_tex[p] = MakeGlowTexture();

		glvert = RGL_BeginUnit(shape, num_vert, GL_MODULATE, tex,
					GL_MODULATE, glow_tex[p], group, blending);
		group++;

		for (int v_idx=0; v_idx < num_vert; v_idx++)
		{
			local_gl_vert_t *dest = glvert + v_idx;

			dest->rgba[3] = alpha;

			vec3_t lit_pos;

			(*func)(func_data, v_idx, &dest->pos, dest->rgba,
					&dest->texc[0], &dest->normal, &lit_pos);

			dest->rgba[0] = 1.00;
			dest->rgba[1] = 0.66;
			dest->rgba[2] = 0.33;

			dest->texc[1].x = 0.5;

			if (p == 0)
				dest->texc[1].y = (lit_pos.z - glow_h[0]) / 64.0;
			else
				dest->texc[1].y = (glow_h[1] - lit_pos.z) / 64.0;
		}

		RGL_EndUnit(num_vert);
	}

	if (true)
	{
		/* TODO: wall mode */
	}
}


//------------------------------------------------------------------------

static inline void Pipeline_Shadow(int& group,
	GLuint shape, int num_vert,
	GLuint tex, float alpha, int blending, int flags,
	void *func_data, pipeline_coord_func_t func)
{
	/* THIRD PASS : shadows */

//	if (! (flags & PIPEF_Shadows))
//		return;
}


//------------------------------------------------------------------------

static const drawthing_t *dlight_list = NULL;

static GLuint dlight_tex = 0;  // temp hack shit


void R_LightPipe_SetList(const struct drawthing_s *list)
{
	dlight_list = list;
}

static inline void Pipeline_DLights(int& group,
	GLuint shape, int num_vert,
	GLuint tex, float alpha, int blending, int flags,
	void *func_data, pipeline_coord_func_t func)
{
	/* FOURTH PASS : dynamic lighting */

	if (flags & PIPEF_NoLight)
		return;

	local_gl_vert_t *glvert;

	blending &= ~BL_Alpha;
	blending |=  BL_Add;

	for (const drawthing_t *DT = dlight_list; DT; DT = DT->next)
	{
		const mobj_t *mo = DT->mo;
	
		//!!!!! FIXME: if (dist_to_light > DL->info->radius) continue;

		SYS_ASSERT(mo->dlight.shader);

		mo->dlight.shader->WorldMix(shape, num_vert, tex,
				alpha, group, blending, func_data,
				(shader_coord_func_t) func);
		group++;
		
///---		GLuint DL_tex = W_ImageCache(mo->dlight[0].image);
///---
///---		glvert = RGL_BeginUnit(shape, num_vert, 0,0, //!!! GL_MODULATE, tex,
///---					GL_MODULATE, DL_tex, group, blending);
///---		group++;
///---
///---		for (int v_idx=0; v_idx < num_vert; v_idx++)
///---		{
///---			local_gl_vert_t *dest = glvert + v_idx;
///---
///---			dest->rgba[3] = alpha;
///---
///---			vec3_t lit_pos;
///---
///---			(*func)(func_data, v_idx, &dest->pos, dest->rgba,
///---					&dest->texc[0], &dest->normal, &lit_pos);
///---
///---			dest->rgba[2] = 1.00;
///---			dest->rgba[0] = 0.66;
///---			dest->rgba[1] = 0.33;
///---
///---			TexCoord_DLight(mo, &lit_pos, &dest->normal,
///---					        dest->rgba, &dest->texc[1]);
///---		}
///---
///---		RGL_EndUnit(num_vert);
	}
}


void R_RunPipeline(GLuint shape, int num_vert,
		           GLuint tex, float alpha, int blending, int flags,
				   void *func_data, pipeline_coord_func_t func)
{
	int group = 0;

	/* TODO: flat-shaded lighting : Pipeline_FlatShade */

	Pipeline_Colormap(group, shape, num_vert,
		tex, alpha, blending, flags, func_data, func);

//	Pipeline_Glows(group, shape, num_vert,
//		tex, alpha, blending, flags, func_data, func);
//
//	Pipeline_Shadow(group, shape, num_vert,
//		tex, alpha, blending, flags, func_data, func);

	Pipeline_DLights(group, shape, num_vert,
		tex, alpha, blending, flags, func_data, func);
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
