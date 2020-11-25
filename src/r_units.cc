//----------------------------------------------------------------------------
//  EDGE OpenGL Rendering (Unit batching)
//----------------------------------------------------------------------------
//
//  Copyright (c) 1999-2018  The EDGE Team.
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

#include "system/i_defs.h"
#include "system/i_defs_gl.h"
#include "system/i_sdlinc.h"


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
#include "r_bumpmap.h"
#include "r_colormap.h"

// define to enable light color and fog per sector code
#define USE_FOG

#ifdef USE_FOG
extern int light_color;
extern int fade_color;
#endif

DEF_CVAR(r_colorlighting, int, "", 1);
DEF_CVAR(r_colormaterial, int, "", 1);

DEF_CVAR(r_dumbsky, int, "", 0);
DEF_CVAR(r_dumbmulti, int, "", 0);
DEF_CVAR(r_dumbcombine, int, "", 0);
DEF_CVAR(r_dumbclamp, int, "", 0);

DEF_CVAR(r_anisotropy, int, "c", 0);

DEF_CVAR(r_gl3_path, int, "c", 0);

static bump_map_shader bmap_shader;

//XXX
/*
const image_c* tmp_image_normal;
const image_c* tmp_image_specular;
static bool tmp_init=false;
*/
static int bmap_light_count = 0;


//XXX ~CA
//"4096" was the previous default, changed for MD5 model support!
#define MAX_L_VERT  16384  


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

	// normal and specular textures (0 if unused)
	GLuint tex_normal;
	GLuint tex_specular;

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

static std::vector<local_gl_unit_t*> local_unit_map;

static int cur_vert;
static int cur_unit;

static bool batch_sort;

bool RGL_GL3Enabled()
{
	return (r_gl3_path && bmap_shader.supported());
}

void RGL_SetAmbientLight(short r, short g, short b)
{	//rgb 0-255
	bmap_shader.lightParamAmbient((float)r / 255.0f, (float)g / 255.0f, (float)b / 255.0f);
}
void RGL_ClearLights()
{
	if (!RGL_GL3Enabled())

	{
		return;
	}

	for (int i = 0; i < bmap_light_count; i++)

	{
		bmap_shader.lightDisable(i);
	}

	bmap_light_count = 0;
}
void RGL_AddLight(mobj_t* mo)
{
	bmap_shader.lightParam(bmap_light_count,
		mo->x, mo->y, mo->z,
		(float)RGB_RED(mo->dlight.color) / 256.0f,
		(float)RGB_BLU(mo->dlight.color) / 256.0f,
		(float)RGB_GRN(mo->dlight.color) / 256.0f,
		mo->dlight.r);

	bmap_light_count++;
}
void RGL_CaptureCameraMatrix()
{
	float cam_matrix[16];
	glGetFloatv(GL_MODELVIEW_MATRIX, cam_matrix);
	bmap_shader.setCamMatrix(cam_matrix);
}


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
	//if (GLEW_VERSION_1_3)
	glActiveTexture(id);
	//else /* GLEW_ARB_multitexture */
	//	glActiveTextureARB(id);
#endif
}

