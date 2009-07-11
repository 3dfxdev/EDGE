//------------------------------------------------------------------------
//  EDIT LOOP
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


void EditorLoop (const char *);
const char *SelectLevel (int levelno);
extern int InputSectorType(int x0, int y0, int *number);
extern int InputLinedefType(int x0, int y0, int *number);
extern int InputThingType(int x0, int y0, int *number);


void EditorKey(int is_key, bool is_shift = false);
void EditorWheel(int delta);
void EditorMousePress(bool is_ctrl);
void EditorMouseRelease (void);
void EditorMouseMotion(int x, int y, int map_x, int map_y, bool drag);


void ed_draw_all();
void ed_highlight_object (Objid& obj);
void ed_forget_highlight ();
//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
