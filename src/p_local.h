//----------------------------------------------------------------------------
//  EDGE Local Header for play sim functions 
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
#define LOOKUPLIMIT    FLOAT_2_ANG(88.0f)
#define LOOKDOWNLIMIT  FLOAT_2_ANG(-88.0f)
#else
#define LOOKUPLIMIT    FLOAT_2_ANG(40.0f)
#define LOOKDOWNLIMIT  FLOAT_2_ANG(-40.0f)
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
bool P_ActMissileContact(mobj_t * object, mobj_t * objecthit);
bool P_ActBulletContact(mobj_t * object, mobj_t * objecthit, 
							 float damage, const damage_t * damtype);
void P_ActTouchyContact(mobj_t * touchy, mobj_t * victim);
bool P_ActUseThing(mobj_t * user, mobj_t * thing, float open_bottom,
						float open_top);
void P_BringCorpseToLife(mobj_t * corpse);

//
// P_WEAPON
//
void P_SetupPsprites(player_t * curplayer);
void P_MovePsprites(player_t * curplayer);
void P_SetPsprite(player_t * p, int position, int stnum);
void P_DropWeapon(player_t * player);
void P_BringUpWeapon(player_t * player);
bool P_CheckWeaponSprite(weaponinfo_t *info);
void P_SelectNewWeapon(player_t * player, int priority, ammotype_e ammo);
void P_Zoom(player_t * player);
void P_RefillClips(player_t * player);

//
// P_USER
//
void P_PlayerThink(player_t * player);
void P_UpdateAvailWeapons(player_t *p);
void P_UpdateTotalArmour(player_t *p);
bool P_AddWeapon(player_t *player, weaponinfo_t *info, int *index);
bool P_RemoveWeapon(player_t *player, weaponinfo_t *info);
void P_GiveInitialBenefits(player_t *player, const mobjinfo_t *info);
void P_AddPlayerToGame(player_t *p);
void P_RemovePlayerFromGame(player_t *p);
void P_RemoveAllPlayers(void);

//
// P_MOBJ
//
#define ONFLOORZ   ((float)INT_MIN)
#define ONCEILINGZ ((float)INT_MAX)

// -ACB- 1998/07/30 Start Pointer the item-respawn-que.
extern iteminque_t *itemquehead;

// -ACB- 1998/08/27 Start Pointer in the mobj list.
extern mobj_t *mobjlisthead;

void P_SpawnPlayer(player_t *p, const spawnpoint_t * sthing);
void P_RemoveMobj(mobj_t * th);
statenum_t P_MobjFindLabel(mobj_t * mobj, const char *label);
bool P_SetMobjState(mobj_t * mobj, statenum_t state);
bool P_SetMobjStateDeferred(mobj_t * mobj, statenum_t state, int tic_skip);
void P_SetMobjDirAndSpeed(mobj_t * mobj, angle_t angle, float slope, float speed);
void P_RunMobjThinkers(void);
void P_SpawnPuff(float x, float y, float z, const mobjinfo_t * puff);
void P_SpawnBlood(float x, float y, float z, float damage, angle_t angle, const mobjinfo_t * blood);
void P_RemoveQueuedMobjs(void);
void P_CalcFullProperties(const mobj_t *mo, region_properties_t *newregp);

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
mobj_t *P_MobjCreateObject(float x, float y, float z, const mobjinfo_t * type);

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

extern dirtype_e opposite[];
extern dirtype_e diags[];
extern float xspeed[8];
extern float yspeed[8];
extern shoot_spot_info_t brain_spots;

void P_NoiseAlert(player_t *p);
void P_NewChaseDir(mobj_t * actor);
bool P_CreateAggression(mobj_t * actor);
bool P_CheckMeleeRange(mobj_t * actor);
bool P_CheckMissileRange(mobj_t * actor);
bool P_Move(mobj_t * actor, bool path);
bool P_LookForPlayers(mobj_t * actor, angle_t range);
void P_LookForShootSpots(const mobjinfo_t *spot_type);
void P_FreeShootSpots(void);

//
// P_MAPUTL
//
#define PT_ADDLINES  1
#define PT_ADDTHINGS 2
#define PT_EARLYOUT  4

typedef enum
{
	INCPT_Line  = 0,
	INCPT_Thing = 1
}
intercept_type_e;

typedef struct intercept_s
{
	float frac;  // along trace line

	intercept_type_e type;

	union
	{
		mobj_t *thing;
		line_t *line;
	}
	d;
}
intercept_t;

typedef bool(*traverser_t) (intercept_t * in);

extern intercept_t **intercepts;
extern int intercept_p;

extern divline_t trace;

