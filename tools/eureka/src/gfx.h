//------------------------------------------------------------------------
//  GRAPHICS ROUTINES
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor (C) 2001-2009 Andrew Apted
//                     (C) 1997-2003 André Majorel et al
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
//
//  Based on Yadex which incorporated code from DEU 5.21 that was put
//  in the public domain in 1994 by Raphaël Quinet and Brendon Wyber.
//
//------------------------------------------------------------------------


#ifndef YH_GFX  /* Prevent multiple inclusion */
#define YH_GFX  /* Prevent multiple inclusion */


/* Width and height of font cell. Those figures are not meant to
   represent the actual size of the character but rather the step
   between two characters. For example, for the original 8x8 BGI
   font, FONTW was 8 and FONTH was 10. DrawScreenText() is supposed
   to draw characters properly _centered_ within that space, taking
   into account the optional underscoring. */
extern unsigned FONTW;
extern unsigned FONTH;
extern int font_xofs;
extern int font_yofs;


/* This number is the Y offset to use when underscoring a character.
   For example, if you want to draw an underscored "A" at (x,y),
   you should do :
     DrawScreenString (x, y,         "A");
     DrawScreenString (x, y + FONTU, "_"); */
#define FONTU  1

/* Narrow spacing around text. That's the distance in pixels
   between the characters and the inner edge of the box. */
#define NARROW_HSPACING  4
#define NARROW_VSPACING  2

/* Wide spacing around text. That's the distance in pixels
   between the characters and the inner edge of the box. */
#define WIDE_HSPACING  FONTW
#define WIDE_VSPACING  (FONTH / 2)

/* Boxes */
#define BOX_BORDER    2  // Offset between outer and inner edges of a 3D box
#define NARROW_BORDER 1  // Same thing for a shallow 3D box
#define HOLLOW_BORDER 1  // Same thing for a hollow box
#define BOX_VSPACING  WIDE_VSPACING  // Vertical space between two hollow boxes

/* Parameters set by command line args and configuration file */

extern const char *font_name; // X: the name of the font to load
        // (if NULL, use the default)

/* Global variables */
extern int   ScrMaxX;
extern int   ScrMaxY;

typedef unsigned long xpv_t;  // The type of a pixel value in X's opinion

extern int  text_dot;     // DrawScreenText()/DrawScreenString() debug flag

/* gfx.cc */
int InitGfx (void);
void TermGfx (void);
void SetWindowSize (int width, int height);

void update_display ();
void SetLineThickness (int thick);
void SetDrawingMode (int _xor);

void DrawScreenLine (int, int, int, int);
void DrawScreenRect (int x, int y, int width, int height);
void DrawScreenBox (int, int, int, int);
void DrawScreenBoxwh (int scrx0, int scry0, int width, int height);
void DrawScreenBox3D (int, int, int, int);
void DrawScreenBox3DShallow (int, int, int, int);
void DrawScreenBoxHollow (int x0, int y0, int x1, int y1, acolour_t colour);
void draw_box_border (int x, int y, int width, int height,
   int thickness, int raised);
void DrawScreenText (int, int, const char *, ...);
void DrawScreenString (int, int, const char *);
void DrawScreenChar (int x, int y, char c);
void DrawPointer (bool);
void DrawScreenLineLen (int x, int y, int width, int height);
int TranslateToDoomColor (int);

void draw_point (int x, int y);

#define  set_colour(X)  do{}while(0)
#define push_colour(X)  do{}while(0)
#define  pop_colour(X)  do{}while(0)


#endif  /* DO NOT ADD ANYTHING AFTER THIS LINE */
//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
