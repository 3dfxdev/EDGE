//----------------------------------------------------------------------------
//  EDGE OpenGL Rendering (Main Stuff)
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2004  The EDGE Team.
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

#include "epi/epistring.h"

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


// FIXME: !!!
extern bool ren_allbright;
extern float ren_red_mul;
extern float ren_grn_mul;
extern float ren_blu_mul;

angle_t fuzz_ang_tl;
angle_t fuzz_ang_tr;
angle_t fuzz_ang_bl;
angle_t fuzz_ang_br;


// -AJA- FIXME: temp hack
#ifndef GL_MAX_TEXTURE_UNITS
#define GL_MAX_TEXTURE_UNITS  0x84E2
#endif


#define Z_NEAR  1.0f
#define Z_FAR   200000.0f

#define RGB_RED(rgbcol)  ((float)((rgbcol >> 16) & 0xFF) / 255.0f)
#define RGB_GRN(rgbcol)  ((float)((rgbcol >>  8) & 0xFF) / 255.0f)
#define RGB_BLU(rgbcol)  ((float)((rgbcol      ) & 0xFF) / 255.0f)

#define PAL_RED(pix)  ((float)(playpal_data[0][pix][0]) / 255.0f)
#define PAL_GRN(pix)  ((float)(playpal_data[0][pix][1]) / 255.0f)
#define PAL_BLU(pix)  ((float)(playpal_data[0][pix][2]) / 255.0f)

#define LT_RED(light)  (MIN(255,light) * ren_red_mul / 255.0f)
#define LT_GRN(light)  (MIN(255,light) * ren_grn_mul / 255.0f)
#define LT_BLU(light)  (MIN(255,light) * ren_blu_mul / 255.0f)


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

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	DEV_ASSERT2(currmap);
	if (currmap->lighting != rgl_light_model)
		SetupLightMap(currmap->lighting);
}

void RGL_UpdateTheFuzz(void)
{
	// fuzzy warping effect

	fuzz_ang_tl += FLOAT_2_ANG(90.0f / 17.0f);
	fuzz_ang_tr += FLOAT_2_ANG(90.0f / 11.0f);
	fuzz_ang_bl += FLOAT_2_ANG(90.0f /  8.0f);
	fuzz_ang_br += FLOAT_2_ANG(90.0f / 21.0f);
}

