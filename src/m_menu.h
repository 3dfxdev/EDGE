//----------------------------------------------------------------------------
//  EDGE Main Menu Code
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
//
//  Based on the DOOM source code, released by Id Software under the
//  following copyright:
//
//    Copyright (C) 1993-1996 by id Software, Inc.
//
//----------------------------------------------------------------------------

#ifndef __M_MENU__
#define __M_MENU__

#include "e_event.h"

//
// MENUS
//
// Called by main loop,
// saves config file and calls I_Quit when user exits.
// Even when the menu is not displayed,
// this can resize the view and change game parameters.
// Does all the real work of the menu interaction.
boolean_t M_Responder(event_t * ev);

// Called by main loop,
// only used for menu (skull cursor) animation.
void M_Ticker(void);

// Called by main loop,
// draws the menus directly into the screen buffer.
void M_Drawer(void);

// Called by D_DoomMain,
// loads the config file.
boolean_t M_Init(void);

// Called by intro code to force menu up upon a keypress,
// does nothing if menu is already up.
void M_StartControlPanel(void);

// hu_font size routines
int M_CharWidth(int c);
int M_TextMaxLen(int max_w, const char *str);
int M_StringWidth(const char *string);
int M_StringHeight(const char *string);

void M_WriteText(int x, int y, const char *string);
void M_WriteTextTrans(int x, int y, const byte *trans, const char *string);

// 25-6-98 KM
void M_StartMessage(const char *string, void (* routine)(int response), 
    boolean_t input);

// -KM- 1998/07/21
// String will be printed as a prompt.
// Routine should be void Foobar(char *string)
// and will be called with the input returned
// or NULL if user pressed escape.

void M_StartMessageInput(const char *string, 
    void (* routine)(char *response));

void M_DoSave(int slot);
void M_QuickSave(void);

void M_QuitEDGE(int choice);

#endif
