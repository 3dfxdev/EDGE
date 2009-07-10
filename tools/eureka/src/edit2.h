/*
 *  _edit.h
 *  AYM 1998-09-06
 */


/*
This file is part of Yadex.

Yadex incorporates code from DEU 5.21 that was put in the public domain in
1994 by Raphaël Quinet and Brendon Wyber.

The rest of Yadex is Copyright © 1997-2003 André Majorel and others.

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation; either version 2 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program; if not, write to the Free Software Foundation, Inc., 59 Temple
Place, Suite 330, Boston, MA 02111-1307, USA.
*/


#ifndef YH__EDIT  /* DO NOT INSERT ANYTHING BEFORE THIS LINE */
#define YH__EDIT




class menubar_c;
class selbox_c;
class spot_c;
class highlight_c;


// The numbers of the items on the menu bar
enum
{
  MBI_FILE,
  MBI_EDIT,
  MBI_VIEW,
  MBI_SEARCH,
  MBI_MISC,
  MBI_OBJECTS,
  MBI_CHECK,
  MBI_HELP,
  MBI_COUNT
};


// The numbers of the actual menus (Menu objects)
enum
{
  MBM_FILE,
  MBM_EDIT,
  MBM_VIEW,
  MBM_SEARCH,
  MBM_MISC_L,  // The "Misc. operations" menus changes with the mode
  MBM_MISC_S,
  MBM_MISC_T,
  MBM_MISC_V,
  MBM_OBJECTS,
  MBM_CHECK,
  MBM_HELP,
  MBM_COUNT
};


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
    SelPtr Selected;    // Linked list of selected objects (or NULL)

    selbox_c    *selbox;   // The selection box
    spot_c      *spot;   // The insertion spot
    highlight_c *highlight;

    int RedrawMap;

public:
  Editor_State_c();
  virtual ~Editor_State_c();
};

extern Editor_State_c edit;


#endif  /* DO NOT ADD ANYTHING AFTER THIS LINE */
//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
