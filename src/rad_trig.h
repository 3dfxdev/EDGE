//----------------------------------------------------------------------------
//  Radius Trigger header file
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

#ifndef __RAD_TRIG__
#define __RAD_TRIG__

#include "dm_type.h"
#include "e_player.h"
#include "rad_main.h"


#define DEBUG_RTS  0

#define MAXSTRLEN  512

extern rad_script_t *r_scripts;
extern rad_trigger_t *r_triggers;
extern int rad_itemsread;

// Tip Prototypes
void RAD_InitTips(void);
void RAD_ResetTips(void);
void RAD_DisplayTips(void);

// RadiusTrigger & Scripting Prototypes
bool RAD_Init(void);
void RAD_ReadScript(void *data, int size);
bool RAD_LoadParam(void);
void RAD_SpawnTriggers(const char *map_name);
void RAD_ClearTriggers(void);
void RAD_GroupTriggerTags(rad_trigger_t *trig);

void RAD_DoRadiTrigger(player_t * p);
void RAD_Ticker(void);
bool RAD_WithinRadius(mobj_t * mo, rad_script_t * r);
rad_script_t *RAD_FindScriptByName(const char *map_name, const char *name);
rad_trigger_t *RAD_FindTriggerByName(const char *name);
rts_state_t *RAD_FindStateByLabel(rad_script_t *scr, char *label);
void RAD_EnableByTag(mobj_t *actor, int tag, bool disable);

// Path support
bool RAD_CheckReachedTrigger(mobj_t * thing);

//
//  PARSING
//
void RAD_ParserBegin(void);
void RAD_ParserDone(void);
void RAD_ParseLine(char *s);
int RAD_StringHashFunc(const char *s);

void RAD_Error(const char *err, ...);
void RAD_Warning(const char *err, ...);
void RAD_WarnError(const char *err, ...);

extern int rad_cur_linenum;
extern char *rad_cur_filename;

//
//  ACTIONS
//

void RAD_ActNOP(rad_trigger_t *R, mobj_t *actor, void *param);
void RAD_ActTip(rad_trigger_t *R, mobj_t *actor, void *param);
void RAD_ActTipProps(rad_trigger_t *R, mobj_t *actor, void *param);
void RAD_ActSpawnThing(rad_trigger_t *R, mobj_t *actor, void *param);
void RAD_ActPlaySound(rad_trigger_t *R, mobj_t *actor, void *param);
void RAD_ActKillSound(rad_trigger_t *R, mobj_t *actor, void *param);
void RAD_ActChangeMusic(rad_trigger_t *R, mobj_t *actor, void *param);
void RAD_ActChangeTex(rad_trigger_t *R, mobj_t *actor, void *param);

void RAD_ActMoveSector(rad_trigger_t *R, mobj_t *actor, void *param);
void RAD_ActLightSector(rad_trigger_t *R, mobj_t *actor, void *param);
void RAD_ActEnableScript(rad_trigger_t *R, mobj_t *actor, void *param);
void RAD_ActActivateLinetype(rad_trigger_t *R, mobj_t *actor, void *param);
void RAD_ActUnblockLines(rad_trigger_t *R, mobj_t *actor, void *param);
void RAD_ActJump(rad_trigger_t *R, mobj_t *actor, void *param);
void RAD_ActSleep(rad_trigger_t *R, mobj_t *actor, void *param);
void RAD_ActRetrigger(rad_trigger_t *R, mobj_t *actor, void *param);

void RAD_ActDamagePlayers(rad_trigger_t *R, mobj_t *actor, void *param);
void RAD_ActHealPlayers(rad_trigger_t *R, mobj_t *actor, void *param);
void RAD_ActArmourPlayers(rad_trigger_t *R, mobj_t *actor, void *param);
void RAD_ActBenefitPlayers(rad_trigger_t *R, mobj_t *actor, void *param);
void RAD_ActDamageMonsters(rad_trigger_t *R, mobj_t *actor, void *param);
void RAD_ActThingEvent(rad_trigger_t *R, mobj_t *actor, void *param);
void RAD_ActSkill(rad_trigger_t *R, mobj_t *actor, void *param);
void RAD_ActGotoMap(rad_trigger_t *R, mobj_t *actor, void *param);
void RAD_ActExitLevel(rad_trigger_t *R, mobj_t *actor, void *param);

#endif
