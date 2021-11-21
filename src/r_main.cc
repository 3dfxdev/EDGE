//----------------------------------------------------------------------------
//  EDGE OpenGL Rendering (Main Stuff)
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

#include "system/i_defs.h"
#include "system/i_defs_gl.h"

#include "../epi/str_format.h"

#include "g_game.h"
#include "r_misc.h"
#include "r_gldefs.h"
#include "r_units.h"
#include "r_colormap.h"
#include "r_draw.h"
#include "r_modes.h"
#include "r_image.h"
#include "r_renderbuffers.h"

#include "m_argv.h"
#define DEBUG  0

static std::vector<std::string> m_Extensions;

// implementation limits

int glmax_lights;
int glmax_clip_planes;
int glmax_tex_size;
int glmax_tex_units;
extern float max_anisotropic;

extern int r_bloom;
extern int r_lens;
extern int r_fxaa;
extern int r_fxaa_quality;
extern int r_gl3_path;
extern int r_anisotropy;
DEF_CVAR(r_aspect, float, "c", 1.777f);

DEF_CVAR(r_nearclip, float, "c", 4.0f);
DEF_CVAR(r_farclip, float, "c", 64000.0f);

DEF_CVAR(r_stretchworld, int, "c", 1);

typedef enum
{
	PFT_LIGHTING = (1 << 0),
	PFT_COLOR_MAT = (1 << 1),
	PFT_SKY = (1 << 2),
	PFT_MULTI_TEX = (1 << 3),
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
	//{ "Radeon",   NULL, NULL, PFT_LIGHTING | PFT_COLOR_MAT, 0 },
	//{ "RADEON",   NULL, NULL, PFT_LIGHTING | PFT_COLOR_MAT, 0 },

	//	{ "R200",     NULL, "Mesa 6.4", PFT_VERTEX_ARRAY, 0 },
	//	{ "R200",     NULL, "Mesa 6.5", PFT_VERTEX_ARRAY, 0 },
	//	{ "Intel",    NULL, "Mesa 6.5", PFT_VERTEX_ARRAY, 0 },

		{ "TNT2",     NULL, NULL, PFT_COLOR_MAT, 0 },

		{ "Velocity", NULL, NULL, PFT_COLOR_MAT | PFT_SKY, 0 },
		{ "Voodoo3",  NULL, NULL, PFT_SKY | PFT_MULTI_TEX, 0 },
};

#define NUM_DRIVER_BUGS  (sizeof(driver_bugs) / sizeof(driver_bug_t))

