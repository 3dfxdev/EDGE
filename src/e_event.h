//----------------------------------------------------------------------------
//  EDGE Event handling Header
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2001  The EDGE Team.
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

#ifndef __E_EVENT_H__
#define __E_EVENT_H__

#include "dm_type.h"

//
// Event handling.
//

// Input event types.
// -KM- 1998/09/01 Amalgamate joystick/mouse into analogue
typedef enum
{
  ev_keydown,
  ev_keyup,
  ev_analogue
}
evtype_t;

// Event structure.
typedef struct
{
  evtype_t type;

  union
  {
    int key;
    struct
    {
      int axis;
      int amount;
    } analogue;
  } value;
}
event_t;

// -KM- 1998/11/25 Added support for finales before levels
typedef enum
{
  ga_nothing,
  ga_loadlevel,
  ga_newgame,
  ga_loadgame,
  ga_savegame,
  ga_playdemo,
  ga_completed,
  ga_worlddone,
  ga_screenshot,
  ga_briefing
}
gameaction_t;

//
// Button/action code definitions.
//
typedef enum
{
  // Press "Fire".
  BT_ATTACK = 1,

  // Use button, to open doors, activate switches.
  BT_USE = 2,

  // Flag: game events, not really buttons.
  BT_SPECIAL = 128,
  BT_SPECIALMASK = 3,

  // Flag, weapon change pending.
  // If true, the next 3 bits hold weapon num.
  BT_CHANGE = 4,

  // The 3bit weapon mask and shift, convenience.
  BT_WEAPONMASK = (8 + 16 + 32 + 64),
  BT_WEAPONSHIFT = 3,

  // Pause the game.
  BTS_PAUSE = 1,

  // Save the game at each console.
  BTS_SAVEGAME = 2,

  // Savegame slot numbers
  //  occupy the second byte of buttons.    
  BTS_SAVEMASK = (4 + 8 + 16),
  BTS_SAVESHIFT = 2
}
buttoncode_t;

//
// Extended Buttons: EDGE Specfics
// -ACB- 1998/07/03
//
typedef enum
{
  EBT_JUMP = 1,
  EBT_MLOOK = 2,
  EBT_CENTER = 4,

  // -AJA- 2000/02/08: support for second attack.
  EBT_SECONDATK = 8,

  // -AJA- 2000/03/18: more control over zooming
  EBT_ZOOM = 16
}
extbuttoncode_t;

//
// GLOBAL VARIABLES
//
#define MAXEVENTS 128

extern event_t events[MAXEVENTS];
extern int eventhead;
extern int eventtail;

extern gameaction_t gameaction;

#endif
