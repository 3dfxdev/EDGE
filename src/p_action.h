//----------------------------------------------------------------------------
//  EDGE Play Simulation Action routines
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2008  The EDGE Team.
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

struct mobj_s;

// Weapon Action Routine pointers
void A_Light0(struct mobj_s *mo, void *data);
void A_Light1(struct mobj_s *mo, void *data);
void A_Light2(struct mobj_s *mo, void *data);

void A_WeaponReady(struct mobj_s *mo, void *data);
void A_WeaponEmpty(struct mobj_s *mo, void *data);
void A_WeaponShoot(struct mobj_s *mo, void *data);
void A_WeaponEject(struct mobj_s *mo, void *data);
void A_WeaponJump(struct mobj_s *mo, void *data);
void A_Lower(struct mobj_s *mo, void *data);
void A_Raise(struct mobj_s *mo, void *data);
void A_ReFire(struct mobj_s *mo, void *data);
void A_NoFire(struct mobj_s *mo, void *data);
void A_NoFireReturn(struct mobj_s *mo, void *data);
void A_CheckReload(struct mobj_s *mo, void *data);
void A_SFXWeapon1(struct mobj_s *mo, void *data);
void A_SFXWeapon2(struct mobj_s *mo, void *data);
void A_SFXWeapon3(struct mobj_s *mo, void *data);
void A_WeaponPlaySound(struct mobj_s *mo, void *data);
void A_WeaponKillSound(struct mobj_s *mo, void *data);
void A_WeaponTransSet(struct mobj_s *mo, void *data);
void A_WeaponTransFade(struct mobj_s *mo, void *data);
void A_WeaponEnableRadTrig(struct mobj_s *mo, void *data);
void A_WeaponDisableRadTrig(struct mobj_s *mo, void *data);

void A_SetCrosshair(struct mobj_s *mo, void *data);
void A_TargetJump(struct mobj_s *mo, void *data);
void A_FriendJump(struct mobj_s *mo, void *data);
void A_GunFlash(struct mobj_s *mo, void *data);
void A_WeaponKick(struct mobj_s *mo, void *data);
void A_WeaponSetSkin(struct mobj_s *mo, void *data);

void A_WeaponShootSA(struct mobj_s *mo, void *data);
void A_ReFireSA(struct mobj_s *mo, void *data);
void A_NoFireSA(struct mobj_s *mo, void *data);
void A_NoFireReturnSA(struct mobj_s *mo, void *data);
void A_CheckReloadSA(struct mobj_s *mo, void *data);
void A_GunFlashSA(struct mobj_s *mo, void *data);

// Needed for the bossbrain.
void A_BrainScream(struct mobj_s *mo, void *data);
void A_BrainDie(struct mobj_s *mo, void *data);
void A_BrainSpit(struct mobj_s *mo, void *data);
void A_CubeSpawn(struct mobj_s *mo, void *data);
void A_BrainMissileExplode(struct mobj_s *mo, void *data);

// Visibility Actions
void A_TransSet(struct mobj_s *mo, void *data);
void A_TransFade(struct mobj_s *mo, void *data);
void A_TransMore(struct mobj_s *mo, void *data);
void A_TransLess(struct mobj_s *mo, void *data);
void A_TransAlternate(struct mobj_s *mo, void *data);

// Sound Actions
void A_PlaySound(struct mobj_s *mo, void *data);
void A_KillSound(struct mobj_s *mo, void *data);
void A_MakeAmbientSound(struct mobj_s *mo, void *data);
void A_MakeAmbientSoundRandom(struct mobj_s *mo, void *data);
void A_MakeCloseAttemptSound(struct mobj_s *mo, void *data);
void A_MakeDyingSound(struct mobj_s *mo, void *data);
void A_MakeOverKillSound(struct mobj_s *mo, void *data);
void A_MakePainSound(struct mobj_s *mo, void *data);
void A_MakeRangeAttemptSound(struct mobj_s *mo, void *data);
void A_MakeActiveSound(struct mobj_s *mo, void *data);
void A_PlayerScream(struct mobj_s *mo, void *data);

// Explosion Damage Actions
void A_DamageExplosion(struct mobj_s *mo, void *data);
void A_Thrust(struct mobj_s *mo, void *data);

