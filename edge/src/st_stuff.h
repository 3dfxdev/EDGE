//----------------------------------------------------------------------------
//  EDGE Status Bar Code
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
//
//  Based on the DOOM source code, released by Id Software under the
//  following copyright:
//
//    Copyright (C) 1993-1996 by id Software, Inc.
//
//----------------------------------------------------------------------------

#ifndef __STSTUFF_H__
#define __STSTUFF_H__

#include "dm_type.h"
#include "e_event.h"

// Size of statusbar.
#define ST_HEIGHT       32
#define ST_WIDTH        320

#define ST_X            0
#define ST_Y            (200 - ST_HEIGHT)

//
// STATUS BAR
//

// Called by main loop.
bool ST_Responder(event_t * ev);

// Called by main loop.
void ST_Ticker(void);

// Called by main loop.
void ST_Drawer(void);

// Called when the console player is spawned on each level.
void ST_Start(void);

// Called when changing resolution
void ST_ReInit(void);

// Called by startup code.
bool ST_Init(void);

// States for the chat code.
typedef enum
{
  StartChatState,
  WaitDestState,
  GetChatState
}
st_chatstateenum_t;

#endif
