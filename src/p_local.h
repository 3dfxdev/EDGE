//----------------------------------------------------------------------------
//  EDGE Local Header for play sim functions 
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
// -ACB- 1998/07/27 Cleaned up, Can read it now :).
//

#ifndef __P_LOCAL__
#define __P_LOCAL__

#ifndef __R_LOCAL__
#include "r_local.h"
#endif

#define DEATHVIEWHEIGHT  6.0f
#define CROUCH_SLOWDOWN  0.6f

#ifdef USE_GL
#define LOOKUPLIMIT    0.9f
#define LOOKDOWNLIMIT -0.9f
#else
#define LOOKUPLIMIT    0.5f
#define LOOKDOWNLIMIT -0.5f
#endif

// mapblocks are used to check movement
// against lines and things
#define MAPBLOCKUNITS 128

// MAXRADIUS is for precalculated sector block boxes
// the spider demon is larger, but we do not have any moving sectors nearby
#define MAXRADIUS    (32.0f)
#define MAXDLIGHTRADIUS    (200.0f)

#define MAXMOVE      (100.0f)
#define STEPMOVE     (16.0f)
#define USERANGE     (64.0f)
#define USE_Z_RANGE  (32.0f)
#define MELEERANGE   (64.0f)
#define MISSILERANGE (2000.0f)

#define RESPAWN_DELAY  (TICRATE / 2)

// Weapon sprite speeds
#define LOWERSPEED (6.0f)
#define RAISESPEED (6.0f)

#define WPNLOWERSPEED (6.0f)
#define WPNRAISESPEED (6.0f)

#define WEAPONBOTTOM     128
#define WEAPONTOP        32

// follow a player exlusively for 3 seconds
#define BASETHRESHOLD 100

extern mobj_t *RandomTarget;

// useful macro for the vertical center of an object
#define MO_MIDZ(mo)  ((mo)->z + (mo)->height / 2)

//
// P_ACTION
//
void P_ActPlayerAttack(mobj_t * playerobj, const attacktype_t * attack);
void P_ActSlammedIntoObject(mobj_t * object, mobj_t * objecthit);
boolean_t P_ActMissileContact(mobj_t * object, mobj_t * objecthit);
boolean_t P_ActBulletContact(mobj_t * object, mobj_t * objecthit, 
    float_t damage, const damage_t * damtype);
void P_ActTouchyContact(mobj_t * touchy, mobj_t * victim);
boolean_t P_ActUseThing(mobj_t * user, mobj_t * thing, float_t open_bottom,
    float_t open_top);
void P_BringCorpseToLife(mobj_t * corpse);

//
// P_WEAPON
//
void P_SetupPsprites(player_t * curplayer);
void P_MovePsprites(player_t * curplayer);
void P_SetPsprite(player_t * p, int position, int stnum);
void P_DropWeapon(player_t * player);
void P_BringUpWeapon(player_t * player);
boolean_t P_CheckWeaponSprite(weaponinfo_t *info);
void P_SelectNewWeapon(player_t * player, int priority, ammotype_t ammo);
void P_Zoom(player_t * player);
void P_RefillClips(player_t * player);

//
// P_USER
//
void P_PlayerThink(player_t * player);
void P_UpdateAvailWeapons(player_t *p);
boolean_t P_AddWeapon(player_t *player, weaponinfo_t *info, int *index);
boolean_t P_RemoveWeapon(player_t *player, weaponinfo_t *info);
void P_GiveInitialBenefits(player_t *player, const mobjinfo_t *info);
void P_AddPlayerToGame(player_t *p);
void P_RemovePlayerFromGame(player_t *p);
void P_RemoveAllPlayers(void);

//
// P_MOBJ
//
#define ONFLOORZ   ((float_t)INT_MIN)
#define ONCEILINGZ ((float_t)INT_MAX)

// -ACB- 1998/07/30 Start Pointer the item-respawn-que.
extern iteminque_t *itemquehead;

// -ACB- 1998/08/27 Start Pointer in the mobj list.
extern mobj_t *mobjlisthead;