static inline void myMultiTexCoord2f(GLuint id, GLfloat s, GLfloat t)
{
	//#ifndef DREAMCAST
		//if (GLEW_VERSION_1_3)
	glMultiTexCoord2f(id, s, t);
	//else /* GLEW_ARB_multitexture */
	//	glMultiTexCoord2fARB(id, s, t);
//#endif
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
local_gl_vert_t* RGL_BeginUnit(GLuint shape, int max_vert,
	GLuint env1, GLuint tex1,
	GLuint env2, GLuint tex2,
	int pass, int blending)
{
	local_gl_unit_t* unit;

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

	unit->shape = shape;
	unit->env[0] = env1;
	unit->env[1] = env2;
	unit->tex[0] = tex1;
	unit->tex[1] = tex2;
	unit->tex_normal = 0;
	unit->tex_specular = 0;

	unit->pass = pass;
	unit->blending = blending;
	unit->first = cur_vert;  // count set later

	return local_verts + cur_vert;
}
void RGL_SetUnitMaps(GLuint tex_normal, GLuint tex_specular)
{
	local_gl_unit_t* unit = local_units + cur_unit;
	unit->tex_normal = tex_normal;
	unit->tex_specular = tex_specular;
}

//
// RGL_EndUnit
//
void RGL_EndUnit(int actual_vert)
{
	local_gl_unit_t* unit;

	SYS_ASSERT(actual_vert > 0);
	SYS_ASSERT(cur_unit < MAX_L_UNIT);

	unit = local_units + cur_unit;

	unit->count = actual_vert;

	// adjust colors (for special effects)
	for (int i = 0; i < actual_vert; i++)
	{
		local_gl_vert_t* v = &local_verts[cur_vert + i];

		v->rgba[0] *= ren_red_mul;
		v->rgba[1] *= ren_grn_mul;
		v->rgba[2] *= ren_blu_mul;
	}

	cur_vert += actual_vert;
	cur_unit++;

	// Assertion Error was fixed by upping the max verticies for MD5
	// Additional: Check to see if upping this causes problems for MD3
	SYS_ASSERT(cur_vert <= MAX_L_VERT);
	SYS_ASSERT(cur_unit <= MAX_L_UNIT);
}


struct Compare_Unit_pred
{
	inline bool operator() (const local_gl_unit_t* A, const local_gl_unit_t* B) const
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

static inline void RGL_SendRawVector(const local_gl_vert_t* V)
{
#ifndef DREAMCAST
	if (r_colormaterial || !r_colorlighting) {
		glColor4fv(V->rgba);
	}
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
	glEdgeFlag(V->edge);
#endif

	// vertex must be last
	glVertex3f(V->pos.x, V->pos.y, V->pos.z);
}
//also sends tangent
static inline void RGL_SendRawVector2(const local_gl_vert_t* V)
{

#ifndef DREAMCAST
	if (r_colormaterial || ! r_colorlighting) {
		glColor4fv(V->rgba);
	}
	else
	{
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, V->rgba);
		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, V->rgba);
	}
#else
	glColor4fv(V->rgba);
#endif

	myMultiTexCoord2f(GL_TEXTURE0, V->texc[0].x, V->texc[0].y);
	//XXX TODO:: NOTE:: FIXME ::
	//!!!: CA: myMultiTexCoord2f ->  (GL_TEXTURE1 line) *used* to be disabled!
	myMultiTexCoord2f(GL_TEXTURE1, V->texc[1].x, V->texc[1].y);

	glNormal3f(V->normal.x, V->normal.y, V->normal.z);
#ifndef NO_EDGEFLAG
	glEdgeFlag(V->edge);
#endif

	glVertexAttrib3f(bmap_shader.attr_tan, V->tangent.x, V->tangent.y, V->tangent.z);

	// vertex must be last
	glVertex3f(V->pos.x, V->pos.y, V->pos.z);
}

void calc_tan(local_gl_vert_t* v1, local_gl_vert_t* v2, local_gl_vert_t* v3)
{
	vec2_t d_uv1;
	vec2_t d_uv2;
	vec3_t d_pos1;
	vec3_t d_pos2;

	d_uv1.x = v2->texc[0].x - v1->texc[0].x;
	d_uv1.y = v2->texc[0].y - v1->texc[0].y;
	d_uv2.x = v3->texc[0].x - v1->texc[0].x;
	d_uv2.y = v3->texc[0].y - v1->texc[0].y;

	d_pos1.x = v2->pos.x - v1->pos.x;
	d_pos1.y = v2->pos.y - v1->pos.y;
	d_pos1.z = v2->pos.z - v1->pos.z;
	d_pos2.x = v3->pos.x - v1->pos.x;
	d_pos2.y = v3->pos.y - v1->pos.y;
	d_pos2.z = v3->pos.z - v1->pos.z;

	float r = 1.0f / (d_uv1.x * d_uv2.y - d_uv1.y * d_uv2.x);
	v1->tangent.x = r * (d_pos1.x * d_uv2.y - d_pos2.x * d_uv1.y);
	v1->tangent.y = r * (d_pos1.y * d_uv2.y - d_pos2.y * d_uv1.y);
	v1->tangent.z = r * (d_pos1.z * d_uv2.y - d_pos2.z * d_uv1.y);
}

