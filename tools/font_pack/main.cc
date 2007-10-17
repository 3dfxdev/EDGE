//------------------------------------------------------------------------
//  FONT_PACK Utility
//------------------------------------------------------------------------
//
//  Copyright (c) 2007  Andrew J Apted
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
//------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include <string.h>
#include <ctype.h>
#include <math.h>


#include <ft2build.h>
#include FT_FREETYPE_H


typedef unsigned char byte;


const char *ttf_filename = "../VeraMono.ttf";

const char *out_filename = "out.ppm";

int font_h = 16;
int font_w = 16;

byte *font_image;


FT_Library ft_lib;

FT_Face moon_face;


typedef struct
{
    short w, h;
    short dx, dy;
    short advance;
}
font_metric_t;


static int Font_BlitChar(int x, int y, int ch)
{
    int hash;

    SDL_Rect src;
    SDL_Rect dest;

    ch &= 255;

    // FIXME: background??
    if (isspace(ch))
        return mono_font ? font_w : font_w * 7 / 20;

    font_metric_t *met = Font_NeedChar(ch);

    hash = Font_Hash(fg_color, bg_color, ch);

    src.x = (hash % CACHE_W) * font_w;
    src.y = (hash / CACHE_W) * font_h;
    src.w = met->w;
    src.h = met->h;

    dest.x = x + met->dx;
    dest.y = y + met->dy;
    dest.w = src.w;
    dest.h = src.h;

    SDL_BlitSurface(font_cache, &src, my_vis, &dest);

    Dirty_Add(dest.x, dest.y, dest.w, dest.h);

    //  { static int JJ = 0; JJ++; if (!(JJ&3)) Video_Update(); }

    return met->advance;
}


//------------------------------------------------------------------------
//  FREETYPE STUFF...
//------------------------------------------------------------------------


void TTF_Init()
{
    printf("Initialising FreeType library...\n");

    int error = FT_Init_FreeType(&ft_lib);

    if (error)
    {
        fprintf(stderr, "Unable to initialise FreeType library (error %d)", error);
        exit(1);
    }
}

void TTF_Quit()
{
    printf("FT", "Closing FreeType library...\n");

    FT_Done_FreeType(ft_lib);
}

void TTF_LoadFont()
{
    int error = FT_New_Face(ft_lib, ttf_filename, 0, &moon_face);

    if (error)
    {
        fprintf(stderr, "Unable to open VERA font (error %d)", error);
        exit(1);
    }

    if (SCREEN_W < 720)
        font_h = 20;
    else if (SCREEN_W < 960)
        font_h = 24;
    else
        font_h = 30;

    font_h = 32; //!!!!!! RESTORE

    font_w = font_h;
}

bool TTF_ValidChar(unsigned char ch)
{
    return (0x20 <= ch && ch <= 0x7E) ||
        (0xC0 <= ch && ch <= 0xFF);
}

int TTF_CharIndex(unsigned char ch)
{
    SYS_ASSERT(TTF_ValidChar(ch));

    if (ch >= 0xC0)
        ch -= 0x60;
    else
        ch -= 0x20;

    SYS_ASSERT(ch < TOTAL_CHARS);

    return ch;
}

void TTF_BuildChar(int ch, font_metric_t *met, u8_t *pixels, int cx, int cy, int pitch)
{
    int pixel_size = font_h * 3 / 4;

    int error = FT_Set_Pixel_Sizes(moon_face, 0, pixel_size);  /// CREATE SIZE OBJS
    if (error)
    {
        fprintf(stderr, "Unable to set pixel size (error %d).\n", error);
        exit(1);
    }

    SYS_ASSERT(TTF_ValidChar(ch));

    if (ch == 32 || ch == 127)
        return;

    FT_Int32 load_flags = FT_LOAD_RENDER;

    // if (FOO) load_flags |= FT_LOAD_NO_AUTOHINT;

    error = FT_Load_Char(moon_face, ch, load_flags);
    if (error)
    {
        printf("FT_Load_Glyph #%d failed (error %d)\n", ch, error);
        return;
    }

    FT_Bitmap& bm = moon_face->glyph->bitmap;

    int bm_h = bm.rows;
    int bm_w = bm.width;

//  fprintf(stderr, "Glyph #%d = %dx%d\n", ch, bm_w, bm_h);

    if (bm_h > font_h)
    {
        bm_h = font_h;
        printf("Glyph #%d was too tall -- clipped\n", ch);
    }
    if (bm_w > font_w)
    {
        bm_w = font_w;
        printf("Glyph #%d was too wide -- clipped\n", ch);
    }

    int fg_r = basic_palette[fg_color*3+0];
    int fg_g = basic_palette[fg_color*3+1];
    int fg_b = basic_palette[fg_color*3+2];

    int bg_r = basic_palette[bg_color*3+0];
    int bg_g = basic_palette[bg_color*3+1];
    int bg_b = basic_palette[bg_color*3+2];

    for (int by = 0; by < bm_h; by++)
    {
        int dest_y = (bm.pitch < 0) ? (bm_h - 1 - by) : by;

        u8_t *src  = bm.buffer + by * abs(bm.pitch);
        u8_t *dest = pixels + (cy + dest_y) * pitch + cx;

        for (int bx = 0; bx < bm_w; bx++, src++)
        {
            int ity = (int) *src;

            // FIXME: more options:
            //   (a) when FG and BG are grayscale
            //   (b) AA limit: none (2 color), 3 color, 5 color, unlimited
            int r = (fg_r * (ity+1) + bg_r * (ity^255)) >> 8;
            int g = (fg_g * (ity+1) + bg_g * (ity^255)) >> 8;
            int b = (fg_b * (ity+1) + bg_b * (ity^255)) >> 8;

            *dest++ = (ity & 0x80) ? fg_color : bg_color; //!!!!!! RGB_TO_PAL(r, g, b);
        }
    }

    // get metrics.
    FT_Glyph_Metrics *cur_met = &moon_face->glyph->metrics;

    // FIXME !!!! round up??
    met->w  = cur_met->width >> 6;
    met->h  = cur_met->height >> 6;
    met->dx = cur_met->horiBearingX >> 6;
    // FIXME !!!! right dist to baseline ?
    met->dy = font_h *3/4 - (cur_met->horiBearingY >> 6);
    met->advance = 1 + (cur_met->horiAdvance >> 6);
}


//------------------------------------------------------------------------


void DumpImage(void)
{
    FILE *fp = fopen(out_filename, "wb");

    if (! fp)
    {
        fprintf(stderr, "Cannot create output file: %s\n", out_filename);
        exit(1);
    }

    fprintf(fp, "P6\n%d %d 255\n", font_w * 16, font_h * 16);

    for (int y = 0; y < font_h*16; y++)
    for (int x = 0; x < font_w*16; x++)
    {
        const byte pix = font_image[y*font_w*16+x];

        if (pix == 0)
        {
            // transparent color
            fputc(fp, 0xFF);
            fputc(fp, 0x00);
            fputc(fp, 0xFF);
        }
        else
        {
            fputc(fp, pix);  // red
            fputc(fp, pix);  // green
            fputc(fp, pix);  // blue
        }
    }

    fclose(fp);
}


int main(int argc, char **argv)
{
    TTF_Init();
    TTF_LoadFont();

    CreateImage();
    RenderChars();
    DumpImage();

    TTF_Quit();

    return 0;
}

//--- editor settings ---
// vi:ts=4:sw=4:expandtab
