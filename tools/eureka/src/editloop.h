//------------------------------------------------------------------------
//  EDIT LOOP
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2009 Andrew Apted
//  Copyright (C) 1997-2003 André Majorel et al
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

#ifndef __EDITLOOP_H__
#define __EDITLOOP_H__

class menubar_c;
class highlight_c;


/* This structure holds all the data necessary to an edit window. */
// FIXME: make a class of it.

class Editor_State_c
{
public:
    int move_speed;   // Movement speed.
    int extra_zoom;   // Act like the zoom was 4 times what it is
    int obj_type;   // The mode (OBJ_LINEDEF, OBJ_SECTOR...)

    bool show_object_numbers; // Whether the object numbers are shown
    bool show_things_squares; // Whether the things squares are shown
    bool show_things_sprites; // Whether the things sprites are shown
    int rulers_shown;   // Whether the rulers are shown (unused ?)
    int map_x;    // Map coordinates of pointer
    int map_y;
    int pointer_in_window;  // If false, pointer_[xy] are not meaningful.
    Objid clicked;    // The object that was under the pointer when
        // when the left click occurred. If clicked on
        // empty space, == CANVAS.
    int click_ctrl;   // Was Ctrl pressed at the moment of the click?
    unsigned long click_time; // Date of last left click in ms
    
	Objid highlighted;    // The highlighted object

    selection_c *Selected;    // Linked list of selected objects (or NULL)

    highlight_c *highlight;

    int RedrawMap;

public:
  Editor_State_c();
  virtual ~Editor_State_c();
};

extern Editor_State_c edit;



void EditorLoop (const char *);

extern int InputSectorType(int x0, int y0, int *number);
extern int InputLinedefType(int x0, int y0, int *number);
extern int InputThingType(int x0, int y0, int *number);


void EditorKey(int is_key, bool is_shift = false);
void EditorWheel(int delta);
void EditorMousePress(bool is_ctrl);
void EditorMouseRelease (void);
void EditorMouseMotion(int x, int y, int map_x, int map_y, bool drag);


void ed_highlight_object (Objid& obj);
void ed_forget_highlight ();

#endif /* __EDITLOOP_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
