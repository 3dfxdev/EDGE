//----------------------------------------------------------------------------
//  EDGE OpenGL Rendering (Main Stuff)
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2002  The EDGE Team.
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
#ifdef USE_GL

#include "i_defs.h"

#include "am_map.h"
#include "dm_defs.h"
#include "dm_state.h"
#include "e_search.h"
#include "m_bbox.h"
#include "m_random.h"
#include "p_local.h"
#include "p_mobj.h"
#include "r_defs.h"
#include "r_main.h"
#include "r_plane.h"
#include "r_sky.h"
#include "r_state.h"
#include "r_things.h"
#include "r2_defs.h"
#include "rgl_defs.h"
#include "v_colour.h"
#include "v_ctx.h"
#include "v_res.h"
#include "w_textur.h"
#include "w_wad.h"
#include "z_zone.h"


#define DEBUG  0


// OpenGL video context
video_context_t vctx;


// implementation limits

int glmax_lights;
int glmax_clip_planes;
int glmax_tex_size;

int rgl_light_map[256];
static lighting_model_e rgl_light_model = LMODEL_Invalid;

char *glstr_vendor = NULL;
char *glstr_renderer = NULL;
char *glstr_version = NULL;
char *glstr_extensions = NULL;

bool glcap_hardware = true;
bool glcap_multitex = false;
bool glcap_paletted = false;

angle_t oned_side_angle;


int glpar_invuln = 0;

bool ren_allbright;
float ren_red_mul;
float ren_grn_mul;
float ren_blu_mul;

extern int rgl_currtimeval;  // hack..


#define RGB_RED(rgbcol)  ((float)((rgbcol >> 16) & 0xFF) / 255.0)
#define RGB_GRN(rgbcol)  ((float)((rgbcol >>  8) & 0xFF) / 255.0)
#define RGB_BLU(rgbcol)  ((float)((rgbcol      ) & 0xFF) / 255.0)

#define PAL_RED(pix)  ((float)(playpal_data[0][pix][0]) / 255.0)
#define PAL_GRN(pix)  ((float)(playpal_data[0][pix][1]) / 255.0)
#define PAL_BLU(pix)  ((float)(playpal_data[0][pix][2]) / 255.0)

#define LT_RED(light)  (MIN(255,light) * ren_red_mul / 255.0)
#define LT_GRN(light)  (MIN(255,light) * ren_grn_mul / 255.0)
#define LT_BLU(light)  (MIN(255,light) * ren_blu_mul / 255.0)


static void SetupLightMap(int model)
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
		//    [72,112] --> [16,48]
		//    [112,256] --> [48,240]   (bit of room for DLight)

		if (i <= 72)
			rgl_light_map[i] = i * 2 / 9;
		else if (i <= 112)
			rgl_light_map[i] = 16 + (i - 72) * 32 / 40;
		else
			rgl_light_map[i] = 48 + (i - 112) * 192 / 144;
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
	glOrtho(0.0, (float)SCREENWIDTH, 
			0.0, (float)SCREENHEIGHT, -1.0, 1.0);

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

#define Z_NEAR  1.0
#define Z_FAR   32000.0