void SetupStencilBuffer(void)
{
	glDisable(GL_BLEND);
	glEnable(GL_STENCIL_TEST);
	glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
	glStencilFunc(GL_ALWAYS, 1, 0xffffffff);
	glDisable(GL_DEPTH_TEST);
	glColorMask(0, 0, 0, 1); /* Just update destination alpha. */
	glEnable(GL_TEXTURE_2D);
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

	glViewport(viewwindow_x, viewwindow_y, viewwindow_w, viewwindow_h);

	// calculate perspective matrix

	glMatrixMode(GL_PROJECTION);

	glLoadIdentity();

	glFrustum(-view_x_slope * r_nearclip, view_x_slope * r_nearclip,
		-view_y_slope * r_nearclip, view_y_slope * r_nearclip,
		r_nearclip, r_farclip);

	// calculate look-at matrix

	glMatrixMode(GL_MODELVIEW);

	glLoadIdentity();

	if (r_stretchworld == 1)
	{
		// We have to scale the pitch to account for the pixel stretching, because the playsim doesn't know about this and treats it as 1:1.
		// This code taken from GZDoom.
		double radPitch = ANG_2_FLOAT(viewvertangle) / 57.2958;
		double angx = cos(radPitch);
		double angy = sin(radPitch) * 1.2;
		double alen = sqrt(angx*angx + angy*angy);
		float vvang = (float)(asin(angy / alen)) * 57.2958;

		//glScalef(1.0f, 1.0f, 1/1.2f);
		glRotatef(270.0f - vvang, 1.0f, 0.0f, 0.0f);
		glRotatef(90.0f - ANG_2_FLOAT(viewangle), 0.0f, 0.0f, 1.0f);
		glScalef(1.0f, 1.0f, 1.2f);
		glRotatef(cameraroll, viewforward.x, viewforward.y, viewforward.z);
		glTranslatef(-viewx, -viewy, -viewz);
	}
	else
	{
		glRotatef(270.0f - ANG_2_FLOAT(viewvertangle), 1.0f, 0.0f, 0.0f);
		glRotatef(90.0f - ANG_2_FLOAT(viewangle), 0.0f, 0.0f, 1.0f);
		glRotatef(cameraroll, viewforward.x, viewforward.y, viewforward.z);
		glTranslatef(-viewx, -viewy, -viewz);
	}

	RGL_CaptureCameraMatrix();

	// turn on lighting.  Some drivers (e.g. TNT2) don't work properly
	// without it.
	if (r_colorlighting)
	{
		glEnable(GL_LIGHTING);
#ifndef DREAMCAST
		glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient);
#endif
	}
	else
		glDisable(GL_LIGHTING);

	if (r_colormaterial)
	{
		glEnable(GL_COLOR_MATERIAL);
#ifndef DREAMCAST
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
#endif
	}
	else
		glDisable(GL_COLOR_MATERIAL);

	//!!!
	 glBlendFunc(GL_SRC_ALPHA, GL_ONE);  // Additive lighting
}

static inline const char *SafeStr(const void *s)
{
	return s ? (const char *)s : "";
}

#define FUDGE_FUNC(name, ext) 	if (_GLEW_GET_FUN_##name == NULL) _ptrc_##name = _ptrc_##name##ext;


static void RGL_CollectExtensions()
{
	I_Printf("OpenGL: Collecting Extensions...\n");
	const char *extension;

	int max = 0;
	glGetIntegerv(GL_NUM_EXTENSIONS, &max);

	if (0 == max)
	{
		// Try old method to collect extensions
		const char *supported = (char *)glGetString(GL_EXTENSIONS);

		if (nullptr != supported)
		{
			char *extensions = new char[strlen(supported) + 1];
			strcpy(extensions, supported);

			char *extension = strtok(extensions, " ");

			while (extension)
			{
				m_Extensions.push_back(std::string(extension));
				extension = strtok(nullptr, " ");
			}

			delete[] extensions;
		}
	}
	else
	{
		// Use modern method to collect extensions
		for (int i = 0; i < max; i++)
		{
			extension = (const char*)glGetStringi(GL_EXTENSIONS, i);
			m_Extensions.push_back(std::string(extension));
		}
	}
}

static bool RGL_CheckExtension(const char *ext)
{
	for (unsigned int i = 0; i < m_Extensions.size(); ++i)
	{
		if (m_Extensions[i].compare(ext) == 0) return true;
	}

	return false;
}

//#define name GLEW_GET_FUN

#undef FUDGE_FUNC
#define FUDGE_FUNC(name, ext) 	if (_ptrc_##name == NULL) _ptrc_##name = _ptrc_##name##ext;

// forward declaration to detect outdated opengl
extern bool no_render_buffers;

