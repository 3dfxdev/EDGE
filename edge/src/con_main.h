//----------------------------------------------------------------------------
//  EDGE Console Main
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

#include "dm_type.h"

#ifdef __GUI_GUI_H__
#ifndef CON_MAIN_GUI_H
// console stuff that needs gui stuff. Included if you include gui_gui.h.
#define CON_MAIN_GUI_H
// The console 'application' functions.
// Adds the console to the gui.
void CON_Start(gui_t ** gui);

// Ticker.  Animates the opening/closing effect of the console
void CON_Ticker(gui_t * gui);

// Responder obeys events.
boolean_t CON_Responder(gui_t * gui, guievent_t * event);

// Drawer. Draws the console.
void CON_Drawer(gui_t * gui);

// Re-inits the console after a resolution change.
void CON_InitResolution(void);

// Inits the console for the given dimensions.
void CON_InitConsole(int width, int height, int gfxmode);

#endif
#endif

#ifndef CON_MAIN_H
#define CON_MAIN_H

void CON_TryCommand(const char *cmd);

// Prints messages.  cf printf.
void CON_Printf(const char *message,...) GCCATTR(format(printf, 1, 2));

// Like CON_Printf, but appends an extra '\n'. Should be used for player
// messages that need more than MessageLDF.

void CON_Message(const char *message,...) GCCATTR(format(printf, 1, 2));

// Looks up the string in LDF, appends an extra '\n', and then writes it to
// the console. Should be used for most player messages.
void CON_MessageLDF(const char *message,...);

// -ACB- 1999/09/22
// Introduced because MSVC and DJGPP handle #defines differently
void CON_PlayerMessage(player_t *plyr, const char *message, ...) GCCATTR(format(printf, 2, 3));
// Looks up in LDF.
void CON_PlayerMessageLDF(player_t *plyr, const char *message, ...);

typedef enum
{
  // invisible
  vs_notvisible,
  // fullscreen + a command line
  vs_maximal,
  NUMVIS
}
visible_t;

// Displays/Hides the console.
void CON_SetVisible(visible_t v);

#endif