void RGL_SetupMatrices3D(void)
{
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
	glRotatef(270.0 - atan(viewvertangle) * 180.0 / M_PI, 1.0, 0.0, 0.0);
	glRotatef(90.0 - ANG_2_FLOAT(viewangle), 0.0, 0.0, 1.0);
	glTranslatef(-viewx, -viewy, -viewz);

	// turn on lighting.  Some drivers (e.g. TNT2) don't work properly
	// without it.
	glEnable(GL_LIGHTING);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	DEV_ASSERT2(currentmap);
	if (currentmap->lighting != rgl_light_model)
		SetupLightMap(currentmap->lighting);
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
	if (trans <= 0.99)
		glEnable(GL_BLEND);
	else
		glEnable(GL_ALPHA_TEST);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, tex_id);

	tex_top_h = 1.00; // 0.98;
	tex_bot_h = 1.00 - bottom;  // 1.02 - bottom;

	tex_x1 = 0.01;
	tex_x2 = right - 0.01;

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
		L_r = rgl_weapon_r + lit_Nom * c_r;
		L_g = rgl_weapon_g + lit_Nom * c_g;
		L_b = rgl_weapon_b + lit_Nom * c_b;
	}
	else
	{
		L_r = L_g = L_b = 255;
	}

	x1b = x1t = (160.0 + tx1) * viewwindowwidth / 320.0;
	x2b = x2t = (160.0 + tx2) * viewwindowwidth / 320.0;

	y1b = y2b = (ty2) * viewwindowheight / 200.0;
	y1t = y2t = (ty1) * viewwindowheight / 200.0;

	if (fuzzy)
	{
		float range_x = fabs(x2b - x1b) / 12.0;
		float range_y = fabs(y1t - y1b) / 12.0;

		float bl_x = sin(rgl_currtimeval / 5.0);
		float tl_x = sin(rgl_currtimeval / 11.0);
		float tr_x = sin(rgl_currtimeval / 7.0);
		float br_x = sin(rgl_currtimeval / 13.0);

		float tl_y = cos(rgl_currtimeval / 11.0);
		float tr_y = cos(rgl_currtimeval / 7.0);
    
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
		weaponinfo_t *w;
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
// RGL_RainbowEffect
//
// Effects that modify all colours, e.g. nightvision green.
//
void RGL_RainbowEffect(player_t *player)
{
	float s;
  
	ren_allbright = false;
	ren_red_mul = ren_grn_mul = ren_blu_mul = 1.0;

	s = player->powers[PW_Invulnerable];  

	if (s > 0)
	{
		s = MIN(128.0, s);
		ren_allbright = true;
		ren_red_mul = ren_grn_mul = ren_blu_mul = s / 256.0;
		return;
	}

	s = player->powers[PW_NightVision];

	if (s > 0)
	{
		s = MIN(128.0, s);
		ren_red_mul = ren_blu_mul = 1.0 - s / 128.0;
		return;
	}

	// do berserk here... (rather than as a palette effect)
}


//
// RGL_ColourmapEffect
//
// For example: all white for invulnerability.
//
void RGL_ColourmapEffect(player_t *player)
{
	int x1, y1;
	int x2, y2;

	if (player->powers[PW_Invulnerable] <= 0)
		return;

	{
		float s = (float) player->powers[PW_Invulnerable];
    
		s = MIN(128.0, s) / 128.0;

		if (glpar_invuln == 0)
		{
			glColor4f(1.0, 1.0, 1.0, 0.0);
			glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ZERO);
		}
		else
		{
			glColor4f(0.75 * s, 0.75 * s, 0.75 * s, 0.0);
			glBlendFunc(GL_ONE, GL_ONE);
		}
	}

	glEnable(GL_BLEND);

	glBegin(GL_QUADS);
  
	x1 = viewwindowx;
	x2 = viewwindowx + viewwindowwidth;

	y1 = SCREENHEIGHT - viewwindowy;
	y2 = SCREENHEIGHT - viewwindowy - viewwindowheight;

	glVertex2i(x1, y1);
	glVertex2i(x2, y1);
	glVertex2i(x2, y2);
	glVertex2i(x1, y2);

	glEnd();
  
	glDisable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}


//
// RGL_PaletteEffect
//
// For example: red wash for pain.
//
void RGL_PaletteEffect(player_t *player)
{
	byte rgb_data[3];
	int rgb_max;

	if (player->powers[PW_Invulnerable] > 0)
		return;

	V_IndexColourToRGB(pal_black, rgb_data);

	rgb_max = MAX(rgb_data[0], MAX(rgb_data[1], rgb_data[2]));

	if (rgb_max == 0)
		return;
  
	rgb_max = MIN(200, rgb_max);

	glColor4f((float) rgb_data[0] / (float) rgb_max,
			  (float) rgb_data[1] / (float) rgb_max,
			  (float) rgb_data[2] / (float) rgb_max,
			  (float) rgb_max / 255.0);

	glEnable(GL_BLEND);

	glBegin(GL_QUADS);
  
	glVertex2i(0, SCREENHEIGHT);
	glVertex2i(SCREENWIDTH, SCREENHEIGHT);
	glVertex2i(SCREENWIDTH, 0);
	glVertex2i(0, 0);

	glEnd();
  
	glDisable(GL_BLEND);
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
}

