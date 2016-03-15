//----------------------------------------------------------------------------
//  EDGE2 2D DRAWING STUFF
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2009  The EDGE2 Team.
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
#include "r_colormap.h"
#include "r_draw.h"
#include "r_modes.h"
#include "r_image.h"


static int glbsp_last_prog_time = 0;


void RGL_NewScreenSize(int width, int height, int bits)
{
	//!!! quick hack
	RGL_SetupMatrices2D();

	// prevent a visible border with certain cards/drivers
	glCleargbcol_t(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
}


void RGL_DrawImage(float x, float y, float w, float h, const image_c *image, 
				   float tx1, float ty1, float tx2, float ty2,
				   const colourmap_c *textmap, float alpha,
				   const colourmap_c *palremap)
{
	int x1 = I_ROUND(x);
	int y1 = I_ROUND(y);
	int x2 = I_ROUND(x+w+0.25f);
	int y2 = I_ROUND(y+h+0.25f);

	if (x1 == x2 || y1 == y2)
		return;

	float r = 1.0f, g = 1.0f, b = 1.0f;

	GLuint tex_id = W_ImageCache(image, true,
		(textmap && (textmap->special & COLSP_Whiten)) ? font_whiten_map : palremap);

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

	if (textmap)
	{
		rgbcol_t col = V_GetFontColor(textmap);

		r = RGB_RED(col) / 255.0;
		g = RGB_GRN(col) / 255.0;
		b = RGB_BLU(col) / 255.0;
	}

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


void RGL_ReadScreen(int x, int y, int w, int h, byte *rgb_buffer)
{
#ifndef DREAMCAST
	glFlush();

	glPixelZoom(1.0f, 1.0f);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	for (; h > 0; h--, y++)
	{
		glReadPixels(x, y, w, 1, GL_RGB, GL_UNSIGNED_BYTE, rgb_buffer);

		rgb_buffer += w * 3;
	}
#endif
}


void ST_FlashingScreen(byte r, byte g, byte b, byte a) 
{
    rcolor c = D_RGBA(r, g, b, a);

    GL_SetState(GLSTATE_BLEND, 1);
    GL_SetOrtho(1);

    glDisable(GL_TEXTURE_2D);
    glColor4ubv((byte*)&c);
    glRecti(SCREENWIDTH, SCREENHEIGHT, 0, 0);
    glEnable(GL_TEXTURE_2D);

    GL_SetState(GLSTATE_BLEND, 0);
}

//
//
// STRING DRAWING ROUTINES
//
//

static vtx_t vtxstring[MAX_MESSAGE_SIZE];

//
// Draw_Text
//

int Draw_Text(int x, int y, rgbcol_t color, float scale,
              bool wrap, const char* string, ...) {
    int c;
    int i;
    int vi = 0;
    int col;
    const float size = 0.03125f;
    float fcol, frow;
    int start = 0;
    bool fill = false;
    char msg[MAX_MESSAGE_SIZE];
    va_list    va;
    const int ix = x;

    va_start(va, string);
    vsprintf(msg, string, va);
    va_end(va);

    GL_SetState(GLSTATE_BLEND, 1);

    if(!r_fillmode.d) {
        glEnable(GL_TEXTURE_2D);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        r_fillmode.d = 1.0f;
        fill = true;
    }

    GL_BindTexture("SFONT", true);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    GL_SetOrthoScale(scale);
    GL_SetOrtho(0);

    glSetVertex(vtxstring);

    for(i = 0, vi = 0; i < dstrlen(msg); i++, vi += 4) {
        c = toupper(msg[i]);
        if(c == '\t') {
            while(x % 64) {
                x++;
            }
            continue;
        }
        if(c == '\n') {
            y += ST_FONTWHSIZE;
            x = ix;
            continue;
        }
        if(c == 0x20) {
            if(wrap) {
                if(x > 192) {
                    y += ST_FONTWHSIZE;
                    x = ix;
                    continue;
                }
            }
        }
        else {
            start = (c - ST_FONTSTART);
            col = start & (ST_FONTNUMSET - 1);

            fcol = (col * size);
            frow = (start >= ST_FONTNUMSET) ? 0.5f : 0.0f;

            vtxstring[vi + 0].x     = (float)x;
            vtxstring[vi + 0].y     = (float)y;
            vtxstring[vi + 0].tu    = fcol + 0.0015f;
            vtxstring[vi + 0].tv    = frow + size;
            vtxstring[vi + 1].x     = (float)x + ST_FONTWHSIZE;
            vtxstring[vi + 1].y     = (float)y;
            vtxstring[vi + 1].tu    = (fcol + size) - 0.0015f;
            vtxstring[vi + 1].tv    = frow + size;
            vtxstring[vi + 2].x     = (float)x + ST_FONTWHSIZE;
            vtxstring[vi + 2].y     = (float)y + ST_FONTWHSIZE;
            vtxstring[vi + 2].tu    = (fcol + size) - 0.0015f;
            vtxstring[vi + 2].tv    = frow + 0.5f;
            vtxstring[vi + 3].x     = (float)x;
            vtxstring[vi + 3].y     = (float)y + ST_FONTWHSIZE;
            vtxstring[vi + 3].tu    = fcol + 0.0015f;
            vtxstring[vi + 3].tv    = frow + 0.5f;

            glSetVertexColor(vtxstring + vi, color, 4);

            glTriangle(vi + 0, vi + 1, vi + 2);
            glTriangle(vi + 0, vi + 2, vi + 3);

            if(devparm) {
                vertCount += 4;
            }


        }
        x += ST_FONTWHSIZE;
    }

    if(vi) {
        glDrawGeometry(vi, vtxstring);
    }

    GL_ResetViewport();

    if(fill) {
        dglDisable(GL_TEXTURE_2D);
        dglPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        r_fillmode.d = 0.0f;
    }

    GL_SetState(GLSTATE_BLEND, 0);
    GL_SetOrthoScale(1.0f);

    return x;
}

const symboldata_t symboldata[] = {  //0x5B9BC
    { 120, 14, 13, 13 },
    { 134, 14, 9, 13 },
    { 144, 14, 14, 13 },
    { 159, 14, 14, 13 },
    { 174, 14, 16, 13 },
    { 191, 14, 13, 13 },
    { 205, 14, 13, 13 },
    { 219, 14, 14, 13 },
    { 234, 14, 14, 13 },
    { 0, 29, 13, 13 },
    { 67, 28, 14, 13 },    // -
    { 36, 28, 15, 14 },    // %
    { 28, 28, 7, 14 },    // !
    { 14, 29, 6, 13 },    // .
    { 52, 28, 13, 13 },    // ?
    { 21, 29, 6, 13 },    // :
    { 0, 0, 13, 13 },
    { 14, 0, 13, 13 },
    { 28, 0, 13, 13 },
    { 42, 0, 14, 13 },
    { 57, 0, 14, 13 },
    { 72, 0, 10, 13 },
    { 87, 0, 15, 13 },
    { 103, 0, 15, 13 },
    { 119, 0, 6, 13 },
    { 126, 0, 13, 13 },
    { 140, 0, 14, 13 },
    { 155, 0, 11, 13 },
    { 167, 0, 15, 13 },
    { 183, 0, 16, 13 },
    { 200, 0, 15, 13 },
    { 216, 0, 13, 13 },
    { 230, 0, 15, 13 },
    { 246, 0, 13, 13 },
    { 0, 14, 14, 13 },
    { 15, 14, 14, 13 },
    { 30, 14, 13, 13 },
    { 44, 14, 15, 13 },
    { 60, 14, 15, 13 },
    { 76, 14, 15, 13 },
    { 92, 14, 13, 13 },
    { 106, 14, 13, 13 },
    { 83, 31, 10, 11 },
    { 93, 31, 10, 11 },
    { 103, 31, 11, 11 },
    { 114, 31, 11, 11 },
    { 125, 31, 11, 11 },
    { 136, 31, 11, 11 },
    { 147, 31, 12, 11 },
    { 159, 31, 12, 11 },
    { 171, 31, 4, 11 },
    { 175, 31, 10, 11 },
    { 185, 31, 11, 11 },
    { 196, 31, 9, 11 },
    { 205, 31, 12, 11 },
    { 217, 31, 13, 11 },
    { 230, 31, 12, 11 },
    { 242, 31, 11, 11 },
    { 0, 43, 12, 11 },
    { 12, 43, 11, 11 },
    { 23, 43, 11, 11 },
    { 34, 43, 10, 11 },
    { 44, 43, 11, 11 },
    { 55, 43, 12, 11 },
    { 67, 43, 13, 11 },
    { 80, 43, 13, 11 },
    { 93, 43, 10, 11 },
    { 103, 43, 11, 11 },
    { 0, 95, 108, 11 },
    { 108, 95, 6, 11 },
    { 0, 54, 32, 26 },
    { 32, 54, 32, 26 },
    { 64, 54, 32, 26 },
    { 96, 54, 32, 26 },
    { 128, 54, 32, 26 },
    { 160, 54, 32, 26 },
    { 192, 54, 32, 26 },
    { 224, 54, 32, 26 },
    { 134, 97, 7, 11 },
    { 114, 95, 20, 18 },
    { 105, 80, 15, 15 },
    { 120, 80, 15, 15 },
    { 135, 80, 15, 15 },
    { 150, 80, 15, 15 },
    { 45, 80, 15, 15 },
    { 60, 80, 15, 15 },
    { 75, 80, 15, 15 },
    { 90, 80, 15, 15 },
    { 165, 80, 15, 15 },
    { 180, 80, 15, 15 },
    { 0, 80, 15, 15 },
    { 15, 80, 15, 15 },
    { 195, 80, 15, 15 },
    { 30, 80, 15, 15 },
    { 156, 96, 13, 13 },
    { 143, 96, 13, 13 },
    { 169, 96, 7, 13 },
    { -1, -1, -1, -1 }
};

//
// Center_Text
//

int Center_Text(const char* string) {
    int width = 0;
    char t = 0;
    int id = 0;
    int len = 0;
    int i = 0;
    float scale;

    len = dstrlen(string);

    for(i = 0; i < len; i++) {
        t = string[i];

        switch(t) {
        case 0x20:
            width += 6;
            break;
        case '-':
            width += symboldata[SM_MISCFONT].w;
            break;
        case '%':
            width += symboldata[SM_MISCFONT + 1].w;
            break;
        case '!':
            width += symboldata[SM_MISCFONT + 2].w;
            break;
        case '.':
            width += symboldata[SM_MISCFONT + 3].w;
            break;
        case '?':
            width += symboldata[SM_MISCFONT + 4].w;
            break;
        case ':':
            width += symboldata[SM_MISCFONT + 5].w;
            break;
        default:
            if(t >= 'A' && t <= 'Z') {
                id = t - 'A';
                width += symboldata[SM_FONT1 + id].w;
            }
            if(t >= 'a' && t <= 'z') {
                id = t - 'a';
                width += symboldata[SM_FONT2 + id].w;
            }
            if(t >= '0' && t <= '9') {
                id = t - '0';
                width += symboldata[SM_NUMBERS + id].w;
            }
            break;
        }
    }

    scale = GL_GetOrthoScale();

    if(scale != 1.0f) {
        return ((int)(160.0f / scale) - (width / 2));
    }

    return (160 - (width / 2));
}

//
// Draw_BigText
//

int Draw_BigText(int x, int y, rgbcol_t color, const char* string) {
    int c = 0;
    int i = 0;
    int vi = 0;
    int index = 0;
    float vx1 = 0.0f;
    float vy1 = 0.0f;
    float vx2 = 0.0f;
    float vy2 = 0.0f;
    float tx1 = 0.0f;
    float tx2 = 0.0f;
    float ty1 = 0.0f;
    float ty2 = 0.0f;
    float smbwidth;
    float smbheight;
    int pic;

    if(x <= -1) {
        x = Center_Text(string);
    }

    y += 14;

    pic = GL_BindTexture("SYMBOLS", true);

    smbwidth = (float)gfxwidth[pic];
    smbheight = (float)gfxheight[pic];

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    glSetVertex(vtxstring);

    GL_SetState(GLSTATE_BLEND, 1);
    GL_SetOrtho(0);

    for(i = 0, vi = 0; i < strlen(string); i++, vi += 4) {
        vx1 = (float)x;
        vy1 = (float)y;

        c = string[i];
        if(c == '\n' || c == '\t') {
            continue;    // villsa: safety check
        }
        else if(c == 0x20) {
            x += 6;
            continue;
        }
        else {
            if(c >= '0' && c <= '9') {
                index = (c - '0') + SM_NUMBERS;
            }
            if(c >= 'A' && c <= 'Z') {
                index = (c - 'A') + SM_FONT1;
            }
            if(c >= 'a' && c <= 'z') {
                index = (c - 'a') + SM_FONT2;
            }
            if(c == '-') {
                index = SM_MISCFONT;
            }
            if(c == '%') {
                index = SM_MISCFONT + 1;
            }
            if(c == '!') {
                index = SM_MISCFONT + 2;
            }
            if(c == '.') {
                index = SM_MISCFONT + 3;
            }
            if(c == '?') {
                index = SM_MISCFONT + 4;
            }
            if(c == ':') {
                index = SM_MISCFONT + 5;
            }

            // [kex] use 'printf' style formating for special symbols
            if(c == '/') {
                c = string[++i];

                switch(c) {
                // up arrow
                case 'u':
                    index = SM_MICONS + 17;
                    break;
                // down arrow
                case 'd':
                    index = SM_MICONS + 16;
                    break;
                // right arrow
                case 'r':
                    index = SM_MICONS + 18;
                    break;
                // left arrow
                case 'l':
                    index = SM_MICONS;
                    break;
                // cursor box
                case 'b':
                    index = SM_MICONS + 1;
                    break;
                // thermbar
                case 't':
                    index = SM_THERMO;
                    break;
                // thermcursor
                case 's':
                    index = SM_THERMO + 1;
                    break;
                default:
                    return 0;
                }
            }

            vx2 = vx1 + symboldata[index].w;
            vy2 = vy1 - symboldata[index].h;

            tx1 = ((float)symboldata[index].x / smbwidth) + 0.001f;
            tx2 = (tx1 + (float)symboldata[index].w / smbwidth) - 0.002f;

            ty1 = ((float)symboldata[index].y / smbheight);
            ty2 = ty1 + (((float)symboldata[index].h / smbheight));

            vtxstring[vi + 0].x     = vx1;
            vtxstring[vi + 0].y     = vy1;
            vtxstring[vi + 0].tu    = tx1;
            vtxstring[vi + 0].tv    = ty2;
            vtxstring[vi + 1].x     = vx2;
            vtxstring[vi + 1].y     = vy1;
            vtxstring[vi + 1].tu    = tx2;
            vtxstring[vi + 1].tv    = ty2;
            vtxstring[vi + 2].x     = vx2;
            vtxstring[vi + 2].y     = vy2;
            vtxstring[vi + 2].tu    = tx2;
            vtxstring[vi + 2].tv    = ty1;
            vtxstring[vi + 3].x     = vx1;
            vtxstring[vi + 3].y     = vy2;
            vtxstring[vi + 3].tu    = tx1;
            vtxstring[vi + 3].tv    = ty1;

            glSetVertexColor(vtxstring + vi, color, 4);

            glTriangle(vi + 2, vi + 1, vi + 0);
            glTriangle(vi + 3, vi + 2, vi + 0);

            /* if(devparm) {
                vertCount += 4;
            } */

            x += symboldata[index].w;
        }
    }

    if(vi) {
        glDrawGeometry(vi, vtxstring);
    }

    GL_ResetViewport();
    GL_SetState(GLSTATE_BLEND, 0);

    return x;
}

//
// Draw_Number
//
//

void Draw_Number(int x, int y, int num, int type, rgbcol_t c) {
    int digits[16];
    int nx = 0;
    int count;
    int j;
    char str[2];

    for(count = 0, j = 0; count < 16; count++, j++) {
        digits[j] = num % 10;
        nx += symboldata[SM_NUMBERS + digits[j]].w;

        num /= 10;

        if(!num) {
            break;
        }
    }

    if(type == 0) {
        x -= (nx >> 1);
    }

    if(type == 0 || type == 1) {
        if(count < 0) {
            return;
        }

        while(count >= 0) {
            sprintf(str, "%i", digits[j]);
            Draw_BigText(x, y, c, str);

            x += symboldata[SM_NUMBERS + digits[j]].w;

            count--;
            j--;
        }
    }
    else {
        if(count < 0) {
            return;
        }

        j = 0;

        while(count >= 0) {
            x -= symboldata[SM_NUMBERS + digits[j]].w;

            sprintf(str, "%i", digits[j]);
            Draw_BigText(x, y, c, str);

            count--;
            j++;
        }
    }
}


static void ProgressSection(const byte *logo_lum, int lw, int lh,
	const byte *text_lum, int tw, int th,
	float cr, float cg, float cb,
	int *y, int perc, float alpha)
{
	float zoom = 1.0f;
#ifndef DREAMCAST
	(*y) -= (int)(lh * zoom);

	glRasterPos2i(20, *y);
	glPixelZoom(zoom, zoom);
	glDrawPixels(lw, lh, GL_LUMINANCE, GL_UNSIGNED_BYTE, logo_lum);

	(*y) -= th + 20;

	glRasterPos2i(20, *y);
	glPixelZoom(1.0f, 1.0f);
	glDrawPixels(tw, th, GL_LUMINANCE, GL_UNSIGNED_BYTE, text_lum);
#endif
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
	/* show EDGE2 logo and a progress indicator */
printf("Drawing progress %i %i\n",perc,glbsp_perc);
	glCleargbcol_t(0.0f, 0.0f, 0.0f, 0.0f);
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




//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
