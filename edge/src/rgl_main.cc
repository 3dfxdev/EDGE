//----------------------------------------------------------------------------
//  EDGE OpenGL Rendering (Main Stuff)
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

// this conditional applies to the whole file
#include "i_defs.h"

#include "g_game.h"
#include "r_main.h"
#include "r2_defs.h"
#include "rgl_defs.h"
#include "v_colour.h"
#include "v_ctx.h"

#include "epi/strings.h"

#define DEBUG  0

// implementation limits

int glmax_lights;
int glmax_clip_planes;
int glmax_tex_size;
int glmax_tex_units;

int rgl_light_map[256];
static lighting_model_e rgl_light_model = LMODEL_Invalid;

bool glcap_hardware = true;
bool glcap_multitex = false;
bool glcap_paletted = false;
bool glcap_edgeclamp = false;

angle_t oned_side_angle;

static int glbsp_last_prog_time = 0;


// -AJA- FIXME: temp hack
#ifndef GL_MAX_TEXTURE_UNITS
#define GL_MAX_TEXTURE_UNITS  0x84E2
#endif


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
	glFrustum(rightslope, leftslope, bottomslope, topslope, Z_NEAR, Z_FAR);
#if 0
	glOrtho(rightslope * 240, leftslope * 240, bottomslope * 240, topslope * 240, Z_NEAR, Z_FAR);
#endif

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

	DEV_ASSERT2(currmap);
	if (currmap->lighting != rgl_light_model)
		SetupLightMap(currmap->lighting);
}


//
//  VIDEO CONTEXT STUFF
//

//
// RGL_NewcreenSize
//
void RGL_NewScreenSize(int width, int height, int bits)
{
	//!!! quick hack
	RGL_SetupMatrices2D();

	// prevent a visible border with certain cards/drivers
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
}

//
// RGL_DrawImage
//
void RGL_DrawImage(float x, float y, float w, float h, const image_t *image, 
				   float tx1, float ty1, float tx2, float ty2,
				   const colourmap_c *colmap, float alpha)
{
	int x1 = I_ROUND(x);
	int y1 = I_ROUND(y);
	int x2 = I_ROUND(x+w+0.25f);
	int y2 = I_ROUND(y+h+0.25f);

	if (x1 == x2 || y1 == y2)
		return;

	float r = 1.0f, g = 1.0f, b = 1.0f;

	const cached_image_t *cim = W_ImageCache(image, false,
		(colmap && (colmap->special & COLSP_Whiten)) ? font_whiten_map : NULL);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, W_ImageGetOGL(cim));
 
	if (alpha < 0.99f || !image->img_solid)
		glEnable(GL_BLEND);

///  else if (!image->img_solid)
///    glEnable(GL_ALPHA_TEST);
 
	if (colmap)
		V_GetColmapRGB(colmap, &r, &g, &b, true);

	glColor4f(r, g, b, alpha);

	glBegin(GL_QUADS);
  
	glTexCoord2f(tx1, 1.0f - ty1);
	glVertex2i(x1, SCREENHEIGHT - y1);

	glTexCoord2f(tx2, 1.0f - ty1); 
	glVertex2i(x2, SCREENHEIGHT - y1);
  
	glTexCoord2f(tx2, 1.0f - ty2);
	glVertex2i(x2, SCREENHEIGHT - y2);
  
	glTexCoord2f(tx1, 1.0f - ty2);
	glVertex2i(x1, SCREENHEIGHT - y2);
  
	glEnd();

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_BLEND);

	W_ImageDone(cim);
}


//
// RGL_SolidBox
//
void RGL_SolidBox(int x, int y, int w, int h, rgbcol_t colour, float alpha)
{
	if (alpha < 0.99f)
		glEnable(GL_BLEND);
  
	glColor4f(RGB_RED(colour), RGB_GRN(colour), RGB_BLU(colour), alpha);
  
	glBegin(GL_QUADS);

	glVertex2i(x,   (SCREENHEIGHT - y));
	glVertex2i(x,   (SCREENHEIGHT - y - h));
	glVertex2i(x+w, (SCREENHEIGHT - y - h));
	glVertex2i(x+w, (SCREENHEIGHT - y));
  
	glEnd();
	glDisable(GL_BLEND);
}