//
// RGL_BeginDraw
//
void RGL_BeginDraw(int x1, int y1, int x2, int y2)
{
	//!!! set the SCISSORS test (& optimise)

	//!!! optimise: don't setup 2D matrices when already setup
	RGL_SetupMatrices2D();
}

//
// RGL_EndDraw
//
void RGL_EndDraw(void)
{
	//!!! unset the SCISSORS test
}

//
// RGL_DrawImage
//
void RGL_DrawImage(int x, int y, int w, int h, const image_t *image, 
				   float tx1, float ty1, float tx2, float ty2,
				   const colourmap_t *colmap, float alpha)
{
	float r = 1.0, g = 1.0, b = 1.0;

	const cached_image_t *cim = W_ImageCache(image, IMG_OGL, 0, false);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, W_ImageGetOGL(cim));
 
	if (alpha < 0.99 || !image->solid)
		glEnable(GL_BLEND);

///  else if (!image->solid)
///    glEnable(GL_ALPHA_TEST);
  
	if (colmap)
		V_GetColmapRGB(colmap, &r, &g, &b, true);

	glColor4f(r, g, b, alpha);

	glBegin(GL_QUADS);
  
	glTexCoord2f(tx1, 1.0 - ty1);
	glVertex2i(x, SCREENHEIGHT - y);

	glTexCoord2f(tx2, 1.0 - ty1); 
	glVertex2i(x+w, SCREENHEIGHT - y);
  
	glTexCoord2f(tx2, 1.0 - ty2);
	glVertex2i(x+w, SCREENHEIGHT - y - h);
  
	glTexCoord2f(tx1, 1.0 - ty2);
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
	if (alpha < 0.99)
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
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glReadPixels(x, SCREENHEIGHT-y, w, h, GL_RGB, GL_UNSIGNED_BYTE, rgb_buffer);
}


//
// RGL_CheckExtensions
//
// Based on code by Bruce Lewis.
//
void RGL_CheckExtensions(void)
{
	const GLubyte *str;

	str = glGetString(GL_VERSION);
	glstr_version = Z_StrDup((char*)str);
	I_Printf("OpenGL: Version: %s\n", glstr_version);

	str = glGetString(GL_VENDOR);
	glstr_vendor = Z_StrDup((char*)str);
	I_Printf("OpenGL: Vendor: %s\n", glstr_vendor);

	glstr_renderer = Z_StrDup((char*)glGetString(GL_RENDERER));
	I_Printf("OpenGL: Renderer: %s\n", glstr_renderer);

	if (DDF_CompareName(glstr_vendor, "Microsoft Corporation") == 0 &&
		DDF_CompareName(glstr_renderer, "GDI Generic") == 0)
	{
		I_Error("OpenGL: SOFTWARE Renderer !\n");
		// glcap_hardware = false;
	}

	glstr_extensions = Z_StrDup((char*)glGetString(GL_EXTENSIONS));

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
	glAlphaFunc(GL_GREATER, 1.0 / 32.0);

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
	glGetIntegerv(GL_MAX_LIGHTS,       &glmax_lights);
	glGetIntegerv(GL_MAX_CLIP_PLANES,  &glmax_clip_planes);
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &glmax_tex_size);

	I_Printf("OpenGL: Lights: %d  Clippers: %d  Max_tex: %d\n",
			 glmax_lights, glmax_clip_planes, glmax_tex_size);
  
	R2_InitUtil();

	// initialise unit system
	RGL_InitUnits();

	RGL_SetupMatrices2D();

	// setup video context

	vctx.double_buffered = true;

	vctx.NewScreenSize = RGL_NewScreenSize;
	vctx.RenderScene = RGL_RenderScene;
	vctx.BeginDraw = RGL_BeginDraw;
	vctx.EndDraw = RGL_EndDraw;
	vctx.DrawImage = RGL_DrawImage;
	vctx.SolidBox = RGL_SolidBox;
	vctx.SolidLine = RGL_SolidLine;
	vctx.ReadScreen = RGL_ReadScreen;
}


#endif  // USE_GL