void RGL_LoadExtensions()
{
	I_Printf("RGL_LoadContextExtensions...\n");
	GLenum err = ogl_LoadFunctions();
#ifdef GLEWISDEAD
	//FIXME: REMOVE GLEW!!!
	GLenum err = glewInit();

	if (err != GLEW_OK)
		I_Error("Unable to initialize GLEW: %s\n",
			glewGetErrorString(err));
#endif
	RGL_CollectExtensions();

	const char *glversion = (const char*)glGetString(GL_VERSION);
	gl.es = false;

	if (glversion && strlen(glversion) > 10 && memcmp(glversion, "OpenGL ES ", 10) == 0)
	{
		glversion += 10;
		gl.es = true;
	}
	
	const char *version = M_GetParm("-glversion");

	if (version == NULL)
	{
		version = glversion;
	}
	else
	{
		double v1 = strtod(version, NULL);
		double v2 = strtod(glversion, NULL);
		if (v2 < v1) version = glversion;
		else
			I_Printf("Emulating OpenGL v %s\n", version);
	}

	float gl_version = (float)strtod(version, NULL) + 0.01f;

	// Broadcom V3D driver check for OpenGLES. Performs a major version check to manually set OpenGL ES version
	const char *glrenderer = (const char*)glGetString(GL_RENDERER);
	if (glrenderer && strlen(glversion) > 5) {
		if (memcmp(glrenderer, "V3D 4", 5) == 0) {
			gl.es = true;
			gl_version = 3.1;
		} else if (memcmp(glrenderer, "V3D 3", 5) == 0) {
			gl.es = true;
			gl_version = 2.0;
		}	
	}
	
	if (gl.es)
	{
		if (gl_version < 2.0f)
		{
			I_Error("Unsupported OpenGL ES version.\nAt least OpenGL ES 2.0 is required to run EDGE.\n");
		}

		const char *glslversion = (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);

		if (glslversion && strlen(glslversion) > 18 && memcmp(glslversion, "OpenGL ES GLSL ES ", 10) == 0) //TODO: V512 https://www.viva64.com/en/w/v512/ A call of the 'memcmp' function will lead to underflow of the buffer '"OpenGL ES GLSL ES "'.
		{
			glslversion += 18;
		}

		// add 0.01 to account for roundoff errors making the number a tad smaller than the actual version
		gl.glslversion = strtod(glslversion, NULL) + 0.01f;
		gl.vendorstring = (char*)glGetString(GL_VENDOR);

		// Use the slowest/oldest modern path for now
		gl.legacyMode = false;
		//gl.lightmethod = LM_DEFERRED;
		//gl.buffermethod = BM_DEFERRED;
		
		/* Force no_render_buffers for Pi 4 until GLSL 1.20 shaders can be implemented as an alternative.
		Comment this out to use GL2/3 renderer, but be aware that attempting to enable FXAA/Bloom will crash EDGE. - Dasho */
		if (glrenderer && strlen(glversion) > 4) {
			if (memcmp(glrenderer, "V3D ", 4) == 0) {
				no_render_buffers = true;
			}	
		}
	}
	else
	{

		// Don't even start if it's lower than 2.0 or no framebuffers are available (The framebuffer extension is needed for glGenerateMipmapsEXT!)
		if ((gl_version < 2.0f || !RGL_CheckExtension("GL_EXT_framebuffer_object")) && gl_version < 3.0f)
		{
			I_Error("Unsupported OpenGL version!\nAt least OpenGL 2.0 with framebuffer support is required to run EDGE.\nRestart EDGE with -norenderbuffers for OpenGL 1.1!");
		}

		gl.es = false;

		// add 0.01 to account for roundoff errors making the number a tad smaller than the actual version
		gl.glslversion = strtod((char*)glGetString(GL_SHADING_LANGUAGE_VERSION), NULL) + 0.01f;

		gl.vendorstring = (char*)glGetString(GL_VENDOR);

		// first test for optional features
		if (RGL_CheckExtension("GL_ARB_texture_compression")) gl.flags |= RFL_TEXTURE_COMPRESSION;
		if (
			RGL_CheckExtension("GL_EXT_texture_compression_s3tc")) gl.flags |= RFL_TEXTURE_COMPRESSION_S3TC;

		if ((gl_version >= 3.3f || RGL_CheckExtension("GL_ARB_sampler_objects")) && !M_CheckParm("-nosampler"))
		{
			gl.flags |= RFL_SAMPLER_OBJECTS;
		}

		// The minimum requirement for the modern render path are GL 3.0 + uniform buffers.
		// Also exclude the Linux Mesa driver at GL 3.0 because it errors out on shader compilation.
		if (gl_version < 3.0f || (gl_version < 3.1f && (!RGL_CheckExtension("GL_ARB_uniform_buffer_object") || strstr(gl.vendorstring, "X.Org") != nullptr)))
		{
			no_render_buffers = true;
			gl.legacyMode = true;
			//gl.lightmethod = LM_LEGACY;
			//gl.buffermethod = BM_LEGACY;
			gl.glslversion = 0;
			gl.flags |= RFL_NO_CLIP_PLANES;
		}
		else
		{
			gl.legacyMode = false;
			no_render_buffers = false;
			//gl.lightmethod = LM_DEFERRED;
			//gl.buffermethod = BM_DEFERRED;
			if (gl_version < 4.f)
			{
#ifdef _WIN32
				if (strstr(gl.vendorstring, "ATI Tech"))
				{
					gl.flags |= RFL_NO_CLIP_PLANES;	// gl_ClipDistance is horribly broken on ATI GL3 drivers for Windows.
				}
#endif
			}
			else if (gl_version < 4.5f)
			{
				// don't use GL 4.x features when running a GL 3.x context.
				if (RGL_CheckExtension("GL_ARB_buffer_storage"))
				{
					// work around a problem with older AMD drivers: Their implementation of shader storage buffer objects is piss-poor and does not match uniform buffers even closely.
					// Recent drivers, GL 4.4 don't have this problem, these can easily be recognized by also supporting the GL_ARB_buffer_storage extension.
					if (RGL_CheckExtension("GL_ARB_shader_storage_buffer_object"))
					{
						// Intel's GLSL compiler is a bit broken with extensions, so unlock the feature only if not on Intel or having GL 4.3.
						if (strstr(gl.vendorstring, "Intel") == NULL || gl_version >= 4.3f)
						{
							gl.flags |= RFL_SHADER_STORAGE_BUFFER;
						}
					}
					gl.flags |= RFL_BUFFER_STORAGE;
					//gl.lightmethod = LM_DIRECT;
					//gl.buffermethod = BM_PERSISTENT;
				}
			}
			else
			{
				// Assume that everything works without problems on GL 4.5 drivers where these things are core features.
				gl.flags |= RFL_SHADER_STORAGE_BUFFER | RFL_BUFFER_STORAGE;
				//gl.lightmethod = LM_DIRECT;
				//gl.buffermethod = BM_PERSISTENT;
			}

			if (gl_version >= 4.3f || RGL_CheckExtension("GL_ARB_invalidate_subdata")) gl.flags |= RFL_INVALIDATE_BUFFER;
			if (gl_version >= 4.3f || RGL_CheckExtension("GL_KHR_debug")) gl.flags |= RFL_DEBUG;

			//const char *lm = Args->CheckValue("-lightmethod");
			//if (lm != NULL)
			//{
			//	if (!stricmp(lm, "deferred") && gl.lightmethod == LM_DIRECT)
			//		gl.lightmethod = LM_DEFERRED;
			//}

			//lm = Args->CheckValue("-buffermethod");
			//if (lm != NULL)
			//{
			//	if (!stricmp(lm, "deferred") && gl.buffermethod == BM_PERSISTENT) gl.buffermethod = BM_DEFERRED;
			//}
		}
	}

	int v;

	if (!gl.legacyMode && !(gl.flags & RFL_SHADER_STORAGE_BUFFER))
	{
		glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS, &v);
		gl.maxuniforms = v;
		glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &v);
		gl.maxuniformblock = v;
		glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &v);
		gl.uniformblockalignment = v;
	}
	else
	{
		//no_render_buffers = true;
		gl.maxuniforms = 0;
		gl.maxuniformblock = 0;
		gl.uniformblockalignment = 0;
	}


	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &gl.max_texturesize);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	if (gl.legacyMode)
	{
		// fudge a bit with the framebuffer stuff to avoid redundancies in the main code.
		// Some of the older cards do not have the ARB stuff but the calls are nearly identical.
		FUDGE_FUNC(glGenerateMipmap, EXT); //TODO: V760 https://www.viva64.com/en/w/v760/ Two identical blocks of text were found. The second block begins from line 477.
		FUDGE_FUNC(glGenFramebuffers, EXT);
		FUDGE_FUNC(glBindFramebuffer, EXT);
		FUDGE_FUNC(glDeleteFramebuffers, EXT);
		FUDGE_FUNC(glFramebufferTexture2D, EXT);
		FUDGE_FUNC(glGenerateMipmap, EXT);
		FUDGE_FUNC(glGenFramebuffers, EXT);
		FUDGE_FUNC(glBindFramebuffer, EXT);
		FUDGE_FUNC(glDeleteFramebuffers, EXT);
		FUDGE_FUNC(glFramebufferTexture2D, EXT);
		FUDGE_FUNC(glFramebufferRenderbuffer, EXT);
		FUDGE_FUNC(glGenRenderbuffers, EXT);
		FUDGE_FUNC(glDeleteRenderbuffers, EXT);
		FUDGE_FUNC(glRenderbufferStorage, EXT);
		FUDGE_FUNC(glBindRenderbuffer, EXT);
		FUDGE_FUNC(glCheckFramebufferStatus, EXT);
		//gl_PatchMenu();
	}
}


