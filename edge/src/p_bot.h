//----------------------------------------------------------------------------
//  EDGE Play Simulation Action routines: 'DeathBots'
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

#ifndef __P_BOT_H__
#define __P_BOT_H__

#include "e_player.h"

typedef struct bot_s
{
  player_t player;
  boolean_t strafedir;
  int confidence;
  struct bot_s *next;
}
bot_t;

void BOT_DMSpawn(void);
void P_RemoveBots(void);

#endif