//
// RGL_SolidLine
//
void RGL_SolidLine(int x1, int y1, int x2, int y2, int colour)
{
	glColor3f(PAL_RED(colour), PAL_GRN(colour), PAL_BLU(colour));
  
	glBegin(GL_LINES);

	glVertex2i(x1, (SCREENHEIGHT - y1));
	glVertex2i(x2, (SCREENHEIGHT - y2));
  
	glEnd();
}

//
// RGL_ReadScreen
//
// -ACB- 29/04/2002: Implementation
//
void RGL_ReadScreen(int x, int y, int w, int h, byte *rgb_buffer)
{
	glReadBuffer(GL_FRONT);
	glPixelZoom(1.0f, 1.0f);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	// output needs to be top-down, but GL coords are bottom-up.
	for (; h > 0; h--, y++)
	{
		glReadPixels(x, y, w, 1, GL_RGB, GL_UNSIGNED_BYTE, rgb_buffer);

		rgb_buffer += w * 3;
	}
}


//
// RGL_CheckExtensions
//
// Based on code by Bruce Lewis.
//
void RGL_CheckExtensions(void)
{
	// -ACB- 2004/08/11 Made local: these are not yet used elsewhere
	epi::strent_c glstr_vendor;
	epi::strent_c glstr_renderer;
	epi::strent_c glstr_version;
	epi::strent_c glstr_extensions;

	epi::string_c s;
	
	glstr_version.Set((const char*)glGetString(GL_VERSION));
	I_Printf("OpenGL: Version: %s\n", glstr_version.GetString());

	glstr_vendor.Set((const char*)glGetString(GL_VENDOR));
	I_Printf("OpenGL: Vendor: %s\n", glstr_vendor.GetString());

	glstr_renderer.Set((const char*)glGetString(GL_RENDERER));
	I_Printf("OpenGL: Renderer: %s\n", glstr_renderer.GetString());

	// Check for a windows software renderer
	s = glstr_vendor.GetString();
	if (s.CompareNoCase("Microsoft Corporation") == 0)
	{
		s = glstr_renderer.GetString();
		if (s.CompareNoCase("GDI Generic") == 0)
		{
			I_Error("OpenGL: SOFTWARE Renderer!\n");
			//glcap_hardware = false;
		}		
	}
	
	glstr_extensions.Set((const char*)glGetString(GL_EXTENSIONS));
	
	if ((strstr(glstr_extensions, "ARB_multitexture") != NULL) ||
		(strstr(glstr_extensions, "EXT_multitexture") != NULL))
	{
		I_Printf("OpenGL: Multitexture extension found.\n");
		glcap_multitex = true;
	}

	if (strstr(glstr_extensions, "EXT_paletted_texture") != NULL)
	{
		I_Printf("OpenGL: Paletted texture extension found.\n");
		glcap_paletted = true;
	}

	// -AJA- FIXME: temp hack, improve extension handling after 1.28
	if (glstr_version[0] >= '2' ||
	    glstr_version[0] == '1' && glstr_version[1] == '.' &&
		glstr_version[2] >= '2')
	{
		glcap_edgeclamp = true;
	}
	else if (strstr(glstr_extensions, "GL_EXT_texture_edge_clamp") != NULL ||
	         strstr(glstr_extensions, "GL_SGIS_texture_edge_clamp") != NULL)
	{
		I_Printf("OpenGL: EdgeClamp extension found.\n");
		glcap_edgeclamp = true;
	}

	// --- Detect buggy drivers, enable workarounds ---
	// FIXME: put the driver specifics into a table.

	if (strstr(glstr_renderer, "Radeon") != NULL)
	{
		I_Printf("OpenGL: Enabling workarounds for Radeon card.\n");
		use_lighting = false;
		use_color_material = false;
	}
	else if (strstr(glstr_renderer, "TNT2") != NULL)
	{
		I_Printf("OpenGL: Enabling workarounds for TNT2 card.\n");
		use_color_material = false;
		use_vertex_array = false;
	}
	else if (strstr(glstr_renderer, "Velocity") != NULL)
	{
		I_Printf("OpenGL: Enabling workarounds for Velocity card.\n");
		use_color_material = false;
		use_vertex_array = false;
		dumb_sky = true;
	}
	else if (strstr(glstr_renderer, "Voodoo3") != NULL)
	{
		I_Printf("OpenGL: Enabling workarounds for Voodoo3 card.\n");
		use_vertex_array = false;
		dumb_sky = true;
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

	glHint(GL_FOG_HINT, GL_FASTEST);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
}

//
// RGL_Init
//
void RGL_Init(void)
{
	I_Printf("OpenGL: Initialising...\n");

	RGL_SoftInit();
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
  
	R2_InitUtil();

	// initialise unit system
	RGL_InitUnits();

	RGL_SetupMatrices2D();
}

static void ProgressSection(const byte *logo_lum, int lw, int lh,
	const byte *text_lum, int tw, int th,
	float cr, float cg, float cb,
	int *y, int perc, float alpha)
{
	float zoom = 1.0f;

	(*y) -= (int)(lh * zoom);

	glRasterPos2i(20, *y);
	glPixelZoom(zoom, zoom);
	glDrawPixels(lw, lh, GL_LUMINANCE, GL_UNSIGNED_BYTE, logo_lum);

	(*y) -= th + 20;

	glRasterPos2i(20, *y);
	glPixelZoom(1.0f, 1.0f);
	glDrawPixels(tw, th, GL_LUMINANCE, GL_UNSIGNED_BYTE, text_lum);

	int px = 20;
	int pw = SCREENWIDTH - 80;
	int ph = 30;
	int py = *y - ph - 20;

	int x = (pw-6) * perc / 100;

	glColor4f(1.0f, 1.0f, 1.0f, alpha);
	glBegin(GL_LINE_LOOP);
	glVertex2i(px, py);  glVertex2i(px, py+ph);
	glVertex2i(px+pw, py+ph); glVertex2i(px+pw, py);
	glVertex2i(px, py);
	glEnd();

	glColor4f(cr, cg, cb, alpha);
	glBegin(GL_POLYGON);
	glVertex2i(px+3, py+3);  glVertex2i(px+3, py+ph-4);
	glVertex2i(px+3+x, py+ph-4); glVertex2i(px+3+x, py+3);
	glEnd();

	(*y) = py;
}

//
// RGL_DrawProgress
//
// Show EDGE logo and a progress indicator.
//
void RGL_DrawProgress(int perc, int glbsp_perc)
{
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glEnable(GL_BLEND);

	int y = SCREENHEIGHT - 20;
	
	const byte *logo_lum; int lw, lh;
	const byte *text_lum; int tw, th;

	logo_lum = RGL_LogoImage(&lw, &lh);
	text_lum = RGL_InitImage(&tw, &th);

	ProgressSection(logo_lum, lw, lh, text_lum, tw, th,
		0.4f, 0.6f, 1.0f, &y, perc, 1.0f);

	y -= 40;

	if (glbsp_perc >= 0 || glbsp_last_prog_time > 0)
	{
		// logic here is to avoid the brief flash of progress
		int tim = I_GetTime();
		float alpha = 1.0f;

		if (glbsp_perc >= 0)
			glbsp_last_prog_time = tim;
		else
		{
			alpha = 1.0f - float(tim - glbsp_last_prog_time) / (TICRATE*3/2);

			if (alpha < 0)
			{
				alpha = 0;
				glbsp_last_prog_time = 0;
			}

			glbsp_perc = 100;
		}

		logo_lum = RGL_GlbspImage(&lw, &lh);
		text_lum = RGL_BuildImage(&tw, &th);

		ProgressSection(logo_lum, lw, lh, text_lum, tw, th,
			1.0f, 0.2f, 0.1f, &y, glbsp_perc, alpha);
	}

	glDisable(GL_BLEND);

	I_FinishFrame();
	I_StartFrame();
}

//
// RGL_DrawBeta
//
void RGL_DrawBeta(void)
{
	int bw, bh;

	const byte *beta_lum = RGL_BetaImage(&bw, &bh);

	float zoom = SCREENWIDTH / 800.0f;

	int x = SCREENWIDTH  - (int)(bw * zoom);
	int y = SCREENHEIGHT - (int)(bh * zoom);

	glRasterPos2i(x, y);
	glPixelZoom(zoom, zoom);
	glDrawPixels(bw, bh, GL_LUMINANCE, GL_UNSIGNED_BYTE, beta_lum);
}