//
// RGL_CheckExtensions
//
// Based on code by Bruce Lewis.
//
#if 0
void RGL_CheckExtensions_Old(void)
{
#ifndef DREAMCAST
	GLenum err = glewInit();

	if (err != GLEW_OK)
		I_Error("Unable to initialize GLEW: %s\n",
			glewGetErrorString(err));

	I_Printf("OpenGL: Collecting Extensions (-oldglchecks is set)...\n");

	// -ACB- 2004/08/11 Made local: these are not yet used elsewhere
	std::string glstr_version(SafeStr(glGetString(GL_VERSION)));
	std::string glstr_renderer(SafeStr(glGetString(GL_RENDERER)));
	std::string glstr_vendor(SafeStr(glGetString(GL_VENDOR)));
	std::string glstr_glsl(SafeStr(glGetString(GL_SHADING_LANGUAGE_VERSION)));

	I_Printf("OpenGL: Version: %s\n", glstr_version.c_str());
	I_Printf("OpenGL: Renderer: %s\n", glstr_renderer.c_str());
	I_Printf("OpenGL: Vendor: %s\n", glstr_vendor.c_str());
	I_Printf("OpenGL: GLEW version: %s\n", glewGetString(GLEW_VERSION));
	I_Printf("OpenGL: GLSL version: %s\n", glstr_glsl.c_str());

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
	{ /* OK */
	}
	else
		I_Error("OpenGL driver does not support Multi-texturing.\n");

	if (GLEW_VERSION_1_3 || GLEW_EXT_texture_filter_anisotropic)
	{
		I_Printf("OpenGL: driver verified to support anisotropic filtering!\n");
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max_anisotropic);
	}
	else
		I_Printf("OpenGL: Does not support anisotropy!\n");

	if (GLEW_ARB_vertex_shader || GLEW_ARB_fragment_shader)
		I_Printf("OpenGL: GLSLv%s Initialized\n", glstr_glsl.c_str());
	else if (!GLEW_VERSION_2_1)
	{
		I_Warning("OpenGL: GLSLv%s is less than 2.1! Disabling GLSL\n", glstr_glsl.c_str());
		no_render_buffers = true;
		r_bloom = 0;
		r_fxaa = 0;
		r_lens = 0;
		r_gl3_path = 0;
	}

	if (GLEW_VERSION_1_3 ||
		GLEW_ARB_texture_env_combine ||
		GLEW_EXT_texture_env_combine)
	{ /* OK */
	}
	else
	{
		I_Warning("OpenGL driver does not support COMBINE.\n");
		r_dumbcombine = 1;
	}

	if (GLEW_VERSION_1_2 ||
		GLEW_EXT_texture_edge_clamp ||
		GLEW_SGIS_texture_edge_clamp)
	{ /* OK */
	}
	else
	{
		I_Warning("OpenGL driver does not support Edge-Clamp.\n");
		r_dumbclamp = 1;
	}

	// --- Detect buggy drivers, enable workarounds ---

	for (int j = 0; j < (int)NUM_DRIVER_BUGS; j++)
	{
		const driver_bug_t *bug = &driver_bugs[j];

		if (bug->renderer && !strstr(glstr_renderer.c_str(), bug->renderer))
			continue;

		if (bug->vendor && !strstr(glstr_vendor.c_str(), bug->vendor))
			continue;

		if (bug->version && !strstr(glstr_version.c_str(), bug->version))
			continue;

		I_Printf("OpenGL: Enabling workarounds for %s.\n",
			bug->renderer ? bug->renderer :
			bug->vendor ? bug->vendor : "the Axis of Evil");

		if (bug->disable & PFT_LIGHTING)  r_colorlighting = 0;
		if (bug->disable & PFT_COLOR_MAT) r_colormaterial = 0;
		if (bug->disable & PFT_SKY)       r_dumbsky = 1;
		if (bug->disable & PFT_MULTI_TEX) r_dumbmulti = 1;

		if (bug->enable & PFT_LIGHTING)   r_colorlighting = 1;
		if (bug->enable & PFT_COLOR_MAT)  r_colormaterial = 1;
		if (bug->enable & PFT_SKY)        r_dumbsky = 0;
		if (bug->enable & PFT_MULTI_TEX)  r_dumbmulti = 0;
	}
