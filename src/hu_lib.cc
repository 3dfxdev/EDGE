//----------------------------------------------------------------------------
//  EDGE Heads-up-display library Code
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2001  The EDGE Team.
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
//  Based on the DOOM source code, released by Id Software under the
//  following copyright:
//
//    Copyright (C) 1993-1996 by id Software, Inc.
//
//----------------------------------------------------------------------------
//
// -KM- 1998/10/29 Modified to allow foreign characters like '„'
//

#include "i_defs.h"
#include "hu_lib.h"

#include "dm_defs.h"
#include "dm_state.h"
#include "hu_stuff.h"
#include "m_inline.h"
#include "m_swap.h"
#include "r_local.h"
#include "v_colour.h"
#include "v_ctx.h"
#include "v_res.h"

#define DUMMY_WIDTH(font)  (4)

//
// HL_Init
//
void HL_Init(void)
{
  /* nothing to init */
}

//
// HL_CharWidth
//
// Returns the width of the IBM cp437 char in the font.
//
int HL_CharWidth(const H_font_t *font, int ch)
{
  // catch sign extension from (char)
  DEV_ASSERT2(ch >= 0);

  if (ch < 128)
    ch = toupper(ch);

  if (ch < font->first_ch || ch > font->last_ch)
    return DUMMY_WIDTH(font);

  return IM_WIDTH(HFONT_GET_CH(font, ch));
}

//
// HL_TextMaxLen
//
// Returns the maximum number of characters which can fit within max_w
// pixels.  The string may not contain any newline characters.
//
int HL_TextMaxLen(int max_w, const char *str)
{
  int w;
  const char *s;

  // just add one char at a time until it gets too wide or the string ends.
  for (w=0, s=str; *s; s++)
  {
    w += HL_CharWidth(&hu_font, *s);

    if (w > max_w)
    {
      // if no character could fit, an infinite loop would probably start,
      // so it's better to just imagine that one character fits.
      if (s == str)
        s = str + 1;

      break;
    }
  }

  // extra spaces at the end of the line can always be added
  while (*s == ' ')
    s++;

  return s - str;
}

//
// HL_StringWidth
//
// Find string width from hu_font chars.  The string may not contain
// any newline characters.
//
int HL_StringWidth(const char *str)
{
  int w;

  for (w=0; *str; str++)
  {
    w += HL_CharWidth(&hu_font, *str);
  }

  return w;
}

//
// HL_StringHeight
//
// Find string height from hu_font chars
//
int HL_StringHeight(const char *string)
{
  int h;
  int height = hu_font.height;

  h = height;

  for (; *string; string++)
    if (*string == '\n')
      h += height;

  return h;
}

//
// HL_WriteChar
//
static void HL_WriteChar(int x, int y, const H_font_t *font, int c,
    const colourmap_t *colmap, fixed_t alpha)
{
  const image_t *image = HFONT_GET_CH(font, c);
  
  vctx.DrawImage(FROM_320(x - image->offset_x), 
      FROM_200(y - image->offset_y),
      FROM_320(IM_WIDTH(image)), FROM_200(IM_HEIGHT(image)), image,
      0.0, 0.0, IM_RIGHT(image), IM_BOTTOM(image), colmap,
      M_FixedToFloat(alpha));
}

//
// HL_WriteTextTrans
//
// Write a string using the hu_font and index translator.
//
void HL_WriteTextTrans(int x, int y, const colourmap_t *colmap, 
    const char *string)
{
  int w, c, cx, cy;

  cx = x;
  cy = y;

  for (; *string; string++)
  {
    c = *string;

    if (c == '\n')
    {
      cx = x;
      cy += 12;
      continue;
    }

    if (c < 128)
      c = toupper(c);

    if (c < hu_font.first_ch || c > hu_font.last_ch)
    {
      cx += DUMMY_WIDTH(&hu_font);
      continue;
    }

    w = HL_CharWidth(&hu_font, c);

    if (cx + w > 320)
      continue;

    HL_WriteChar(cx, cy, &hu_font, c, colmap, FRACUNIT);

    cx += w;
  }
}

//
// HL_WriteText
//
// Write a string using the hu_font.
//
void HL_WriteText(int x, int y, const char *string)
{
  HL_WriteTextTrans(x, y, text_red_map, string);
}

//----------------------------------------------------------------------------