//
// RGL_DrawPSprite
//
static void RGL_DrawPSprite(pspdef_t * psp, int which,
							player_t * player, region_properties_t *props, const state_t *state)
{
	const image_t *image;
	const cached_image_t *cim;
	bool flip;

	int fuzzy = (player->mo->flags & MF_FUZZY);

	float x1b, y1b, x1t, y1t, x2b, y2b, x2t, y2t;  // screen coords
	float tx1, ty1, tx2, ty2;  // texture coords

	float tex_x1, tex_x2;
	float tex_top_h, tex_bot_h;
	float trans;

	GLuint tex_id;
	int w, h;
	float right, bottom;

	int lit_Nom, L_r, L_g, L_b;
	float c_r, c_g, c_b;

	lit_Nom = (ren_allbright || state->bright) ? 240 :
		(props->lightlevel * 240 / 255);

	if (effect_infrared)
		lit_Nom += (int)(effect_strength * 255);

	lit_Nom = GAMMA_CONV(MIN(255,lit_Nom));

	// determine sprite patch
	image = R2_GetOtherSprite(state->sprite, state->frame, &flip);

	if (!image)
		return;

	cim = W_ImageCache(image, IMG_OGL, 0, true);

	tex_id = W_ImageGetOGL(cim);

	w = IM_WIDTH(image);
	h = IM_HEIGHT(image);
	right = IM_RIGHT(image);
	bottom = IM_BOTTOM(image);

	trans = fuzzy ? FUZZY_TRANS : player->mo->visibility;
	trans *= psp->visibility;

	// psprites are never totally opaque
	if (trans <= 0.99f)
		glEnable(GL_BLEND);
	else
		glEnable(GL_ALPHA_TEST);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, tex_id);

	tex_top_h = 1.00f; // 0.98;
	tex_bot_h = 1.00f - bottom;  // 1.02 - bottom;

	tex_x1 = 0.01f;
	tex_x2 = right - 0.01f;

	if (flip)
	{
		tex_x1 = right - tex_x1;
		tex_x2 = right - tex_x2;
	}

	tx1 = psp->sx - BASEXCENTER - IM_OFFSETX(image);
	tx2 = tx1 + w;

	ty1 = psp->sy - IM_OFFSETY(image);
	ty2 = ty1 + h;

	// compute lighting

	V_GetColmapRGB(props->colourmap, &c_r, &c_g, &c_b, false);

	if (which == ps_weapon || which == ps_flash)
	{
		L_r = (int)(rgl_weapon_r + lit_Nom * c_r);
		L_g = (int)(rgl_weapon_g + lit_Nom * c_g);
		L_b = (int)(rgl_weapon_b + lit_Nom * c_b);
	}
	else
	{
		L_r = L_g = L_b = 255;
	}

	x1b = x1t = (160.0f + tx1) * viewwindowwidth / 320.0f;
	x2b = x2t = (160.0f + tx2) * viewwindowwidth / 320.0f;

	y1b = y2b = (ty2) * viewwindowheight / 200.0f;
	y1t = y2t = (ty1) * viewwindowheight / 200.0f;

	if (fuzzy)
	{
		float range_x = (float)fabs(x2b - x1b) / 12.0f;
		float range_y = (float)fabs(y1t - y1b) / 12.0f;

		float tl_x = M_Sin(fuzz_ang_tl);
		float tr_x = M_Sin(fuzz_ang_tr);
		float bl_x = M_Sin(fuzz_ang_bl);
		float br_x = M_Sin(fuzz_ang_br);

		float tl_y = M_Cos(fuzz_ang_tl);
		float tr_y = M_Cos(fuzz_ang_tr);
    
		// don't adjust the bottom Y positions
    
		x1b += bl_x * range_x;
		x1t += tl_x * range_x; y1t += tl_y * range_y;
		x2t += tr_x * range_x; y2t += tr_y * range_y;
		x2b += br_x * range_x;

		L_r = L_g = L_b = 0;
	}

	// clip psprite to view window
	glEnable(GL_SCISSOR_TEST);

	glScissor(viewwindowx, SCREENHEIGHT-viewwindowheight-viewwindowy,
			  viewwindowwidth, viewwindowheight);

	x1b = (float)viewwindowx + x1b;
	x1t = (float)viewwindowx + x1t;
	x2t = (float)viewwindowx + x2t;
	x2b = (float)viewwindowx + x2b;

	y1b = (float)(SCREENHEIGHT - viewwindowy) - y1b - 1;
	y1t = (float)(SCREENHEIGHT - viewwindowy) - y1t - 1;
	y2t = (float)(SCREENHEIGHT - viewwindowy) - y2t - 1;
	y2b = (float)(SCREENHEIGHT - viewwindowy) - y2b - 1;
 
	glColor4f(LT_RED(L_r), LT_GRN(L_g), LT_BLU(L_b), trans);

	glBegin(GL_QUADS);
  
	glTexCoord2f(tex_x1, tex_bot_h);
	glVertex2f(x1b, y1b);
  
	glTexCoord2f(tex_x1, tex_top_h);
	glVertex2f(x1t, y1t);
  
	glTexCoord2f(tex_x2, tex_top_h);
	glVertex2f(x2t, y1t);
  
	glTexCoord2f(tex_x2, tex_bot_h);
	glVertex2f(x2b, y2b);
  
	glEnd();

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_BLEND);
	glDisable(GL_SCISSOR_TEST);

	W_ImageDone(cim);
}


//
// RGL_DrawPlayerSprites
//
void RGL_DrawPlayerSprites(player_t * p)
{
	int i;

	DEV_ASSERT2(p->in_game);

	// special handling for zoom: show viewfinder
	if (viewiszoomed)
	{
		weapondef_c *w;
		pspdef_t *psp = &p->psprites[ps_weapon];

		if ((p->ready_wp < 0) || (psp->state == S_NULL))
			return;

		w = p->weapons[p->ready_wp].info;

		if (w->zoom_state <= 0)
			return;

		RGL_DrawPSprite(psp, ps_weapon, p, view_props,
						states + w->zoom_state);
    
		return;
	}

	// add all active psprites
	// Note: order is significant

	for (i = 0; i < NUMPSPRITES; i++)
	{
		pspdef_t *psp = &p->psprites[i];

		if (psp->state == S_NULL)
			continue;

		RGL_DrawPSprite(psp, i, p, view_props, psp->state);
	}
}