float P_ApproxDistance(float dx, float dy);
float P_ApproxSlope(float dx, float dy, float dz);
int P_PointOnDivlineSide(float x, float y, divline_t *div);
int P_PointOnDivlineThick(float x, float y, divline_t *div,
						  float div_len, float thickness);
float P_InterceptVector(divline_t * v2, divline_t * v1);
int P_BoxOnLineSide(float * tmbox, line_t * ld);
int P_FindThingGap(vgap_t * gaps, int gap_num, float z1, float z2);
void P_ComputeGaps(line_t * ld);
float P_ComputeThingGap(mobj_t * thing, sector_t * sec, float z, float * f, float * c);
void P_AddExtraFloor(sector_t *sec, line_t *line);
void P_ComputeWallTiles(line_t *ld, int sidenum);
void P_RecomputeGapsAroundSector(sector_t *sec);
void P_RecomputeTilesInSector(sector_t *sec);
void P_FloodExtraFloors(sector_t *sector);
void P_UnsetThingFinally(mobj_t * thing);
void P_SetThingPosition(mobj_t * thing);
void P_ChangeThingPosition(mobj_t * thing, float x, float y, float z);
void P_FreeSectorTouchNodes(sector_t *sec);

bool P_BlockLinesIterator(int x, int y, bool(*func) (line_t *));
bool P_BlockThingsIterator(int x, int y, bool(*func) (mobj_t *));
bool P_ThingsInArea(float *bbox);
bool P_ThingsOnLine(line_t *ld);

typedef enum
{
	EXFIT_Ok = 0,
	EXFIT_StuckInCeiling,
	EXFIT_StuckInFloor,
	EXFIT_StuckInExtraFloor
}
exfloor_fit_e;

exfloor_fit_e P_ExtraFloorFits(sector_t *sec, float z1, float z2);

bool P_PathTraverse(float x1, float y1, float x2, float y2, int flags, traverser_t trav);

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
extern bool floatok;
extern float float_destz;

extern mobj_t *linetarget;  // who got hit (or NULL)

extern bool mobj_hit_sky;
extern line_t *blockline;

extern stack_array_t spechit_a;
extern line_t **spechit;
extern int numspechit;

bool P_MapInit(void);
bool P_MapCheckBlockingLine(mobj_t * thing, mobj_t * spawnthing);
mobj_t *P_MapFindCorpse(mobj_t * thing);
mobj_t *P_MapTargetAutoAim(mobj_t * source, angle_t angle, float distance, bool force_aim);
mobj_t *P_MapTargetTheory(mobj_t * source);

float P_AimLineAttack(mobj_t * t1, angle_t angle, float distance);
void P_UpdateMultipleFloors(sector_t * sector);
bool P_CheckSolidSectorMove(sector_t *sec, bool is_ceiling, float dh);
bool P_SolidSectorMove(sector_t *sec, bool is_ceiling, float dh, bool crush, bool nocarething);
void P_ChangeThingSize(mobj_t *mo);
bool P_CheckAbsPosition(mobj_t * thing, float x, float y, float z);
bool P_CheckSight(mobj_t * src, mobj_t * dest);
bool P_CheckSightApproxVert(mobj_t * src, mobj_t * dest);
void P_RadiusAttack(mobj_t * spot, mobj_t * source, float radius, float damage, const damage_t * damtype, bool thrust_only);

bool P_TeleportMove(mobj_t * thing, float x, float y, float z);
bool P_TryMove(mobj_t * thing, float x, float y);
void P_SlideMove(mobj_t * mo, float x, float y);
void P_UseLines(player_t * player);
void P_LineAttack(mobj_t * t1, angle_t angle, float distance, float slope, float damage, const damage_t * damtype, const mobjinfo_t *puff);


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

extern float bmaporgx;
extern float bmaporgy;  // origin of block map

extern mobj_t **blocklinks;   // for thing chains
extern mobj_t **blocklights;  // for dynamic lights

#define BLOCKMAP_GET_X(x)  ((int) ((x) - bmaporgx) / MAPBLOCKUNITS)
#define BLOCKMAP_GET_Y(y)  ((int) ((y) - bmaporgy) / MAPBLOCKUNITS)

//
// P_INTER
//

void P_TouchSpecialThing(mobj_t * special, mobj_t * toucher);
void P_ThrustMobj(mobj_t * target, mobj_t * inflictor, float thrust);
void P_DamageMobj(mobj_t * target, mobj_t * inflictor, mobj_t * source,
				  float amount, const damage_t * damtype);
void P_KillMobj(mobj_t * source, mobj_t * target, const damage_t * damtype);
bool P_GiveBenefitList(player_t *player, mobj_t *special,
							benefit_t *list, bool lose_em);


//
// P_SPEC
//
#include "p_spec.h"

#endif // __P_LOCAL__
