//----------------------------------------------------------------------------
//  EDGE Play Simulation Action routines: 'DeathBots'
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
// It will be translated to a ticcmd_t by P_BotPlayerBuilder.

//
// -ACB- 2003/05/15 Moved followtype and face enum outside the structure. This is to fixed 
// issues with declaring enums in structures for GCC3 and VC.NET
//
typedef enum
{
	BOTCMD_FOLLOW_NONE = 0,
	BOTCMD_FOLLOW_MOBJ,
	BOTCMD_FOLLOW_XY,
	BOTCMD_FOLLOW_DIR
} 
botcmd_follow_e;

typedef enum
{
	BOTCMD_FACE_NONE = 0,
	BOTCMD_FACE_MOBJ,
	BOTCMD_FACE_XYZ,
	BOTCMD_FACE_ANGLE
}
botcmd_face_e;
	
typedef struct botcmd_s
{
	int followtype;

	union
	{
		struct {float x,y;} xyz;
		struct {angle_t angle; float distance;} dir;
		mobj_t *mo;
	} 
	followobj;

	// If we want to face someone, do this here.
	// Either face a mobj, a specified map coordinate, or a given angle.
	int	facetype;

	union
	{
		struct {float x,y,z;} xyz;
		struct {angle_t angle; float slope;} angle;
		mobj_t *mo;
	}
	faceobj;

	// The weapon we want to use. -1 if the current one is fine.
	int new_weapon;

	bool attack;
	bool second_attack;
	bool use;
	bool jump;
} 
botcmd_t;


typedef struct bot_s
{
	const player_t *pl;

	bool strafedir;
	int confidence;
	int threshold;
	int movecount;

	mobj_t *target;
	mobj_t *supportobj;
	angle_t angle;

	botcmd_t cmd;
}
bot_t;

void P_BotCreate(player_t *p, bool recreate);
void P_RemoveBots(void);

#endif
