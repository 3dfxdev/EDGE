//----------------------------------------------------------------------------
//  EDGE 2D DRAWING STUFF
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2009  The EDGE Team.
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

#include "ddf/font.h"

#include "g_game.h"
#include "r_misc.h"
#include "r_gldefs.h"
#include "r_units.h"
#include "r_colormap.h"
#include "hu_draw.h"
#include "r_modes.h"
#include "r_image.h"


#define DUMMY_WIDTH(font)  (4)

#define HU_CHAR(ch)  (islower(ch) ? toupper(ch) : (ch))
#define HU_INDEX(c)  ((unsigned char) HU_CHAR(c))

static font_c *default_font;


// current state
static float cur_coord_W;
static float cur_coord_H;

static font_c *cur_font;
static rgbcol_t cur_color;
static float cur_scale;
static float cur_alpha;

#define COORD_X(x)  ((x) * SCREENWIDTH  / cur_coord_W)
#define COORD_Y(y)  ((y) * SCREENHEIGHT / cur_coord_H)


static int glbsp_last_prog_time = 0;


void HUD_SetCoordSys(int width, int height)
{
	cur_coord_W = width;
	cur_coord_H = height;
}

void HUD_SetFont(font_c *font)
{
	SYS_ASSERT(font);

	cur_font = font;
}

void HUD_SetTextScale(float scale)
{
	cur_scale = scale;
}

void HUD_SetTextColor(rgbcol_t color)
{
	cur_color = color;
}

void HUD_SetAlpha(float alpha)
{
	cur_alpha = alpha;
}

void HUD_Reset(const char *what)
{
	for (; *what; what++)
	{
		switch (*what)
		{
			case 'f': cur_font  = default_font; break;
			case 's': cur_scale = 1.0f; break;
			case 'c': cur_color = RGB_NO_VALUE; break;
			case 'a': cur_alpha = 1.0f; break;
		}
	}
}


void HUD_FrameSetup(void)
{
	if (! default_font)
	{
		// FIXME: get default font from DDF gamedef
		fontdef_c *DEF = fontdefs.Lookup("DOOM");
		SYS_ASSERT(DEF);

		default_font = HU_LookupFont(DEF);
		SYS_ASSERT(default_font);
	}

	cur_coord_W = 320.0f;
	cur_coord_H = 200.0f;

	HUD_Reset();
}


void HL_Init(void)
{
	/* nothing to init */
}


void RGL_DrawImage(float x, float y, float w, float h, const image_c *image, 
				   float tx1, float ty1, float tx2, float ty2,
				   float alpha, rgbcol_t text_col,
				   const colourmap_c *palremap)
{
	int x1 = I_ROUND(x);
	int y1 = I_ROUND(y);
	int x2 = I_ROUND(x+w+0.25f);
	int y2 = I_ROUND(y+h+0.25f);

	if (x1 == x2 || y1 == y2)
		return;

	float r = 1.0f, g = 1.0f, b = 1.0f;

	if (text_col != RGB_NO_VALUE)
	{
		r = RGB_RED(text_col) / 255.0;
		g = RGB_GRN(text_col) / 255.0;
		b = RGB_BLU(text_col) / 255.0;

		palremap = font_whiten_map;
	}

	GLuint tex_id = W_ImageCache(image, true, palremap);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, tex_id);
 
	if (alpha >= 0.99f && image->opacity == OPAC_Solid)
		glDisable(GL_ALPHA_TEST);
	else
	{
		glEnable(GL_ALPHA_TEST);

		if (! (alpha < 0.11f || image->opacity == OPAC_Complex))
			glAlphaFunc(GL_GREATER, alpha * 0.66f);
	}

	if (image->opacity == OPAC_Complex || alpha < 0.99f)
		glEnable(GL_BLEND);

	glColor4f(r, g, b, alpha);

	glBegin(GL_QUADS);
  
	glTexCoord2f(tx1, ty1);
	glVertex2i(x1, y1);

	glTexCoord2f(tx2, ty1); 
	glVertex2i(x2, y1);
  
	glTexCoord2f(tx2, ty2);
	glVertex2i(x2, y2);
  
	glTexCoord2f(tx1, ty2);
	glVertex2i(x1, y2);
  
	glEnd();


	glDisable(GL_TEXTURE_2D);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_BLEND);

	glAlphaFunc(GL_GREATER, 0);
}


void RGL_Image320(float x, float y, float w, float h, const image_c *image)
{
    RGL_DrawImage(
			FROM_320(x-IM_OFFSETX(image)),
            SCREENHEIGHT - FROM_200(y-IM_OFFSETY(image)) - FROM_200(h),
            FROM_320(w), FROM_200(h), image,
			0, 0, IM_RIGHT(image), IM_TOP(image));
}

void RGL_ImageEasy320(float x, float y, const image_c *image)
{
    RGL_Image320(x, y, IM_WIDTH(image), IM_HEIGHT(image), image);
}


void RGL_SolidBox(int x, int y, int w, int h, rgbcol_t col, float alpha)
{
	if (alpha < 0.99f)
		glEnable(GL_BLEND);
  
	glColor4f(RGB_RED(col)/255.0, RGB_GRN(col)/255.0, RGB_BLU(col)/255.0, alpha);
  
	glBegin(GL_QUADS);

	glVertex2i(x,   y);
	glVertex2i(x,   y+h);
	glVertex2i(x+w, y+h);
	glVertex2i(x+w, y);
  
	glEnd();
	glDisable(GL_BLEND);
}