// Only open/close a glBegin/glEnd if needed
void RGL_BatchShape(GLuint shape)
{
	static GLuint current_shape = 0;

	if (current_shape == shape)
		return;

	if (current_shape != 0)
		glEnd();

	current_shape = shape;

	if (current_shape != 0)
		glBegin(shape);
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

	for (int i = 0; i < cur_unit; i++)
		local_unit_map[i] = &local_units[i];

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

#ifdef USE_FOG
	if (fade_color)
	{

		float fog_col[4];
		fog_col[0] = (float)RGB_RED(fade_color) / 255.0f;
		fog_col[1] = (float)RGB_GRN(fade_color) / 255.0f;
		fog_col[2] = (float)RGB_BLU(fade_color) / 255.0f;
		fog_col[3] = 1.0f;

		glClearColor(0.5f, 0.5f, 0.5f, 1.0f);   // We'll Clear To The Color Of The Fog ( Modified )
		glFogi(GL_FOG_MODE, GL_EXP2);        // Fog Mode
		glFogf(GL_FOG_DENSITY, 0.002f);      // How Dense Will The Fog Be
		glFogfv(GL_FOG_COLOR, fog_col);      // Set Fog Color
		glHint(GL_FOG_HINT, GL_NICEST);      // Fog Hint Value
		glFogf(GL_FOG_START, 1.0f);          // Fog Start Depth
		glFogf(GL_FOG_END, 5.0f);            // Fog End Depth
		glEnable(GL_FOG);                    // Enables GL_FOG
	}
#endif

	for (int j = 0; j < cur_unit; j++)
	{
		local_gl_unit_t* unit = local_unit_map[j];

		SYS_ASSERT(unit->count > 0);

		// detect changes in texture/alpha/blending state

		if (active_pass != unit->pass)
		{
			active_pass = unit->pass;
			RGL_BatchShape(0);
			glPolygonOffset(0, -active_pass);
		}

		if ((active_blending ^ unit->blending) & (BL_Masked | BL_Less))
		{
			RGL_BatchShape(0);
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
			RGL_BatchShape(0);
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
			RGL_BatchShape(0);
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
			RGL_BatchShape(0);
			glDepthMask((unit->blending & BL_NoZBuf) ? GL_FALSE : GL_TRUE);
		}

		active_blending = unit->blending;

		if (active_blending & BL_Less)
		{
			// NOTE: assumes alpha is constant over whole polygon
			float a = local_verts[unit->first].rgba[3];
			RGL_BatchShape(0);
			glAlphaFunc(GL_GREATER, a * 0.66f);
		}

		for (int t = 1; t >= 0; t--)
		{
			if (active_tex[t] != unit->tex[t] || active_env[t] != unit->env[t])
			{
				RGL_BatchShape(0);
				myActiveTexture(GL_TEXTURE0 + t);
			}

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
			RGL_BatchShape(0);
			glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, &old_clamp);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
				r_dumbclamp ? GL_CLAMP : GL_CLAMP_TO_EDGE);
		}


		//disable if unit has multiple textures (level geometry lightmap for instance)
		if (RGL_GL3Enabled() && unit->tex[1] == 0)
		{
			RGL_BatchShape(0);

			//use normal and specular map
			bmap_shader.bind();
			bmap_shader.lightApply();

			myActiveTexture(GL_TEXTURE0 + 2);
			glBindTexture(GL_TEXTURE_2D, (unit->tex_specular != 0 ? unit->tex_specular : bmap_shader.tex_default_specular));
			myActiveTexture(GL_TEXTURE0 + 1);
			glBindTexture(GL_TEXTURE_2D, (unit->tex_normal != 0 ? unit->tex_normal : bmap_shader.tex_default_normal));
			myActiveTexture(GL_TEXTURE0);

			//calc tangents, works for GL_TRIANGLE_STRIP
			//TODO: don't re-calc every frame
			if (unit->shape == GL_TRIANGLE_STRIP)
			{
				for (int v_idx = 0; v_idx < unit->count - 2; v_idx += 3)
				{
					local_gl_vert_t* v = local_verts + unit->first + v_idx;
					calc_tan(v + 0, v + 1, v + 2);
					calc_tan(v + 1, v + 2, v + 0);
					calc_tan(v + 2, v + 0, v + 1);
				}
			}

			RGL_BatchShape(unit->shape);
			glBegin(unit->shape);
			if (bmap_shader.attr_tan != -1)
			{
				for (int v_idx = 0; v_idx < unit->count; v_idx++)
				{
					RGL_SendRawVector2(local_verts + unit->first + v_idx);
				}
			}
			else
			{
				for (int v_idx = 0; v_idx < unit->count; v_idx++)
				{
					RGL_SendRawVector(local_verts + unit->first + v_idx);
				}
			}

			glEnd();

			bmap_shader.unbind();


#ifdef DEBUG_GL3
			//bmap_shader.debugDrawLights();
			//XXX normals and tangent debug

			glBegin(GL_LINES);
			for (int i = 0; i < unit->count; i++) {
				local_gl_vert_t* v = local_verts + unit->first + i;

				float s = 1.0;
				glColor3f(1, 0, 0);
				glVertex3f(v->pos.x, v->pos.y, v->pos.z);
				glVertex3f(v->pos.x + v->tangent.x * s, v->pos.y + v->tangent.y * s, v->pos.z + v->tangent.z * s);

				glColor3f(0, 0, 1);
				s = 1.0;
				glVertex3f(v->pos.x, v->pos.y, v->pos.z);
				glVertex3f(v->pos.x + v->normal.x * s, v->pos.y + v->normal.y * s, v->pos.z + v->normal.z * s);
			}
			glEnd();

#endif  



		}
		else
		{
			// Simplify things into triangles as that allows us to keep a single glBegin open for longer
			if (unit->shape == GL_POLYGON || unit->shape == GL_TRIANGLE_FAN)
			{
				RGL_BatchShape(GL_TRIANGLES);
				for (int v_idx = 2; v_idx < unit->count; v_idx++)
				{
					RGL_SendRawVector(local_verts + unit->first);
					RGL_SendRawVector(local_verts + unit->first + v_idx - 1);
					RGL_SendRawVector(local_verts + unit->first + v_idx);
				}
			}
			else if (unit->shape == GL_QUADS)
			{
				RGL_BatchShape(GL_TRIANGLES);

				for (int v_idx = 0; v_idx + 3 < unit->count; v_idx += 4)
				{
					RGL_SendRawVector(local_verts + unit->first + v_idx);
					RGL_SendRawVector(local_verts + unit->first + v_idx + 1);
					RGL_SendRawVector(local_verts + unit->first + v_idx + 2);

					RGL_SendRawVector(local_verts + unit->first + v_idx);
					RGL_SendRawVector(local_verts + unit->first + v_idx + 2);
					RGL_SendRawVector(local_verts + unit->first + v_idx + 3);
				}
			}
			else
			{
				RGL_BatchShape(unit->shape);

				for (int v_idx = 0; v_idx < unit->count; v_idx++)
				{
					RGL_SendRawVector(local_verts + unit->first + v_idx);
				}

				// Force a glEnd if it is a type that can't be kept open.
				if (unit->shape != GL_TRIANGLES && unit->shape != GL_LINES && unit->shape != GL_QUADS)
					RGL_BatchShape(0);
			}
		}

		// restore the clamping mode
		if (old_clamp != DUMMY_CLAMP)
		{
			RGL_BatchShape(0);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, old_clamp);
		}
	}

	RGL_BatchShape(0);

	// all done
	cur_vert = cur_unit = 0;

	glPolygonOffset(0, 0);

	for (int t = 1; t >= 0; t--)
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

#ifdef USE_FOG
	glDisable(GL_FOG);
#endif
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_BLEND);
	glDisable(GL_CULL_FACE);
}


//#endif

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab