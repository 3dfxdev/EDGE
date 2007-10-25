//----------------------------------------------------------------------------
//  EDGE OpenGL Rendering (Main Stuff)
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

#include "g_game.h"
#include "r_misc.h"
#include "r_gldefs.h"
#include "r_units.h"
#include "r_colors.h"
#include "r_draw.h"
#include "r_modes.h"
#include "r_image.h"

#define DEBUG  0

// implementation limits

int glmax_lights;
int glmax_clip_planes;
int glmax_tex_size;
int glmax_tex_units;

int rgl_light_map[256];
static lighting_model_e rgl_light_model = LMODEL_Invalid;

int var_nearclip = 4;
int var_farclip  = 64000;


///--- // -AJA- FIXME: temp hack
///--- #ifndef GL_MAX_TEXTURE_UNITS
///--- #define GL_MAX_TEXTURE_UNITS  0x84E2
///--- #endif


typedef enum
{
	PFT_LIGHTING   = (1 << 0),
	PFT_COLOR_MAT  = (1 << 1),
	PFT_SKY        = (1 << 2),
	PFT_MULTI_TEX  = (1 << 3),
}
problematic_feature_e;

typedef struct
{
	// these are substrings that may be matched anywhere
	const char *renderer;
	const char *vendor;
	const char *version;

	// problematic features to force on or off (bitmasks)
	int disable;
	int enable;
}
driver_bug_t;

static const driver_bug_t driver_bugs[] =
{
	{ "Radeon",   NULL, NULL, PFT_LIGHTING | PFT_COLOR_MAT, 0 },
	{ "RADEON",   NULL, NULL, PFT_LIGHTING | PFT_COLOR_MAT, 0 },

//	{ "R200",     NULL, "Mesa 6.4", PFT_VERTEX_ARRAY, 0 },
//	{ "R200",     NULL, "Mesa 6.5", PFT_VERTEX_ARRAY, 0 },
//	{ "Intel",    NULL, "Mesa 6.5", PFT_VERTEX_ARRAY, 0 },

	{ "TNT2",     NULL, NULL, PFT_COLOR_MAT, 0 },

	{ "Velocity", NULL, NULL, PFT_COLOR_MAT | PFT_SKY, 0 },
	{ "Voodoo3",  NULL, NULL, PFT_SKY | PFT_MULTI_TEX, 0 },
};

#define NUM_DRIVER_BUGS  (sizeof(driver_bugs) / sizeof(driver_bug_t))


static void SetupLightMap(lighting_model_e model)
{
	int i;
  
	rgl_light_model = (lighting_model_e)model;

	for (i=0; i < 256; i++)
	{
		if (model >= LMODEL_Flat)
		{
			rgl_light_map[i] = i;
			continue;
		}

		// Approximation of standard Doom lighting: 
		// (based on side-by-side comparison)
		//    [0,72] --> [0,16]
		//    [72,112] --> [16,56]
		//    [112,255] --> [56,255]

		if (i <= 72)
			rgl_light_map[i] = i * 16 / 72;
		else if (i <= 112)
			rgl_light_map[i] = 16 + (i - 72) * 40 / 40;
		else if (i < 255)
			rgl_light_map[i] = 56 + (i - 112) * 200 / 144;
		else
			rgl_light_map[i] = 255;
	}
}

//
// RGL_SetupMatrices2D
//
// Setup the GL matrices for drawing 2D stuff.
//
void RGL_SetupMatrices2D(void)
{
	glViewport(0, 0, SCREENWIDTH, SCREENHEIGHT);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0f, (float)SCREENWIDTH, 
			0.0f, (float)SCREENHEIGHT, -1.0f, 1.0f);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// turn off lighting stuff
	glDisable(GL_LIGHTING);
	glDisable(GL_COLOR_MATERIAL);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}


//
// RGL_SetupMatrices3D
//
// Setup the GL matrices for drawing 3D stuff.
//
void RGL_SetupMatrices3D(void)
{
	GLfloat ambient[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

	glViewport(viewwindowx, SCREENHEIGHT - viewwindowy - viewwindowheight,
			   viewwindowwidth, viewwindowheight);

	// calculate perspective matrix

	glMatrixMode(GL_PROJECTION);

	glLoadIdentity();
	glFrustum(rightslope * var_nearclip, leftslope * var_nearclip,
			  bottomslope * var_nearclip, topslope * var_nearclip,
			  var_nearclip, var_farclip);

	// calculate look-at matrix

	glMatrixMode(GL_MODELVIEW);

	glLoadIdentity();
	glRotatef(270.0f - ANG_2_FLOAT(viewvertangle), 1.0f, 0.0f, 0.0f);
	glRotatef(90.0f - ANG_2_FLOAT(viewangle), 0.0f, 0.0f, 1.0f);
	glTranslatef(-viewx, -viewy, -viewz);

	// turn on lighting.  Some drivers (e.g. TNT2) don't work properly
	// without it.
	if (use_lighting)
	{
		glEnable(GL_LIGHTING);
		glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient);
	}
	else
		glDisable(GL_LIGHTING);

	if (use_color_material)
	{
		glEnable(GL_COLOR_MATERIAL);
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	}
	else
		glDisable(GL_COLOR_MATERIAL);

	/* glBlendFunc(GL_SRC_ALPHA, GL_ONE);  // Additive lighting */

	SYS_ASSERT(currmap);
	if (currmap->lighting != rgl_light_model)
		SetupLightMap(currmap->lighting);
}

static inline const char *SafeStr(const void *s)
{
	return s ? (const char *)s : "";
}

//
// RGL_CheckExtensions
//
// Based on code by Bruce Lewis.
//
void RGL_CheckExtensions(void)
{
	GLenum err = glewInit();

	if (err != GLEW_OK)
		I_Error("Unable to initialise GLEW: %s\n",
			glewGetErrorString(err));

	// -ACB- 2004/08/11 Made local: these are not yet used elsewhere
	std::string glstr_vendor  (SafeStr(glGetString(GL_VERSION)));
	std::string glstr_renderer(SafeStr(glGetString(GL_VENDOR)));
	std::string glstr_version (SafeStr(glGetString(GL_RENDERER)));

	I_Printf("OpenGL: Version: %s\n", glstr_version.c_str());
	I_Printf("OpenGL: Vendor: %s\n", glstr_vendor.c_str());
	I_Printf("OpenGL: Renderer: %s\n", glstr_renderer.c_str());
	I_Printf("OpenGL: GLEW version: %s\n", glewGetString(GLEW_VERSION));

#if 0  // FIXME: this crashes (buffer overflow?)
	I_Printf("OpenGL: EXTENSIONS: %s\n", glGetString(GL_EXTENSIONS));
#endif

	// Check for a windows software renderer
	if (stricmp(glstr_vendor.c_str(), "Microsoft Corporation") == 0)
	{
		if (stricmp(glstr_renderer.c_str(), "GDI Generic") == 0)
		{
			I_Error("OpenGL: SOFTWARE Renderer!\n");
		}		
	}

	// Check for various extensions

	if (GLEW_VERSION_1_3 || GLEW_ARB_multitexture)
	{ /* OK */ }
	else
		I_Error("OpenGL driver does not support Multitexturing.\n");

	if (GLEW_VERSION_1_3 ||
		GLEW_ARB_texture_env_combine ||
		GLEW_EXT_texture_env_combine)
	{ /* OK */ }
	else
	{
		I_Warning("OpenGL driver does not support COMBINE.\n");
		dumb_combine = true;
	}

	if (GLEW_VERSION_1_2 ||
		GLEW_EXT_texture_edge_clamp ||
		GLEW_SGIS_texture_edge_clamp)
	{ /* OK */ }
	else
	{
		I_Warning("OpenGL driver does not support Edge-Clamp.\n");
		dumb_clamp = true;
	}


///---	// -AJA- FIXME: temp hack, improve extension handling after 1.29
///---	if (glstr_version[0] >= '2' ||
///---	    glstr_version[0] == '1' && glstr_version[1] == '.' &&
///---		glstr_version[2] >= '2')
///---	{
///---		glcap_edgeclamp = true;
///---	}
///---	else if (strstr(glstr_extensions, "GL_EXT_texture_edge_clamp") != NULL ||
///---	         strstr(glstr_extensions, "GL_SGIS_texture_edge_clamp") != NULL)
///---	{
///---		I_Printf("OpenGL: EdgeClamp extension found.\n");
///---		glcap_edgeclamp = true;
///---	}

	// --- Detect buggy drivers, enable workarounds ---
	
	for (int j = 0; j < (int)NUM_DRIVER_BUGS; j++)
	{
		const driver_bug_t *bug = &driver_bugs[j];

		if (bug->renderer && strstr(glstr_renderer.c_str(), bug->renderer) == NULL)
			continue;

		if (bug->vendor && strstr(glstr_vendor.c_str(), bug->vendor) == NULL)
			continue;

		if (bug->version && strstr(glstr_version.c_str(), bug->version) == NULL)
			continue;

		I_Printf("OpenGL: Enabling workarounds for %s.\n",
				bug->renderer ? bug->renderer :
				bug->vendor   ? bug->vendor : "the Axis of Evil");

		if (bug->disable & PFT_LIGHTING)  use_lighting = false;
		if (bug->disable & PFT_COLOR_MAT) use_color_material = false;
		if (bug->disable & PFT_SKY)       dumb_sky = true;
		if (bug->disable & PFT_MULTI_TEX) dumb_multi = true;

		if (bug->enable & PFT_LIGHTING)   use_lighting = true;
		if (bug->enable & PFT_COLOR_MAT)  use_color_material = true;
		if (bug->enable & PFT_SKY)        dumb_sky = false;
		if (bug->enable & PFT_MULTI_TEX)  dumb_multi = false;
	}
}

