//----------------------------------------------------------------------------
//  EDGE GUI Controls
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

#ifndef __GUI_CTLS_H__
#define __GUI_CTLS_H__

#include "gui_gui.h"
#include "v_screen.h"

// Standard gui controls.  (Need more!)

// Message box.  Displays a string in a box with an ok button
//    gui_t **g:  the gui subsystem to add to
//    gui_t *parent: the app to notify when the message box is closed.
//    int msg, int id:  When the app is closed, the parents responder will be passed
//            an event with type = msg, data1 = id.
//    char *string:  The string displayed in the message box.
gui_t *GUI_MSGStart(gui_t ** g, gui_t * parent, int msg, int id, char *string);

// Button. (Needs work)   Displays a push button.
//    gui_t **g:  The gui subsystem to add to
//    gui_t *parent: the app to notify when the button is clicked
//    int id: When the button is clicked, the parents responder will be passed an
//            event with type = gev_bnclick and data1 = id
gui_t *GUI_BTStart(gui_t ** g, gui_t * parent, int id, int x, int y, char *string);

// A progress bar, bar chart bar, health bar etc.
//    gui_t **g:  The gui subsystem to attach to
//    char *watch: The console variable to watch
//    int max: The maximum value the cvar is likely to get to.
//
gui_t *GUI_BARStart(gui_t ** g, char *watch, int max);

// A Drag control
//    This invisible control will send gev_drag messages to it's parent
//    when it is dragged.  Move and size it by sending it gev_move and
//    gev_size events.
gui_t *GUI_DRAGStart(gui_t ** g, gui_t * parent, int id);

// Writes the string to the screen.  Same as M_WriteText, but
// doesn't scale to 320x200.
void GUI_WriteText(screen_t * scr, int x, int y, char *string);

#endif
