//----------------------------------------------------------------------------
//  EDGE Heads-up-display library Code (header)
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2000  The EDGE Team.
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

#ifndef __HULIB__
#define __HULIB__

// We are referring to patches.
#include "r_defs.h"

// font stuff
#define HU_CHARERASE	KEYD_BACKSPACE

#define HU_MAXLINES		4
#define HU_MAXLINELENGTH	80

//
// Typedefs of widgets
//

// Text Line widget

typedef struct
{
  // position of top/left corner of text
  int x;
  int y;

  // font;
  const patch_t **f;  // font

  // start character in font
  int sc;

  // line of text
  unsigned char l[HU_MAXLINELENGTH + 1];

  // current line length
  int len;

  // whether this line needs to be udpated
  int needsupdate;

  // centre text horizontally, around x.
  boolean_t centre;
}
hu_textline_t;

// Scrolling Text window widget
//  (child of Text Line widget)

typedef struct
{
  // text lines to draw
  hu_textline_t l[HU_MAXLINES];

  // height in lines
  int h;

  // current line number
  int cl;

  // pointer to boolean_t stating whether to update window
  boolean_t *on;
  boolean_t laston;  // last value of *->on.

}
hu_stext_t;

// Input Text Line widget
//  (child of Text Line widget)

typedef struct
{
  // text line to input on
  hu_textline_t l;

  // left margin past which I am not to delete characters
  int lm;

  // pointer to boolean_t stating whether to update window
  boolean_t *on;
  boolean_t laston;  // last value of *->on;

}
hu_itext_t;

//
// Widget creation, access, and update routines
//

// initialises heads-up widget library
void HUlib_init(void);

//
// Text Line routines
//

// clear a line of text
void HUlib_clearTextLine(hu_textline_t * t);

void HUlib_initTextLine(hu_textline_t * t, int x, int y, const patch_t ** f, int sc);

// returns success
boolean_t HUlib_addCharToTextLine(hu_textline_t * t, char ch);

// returns success
boolean_t HUlib_delCharFromTextLine(hu_textline_t * t);

// draws tline
void HUlib_drawTextLine(hu_textline_t * l, boolean_t drawcursor);

// erases text line
void HUlib_eraseTextLine(hu_textline_t * l);

//
// Scrolling Text window widget routines
//

// initialise new stext
void HUlib_initSText(hu_stext_t * s, int x, int y, int h, const patch_t ** font, int startchar, boolean_t * on);

// add a new line
void HUlib_addLineToSText(hu_stext_t * s);

// ?
void HUlib_addMessageToSText(hu_stext_t * s, const char *prefix, const char *msg);

// draws stext
void HUlib_drawSText(hu_stext_t * s);

// erases all stext lines
void HUlib_eraseSText(hu_stext_t * s);

// Input Text Line widget routines
void HUlib_initIText(hu_itext_t * it, int x, int y, const patch_t ** font, int startchar, boolean_t * on);

// enforces left margin
void HUlib_delCharFromIText(hu_itext_t * it);

// enforces left margin
void HUlib_eraseLineFromIText(hu_itext_t * it);

// resets line and left margin
void HUlib_resetIText(hu_itext_t * it);

// left of left-margin
void HUlib_addPrefixToIText(hu_itext_t * it, char *str);

// whether eaten
boolean_t HUlib_keyInIText(hu_itext_t * it, unsigned char ch);

void HUlib_drawIText(hu_itext_t * it);

// erases all itext lines
void HUlib_eraseIText(hu_itext_t * it);

// -ACB- 1998/06/10
void HUlib_drawTextLineAlpha(hu_textline_t * l, boolean_t drawcursor, 
    const byte *trans, fixed_t alpha);

#define HUlib_drawTextLineTrans(L,DC,TR,AL)  \
    HUlib_drawTextLineAlpha(L,DC,TR,AL,alpha)


#endif