#else
	std::string glstr_version(SafeStr(glGetString(GL_VERSION)));
	std::string glstr_renderer(SafeStr(glGetString(GL_RENDERER)));
	std::string glstr_vendor(SafeStr(glGetString(GL_VENDOR)));

	I_Printf("OpenGL: Version: %s\n", glstr_version.c_str());
	I_Printf("OpenGL: Renderer: %s\n", glstr_renderer.c_str());
	I_Printf("OpenGL: Vendor: %s\n", glstr_vendor.c_str());

	r_dumbcombine = 1;
	r_colorlighting = 1;
	r_colormaterial = 1;
	r_dumbsky = 1;
	r_dumbmulti = 1;
#endif
	}
#endif // 0


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

#ifndef DREAMCAST
	glDisable(GL_LINE_SMOOTH);
	glDisable(GL_POLYGON_SMOOTH);

	if (var_dithering)
		glEnable(GL_DITHER);
	else
		glDisable(GL_DITHER);

	glEnable(GL_NORMALIZE);
#endif

	glShadeModel(GL_SMOOTH);
	glDepthFunc(GL_LEQUAL);
	glAlphaFunc(GL_GREATER, 0);

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
	//ogl_LoadFunctions();

	I_Printf("==============================================================================\n");
	I_Printf("OpenGL Subsystem: Initialising...\n");

	//if (M_CheckParm("-oldGLchecks"))
	//{
	//	RGL_CheckExtensions_Old();
	//}
	//if (!M_CheckParm("-oldGLchecks"))
		//CA -1.21.2018- ~ New, smarter GL extension checker!
	//{