void HL_ClearTextLine(hu_textline_t * t)
{
  t->len = 0;
  t->ch[0] = 0;
  t->needsupdate = true;
  t->centre = false;
}

void HL_InitTextLine(hu_textline_t * t, int x, int y, 
    const H_font_t *font)
{
  t->x = x;
  t->y = y;
  t->font = font;
  HL_ClearTextLine(t);
}

bool HL_AddCharToTextLine(hu_textline_t * t, char ch)
{
  if (t->len >= HU_MAXLINELENGTH-1)
    return false;

  t->ch[t->len++] = ch;
  t->ch[t->len] = 0;
  t->needsupdate = 4;
  return true;
}

bool HL_DelCharFromTextLine(hu_textline_t * t)
{
  if (!t->len)
    return false;

  t->len--;
  t->ch[t->len] = 0;
  t->needsupdate = 4;
  return true;
}

//#define HU_CHAR(ch)  ((ch) < 128 ? toupper(ch) : (ch))
#define HU_CHAR(ch)  ((ch) >= 0 ? toupper(ch) : (ch))

//
// HL_DrawTextLineAlpha
//
// New Procedure: Same as HL_DrawTextLine, but the
// colour is passed through the given translation table
// and scaled if possible.
//
// -ACB- 1998/06/10 Procedure Written.
//
// -ACB- 1998/09/11 Index changed from JC's Pre-Calculated to using
//                  the PALREMAP translation maps.
//
// -AJA- 2000/03/05: Index replaced with pointer to trans table.
// -AJA- 2000/10/22: Renamed for alpha support.
//
void HL_DrawTextLineAlpha(hu_textline_t * L, bool drawcursor, const colourmap_t *colmap, fixed_t alpha)
{
  int i, w, x, y;
  char c;

  // draw the new stuff
  x = L->x;
  y = L->y;

  // -AJA- 1999/09/07: centred text.
  if (L->centre)
  {
    x -= HL_StringWidth(L->ch) / 2;
  }

  for (i=0; (i < L->len) && (x < 320); i++, x += w)
  {
    c = HU_CHAR(L->ch[i]);
    w = HL_CharWidth(L->font, c);

    if (c < L->font->first_ch || c > L->font->last_ch)
      continue;

    if (x < 0)
      continue;

    if (x + w > 320)
      continue;

    HL_WriteChar(x, y, L->font, c, colmap, alpha);
  }

  // draw the cursor if requested
  if (drawcursor && x + L->font->width <= 320)
  {
    HL_WriteChar(x, y, L->font, '_', colmap, alpha);
  }
}

void HL_DrawTextLine(hu_textline_t * L, bool drawcursor)
{
  HL_DrawTextLineAlpha(L, drawcursor, text_red_map, FRACUNIT);
  
#if 0  // OLD CODE
  int i, w, x, y;
  unsigned char c;
  
  // draw the new stuff
  x = L->x;
  y = L->y;

  // -AJA- 1999/09/07: centred text.
  if (L->centre)
  {
    x -= HL_StringWidth(L->ch) / 2;
  }

  for (i=0; (i < L->len) && (x < SCREENWIDTH); i++, x += w)
  {
    c = HU_CHAR(L->ch[i]);
    w = HL_CharWidth(L->font, c);

    if (c < L->font->first_ch || c > L->font->last_ch)
      continue;

    if (x < 0)
      continue;

    // -ACB- 1998/06/09 was (x+w > 320):
    //       not displaying all text at high resolutions.
    if (x + w > SCREENWIDTH)
      continue;

#ifdef USE_GL
    /// RGL_DrawPatch(hu_font_nums[c - l->sc], x, l->y);
#else
    V_DrawPatchDirect(main_scr, x, y, HFONT_GET_CH(L->font,c));
#endif
  }

  // draw the cursor if requested
  if (drawcursor && x + L->font->width <= SCREENWIDTH)
    V_DrawPatchDirect(main_scr, x, L->y, HFONT_GET_CH(L->font,'_'));
#endif
}

