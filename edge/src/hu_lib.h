//----------------------------------------------------------------------------
//  EDGE Heads-up-display library Code (header)
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
#include "w_image.h"
#include "hu_font.h"
#include "hu_style.h"

//
//  Font stuff
//


#define HU_MAXLINES		4
#define HU_MAXLINELENGTH	80

//
// Typedefs of widgets
//

// Text Line widget

typedef struct
{
	// position of top/left corner of text
	int x, y;

	// style
	style_c *style;
	int text_type;

	// line of text
	char ch[HU_MAXLINELENGTH + 1];

	// current line length
	int len;

	// centre text horizontally, around x.
	bool centre;
}
hu_textline_t;

// Scrolling Text window widget
//  (child of Text Line widget)

typedef struct
{
	// text lines to draw
	hu_textline_t L[HU_MAXLINES];

	// height in lines
	int h;

	// current line number
	int curline;
}
hu_stext_t;

// Input Text Line widget
//  (child of Text Line widget)

typedef struct
{
	// text line to input on
	hu_textline_t L;

	// left margin past which I am not to delete characters
	int margin;
}
hu_itext_t;

//
// Widget creation, access, and update routines
//

// initialises heads-up widget library
void HL_Init(void);

//
// Text Line routines
//

// clear a line of text
void HL_ClearTextLine(hu_textline_t * t);

void HL_InitTextLine(hu_textline_t * t, int x, int y, 
    style_c *style, int text_type);

// returns success
bool HL_AddCharToTextLine(hu_textline_t * t, char ch);

// returns success
bool HL_DelCharFromTextLine(hu_textline_t * t);

// draws tline
void HL_DrawTextLine(hu_textline_t * l, bool drawcursor);

// erases text line
void HL_EraseTextLine(hu_textline_t * l);

//
// Scrolling Text window widget routines
//

// initialise new stext
void HL_InitSText(hu_stext_t * s, int x, int y, int h, 
    style_c *style, int text_type);

// add a new line
void HL_AddLineToSText(hu_stext_t * s);

// add message to stext
void HL_AddMessageToSText(hu_stext_t * s, 
    const char *prefix, const char *msg);

// draws stext
void HL_DrawSText(hu_stext_t * s);

// erases all stext lines
void HL_EraseSText(hu_stext_t * s);

// Input Text Line widget routines
void HL_InitIText(hu_itext_t * it, int x, int y, 
    style_c *style, int text_type);

// enforces left margin
void HL_DelCharFromIText(hu_itext_t * it);

// enforces left margin
void HL_EraseLineFromIText(hu_itext_t * it);

// resets line and left margin
void HL_ResetIText(hu_itext_t * it);

// left of left-margin
void HL_AddPrefixToIText(hu_itext_t * it, const char *str);

// whether eaten
bool HL_KeyInIText(hu_itext_t * it, char ch);

void HL_DrawIText(hu_itext_t * it);

// erases all itext lines
void HL_EraseIText(hu_itext_t * it);

// -ACB- 1998/06/10
void HL_DrawTextLineAlpha(hu_textline_t * l, bool drawcursor, 
    const colourmap_c *colmap, float alpha);


void HL_WriteText(style_c *style, int text_type, int x, int y, const char *str);
void HL_WriteTextTrans(style_c *style, int text_type, int x, int y,
	const colourmap_c *colmap, const char *str);

#endif  // __HULIB__