#ifndef VITA
	RGL_LoadExtensions();
#endif
	//}
		int v = 0;
		if (!gl.legacyMode)
			glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &v);

		I_Printf("GL_VENDOR: %s\n", glGetString(GL_VENDOR));
		I_Printf("GL_RENDERER: %s\n", glGetString(GL_RENDERER));
		I_Printf("GL_VERSION: %s (%s profile)\n", glGetString(GL_VERSION), (v & GL_CONTEXT_CORE_PROFILE_BIT) ? "Core" : "Compatibility");
		//I_Printf("GLEW_VERSION: %s\n", glewGetString(GLEW_VERSION));
		I_Printf("GL_SHADING_LANGUAGE_VERSION: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

		if ((M_CheckParm("-debugopengl")))
		{
			I_GLf("GL_EXTENSIONS:");
			for (unsigned i = 0; i < m_Extensions.size(); i++)
			{
				I_GLf(" %s", m_Extensions[i].c_str());
			}
		}
		I_Printf("OpenGL: Implementation limits:\n");
		glGetIntegerv(GL_MAX_TEXTURE_SIZE, &v);
		I_Printf("-Maximum texture size: %d\n", v);
		glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &v);
		I_Printf("-Maximum texture units: %d\n", v);
		glGetIntegerv(GL_MAX_VARYING_FLOATS, &v);
		I_Printf("-Maximum varying: %d\n", v);
		glGetIntegerv(GL_MAX_CLIP_PLANES, &v);
		I_Printf("-MAximum clip planes: %d\n", v);

		// read implementation limits
		{
			GLint max_tex_size;

			glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_tex_size);
			glmax_tex_size = max_tex_size;

