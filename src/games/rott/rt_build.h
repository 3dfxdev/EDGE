/*
Copyright (C) 1994-1995 Apogee Software, Ltd.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#pragma once

//***************************************************************************
//
// RT_BUILD.C
//
//***************************************************************************

#define MAX(x,y)               ((x>y) ? (x) : (y))
#define MAXPLANES 10

// Should be 10 with titles
#define MENUOFFY     (10)
#define MENUBACKNAME ("plane")
#define MENUTITLEY 10
#define TEXTUREW     (288)
#define TEXTUREWIDTH ((TEXTUREW*MAX_WIDTH_RES)-1)
#define TEXTUREHEIGHT (158)
#define NORMALVIEW   (0x40400L)
#define NORMALHEIGHTDIVISOR   (156000000)
#define NORMALWIDTHMULTIPLIER (241)
#define FLIPTIME     20//60

typedef struct
{
   int   x1, y1;
   int   x2, y2;
   int   texturewidth;
   int   texture;
   int   origheight;
} plane_t;

extern int Menuflipspeed;
extern byte * intensitytable;

void SetupMenuBuf ( void );
void ShutdownMenuBuf ( void );

void ClearMenuBuf ( void );
void SetAlternateMenuBuf ( void );
void SetMenuTitle ( const char * menutitle );

void PositionMenuBuf( int angle, int distance, boolean drawbackground );
void RefreshMenuBuf( int time );
void FlipMenuBuf ( void );

void DrawMenuBufItem (int x, int y, int shapenum);
void DrawMenuBufIString (int px, int py, const char *string, int color);
void DrawTMenuBufItem (int x, int y, int shapenum);
void DrawIMenuBufItem (int x, int y, int shapenum, int color);
void DrawColoredMenuBufItem (int x, int y, int shapenum, int color);
void DrawMenuBufPic (int x, int y, int shapenum);
void DrawTMenuBufPic (int x, int y, int shapenum);
void EraseMenuBufRegion (int x, int y, int width, int height);

void DrawMenuBufPropString (int px, int py, const char *string);
void DrawTMenuBufPropString (int px, int py, const char *string);

void DrawTMenuBufBox (int x, int y, int width, int height);
void DrawTMenuBufHLine (int x, int y, int width, boolean up);
void DrawTMenuBufVLine (int x, int y, int height, boolean up);
void MenuBufCPrintLine (const char *s);
void MenuBufCPrint (const char *s);
void MenuBufPrint (const char *s);
void MenuTBufPrintLine (const char *s, int shade);

void DrawMenuBufPicture (int x, int y, const byte * pic, int w, int h);