// sorta called by HU_Erase and just better darn get things straight
void HL_EraseTextLine(hu_textline_t * l)
{
#if 0 // OLD STUFF
  int lh;
  int y;
  int yoffset;
  static bool lastautomapactive = true;

  // Only erases when NOT in automap and the screen is reduced,
  // and the text must either need updating or refreshing
  // (because of a recent change back from the automap)

  if (!automapactive && viewwindowx && l->needsupdate)
  {
    lh = SHORT(l->f[0]->height) + 1;
    for (y = l->y, yoffset = y * SCREENWIDTH; y < l->y + lh; y++, yoffset += SCREENWIDTH)
    {
      if (y < viewwindowy || y >= viewwindowy + viewwindowheight)
        R_VideoErase(yoffset, SCREENWIDTH);  // erase entire line

      else
      {
        R_VideoErase(yoffset, viewwindowx);  // erase left border

        R_VideoErase(yoffset + viewwindowx + viewwidth, viewwindowx);
        // erase right border
      }
    }
  }

  lastautomapactive = automapactive;
  if (l->needsupdate)
    l->needsupdate--;
#endif
}

//----------------------------------------------------------------------------

void HL_InitSText(hu_stext_t * s, int x, int y, int h, 
    const H_font_t *font, bool * on)
{
  int i;

  s->h = h;
  s->on = on;
  s->laston = true;
  s->curline = 0;

  for (i = 0; i < h; i++)
  {
    HL_InitTextLine(&s->L[i], x, y - i * (font->height+1), font);
  }
}

void HL_AddLineToSText(hu_stext_t * s)
{
  int i;

  // add a clear line
  if (++s->curline == s->h)
    s->curline = 0;

  HL_ClearTextLine(&s->L[s->curline]);

  // everything needs updating
  for (i=0; i < s->h; i++)
    s->L[i].needsupdate = 4;
}

void HL_AddMessageToSText(hu_stext_t * s, const char *prefix, const char *msg)
{
  HL_AddLineToSText(s);

  if (prefix)
    for (; *prefix; prefix++)
      HL_AddCharToTextLine(&s->L[s->curline], *prefix);

  for (; *msg; msg++)
    HL_AddCharToTextLine(&s->L[s->curline], *msg);
}

void HL_DrawSText(hu_stext_t * s)
{
  int i, idx;

  if (!*s->on)
    return;  // if not on, don't draw

  // draw everything
  for (i=0; i < s->h; i++)
  {
    idx = s->curline - i;
    if (idx < 0)
      idx += s->h;  // handle queue of lines

    // need a decision made here on whether to skip the draw.
    // no cursor, please.
    HL_DrawTextLine(&s->L[idx], false);
  }
}

void HL_EraseSText(hu_stext_t * s)
{
  int i;

  for (i=0; i < s->h; i++)
  {
    if (s->laston && !*s->on)
      s->L[i].needsupdate = 4;

    HL_EraseTextLine(&s->L[i]);
  }
  s->laston = *s->on;
}

//----------------------------------------------------------------------------

void HL_InitIText(hu_itext_t * it, int x, int y, 
    const H_font_t *font, bool * on)
{
  // default left margin is start of text
  it->margin = 0;

  it->on = on;
  it->laston = true;

  HL_InitTextLine(&it->L, x, y, font);
}

// The following deletion routines adhere to the left margin restriction
void HL_DelCharFromIText(hu_itext_t * it)
{
  if (it->L.len != it->margin)
    HL_DelCharFromTextLine(&it->L);
}

void HL_EraseLineFromIText(hu_itext_t * it)
{
  while (it->margin != it->L.len)
    HL_DelCharFromTextLine(&it->L);
}

// Resets left margin as well
void HL_ResetIText(hu_itext_t * it)
{
  it->margin = 0;
  HL_ClearTextLine(&it->L);
}

void HL_AddPrefixToIText(hu_itext_t * it, const char *str)
{
  for (; *str; str++)
    HL_AddCharToTextLine(&it->L, *str);

  it->margin = it->L.len;
}

// wrapper function for handling general keyed input.
// returns true if it ate the key
bool HL_KeyInIText(hu_itext_t * it, const char ch)
{
  if (ch >= ' ' && ch <= '_')
    HL_AddCharToTextLine(&it->L, (char)ch);
  else if (ch == KEYD_BACKSPACE)
    HL_DelCharFromIText(it);
  else if (ch != KEYD_ENTER)
    return false;

  // ate the key
  return true;
}

void HL_DrawIText(hu_itext_t * it)
{
  if (!*it->on)
    return;

  // draw the line with cursor
  HL_DrawTextLine(&it->L, true);

}

void HL_EraseIText(hu_itext_t * it)
{
  if (it->laston && !*it->on)
    it->L.needsupdate = 4;
    
  HL_EraseTextLine(&it->L);

  it->laston = *it->on;
}