#ifndef DREAMCAST
			GLint max_lights;
			GLint max_clip_planes;
			GLint max_tex_units;

			glGetIntegerv(GL_MAX_LIGHTS, &max_lights);
			glGetIntegerv(GL_MAX_CLIP_PLANES, &max_clip_planes);
			glGetIntegerv(GL_MAX_TEXTURE_UNITS, &max_tex_units);

			glmax_lights = max_lights;
			glmax_clip_planes = max_clip_planes;
			glmax_tex_units = max_tex_units;
#else
			glmax_lights = 1;
			glmax_clip_planes = 0;
			glmax_tex_units = 2;
			I_Printf("Legacy OpenGL: Lights: %d  Clip Planes: %d  Tex: %d  Units: %d\n",
					glmax_lights, glmax_clip_planes, glmax_tex_size, glmax_tex_units);
#endif
		}

	GLfloat max_anisotropic;

	// initialise Anisotropic Filter
	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max_anisotropic);

	RGL_SoftInit();

	R2_InitUtil();

	// initialise unit system
	RGL_InitUnits();

	RGL_SetupMatrices2D();

	if ((M_CheckParm("-norenderbuffers")))
	{
		no_render_buffers = true;
		r_bloom = 0;
		r_fxaa = 0;
		r_fxaa_quality = 0;
		r_lens = 0;
		r_gl3_path = 0;
		I_Printf("RGL_Init: GL3/Post-Processing disabled!\n");
		I_Printf("==============================================================================\n");
	}
	else if ((!M_CheckParm("-norenderbuffers")))
		RGL_InitRenderBuffers();
}

//Duplicate of above with less checks (for fullscreen mode swaps)
void RGL_ReInit()
{
	I_Printf("RGL_ReInit!\n");

	RGL_SoftInit();

	R_SoftInitResolution();

	RGL_SetupMatrices2D();

	if ((M_CheckParm("-norenderbuffers")))
	{
		no_render_buffers = true;
		r_bloom = 0;
		r_fxaa = 0;
		r_fxaa_quality = 0;
		r_lens = 0;
		r_gl3_path = 0;
		I_Printf("OpenGL: RenderBuffers/GLSL disabled...\n");
		I_Printf("==============================================================================\n");
	}
	else if ((!M_CheckParm("-norenderbuffers")))
		RGL_InitRenderBuffers();
}

//void print_stack(int);

#if 0
void RGL_Error(int errCode)
{
	const GLubyte *errString;
	errString = gluErrorString(errCode);
	I_Error("OpenGL Error: %s\n", errString);
	print_stack(1);
	//glewGetErrorString(errCode));
	exit(1);
}
void checkGLStatus()
{
	GLenum errCode;

	if ((errCode = glGetError()) != GL_NO_ERROR)
	{
		//GLError(errCode);//bna_removed
	}
}

#endif // 0

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
