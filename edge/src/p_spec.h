//----------------------------------------------------------------------------
//  EDGE Specials Lines, Elevator & Floor Code
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
//
// -KM-  1998/09/01 Lines.ddf
// -ACB- 1998/09/13 Cleaned Up.
// -ACB- 2001/01/14 Added Elevator Types
//

#ifndef __P_SPEC__
#define __P_SPEC__

#include "e_player.h"
#include "ddf_main.h"
#include "r_defs.h"
#include "w_image.h"

#define CEILSPEED   1.0f
#define FLOORSPEED  1.0f

#define GRAVITY     8.0f
#define FRICTION    0.9063f
#define VISCOSITY   0.0f
#define DRAG        0.99f
#define RIDE_FRICTION    0.7f
#define LADDER_FRICTION  0.8f

#define STOPSPEED   0.15f
#define OOF_SPEED   20.0f

#define MENU_GRAV_NORMAL  8

typedef enum
{
  MDT_INVALID  = 0,
  MDT_ELEVATOR = 1,
  MDT_PLANE    = 2,
  MDT_SLIDER   = 3,
  ENDOFMDTTYPES
}
movedat_e;

typedef struct light_s
{
  // type of light effect
  const lighttype_t *type;

  sector_t *sector;

  // countdown value to next change, or 0 if disabled
  int count;

  // dark and bright levels
  int minlight;
  int maxlight;
  
  // current direction for GLOW type, -1 down, +1 up
  int direction;

  // countdown value for FADE type
  int fade_count;
 
  struct light_s *prev, *next;
}
light_t;

typedef enum
{
  BWH_None,
  BWH_Top,
  BWH_Middle,
  BWH_Bottom
}
bwhere_e;

typedef struct button_s
{
  line_t *line;
  bwhere_e where;
  const image_t *bimage;
  int btimer;
  sfx_t *off_sound;
}
button_t;

// -ACB- 2001/01/29 Maybe I'm thinking too OO.
typedef struct gen_move_s
{
  movedat_e whatiam;
  struct gen_move_s *next, *prev;
}
gen_move_t;

typedef struct elev_move_s
{
  movedat_e whatiam;
  struct elev_move_s *next, *prev;

  const elevator_sector_t *type;
  sector_t *sector;

  float_t startheight;
  float_t destheight;
  float_t speed;

  // 1 = up, 0 = waiting at top, -1 = down
  int direction;
  int olddirection;

  int tag;

  // tics to wait when fully open
  int waited;

  boolean_t sfxstarted;

  int newspecial;

  const image_t *new_ceiling_image;
  const image_t *new_floor_image;
}
elev_move_t;

typedef struct plane_move_s
{
  movedat_e whatiam;
  struct plane_move_s *next, *prev;

  const moving_plane_t *type;
  sector_t *sector;

  boolean_t is_ceiling;

  float_t startheight;
  float_t destheight;
  float_t speed;
  boolean_t crush;

  // 1 = up, 0 = waiting at top, -1 = down
  int direction;
  int olddirection;

  int tag;

  // tics to wait when fully open
  int waited;

  boolean_t sfxstarted;

  int newspecial;
  const image_t *new_image;
}
plane_move_t;

typedef struct slider_move_s
{
  movedat_e whatiam;
  struct slider_move_s *next, *prev;

  const sliding_door_t *info;
  line_t *line;

  // current distance it has opened
  float_t opening;

  // target distance
  float_t target;

  // length of line
  float_t line_len;
 
  // 1 = opening, 0 = waiting, -1 = closing
  int direction;

  // tics to wait at the top
  int waited;

  boolean_t sfxstarted;
  boolean_t final_open;
}
slider_move_t;

// End-level timer (-TIMER option)
extern boolean_t levelTimer;
extern int levelTimeCount;

extern int maxbuttons;
extern button_t *buttonlist;
extern light_t *lights;
extern gen_move_t *active_movparts;

extern linedeftype_t donut[2];

// at map load
void P_SpawnSpecials(int autotag);

// every tic
void P_UpdateSpecials(void);

// when needed
boolean_t P_UseSpecialLine(mobj_t * thing, line_t * line, int side,
    float_t open_bottom, float_t open_top);
boolean_t P_CrossSpecialLine(line_t *ld, int side, mobj_t * thing);
void P_ShootSpecialLine(line_t *ld, int side, mobj_t * thing);
void P_RemoteActivation(mobj_t * thing, int typenum, int tag, 
    int side, trigger_e method);
void P_PlayerInSpecialSector(player_t * player, sector_t *sec);

// Utilities...
int P_TwoSided(int sector, int line);
side_t *P_GetSide(int currentSector, int line, int side);
sector_t *P_GetSector(int currentSector, int line, int side);
sector_t *P_GetNextSector(const line_t * line, const sector_t * sec);

// Info Needs....
float_t P_FindSurroundingHeight(const heightref_e ref, const sector_t *sec);
float_t P_FindRaiseToTexture(sector_t * sec);  // -KM- 1998/09/01 New func, old inline

// -AJA- 1999/09/29: added this.
sector_t *P_FindSectorFromTag(int tag);

int P_FindMinSurroundingLight(sector_t * sector, int max);

// start an action...
boolean_t EV_Lights(sector_t * sec, const lighttype_t * type);

void P_RunActiveSectors(void);

void P_RemoveAllActiveParts(void);
void P_AddActivePart(gen_move_t *movpart);

extern line_t * line_speciallist;
extern sector_t * sect_speciallist;
void P_AddSpecialLine(line_t *ld);
void P_AddSpecialSector(sector_t *sec);

void P_RunLights(void);
light_t *P_NewLight(void);
void P_DestroyAllLights(void);
void P_RunSectorSFX(void);
void P_DestroyAllSectorSFX(void);

void EV_LightTurnOn(int tag, int bright);
boolean_t EV_DoDonut(sector_t * s1, sfx_t * sfx[4]);
boolean_t EV_Teleport(line_t * line, int tag, int side, mobj_t * thing, 
    int delay, int special, const mobjinfo_t * ineffectobj,
    const mobjinfo_t * outeffectobj);
boolean_t EV_ManualPlane(line_t * line, mobj_t * thing, const moving_plane_t * type);
boolean_t EV_ManualElevator(line_t * line, mobj_t * thing, const elevator_sector_t * type);

void EV_DoSlider(line_t * line, mobj_t * thing, const sliding_door_t * s);
boolean_t EV_DoPlane(sector_t * sec, const moving_plane_t * type, sector_t * model);
boolean_t EV_DoElevator(sector_t * sec, const elevator_sector_t * type, sector_t * model);

//
//  P_SWITCH
//
boolean_t P_InitSwitchList(void);
void P_ChangeSwitchTexture(line_t * line, boolean_t useAgain, line_special_e specials, boolean_t noSound);
boolean_t P_ButtonCheckPressed(line_t * line);

#endif
