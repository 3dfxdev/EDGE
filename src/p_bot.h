//----------------------------------------------------------------------------
//  EDGE Play Simulation Action routines: 'DeathBots'
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

#ifndef __P_BOT_H__
#define __P_BOT_H__

#include "e_player.h"

// This describes what action the bot wants to do.
// It will be translated to a ticcmd_t by P_BotPlayerThinker.
typedef struct botcmd_s
{
  // The
  enum
  {
    FOLLOW_NONE = 0,
    FOLLOW_MOBJ,
    FOLLOW_XY,
    FOLLOW_DIR
  } 
  followtype;
  
  union
  {
    struct {flo_t x,y;} xyz;
    struct {angle_t angle; flo_t distance;} dir;
    mobj_t *mo;
  } 
  followobj;

  // If we want to face someone, do this here.
  // Either face a mobj, a specified map coordinate, or a given angle.
  enum
  {
    FACE_NONE = 0,
    FACE_MOBJ,
    FACE_XYZ,
    FACE_ANGLE
  }
  facetype;

  union
  {
    struct {flo_t x,y,z;} xyz;
    struct {angle_t angle; flo_t slope;} angle;
    mobj_t *mo;
  }
  faceobj;

  // The weapon we want to use. -1 if the current one is fine.
  int new_weapon;

  boolean_t attack;
  boolean_t second_attack;
  boolean_t use;
  boolean_t jump;
} 
botcmd_t;

typedef struct bot_s
{
  const player_t *pl;

  boolean_t strafedir;
  int confidence;
  int threshold;
  int movecount;

  mobj_t *target;
  mobj_t *supportobj;
  angle_t angle;

  botcmd_t cmd;

  // remember previous movements, just in case the thinker is run twice the
  // same gametic.
  ticcmd_t prev_cmd;
  int prev_gametime;

  struct bot_s *next;
}
bot_t;

void P_BotCreate(player_t *p);
void BOT_DMSpawn(void);
void P_RemoveBots(void);

#endif
