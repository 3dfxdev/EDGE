//----------------------------------------------------------------------------
//  EDGE GUI Main
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

#ifndef __GUI_MAIN_H__
#define __GUI_MAIN_H__

#include "dm_type.h"
#include "e_event.h"

// This function inits the GUI system (&& console)
void GUI_MainInit(void);
void GUI_MouseInit(void);
void GUI_ConInit(void);

// This function calls the tickers for all gui apps.
void GUI_MainTicker(void);

// This function handles input to all guis.
bool GUI_MainResponder(event_t * ev);

// This function draws all the guis to the screen
void GUI_MainDrawer(void);

// Reinits the gui system for a new resolution
void GUI_InitResolution(void);

#endif
