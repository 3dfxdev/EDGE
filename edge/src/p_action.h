//----------------------------------------------------------------------------
//  EDGE Play Simulation Action routines
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

#ifndef __P_ACTION_H__
#define __P_ACTION_H__

#include "dm_type.h"
#include "p_mobj.h"

// Info for the JUMP action
typedef struct act_jump_info_s
{
  // chance value
  percent_t chance; 
}
act_jump_info_t;

// Weapon Action Routine pointers
void A_Light0(mobj_t * object);
void A_Light1(mobj_t * object);
void A_Light2(mobj_t * object);

void A_WeaponReady(mobj_t * object);
void A_WeaponShoot(mobj_t * object);
void A_WeaponEject(mobj_t * object);
void A_WeaponJump(mobj_t * object);
void A_Lower(mobj_t * object);
void A_Raise(mobj_t * object);
void A_ReFire(mobj_t * object);
void A_NoFire(mobj_t * object);
void A_NoFireReturn(mobj_t * object);
void A_CheckReload(mobj_t * object);
void A_SFXWeapon1(mobj_t * object);
void A_SFXWeapon2(mobj_t * object);
void A_SFXWeapon3(mobj_t * object);
void A_WeaponPlaySound(mobj_t * object);
void A_WeaponKillSound(mobj_t * object);
void A_WeaponTransSet(mobj_t * object);
void A_WeaponTransFade(mobj_t * object);
void A_WeaponEnableRadTrig(mobj_t * object);
void A_WeaponDisableRadTrig(mobj_t * object);

void A_SetCrosshair(mobj_t * object);
void A_GotTarget(mobj_t * object);
void A_GunFlash(mobj_t * object);
void A_WeaponKick(mobj_t * object);

void A_WeaponShootSA(mobj_t * object);
void A_ReFireSA(mobj_t * object);
void A_NoFireSA(mobj_t * object);
void A_NoFireReturnSA(mobj_t * object);
void A_CheckReloadSA(mobj_t * object);
void A_GunFlashSA(mobj_t * object);

// Needed for the bossbrain.
void A_BrainScream(mobj_t * object);
void A_BrainDie(mobj_t * object);
void A_BrainSpit(mobj_t * object);
void A_CubeSpawn(mobj_t * object);
void A_BrainMissileExplode(mobj_t * object);

// Visibility Actions
void P_ActTransSet(mobj_t * object);
void P_ActTransFade(mobj_t * object);
void P_ActTransMore(mobj_t * object);
void P_ActTransLess(mobj_t * object);
void P_ActTransAlternate(mobj_t * object);

// Sound Actions
void P_ActPlaySound(mobj_t * object);
void P_ActKillSound(mobj_t * object);
void P_ActMakeAmbientSound(mobj_t * object);
void P_ActMakeAmbientSoundRandom(mobj_t * object);
void P_ActMakeCloseAttemptSound(mobj_t * object);
void P_ActMakeDyingSound(mobj_t * object);
void P_ActMakeOverKillSound(mobj_t * object);
void P_ActMakePainSound(mobj_t * object);
void P_ActMakeRangeAttemptSound(mobj_t * object);
void P_ActMakeActiveSound(mobj_t * object);

// Explosion Damage Actions
void P_ActDamageExplosion(mobj_t * object);
void P_ActThrust(mobj_t * object);

// Stand-by / Looking Actions
void P_ActStandardLook(mobj_t * object);
void P_ActPlayerSupportLook(mobj_t * object);

// Meander, aimless movement actions.
void P_ActStandardMeander(mobj_t * object);
void P_ActPlayerSupportMeander(mobj_t * object);

// Chasing Actions
void P_ActResurrectChase(mobj_t * object);
void P_ActStandardChase(mobj_t * object);
void P_ActWalkSoundChase(mobj_t * object);

// Attacking Actions
void P_ActComboAttack(mobj_t * object);
void P_ActMeleeAttack(mobj_t * object);
void P_ActRangeAttack(mobj_t * object);
void P_ActSpareAttack(mobj_t * object);
void P_ActRefireCheck(mobj_t * object);

// Miscellanous
void P_ActFaceTarget(mobj_t * object);
void P_ActMakeIntoCorpse(mobj_t * object);
void P_ActResetSpreadCount(mobj_t * object);
void P_ActExplode(mobj_t * object);
void P_ActActivateLineType(mobj_t * object);
void P_ActEnableRadTrig(mobj_t * object);
void P_ActDisableRadTrig(mobj_t * object);
void P_ActTouchyRearm(mobj_t * object);
void P_ActTouchyDisarm(mobj_t * object);
void P_ActBounceRearm(mobj_t * object);
void P_ActBounceDisarm(mobj_t * object);
void P_ActPathCheck(mobj_t * object);
void P_ActPathFollow(mobj_t * object);
void P_ActDropItem(mobj_t * object);
void P_ActDLightSet(mobj_t * object);
void P_ActDLightFade(mobj_t * object);
void P_ActDLightRandom(mobj_t * object);

// Movement actions
void P_ActFaceDir(mobj_t * object);
void P_ActTurnDir(mobj_t * object);
void P_ActTurnRandom(mobj_t * object);
void P_ActMlookFace(mobj_t * object);
void P_ActMlookTurn(mobj_t * object);
void P_ActMoveFwd(mobj_t * object);
void P_ActMoveRight(mobj_t * object);
void P_ActMoveUp(mobj_t * object);
void P_ActStopMoving(mobj_t * object);

// Projectiles
void P_ActFixedHomingProjectile(mobj_t * object);
void P_ActRandomHomingProjectile(mobj_t * object);
void P_ActLaunchOrderedSpread(mobj_t * object);
void P_ActLaunchRandomSpread(mobj_t * object);
void P_ActCreateSmokeTrail(mobj_t * object);
void P_ActHomeToSpot(mobj_t * object);
boolean_t P_ActLookForTargets(mobj_t * object);

// Trackers
void P_ActEffectTracker(mobj_t * object);
void P_ActTrackerActive(mobj_t * object);
void P_ActTrackerFollow(mobj_t * object);
void P_ActTrackerStart(mobj_t * object);

// Blood and bullet puffs
void P_ActCheckBlood(mobj_t * object);
void P_ActCheckMoving(mobj_t * object);

void P_ActJump(mobj_t * object);
void A_PlayerScream(mobj_t * object);

#endif //__P_ACTION_H__