void RGL_SolidLine(int x1, int y1, int x2, int y2, rgbcol_t col, float alpha)
{
	if (alpha < 0.99f)
		glEnable(GL_BLEND);
  
	glColor4f(RGB_RED(col)/255.0, RGB_GRN(col)/255.0, RGB_BLU(col)/255.0, alpha);
  
	glBegin(GL_LINES);

	glVertex2i(x1, y1);
	glVertex2i(x2, y2);
  
	glEnd();
	glDisable(GL_BLEND);
}


void RGL_ThinBox(int x, int y, int w, int h, rgbcol_t col, float alpha)
{
	if (alpha < 0.99f)
		glEnable(GL_BLEND);
  
	glColor4f(RGB_RED(col)/255.0, RGB_GRN(col)/255.0, RGB_BLU(col)/255.0, alpha);
  
	glBegin(GL_QUADS);
	glVertex2i(x,   y);   glVertex2i(x,   y+h);
	glVertex2i(x+2, y+h); glVertex2i(x+2, y);
	glEnd();

	glBegin(GL_QUADS);
	glVertex2i(x+w-2, y);   glVertex2i(x+w-2, y+h);
	glVertex2i(x+w,   y+h); glVertex2i(x+w,   y);
	glEnd();

	glBegin(GL_QUADS);
	glVertex2i(x+2,   y);   glVertex2i(x+2,   y+2);
	glVertex2i(x+w-2, y+2); glVertex2i(x+w-2, y);
	glEnd();

	glBegin(GL_QUADS);
	glVertex2i(x+2,   y+h-2); glVertex2i(x+2,   y+h);
	glVertex2i(x+w-2, y+h);   glVertex2i(x+w-2, y+h-2);
	glEnd();

	glDisable(GL_BLEND);
}

void RGL_GradientBox(int x, int y, int w, int h, rgbcol_t *cols, float alpha)
{
	if (alpha < 0.99f)
		glEnable(GL_BLEND);
  
	glBegin(GL_QUADS);

	glColor4f(RGB_RED(cols[1])/255.0, RGB_GRN(cols[1])/255.0,
	          RGB_BLU(cols[1])/255.0, alpha);
	glVertex2i(x, y);

	glColor4f(RGB_RED(cols[0])/255.0, RGB_GRN(cols[0])/255.0,
	          RGB_BLU(cols[0])/255.0, alpha);
	glVertex2i(x, y+h);

	glColor4f(RGB_RED(cols[2])/255.0, RGB_GRN(cols[2])/255.0,
	          RGB_BLU(cols[2])/255.0, alpha);
	glVertex2i(x+w, y+h);

	glColor4f(RGB_RED(cols[3])/255.0, RGB_GRN(cols[3])/255.0,
	          RGB_BLU(cols[3])/255.0, alpha);
	glVertex2i(x+w, y);
  
	glEnd();
	glDisable(GL_BLEND);
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

	int x = (pw-8) * perc / 100;

	glColor4f(0.6f, 0.6f, 0.6f, alpha);
	glBegin(GL_POLYGON);
	glVertex2i(px, py);  glVertex2i(px, py+ph);
	glVertex2i(px+pw, py+ph); glVertex2i(px+pw, py);
	glVertex2i(px, py);
	glEnd();

	glColor4f(0.0f, 0.0f, 0.0f, alpha);
	glBegin(GL_POLYGON);
	glVertex2i(px+2, py+2);  glVertex2i(px+2, py+ph-2);
	glVertex2i(px+pw-2, py+ph-2); glVertex2i(px+pw-2, py+2);
	glEnd();

	glColor4f(cr, cg, cb, alpha);
	glBegin(GL_POLYGON);
	glVertex2i(px+4, py+4);  glVertex2i(px+4, py+ph-4);
	glVertex2i(px+4+x, py+ph-4); glVertex2i(px+4+x, py+4);
	glEnd();

	(*y) = py;
}


void RGL_DrawProgress(int perc, int glbsp_perc)
{
	/* show EDGE logo and a progress indicator */

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
// Write a string using the hu_font and index translator.
//
void HL_WriteTextTrans(style_c *style, int text_type, int x, int y,
	rgbcol_t col, const char *str, float scale, float alpha)
{
	float cx = x;
	float cy = y;

	font_c *font = style->fonts[text_type];

	scale *= style->def->text[text_type].scale;

	if (! font)
		I_Error("Style [%s] is missing a font !\n", style->def->ddf.name.c_str());

	for (; *str; str++)
	{
		char ch = *str;

		if (ch == '\n')
		{
			cx = x;
			cy += 12.0f * scale;  // FIXME: use font's height
			continue;
		}

		if (cx >= 320.0f)
			continue;

		font->DrawChar320(cx, cy, ch, scale,1.0f, col, alpha);

		cx += font->CharWidth(ch) * scale;
	}
}

//
// Write a string using the hu_font.
//
void HL_WriteText(style_c *style, int text_type, int x, int y, const char *str, float scale, float alpha)
{
	HL_WriteTextTrans(style, text_type, x, y,
			V_GetFontColor(style->def->text[text_type].colmap),
			str, scale, alpha);
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