//
// RGL_SoftInit
//
// All the stuff that can be re-initialised multiple times.
// 
void RGL_SoftInit(void)
{
	glDisable(GL_FOG);
	glDisable(GL_BLEND);
	glDisable(GL_LIGHTING);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_STENCIL_TEST);

	glDisable(GL_LINE_SMOOTH);
	glDisable(GL_POLYGON_SMOOTH);

	if (use_dithering)
		glEnable(GL_DITHER);
	else
		glDisable(GL_DITHER);

	glEnable(GL_NORMALIZE);

	glShadeModel(GL_SMOOTH);
	glDepthFunc(GL_LEQUAL);
	glAlphaFunc(GL_GREATER, 1.0f / 32.0f);

	glFrontFace(GL_CW);
	glCullFace(GL_BACK);
	glDisable(GL_CULL_FACE);

	glHint(GL_FOG_HINT, GL_NICEST);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
}

//
// RGL_Init
//
void RGL_Init(void)
{
	I_Printf("OpenGL: Initialising...\n");

	RGL_CheckExtensions();


	// read implementation limits
	{
		GLint max_lights;
		GLint max_clip_planes;
		GLint max_tex_size;
		GLint max_tex_units;

		glGetIntegerv(GL_MAX_LIGHTS,        &max_lights);
		glGetIntegerv(GL_MAX_CLIP_PLANES,   &max_clip_planes);
		glGetIntegerv(GL_MAX_TEXTURE_SIZE,  &max_tex_size);
		glGetIntegerv(GL_MAX_TEXTURE_UNITS, &max_tex_units);

		glmax_lights = max_lights;
		glmax_clip_planes = max_clip_planes;
		glmax_tex_size = max_tex_size;
		glmax_tex_units = max_tex_units;
	}

	I_Printf("OpenGL: Lights: %d  Clips: %d  Tex: %d  Units: %d\n",
			 glmax_lights, glmax_clip_planes, glmax_tex_size, glmax_tex_units);
  
	RGL_SoftInit();

	R2_InitUtil();

	// initialise unit system
	RGL_InitUnits();

	RGL_SetupMatrices2D();
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