void P_SpawnPlayer(player_t *p, const spawnpoint_t * sthing);
void P_RemoveMobj(mobj_t * th);
statenum_t P_MobjFindLabel(mobj_t * mobj, const char *label);
boolean_t P_SetMobjState(mobj_t * mobj, statenum_t state);
boolean_t P_SetMobjStateDeferred(mobj_t * mobj, statenum_t state, int tic_skip);
void P_SetMobjDirAndSpeed(mobj_t * mobj, angle_t angle, float_t slope, float_t speed);
void P_RunMobjThinkers(void);
void P_SpawnPuff(float_t x, float_t y, float_t z, const mobjinfo_t * puff);
void P_SpawnBlood(float_t x, float_t y, float_t z, float_t damage, angle_t angle, const mobjinfo_t * blood);
void P_RemoveQueuedMobjs(void);
void P_CalcFullProperties(const mobj_t *mo, region_properties_t *new);

void P_MobjSetTracer(mobj_t *mo, mobj_t *target);
void P_MobjSetSource(mobj_t *mo, mobj_t *target);
void P_MobjSetTarget(mobj_t *mo, mobj_t *target);
void P_MobjSetSupportObj(mobj_t *mo, mobj_t *target);
void P_MobjSetAboveMo(mobj_t *mo, mobj_t *target);
void P_MobjSetBelowMo(mobj_t *mo, mobj_t *target);
void P_MobjSetRealSource(mobj_t *mo, mobj_t *source);

// -ACB- 1998/08/02 New procedures for DDF etc...
void P_MobjItemRespawn(void);
void P_MobjRemoveMissile(mobj_t * missile);
void P_MobjExplodeMissile(mobj_t * missile);
mobj_t *P_MobjCreateObject(float_t x, float_t y, float_t z, const mobjinfo_t * type);

//
// P_ENEMY
//

typedef struct 
{
  int number;

  // FIXME: big troubles if one of these objects disappears.
  struct mobj_s ** targets;
}
shoot_spot_info_t;

extern dirtype_t opposite[];
extern dirtype_t diags[];
extern float_t xspeed[8];
extern float_t yspeed[8];
extern shoot_spot_info_t brain_spots;

void P_NoiseAlert(player_t *p);
void P_NewChaseDir(mobj_t * actor);
boolean_t P_CreateAggression(mobj_t * actor);
boolean_t P_CheckMeleeRange(mobj_t * actor);
boolean_t P_CheckMissileRange(mobj_t * actor);
boolean_t P_Move(mobj_t * actor, boolean_t path);
boolean_t P_LookForPlayers(mobj_t * actor, angle_t range);
void P_LookForShootSpots(const mobjinfo_t *spot_type);
void P_FreeShootSpots(void);

//
// P_MAPUTL
//
#define PT_ADDLINES  1
#define PT_ADDTHINGS 2
#define PT_EARLYOUT  4

typedef struct
{
  float_t frac;  // along trace line

  enum
  {
    INCPT_Line  = 0,
    INCPT_Thing = 1
  }
  type;

  union
  {
    mobj_t *thing;
    line_t *line;
  }
  d;
}
intercept_t;

typedef boolean_t(*traverser_t) (intercept_t * in);

extern intercept_t **intercepts;
extern int intercept_p;

extern divline_t trace;

float_t P_ApproxDistance(float_t dx, float_t dy);
float_t P_ApproxSlope(float_t dx, float_t dy, float_t dz);
int P_PointOnDivlineSide(float_t x, float_t y, divline_t *div);
int P_PointOnDivlineThick(float_t x, float_t y, divline_t *div,
    float_t div_len, float_t thickness);
float_t P_InterceptVector(divline_t * v2, divline_t * v1);
int P_BoxOnLineSide(float_t * tmbox, line_t * ld);
int P_FindThingGap(vgap_t * gaps, int gap_num, float_t z1, float_t z2);
void P_ComputeGaps(line_t * ld);
float_t P_ComputeThingGap(mobj_t * thing, sector_t * sec, float_t z, float_t * f, float_t * c);
void P_AddExtraFloor(sector_t *sec, line_t *line);
void P_ComputeWallTiles(line_t *ld, int sidenum);
void P_RecomputeGapsAroundSector(sector_t *sec);
void P_RecomputeTilesInSector(sector_t *sec);
void P_FloodExtraFloors(sector_t *sector);
void P_UnsetThingFinally(mobj_t * thing);
void P_SetThingPosition(mobj_t * thing);
void P_ChangeThingPosition(mobj_t * thing, float_t x, float_t y, float_t z);
void P_FreeSectorTouchNodes(sector_t *sec);
  
