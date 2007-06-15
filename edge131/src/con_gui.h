//----------------------------------------------------------------------------
//  EDGE Console Main
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2005  The EDGE Team.
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

#ifndef __CON_MAIN_GUI_H
#define __CON_MAIN_GUI_H

#include "dm_type.h"
#include "gui_gui.h"

// The console 'application' functions.
// Adds the console to the gui.
void CON_Start(gui_t ** gui);

// Ticker.  Animates the opening/closing effect of the console
void CON_Ticker(gui_t * gui);

// Responder obeys events.
bool CON_Responder(gui_t * gui, guievent_t * event);

// Drawer. Draws the console.
void CON_Drawer(gui_t * gui);

// Re-inits the console after a resolution change.
bool CON_InitResolution(void);

// Inits the console for the given dimensions.
void CON_InitConsole(int width, int height, int gfxmode);

#endif // __CON_MAIN_GUI_H

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
