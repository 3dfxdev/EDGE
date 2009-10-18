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
#include "r_misc.h"     //  R_Render


cvar_c am_smoothing;


#define DUMMY_WIDTH(font)  (4)

#define HU_CHAR(ch)  (islower(ch) ? toupper(ch) : (ch))
#define HU_INDEX(c)  ((unsigned char) HU_CHAR(c))

static font_c *default_font;


// current state
static float cur_coord_W;
static float cur_coord_H;

static font_c *cur_font;
static rgbcol_t cur_color;

static float cur_scale, cur_alpha;
static int cur_x_align, cur_y_align;

#define COORD_X(x)  ((x) * SCREENWIDTH  / cur_coord_W)
#define COORD_Y(y)  ((y) * SCREENHEIGHT / cur_coord_H)

#define VERT_SPACING  2.0f


void HUD_SetCoordSys(int width, int height)
{
	cur_coord_W = width;
	cur_coord_H = height;
}

void HUD_SetFont(font_c *font)
{
	cur_font = font ? font : default_font;
}

void HUD_SetScale(float scale)
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

void HUD_SetAlignment(int xa, int ya)
{
	cur_x_align = xa;
	cur_y_align = ya;
}


void HUD_Reset()
{
	cur_coord_W = 320.0f;
	cur_coord_H = 200.0f;

	cur_font  = default_font;
	cur_color = RGB_NO_VALUE;
	cur_scale = 1.0f;
	cur_alpha = 1.0f;
	cur_x_align = cur_y_align = -1;
}


void HUD_FrameSetup(void)
{
	if (! default_font)
	{
		// FIXME: get default font from DDF gamedef
		fontdef_c *DEF = fontdefs.Lookup("DOOM");
		SYS_ASSERT(DEF);

		default_font = hu_fonts.Lookup(DEF);
		SYS_ASSERT(default_font);
	}

	HUD_Reset();

	if (am_smoothing.d)
	{
		glEnable(GL_LINE_SMOOTH);
		glLineWidth(1.5f);
	}
	else
	{
		glDisable(GL_LINE_SMOOTH);
		glLineWidth(1.0f);
	}
}


#define MAX_SCISSOR_STACK  10

static int scissor_stack[MAX_SCISSOR_STACK][4];
static int sci_stack_top = 0;


void HUD_PushScissor(float x1, float y1, float x2, float y2)
{
	SYS_ASSERT(sci_stack_top < MAX_SCISSOR_STACK);

	x1 = COORD_X(x1); y1 = SCREENHEIGHT - COORD_Y(y1);
	x2 = COORD_X(x2); y2 = SCREENHEIGHT - COORD_Y(y2);

	int sx1 = I_ROUND(x1); int sy1 = I_ROUND(y2);
	int sx2 = I_ROUND(x2); int sy2 = I_ROUND(y1);

	if (sci_stack_top == 0)
	{
		glEnable(GL_SCISSOR_TEST);

		sx1 = MAX(sx1, 0);
		sy1 = MAX(sy1, 0);

		sx2 = MIN(sx2, SCREENWIDTH);
		sy2 = MIN(sy2, SCREENHEIGHT);
	}
	else
	{
		// clip to previous scissor
		int *xy = scissor_stack[sci_stack_top-1];

		sx1 = MAX(sx1, xy[0]);
		sy1 = MAX(sy1, xy[1]);

		sx2 = MIN(sx2, xy[2]);
		sy2 = MIN(sy2, xy[3]);
	}

	SYS_ASSERT(sx2 >= sx1);
	SYS_ASSERT(sy2 >= sy1);

	glScissor(sx1, sy1, sx2 - sx1, sy2 - sy1);

	// push current scissor
	int *xy = scissor_stack[sci_stack_top];

	xy[0] = sx1; xy[1] = sy1;
	xy[2] = sx2; xy[3] = sy2;

	sci_stack_top++;
}

void HUD_PopScissor()
{
	SYS_ASSERT(sci_stack_top > 0);

	sci_stack_top--;

	if (sci_stack_top == 0)
	{
		glDisable(GL_SCISSOR_TEST);
	}
	else
	{
		// restore previous scissor
		int *xy = scissor_stack[sci_stack_top];

		glScissor(xy[0], xy[1], xy[2]-xy[0], xy[3]-xy[1]);
	}
}


//----------------------------------------------------------------------------