boolean_t P_BlockLinesIterator(int x, int y, boolean_t(*func) (line_t *));
boolean_t P_BlockThingsIterator(int x, int y, boolean_t(*func) (mobj_t *));
boolean_t P_ThingsInArea(float_t *bbox);
boolean_t P_ThingsOnLine(line_t *ld);

typedef enum
{
  EXFIT_Ok = 0,
  EXFIT_StuckInCeiling,
  EXFIT_StuckInFloor,
  EXFIT_StuckInExtraFloor
}
exfloor_fit_e;

exfloor_fit_e P_ExtraFloorFits(sector_t *sec, float_t z1, float_t z2);

boolean_t P_PathTraverse(float_t x1, float_t y1, float_t x2, float_t y2, int flags, traverser_t trav);

//
// P_MAP
//

typedef enum
{
  // sector move is completely OK
  CHKMOV_Ok = 0,

  // sector move would crush something, but OK
  CHKMOV_Crush = 1,

  // sector can't move (solid floor or uncrushable thing)
  CHKMOV_Nope = 2
}
check_sec_move_e;

// If "floatok" true, move would be OK at float_destz height.
extern boolean_t floatok;
extern float_t float_destz;

extern mobj_t *linetarget;  // who got hit (or NULL)

extern boolean_t mobj_hit_sky;
extern line_t *blockline;

extern stack_array_t spechit_a;
extern line_t **spechit;
extern int numspechit;

boolean_t P_MapInit(void);
boolean_t P_MapCheckBlockingLine(mobj_t * thing, mobj_t * spawnthing);
mobj_t *P_MapFindCorpse(mobj_t * thing);
mobj_t *P_MapTargetAutoAim(mobj_t * source, angle_t angle,
    float_t distance, boolean_t force_aim);
mobj_t *P_MapTargetTheory(mobj_t * source);

float_t P_AimLineAttack(mobj_t * t1, angle_t angle, float_t distance);
void P_UpdateMultipleFloors(sector_t * sector);
boolean_t P_CheckSolidSectorMove(sector_t *sec, boolean_t is_ceiling,
        float_t dh);
boolean_t P_SolidSectorMove(sector_t *sec, boolean_t is_ceiling,
        float_t dh, boolean_t crush, boolean_t nocarething);
void P_ChangeThingSize(mobj_t *mo);
boolean_t P_CheckAbsPosition(mobj_t * thing, float_t x, float_t y, float_t z);
boolean_t P_CheckSight(mobj_t * src, mobj_t * dest);
boolean_t P_CheckSightApproxVert(mobj_t * src, mobj_t * dest);
void P_RadiusAttack(mobj_t * spot, mobj_t * source, float_t radius,
    float_t damage, const damage_t * damtype, boolean_t thrust_only);

boolean_t P_TeleportMove(mobj_t * thing, float_t x, float_t y, float_t z);
boolean_t P_TryMove(mobj_t * thing, float_t x, float_t y);
void P_SlideMove(mobj_t * mo, float_t x, float_t y);
void P_UseLines(player_t * player);
void P_LineAttack(mobj_t * t1, angle_t angle, float_t distance, 
    float_t slope, float_t damage, const damage_t * damtype,
    const mobjinfo_t *puff);


//
// P_SETUP
//
// 23-6-98 KM Short*s changed to int*s, for bigger, better blockmaps
// -AJA- 2000/07/31: line data changed back to shorts.
//

// for fast sight rejection.  Can be NULL
extern const byte *rejectmatrix;

#define BMAP_END  ((unsigned short) 0xFFFF)

extern unsigned short *bmap_lines;
extern unsigned short ** bmap_pointers;

extern int bmapwidth;
extern int bmapheight;  // in mapblocks

extern float_t bmaporgx;
extern float_t bmaporgy;  // origin of block map

extern mobj_t **blocklinks;   // for thing chains
extern mobj_t **blocklights;  // for dynamic lights

//
// P_INTER
//

void P_TouchSpecialThing(mobj_t * special, mobj_t * toucher);
void P_ThrustMobj(mobj_t * target, mobj_t * source, float_t thrust);
void P_DamageMobj(mobj_t * target, mobj_t * inflictor, mobj_t * source,
    float_t amount, const damage_t * damtype);
void P_KillMobj(mobj_t * source, mobj_t * target, const damage_t * damtype);
boolean_t P_GiveBenefitList(player_t *player, mobj_t *special,
    benefit_t *list, boolean_t lose_em);


//
// P_SPEC
//
#include "p_spec.h"

#endif // __P_LOCAL__
