//----------------------------------------------------------------------------
//  EDGE Specials Lines & Floor Code
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
//
// -KM- 1998/09/01 Lines.ddf
//
// -ACB- 1998/09/13 Cleaned Up.
//

#ifndef __P_SPEC__
#define __P_SPEC__

#include "e_player.h"
#include "ddf_main.h"
#include "r_defs.h"

#define MAXSWITCHES 50
#define MAXBUTTONS  32
#define BUTTONTIME  35

#define GLOWSPEED   8.0f
#define CEILSPEED   1.0f
#define FLOORSPEED  1.0f

#define STOPSPEED   0.0625f
#define FRICTION    0.9063f
#define VISCOSITY   0.0f
#define GRAVITY     8.0f
#define OOF_SPEED   20.0f

#define MENU_GRAV_NORMAL  8


typedef struct light_s
{
  sector_t *sector;

  litetype_e type;

  int count;

  int minlight;
  int maxlight;
  int direction;
  int darktime;
  int brighttime;
  int probability;

  struct light_s *prev, *next;
}
light_t;

typedef enum
{
  BWH_Top,
  BWH_Middle,
  BWH_Bottom
}
bwhere_e;

typedef struct button_s
{
  line_t *line;
  bwhere_e where;
  int btexture;
  int btimer;
  mobj_t *soundorg;
}
button_t;

// -KM- 1998/09/01 lines.ddf
typedef struct sec_move_s
{
  const moving_plane_t *type;
  sector_t *sector;

  // Ceiling is true
  boolean_t floorOrCeiling;

  float_t startheight;
  float_t destheight;
  float_t speed;
  boolean_t crush;

  // 1 = up, 0 = waiting at top, -1 = down
  int direction;
  int olddirection;

  int tag;

  // tics to wait at the top
  int waited;

  boolean_t sfxstarted;
  boolean_t completed;

  int newspecial;
  int texture;

  struct sec_move_s *next, *prev;
}
sec_move_t;

typedef enum
{
  RES_Ok,
  RES_Crushed,
  RES_PastDest
}
result_e;

// End-level timer (-TIMER option)
extern boolean_t levelTimer;
extern int levelTimeCount;

extern int maxbuttons;
extern button_t *buttonlist;

// at game start
void P_InitPicAnims(void);
void P_ReInitPicAnims(int pos);

// at map load
void P_SpawnSpecials(int autotag);

// every tic
void P_UpdateSpecials(void);

// when needed
boolean_t P_UseSpecialLine(mobj_t * thing, line_t * line, int side,
    float_t open_bottom, float_t open_top);
boolean_t P_CrossSpecialLine(line_t *ld, int side, mobj_t * thing);
void P_ShootSpecialLine(mobj_t * thing, line_t * line);
void P_RemoteActivation(mobj_t * thing, int typenum, int tag, 
    int side, trigger_e method);
void P_PlayerInSpecialSector(player_t * player, sector_t *sec);

// Utilities...
int P_TwoSided(int sector, int line);
side_t *P_GetSide(int currentSector, int line, int side);
sector_t *P_GetSector(int currentSector, int line, int side);
sector_t *P_GetNextSector(line_t * line, sector_t * sec);

// Info Needs....
float_t P_FindLowestFloorSurrounding(sector_t * sec);
float_t P_FindHighestFloorSurrounding(sector_t * sec);
float_t P_FindLowestCeilingSurrounding(sector_t * sec);
float_t P_FindHighestCeilingSurrounding(sector_t * sec);
float_t P_FindNextHighestFloor(sector_t * sec, float_t height);
float_t P_FindNextLowestFloor(sector_t * sec, float_t height);
float_t P_FindNextHighestCeiling(sector_t * sec, float_t height);
float_t P_FindNextLowestCeiling(sector_t * sec, float_t height);
float_t P_FindRaiseToTexture(sector_t * sec);  // -KM- 1998/09/01 New func, old inline

// -AJA- 1999/09/29: added this.
sector_t *P_FindSectorFromTag(int tag);

int P_FindMinSurroundingLight(sector_t * sector, int max);

// start an action...
boolean_t EV_Lights(sector_t * sec, const lighttype_t * type);

void P_AddActiveSector(sec_move_t * sec);

void P_RunActiveSectors(void);
void P_RemoveAllActiveSectors(void);
void P_RunLights(void);
light_t *P_NewLight(void);
void P_DestroyAllLights(void);
void P_RunSectorSFX(void);
void P_DestroyAllSectorSFX(void);

void EV_LightTurnOn(line_t * line, int bright);
boolean_t EV_DoDonut(sector_t * s1, sfx_t * sfx[4]);
boolean_t EV_Teleport(line_t * line, int tag, int side, mobj_t * thing, 
    int delay, int special, const mobjinfo_t * ineffectobj,
    const mobjinfo_t * outeffectobj);
boolean_t EV_Manual(line_t * line, mobj_t * thing, const moving_plane_t * type);
boolean_t EV_DoSector(sector_t * sec, const moving_plane_t * type, sector_t * model);

//
//  P_SWITCH
//
void P_InitSwitchList(void);
void P_ChangeSwitchTexture(line_t * line, int useAgain, 
    line_flag_e specials);
boolean_t P_ButtonCheckPressed(line_t * line);

#endif