void HUD_RawImage(float x, float y, float w, float h, const image_c *image, 
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
	
	if (x2 < 0 || x1 > SCREENWIDTH ||
		y2 < 0 || y1 > SCREENHEIGHT)
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

void HUD_StretchImage(float x, float y, float w, float h, const image_c *img)
{
	if (cur_x_align >= 0)
		x -= w / (cur_x_align == 0 ? 2.0f : 1.0f);

	if (cur_y_align >= 0)
		y -= h / (cur_y_align == 0 ? 2.0f : 1.0f);

	x -= IM_OFFSETX(img);
	y -= IM_OFFSETY(img);

	w = COORD_X(w); h = COORD_Y(h);
	x = COORD_X(x); y = SCREENHEIGHT - COORD_Y(y) - h;

    HUD_RawImage(x, y, w, h, img, 0, 0, IM_RIGHT(img), IM_TOP(img), cur_alpha);
}

void HUD_DrawImage(float x, float y, const image_c *img)
{
	float w = cur_scale * IM_WIDTH(img);
	float h = cur_scale * IM_HEIGHT(img);

    HUD_StretchImage(x, y, w, h, img);
}

void HUD_TileImage(float x, float y, float w, float h, const image_c *img,
				   float offset_x, float offset_y)
{
	if (cur_x_align >= 0)
		x -= w / (cur_x_align == 0 ? 2.0f : 1.0f);

	if (cur_y_align >= 0)
		y -= h / (cur_y_align == 0 ? 2.0f : 1.0f);

	offset_x /=  w;
	offset_y /= -h;

	float tx_scale = w / IM_TOTAL_WIDTH(img)  / cur_scale;
	float ty_scale = h / IM_TOTAL_HEIGHT(img) / cur_scale;

	w = COORD_X(w); h = COORD_Y(h);
	x = COORD_X(x); y = SCREENHEIGHT - COORD_Y(y) - h;

	HUD_RawImage(x, y, w, h, img,
				  (offset_x) * tx_scale,
				  (offset_y) * ty_scale,
				  (offset_x + 1) * tx_scale,
				  (offset_y + 1) * ty_scale,
				  cur_alpha);
}


void HUD_SolidBox(float x1, float y1, float x2, float y2, rgbcol_t col)
{
	x1 = COORD_X(x1); y1 = SCREENHEIGHT - COORD_Y(y1);
	x2 = COORD_X(x2); y2 = SCREENHEIGHT - COORD_Y(y2);

	if (cur_alpha < 0.99f)
		glEnable(GL_BLEND);
  
	glColor4f(RGB_RED(col)/255.0, RGB_GRN(col)/255.0, RGB_BLU(col)/255.0, cur_alpha);
  
	glBegin(GL_QUADS);

	glVertex2f(x1, y1);
	glVertex2f(x1, y2);
	glVertex2f(x2, y2);
	glVertex2f(x2, y1);
  
	glEnd();
	glDisable(GL_BLEND);
}


void HUD_SolidLine(float x1, float y1, float x2, float y2, rgbcol_t col)
{
	x1 = COORD_X(x1); y1 = SCREENHEIGHT - COORD_Y(y1);
	x2 = COORD_X(x2); y2 = SCREENHEIGHT - COORD_Y(y2);

	if (am_smoothing.d || cur_alpha < 0.99f)
		glEnable(GL_BLEND);
  
	glColor4f(RGB_RED(col)/255.0, RGB_GRN(col)/255.0, RGB_BLU(col)/255.0, cur_alpha);
  
	glBegin(GL_LINES);

	glVertex2i((int)x1, (int)y1);
	glVertex2i((int)x2, (int)y2);
  
	glEnd();
	glDisable(GL_BLEND);
}


void HUD_AutomapLine(float x1, float y1, float x2, float y2,
					 float dx, float dy, rgbcol_t col)
{
	x1 = COORD_X(x1); y1 = SCREENHEIGHT - COORD_Y(y1);
	x2 = COORD_X(x2); y2 = SCREENHEIGHT - COORD_Y(y2);

	dx = COORD_X(dx); dy = COORD_Y(dy);

	if (am_smoothing.d || cur_alpha < 0.99f)
		glEnable(GL_BLEND);
  
	glColor4f(RGB_RED(col)/255.0, RGB_GRN(col)/255.0, RGB_BLU(col)/255.0, cur_alpha);
  
	glBegin(GL_LINES);

	glVertex2i((int)x1 + (int)dx, (int)y1 + (int)dy);
	glVertex2i((int)x2 + (int)dx, (int)y2 + (int)dy);
  
	glEnd();
	glDisable(GL_BLEND);
}


void HUD_ThinBox(float x1, float y1, float x2, float y2, rgbcol_t col)
{
	x1 = COORD_X(x1); y1 = SCREENHEIGHT - COORD_Y(y1);
	x2 = COORD_X(x2); y2 = SCREENHEIGHT - COORD_Y(y2);

	if (cur_alpha < 0.99f)
		glEnable(GL_BLEND);
  
	glColor4f(RGB_RED(col)/255.0, RGB_GRN(col)/255.0, RGB_BLU(col)/255.0, cur_alpha);
  
	glBegin(GL_QUADS);
	glVertex2f(x1,   y1); glVertex2f(x1,   y2);
	glVertex2f(x1+2, y2); glVertex2f(x1+2, y1);
	glEnd();

	glBegin(GL_QUADS);
	glVertex2f(x2-2, y1); glVertex2f(x2-2, y2);
	glVertex2f(x2,   y2); glVertex2f(x2,   y1);
	glEnd();

	glBegin(GL_QUADS);
	glVertex2f(x1+2, y1);   glVertex2f(x1+2, y1+2);
	glVertex2f(x2-2, y1+2); glVertex2f(x2-2, y1);
	glEnd();

	glBegin(GL_QUADS);
	glVertex2f(x1+2,  y2-2); glVertex2f(x1+2, y2);
	glVertex2f(x2-2,  y2);   glVertex2f(x2-2, y2-2);
	glEnd();

	glDisable(GL_BLEND);
}

void HUD_GradientBox(float x1, float y1, float x2, float y2, rgbcol_t *cols)
{
	x1 = COORD_X(x1); y1 = SCREENHEIGHT - COORD_Y(y1);
	x2 = COORD_X(x2); y2 = SCREENHEIGHT - COORD_Y(y2);

	if (cur_alpha < 0.99f)
		glEnable(GL_BLEND);
  
	glBegin(GL_QUADS);

	glColor4f(RGB_RED(cols[1])/255.0, RGB_GRN(cols[1])/255.0,
	          RGB_BLU(cols[1])/255.0, cur_alpha);
	glVertex2f(x1, y1);

	glColor4f(RGB_RED(cols[0])/255.0, RGB_GRN(cols[0])/255.0,
	          RGB_BLU(cols[0])/255.0, cur_alpha);
	glVertex2f(x1, y2);

	glColor4f(RGB_RED(cols[2])/255.0, RGB_GRN(cols[2])/255.0,
	          RGB_BLU(cols[2])/255.0, cur_alpha);
	glVertex2f(x2, y2);

	glColor4f(RGB_RED(cols[3])/255.0, RGB_GRN(cols[3])/255.0,
	          RGB_BLU(cols[3])/255.0, cur_alpha);
	glVertex2f(x2, y1);
  
	glEnd();
	glDisable(GL_BLEND);
}


float HUD_FontWidth(void)
{
	return cur_scale * cur_font->NominalWidth();
}

float HUD_FontHeight(void)
{
	return cur_scale * cur_font->NominalHeight();
}

float HUD_StringWidth(const char *str)
{
	return cur_scale * cur_font->StringWidth(str);
}

float HUD_StringHeight(const char *str)
{
	int lines = cur_font->StringLines(str);

	return lines * HUD_FontHeight() + (lines - 1) * VERT_SPACING;
}


void HUD_DrawChar(float left_x, float top_y, const image_c *img)
{
	float sc_x = cur_scale; // TODO * aspect;
	float sc_y = cur_scale;

	float x = left_x - IM_OFFSETX(img) * sc_x;
	float y = top_y  - IM_OFFSETY(img) * sc_y;

	float w = IM_WIDTH(img)  * sc_x;
	float h = IM_HEIGHT(img) * sc_y;

	w = COORD_X(w); h = COORD_Y(h);
	x = COORD_X(x); y = SCREENHEIGHT - COORD_Y(y) - h;

    HUD_RawImage(x, y, w, h, img, 0, 0, IM_RIGHT(img), IM_TOP(img),
				  cur_alpha, cur_color);
}


//
// Write a string using the current font
//
void HUD_DrawText(float x, float y, const char *str)
{
	SYS_ASSERT(cur_font);

	float cy = y;

	if (cur_y_align >= 0)
	{
		float total_h = HUD_StringHeight(str);

		if (cur_y_align == 0)
			total_h /= 2.0f;

		cy -= total_h;
	}

	// handle each line
	while (*str)
	{
		// get the length of the line
		int len = 0;
		while (str[len] && str[len] != '\n')
			len++;

		float cx = x;
		float total_w = 0;

		for (int i = 0; i < len; i++)
			total_w += cur_font->CharWidth(str[i]) * cur_scale;

		if (cur_x_align >= 0)
		{
			if (cur_x_align == 0)
				total_w /= 2.0f;
			
			cx -= total_w;
		}

		for (int k = 0; k < len; k++)
		{
			char ch = str[k];

			const image_c *img = cur_font->CharImage(ch);

			if (img)
				HUD_DrawChar(cx, cy, img);

			cx += cur_font->CharWidth(ch) * cur_scale;
		}

		if (str[len] == 0)
			break;

		str += (len + 1);
		cy  += HUD_FontHeight() + VERT_SPACING;
	}
}


void HUD_RenderWorld(float x1, float y1, float x2, float y2, mobj_t *camera)
{
	HUD_PushScissor(x1, y1, x2, y2);

	int *xy = scissor_stack[sci_stack_top-1];

	R_Render(xy[0], xy[1], xy[2]-xy[0], xy[3]-xy[1], camera);

	HUD_PopScissor();
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