// Stand-by / Looking Actions
void A_StandardLook(struct mobj_s *mo, void *data);
void A_PlayerSupportLook(struct mobj_s *mo, void *data);

// Meander, aimless movement actions.
void A_StandardMeander(struct mobj_s *mo, void *data);
void A_PlayerSupportMeander(struct mobj_s *mo, void *data);

// Chasing Actions
void A_ResurrectChase(struct mobj_s *mo, void *data);
void A_StandardChase(struct mobj_s *mo, void *data);
void A_WalkSoundChase(struct mobj_s *mo, void *data);

// Attacking Actions
void A_ComboAttack(struct mobj_s *mo, void *data);
void A_MeleeAttack(struct mobj_s *mo, void *data);
void A_RangeAttack(struct mobj_s *mo, void *data);
void A_SpareAttack(struct mobj_s *mo, void *data);
void A_RefireCheck(struct mobj_s *mo, void *data);
void A_ReloadCheck(struct mobj_s *mo, void *data);
void A_ReloadReset(struct mobj_s *mo, void *data);

// Miscellanous
void A_FaceTarget(struct mobj_s *mo, void *data = NULL);
void A_MakeDead(struct mobj_s *mo, void *data);
void A_ResetSpreadCount(struct mobj_s *mo, void *data);
void A_Explode(struct mobj_s *mo, void *data);
void A_ActivateLineType(struct mobj_s *mo, void *data);
void A_EnableRadTrig(struct mobj_s *mo, void *data);
void A_DisableRadTrig(struct mobj_s *mo, void *data);
void A_TouchyRearm(struct mobj_s *mo, void *data);
void A_TouchyDisarm(struct mobj_s *mo, void *data);
void A_BounceRearm(struct mobj_s *mo, void *data);
void A_BounceDisarm(struct mobj_s *mo, void *data);
void A_PathCheck(struct mobj_s *mo, void *data);
void A_PathFollow(struct mobj_s *mo, void *data);

void A_DropItem(struct mobj_s *mo, void *data);
void A_Spawn(struct mobj_s *mo, void *data);
void A_DLightSet(struct mobj_s *mo, void *data);
void A_DLightSet2(struct mobj_s *mo, void *data);
void A_DLightFade(struct mobj_s *mo, void *data);
void A_DLightRandom(struct mobj_s *mo, void *data);
void A_DLightColour(struct mobj_s *mo, void *data);
void A_SetSkin(struct mobj_s *mo, void *data);
void A_Die(struct mobj_s *mo, void *data);
void A_KeenDie(struct mobj_s *mo, void *data);
void A_CheckBlood(struct mobj_s *mo, void *data);
void A_Jump(struct mobj_s *mo, void *data);
void A_Become(struct mobj_s *mo, void *data);
void A_SetInvuln(struct mobj_s *mo, void *data);
void A_ClearInvuln(struct mobj_s *mo, void *data);

// Movement actions
void A_FaceDir(struct mobj_s *mo, void *data);
void A_TurnDir(struct mobj_s *mo, void *data);
void A_TurnRandom(struct mobj_s *mo, void *data);
void A_MlookFace(struct mobj_s *mo, void *data);
void A_MlookTurn(struct mobj_s *mo, void *data);
void A_MoveFwd(struct mobj_s *mo, void *data);
void A_MoveRight(struct mobj_s *mo, void *data);
void A_MoveUp(struct mobj_s *mo, void *data);
void A_StopMoving(struct mobj_s *mo, void *data);
void A_CheckMoving(struct mobj_s *mo, void *data);
void A_CheckActivity(struct mobj_s *mo, void *data);

// Projectiles
void A_HomingProjectile(struct mobj_s *mo, void *data);
void A_LaunchOrderedSpread(struct mobj_s *mo, void *data);
void A_LaunchRandomSpread(struct mobj_s *mo, void *data);
void A_CreateSmokeTrail(struct mobj_s *mo, void *data);
void A_HomeToSpot(struct mobj_s *mo, void *data);
bool A_LookForTargets(struct mobj_s *mo, void *data);

// Trackers
void A_EffectTracker(struct mobj_s *mo, void *data);
void A_TrackerActive(struct mobj_s *mo, void *data);
void A_TrackerFollow(struct mobj_s *mo, void *data);
void A_TrackerStart(struct mobj_s *mo, void *data);

#endif /* __P_ACTION_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