//
//  VIDEO CONTEXT STUFF
//

//
// RGL_NewScreenSize
//
void RGL_NewScreenSize(int width, int height, int bpp)
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
void RGL_DrawImage(int x, int y, int w, int h, const image_t *image, 
				   float tx1, float ty1, float tx2, float ty2,
				   const colourmap_c *colmap, float alpha)
{
	float r = 1.0f, g = 1.0f, b = 1.0f;

	const cached_image_t *cim = W_ImageCache(image, IMG_OGL, 0, false,
		(colmap && (colmap->special & COLSP_Whiten)) ? font_whiten_map : NULL);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, W_ImageGetOGL(cim));
 
	if (alpha < 0.99f || !image->solid)
		glEnable(GL_BLEND);

///  else if (!image->solid)
///    glEnable(GL_ALPHA_TEST);
 
	if (colmap)
		V_GetColmapRGB(colmap, &r, &g, &b, true);

	glColor4f(r, g, b, alpha);

	glBegin(GL_QUADS);
  
	glTexCoord2f(tx1, 1.0f - ty1);
	glVertex2i(x, SCREENHEIGHT - y);

	glTexCoord2f(tx2, 1.0f - ty1); 
	glVertex2i(x+w, SCREENHEIGHT - y);
  
	glTexCoord2f(tx2, 1.0f - ty2);
	glVertex2i(x+w, SCREENHEIGHT - y - h);
  
	glTexCoord2f(tx1, 1.0f - ty2);
	glVertex2i(x, SCREENHEIGHT - y - h);
  
	glEnd();

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_BLEND);

	W_ImageDone(cim);
}


//
// RGL_SolidBox
//
void RGL_SolidBox(int x, int y, int w, int h, int colour, float alpha)
{
	if (alpha < 0.99f)
		glEnable(GL_BLEND);
  
	glColor4f(PAL_RED(colour), PAL_GRN(colour), PAL_BLU(colour), alpha);
  
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

//
// RGL_DrawProgress
//
// Show EDGE logo and a progress indicator.
//
void RGL_DrawProgress(int perc)
{
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	int w, h, y;
	const byte *logo_lum = RGL_LogoImage(&w, &h);

	// don't make logo bigger in 320x200 or 320x240
	float zoom = 1.0f; // (SCREENWIDTH < 600) ? 1.0f : 2.0f;

	y = SCREENHEIGHT - 20;
	y -= (int)(h * zoom);

	glRasterPos2i(20, y);
	glPixelZoom(zoom, zoom);
	glDrawPixels(w, h, GL_LUMINANCE, GL_UNSIGNED_BYTE, logo_lum);
	
	logo_lum = RGL_InitImage(&w, &h);
	y -= 20 + h;

	glRasterPos2i(20, y);
	glPixelZoom(1.0f, 1.0f);
	glDrawPixels(w, h, GL_LUMINANCE, GL_UNSIGNED_BYTE, logo_lum);

	int px = 20;
	int pw = SCREENWIDTH - 80;
	int py = y - 30 - 20;
	int ph = 30;

	int x = (pw-4) * perc / 100;

	glColor3f(1.0f, 1.0f, 1.0f);
	glBegin(GL_LINE_LOOP);
	glVertex2i(px, py);  glVertex2i(px, py+ph);
	glVertex2i(px+pw, py+ph); glVertex2i(px+pw, py);
	glVertex2i(px, py);
	glEnd();

	glColor3f(0.4f, 0.6f, 1.0f);
	glBegin(GL_POLYGON);
	glVertex2i(px+2, py+2);  glVertex2i(px+2, py+ph-3);
	glVertex2i(px+2+x, py+ph-3); glVertex2i(px+2+x, py+2);
	glEnd();

	I_FinishFrame();
	I_StartFrame();
}

