//----------------------------------------------------------------------------
//  EDGE Data Definition File Code (Things - MOBJs)
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
// Moving Object Setup and Parser Code
//
// -ACB- 1998/08/04 Written.
// -ACB- 1998/09/12 Use DDF_MainGetFixed for fixed number references.
// -ACB- 1998/09/13 Use DDF_MainGetTime for Time count references.
// -KM- 1998/11/25 Translucency is now a fixed_t. Fixed spelling of available.
// -KM- 1998/12/16 No limit on number of ammo types.
//

#include "i_defs.h"

#include "ddf_locl.h"
#include "ddf_main.h"

#include "p_action.h"
#include "rgl_fx.h"
#include "z_zone.h"

#include "./epi/epistring.h"

#undef  DF
#define DF  DDF_CMD

#define DDF_MobjHashFunc(x)  (((x) + LOOKUP_CACHESIZE) % LOOKUP_CACHESIZE)

mobjtype_c buffer_mobj;
mobjtype_c *dynamic_mobj;

mobjtype_container_c mobjtypes;

void DDF_MobjGetBenefit(const char *info, void *storage);
void DDF_MobjGetPickupEffect(const char *info, void *storage);
void DDF_MobjGetDLight(const char *info, void *storage);

static void CheckPowerupCompatibility(void);

static dlightinfo_c buffer_dlight;
static haloinfo_c buffer_halo;

#undef  DDF_CMD_BASE
#define DDF_CMD_BASE  buffer_halo

const commandlist_t halo_commands[] =
{
	DF("HEIGHT", height, DDF_MainGetFloat),
	DF("SIZE", size,     DDF_MainGetFloat),
	DF("MINSIZE", minsize, DDF_MainGetFloat),
	DF("MAXSIZE", maxsize, DDF_MainGetFloat),
	DF("TRANSLUCENCY", translucency, DDF_MainGetPercent),
	DF("COLOUR",  colour,  DDF_MainGetRGB),
	DF("GRAPHIC", graphic, DDF_MainGetInlineStr10),

	DDF_CMD_END
};

#undef  DDF_CMD_BASE
#define DDF_CMD_BASE  buffer_dlight

const commandlist_t dlight_commands[] =
{
	DF("TYPE", type, DDF_MobjGetDLight),
	DF("INTENSITY", intensity, DDF_MainGetNumeric),
	DF("COLOUR", colour, DDF_MainGetRGB),
	DF("HEIGHT", height, DDF_MainGetPercent),
  
	DDF_CMD_END
};

#undef  DDF_CMD_BASE
#define DDF_CMD_BASE  buffer_mobj

const commandlist_t thing_commands[] =
{
	// sub-commands
	DDF_SUB_LIST("HALO",   halo,   halo_commands,   buffer_halo),
	DDF_SUB_LIST("DLIGHT", dlight, dlight_commands, buffer_dlight),
	DDF_SUB_LIST("EXPLODE DAMAGE", explode_damage, damage_commands, buffer_damage),
	DDF_SUB_LIST("CHOKE DAMAGE", choke_damage, damage_commands, buffer_damage),

	DF("SPAWNHEALTH", spawnhealth, DDF_MainGetFloat),
	DF("RADIUS", radius, DDF_MainGetFloat),
	DF("HEIGHT", height, DDF_MainGetFloat),
	DF("MASS", mass, DDF_MainGetFloat),
	DF("SPEED", speed, DDF_MainGetFloat),
	DF("FAST", fast, DDF_MainGetFloat),
	DF("SPECIAL", ddf, DDF_MobjGetSpecial),
	DF("EXTRA", ddf, DDF_MobjGetExtra),
	DF("PROJECTILE SPECIAL", ddf, DDF_MobjGetSpecial),
	DF("RESPAWN TIME", respawntime, DDF_MainGetTime),
	DF("FUSE", fuse, DDF_MainGetTime),
	DF("LIFESPAN", fuse, DDF_MainGetTime),
	DF("PALETTE REMAP", palremap, DDF_MainGetColourmap),
	DF("TRANSLUCENCY", translucency, DDF_MainGetPercent),

	DF("INITIAL BENEFIT", initial_benefits, DDF_MobjGetBenefit),
	DF("LOSE BENEFIT", lose_benefits, DDF_MobjGetBenefit),
	DF("PICKUP BENEFIT", pickup_benefits, DDF_MobjGetBenefit),
	DF("PICKUP MESSAGE", pickup_message, DDF_MainGetString),
	DF("PICKUP EFFECT", pickup_effects, DDF_MobjGetPickupEffect),

	DF("PAINCHANCE", painchance, DDF_MainGetPercent),
	DF("MINATTACK CHANCE", minatkchance, DDF_MainGetPercent),
	DF("REACTION TIME", reactiontime, DDF_MainGetTime),
	DF("JUMP DELAY", jump_delay, DDF_MainGetTime),
	DF("JUMP HEIGHT", jumpheight, DDF_MainGetFloat),
	DF("CROUCH HEIGHT", crouchheight, DDF_MainGetFloat),
	DF("VIEW HEIGHT", viewheight, DDF_MainGetPercent),
	DF("SHOT HEIGHT", shotheight, DDF_MainGetPercent),
	DF("MAX FALL", maxfall, DDF_MainGetFloat),
	DF("CASTORDER", castorder, DDF_MainGetNumeric),
	DF("CAST_TITLE", cast_title, DDF_MainGetString),
	DF("PLAYER", playernum, DDF_MobjGetPlayer),
	DF("SIDE", side, DDF_MainGetBitSet),
	DF("CLOSE ATTACK", closecombat, DDF_MainRefAttack),
	DF("RANGE ATTACK", rangeattack, DDF_MainRefAttack),
	DF("SPARE ATTACK", spareattack, DDF_MainRefAttack),
	DF("DROPITEM", dropitem_ref, DDF_MainGetString),
	DF("BLOOD", blood_ref, DDF_MainGetString),
	DF("RESPAWN EFFECT", respawneffect_ref, DDF_MainGetString),
	DF("SPIT SPOT", spitspot_ref, DDF_MainGetString),

	DF("PICKUP SOUND", activesound, DDF_MainLookupSound),
	DF("ACTIVE SOUND", activesound, DDF_MainLookupSound),
	DF("LAUNCH SOUND", seesound, DDF_MainLookupSound),
	DF("AMBIENT SOUND", seesound, DDF_MainLookupSound),
	DF("SIGHTING SOUND", seesound, DDF_MainLookupSound),
	DF("DEATH SOUND", deathsound, DDF_MainLookupSound),
	DF("OVERKILL SOUND", overkill_sound, DDF_MainLookupSound),
	DF("PAIN SOUND", painsound, DDF_MainLookupSound),
	DF("STARTCOMBAT SOUND", attacksound, DDF_MainLookupSound),
	DF("WALK SOUND", walksound, DDF_MainLookupSound),
	DF("JUMP SOUND", jump_sound, DDF_MainLookupSound),
	DF("NOWAY SOUND", noway_sound, DDF_MainLookupSound),
	DF("OOF SOUND", oof_sound, DDF_MainLookupSound),
	DF("GASP SOUND", gasp_sound, DDF_MainLookupSound),

	DF("FLOAT SPEED", float_speed, DDF_MainGetFloat),
	DF("STEP SIZE", step_size, DDF_MainGetFloat),
	DF("SPRITE SCALE", yscale, DDF_MainGetFloat),
	DF("SPRITE ASPECT", xscale, DDF_MainGetFloat),
	DF("BOUNCE SPEED", bounce_speed, DDF_MainGetFloat),
	DF("BOUNCE UP", bounce_up, DDF_MainGetFloat),
	DF("SIGHT SLOPE", sight_slope, DDF_MainGetSlope),
	DF("SIGHT ANGLE", sight_angle, DDF_MainGetAngle),
	DF("RIDE FRICTION", ride_friction, DDF_MainGetFloat),
	DF("BOBBING", bobbing, DDF_MainGetPercent),
	DF("IMMUNITY CLASS", immunity, DDF_MainGetBitSet),
	DF("RESISTANCE CLASS", resistance, DDF_MainGetBitSet),
	DF("SHADOW TRANSLUCENCY", shadow_trans, DDF_MainGetPercent),
	DF("LUNG CAPACITY", lung_capacity, DDF_MainGetTime),
	DF("GASP START", gasp_start, DDF_MainGetTime),
	DF("EXPLODE RADIUS", explode_radius, DDF_MainGetFloat),
	DF("RELOAD SHOTS", reload_shots, DDF_MainGetNumeric),  // -AJA- 2004/11/15

	// -AJA- backwards compatibility cruft...
	DF("!EXPLOD DAMAGE", explode_damage.nominal, DDF_MainGetFloat),
	DF("!EXPLOSION DAMAGE", explode_damage.nominal, DDF_MainGetFloat),
	DF("!EXPLOD DAMAGERANGE", explode_damage.nominal, DDF_MainGetFloat),
	DF("!EXPLOD DAMAGEMULTI", ddf, DDF_DummyFunction),
	DF("!GIB", ddf, DDF_DummyFunction),

	DDF_CMD_END
};

static const state_starter_t thing_starters[] =
{
	{"SPAWN",      "IDLE",     &buffer_mobj.spawn_state},
	{"IDLE",       "IDLE",     &buffer_mobj.idle_state},
	{"CHASE",      "CHASE",    &buffer_mobj.chase_state},
	{"PAIN",       "IDLE",     &buffer_mobj.pain_state},
	{"MISSILE",    "IDLE",     &buffer_mobj.missile_state},
	{"MELEE",      "IDLE",     &buffer_mobj.melee_state},
	{"DEATH",      "REMOVE",   &buffer_mobj.death_state},
	{"OVERKILL",   "REMOVE",   &buffer_mobj.overkill_state},
	{"RESPAWN",    "IDLE",     &buffer_mobj.raise_state},
	{"RESURRECT",  "IDLE",     &buffer_mobj.res_state},
	{"MEANDER",    "MEANDER",  &buffer_mobj.meander_state},
	{"BOUNCE",     "IDLE",     &buffer_mobj.bounce_state},
	{"TOUCH",      "IDLE",     &buffer_mobj.touch_state},
	{"JUMP",       "IDLE",     &buffer_mobj.jump_state},
	{"RELOAD",     "IDLE",     &buffer_mobj.reload_state},
	{"GIB",        "REMOVE",   &buffer_mobj.gib_state},
	{NULL, NULL, NULL}
};

// -KM- 1998/11/25 Added weapon functions.
// -AJA- 1999/08/09: Moved this here from p_action.h, and added an extra
// field `handle_arg' for things like "WEAPON_SHOOT(FIREBALL)".

static const actioncode_t thing_actions[] =
{
	{"NOTHING", NULL, NULL},

	{"CLOSEATTEMPTSND",   P_ActMakeCloseAttemptSound, NULL},
	{"COMBOATTACK",       P_ActComboAttack, NULL},
	{"FACETARGET",        P_ActFaceTarget, NULL},
	{"PLAYSOUND",         P_ActPlaySound, DDF_StateGetSound},
	{"KILLSOUND",         P_ActKillSound, NULL},
	{"MAKESOUND",         P_ActMakeAmbientSound, NULL},
	{"MAKEACTIVESOUND",   P_ActMakeActiveSound, NULL},
	{"MAKESOUNDRANDOM",   P_ActMakeAmbientSoundRandom, NULL},
	{"MAKEDEATHSOUND",    P_ActMakeDyingSound, NULL},
	{"MAKEDEAD",          P_ActMakeIntoCorpse, NULL},
	{"MAKEOVERKILLSOUND", P_ActMakeOverKillSound, NULL},
	{"MAKEPAINSOUND",     P_ActMakePainSound, NULL},
	{"PLAYER_SCREAM",     P_ActPlayerScream, NULL},
	{"CLOSE ATTACK",      P_ActMeleeAttack, DDF_StateGetAttack},
	{"RANGE ATTACK",      P_ActRangeAttack, DDF_StateGetAttack},
	{"SPARE ATTACK",      P_ActSpareAttack, DDF_StateGetAttack},
	{"RANGEATTEMPTSND",   P_ActMakeRangeAttemptSound, NULL},
	{"REFIRE CHECK",      P_ActRefireCheck, NULL},
	{"RELOAD CHECK",      P_ActReloadCheck, NULL},
	{"LOOKOUT",           P_ActStandardLook, NULL},
	{"SUPPORT LOOKOUT",   P_ActPlayerSupportLook, NULL},
	{"CHASE",             P_ActStandardChase, NULL},
	{"RESCHASE",          P_ActResurrectChase, NULL},
	{"WALKSOUND CHASE",   P_ActWalkSoundChase, NULL},
	{"MEANDER",           P_ActStandardMeander, NULL},
	{"SUPPORT MEANDER",   P_ActPlayerSupportMeander, NULL},
	{"EXPLOSIONDAMAGE",   P_ActDamageExplosion, NULL},
	{"THRUST",            P_ActThrust, NULL},
	{"TRACER",            P_ActFixedHomingProjectile, NULL},
	{"RANDOM TRACER",     P_ActRandomHomingProjectile, NULL},
	{"RESET SPREADER",    P_ActResetSpreadCount, NULL},
	{"SMOKING",           P_ActCreateSmokeTrail, NULL},
	{"TRACKERACTIVE",     P_ActTrackerActive, NULL},
	{"TRACKERFOLLOW",     P_ActTrackerFollow, NULL},
	{"TRACKERSTART",      P_ActTrackerStart, NULL},
	{"EFFECTTRACKER",     P_ActEffectTracker, NULL},
	{"CHECKBLOOD",        P_ActCheckBlood, NULL},
	{"CHECKMOVING",       P_ActCheckMoving, NULL},
	{"JUMP",              P_ActJump, DDF_StateGetJump},
	{"EXPLODE",           P_ActExplode, NULL},
	{"ACTIVATE LINETYPE", P_ActActivateLineType, DDF_StateGetIntPair},
	{"RTS ENABLE TAGGED", P_ActEnableRadTrig,  DDF_StateGetInteger},
	{"RTS DISABLE TAGGED",P_ActDisableRadTrig, DDF_StateGetInteger},
	{"TOUCHY REARM",      P_ActTouchyRearm, NULL},
	{"TOUCHY DISARM",     P_ActTouchyDisarm, NULL},
	{"BOUNCE REARM",      P_ActBounceRearm, NULL},
	{"BOUNCE DISARM",     P_ActBounceDisarm, NULL},
	{"PATH CHECK",        P_ActPathCheck, NULL},
	{"PATH FOLLOW",       P_ActPathFollow, NULL},
	{"DROPITEM",          P_ActDropItem, DDF_StateGetMobj},
	{"TRANS SET",         P_ActTransSet, DDF_StateGetPercent},
	{"TRANS FADE",        P_ActTransFade, DDF_StateGetPercent},
	{"TRANS MORE",        P_ActTransMore, DDF_StateGetPercent},
	{"TRANS LESS",        P_ActTransLess, DDF_StateGetPercent},
	{"TRANS ALTERNATE",   P_ActTransAlternate, DDF_StateGetPercent},
	{"DLIGHT SET",        P_ActDLightSet,  DDF_StateGetInteger},
	{"DLIGHT FADE",       P_ActDLightFade, DDF_StateGetInteger},
	{"DLIGHT RANDOM",     P_ActDLightRandom, DDF_StateGetIntPair},

    {"FACE",              P_ActFaceDir, DDF_StateGetAngle},
    {"TURN",              P_ActTurnDir, DDF_StateGetAngle},
    {"TURN RANDOM",       P_ActTurnRandom, DDF_StateGetAngle},
	{"MLOOK FACE",        P_ActMlookFace, DDF_StateGetSlope},
	{"MLOOK TURN",        P_ActMlookTurn, DDF_StateGetSlope},
	{"MOVE FWD",          P_ActMoveFwd, DDF_StateGetFloat},
	{"MOVE RIGHT",        P_ActMoveRight, DDF_StateGetFloat},
	{"MOVE UP",           P_ActMoveUp, DDF_StateGetFloat},
	{"STOP",              P_ActStopMoving, NULL},

	// Boom/MBF compatibility
	{"DIE",               P_ActDie, NULL},

	// bossbrain actions
	{"BRAINSPIT", P_ActBrainSpit, NULL},
	{"CUBESPAWN", P_ActCubeSpawn, NULL},
	{"CUBETRACER", P_ActHomeToSpot, NULL},
	{"BRAINSCREAM", P_ActBrainScream, NULL},
	{"BRAINMISSILEEXPLODE", P_ActBrainMissileExplode, NULL},
	{"BRAINDIE", P_ActBrainDie, NULL},

	// -AJA- backwards compatibility cruft...
	{"!VARIEDEXPDAMAGE",   P_ActDamageExplosion, NULL},
	{"!VARIED THRUST",     P_ActThrust, NULL},
	{"!RANDOMJUMP", NULL, NULL},
	{"!ALTERTRANSLUC",   NULL, NULL},
	{"!ALTERVISIBILITY", NULL, NULL},
	{"!LESSVISIBLE", NULL, NULL},
	{"!MOREVISIBLE", NULL, NULL},
	{NULL, NULL, NULL}
};

const specflags_t keytype_names[] =
{
	{"BLUECARD",    KF_BlueCard,    0},
	{"YELLOWCARD",  KF_YellowCard,  0},
	{"REDCARD",     KF_RedCard,     0},
	{"GREENCARD",   KF_GreenCard,   0},

	{"BLUESKULL",   KF_BlueSkull,   0},
	{"YELLOWSKULL", KF_YellowSkull, 0},
	{"REDSKULL",    KF_RedSkull,    0},
	{"GREENSKULL",  KF_GreenSkull,  0},

	{"GOLD KEY",    KF_GoldKey,     0},
	{"SILVER KEY",  KF_SilverKey,   0},
	{"BRASS KEY",   KF_BrassKey,    0},
	{"COPPER KEY",  KF_CopperKey,   0},
	{"STEEL KEY",   KF_SteelKey,    0},
	{"WOODEN KEY",  KF_WoodenKey,   0},
	{"FIRE KEY",    KF_FireKey,     0},
	{"WATER KEY",   KF_WaterKey,    0},

	// -AJA- compatibility (this way is the easiest)
	{"KEY BLUECARD",    KF_BlueCard,    0},
	{"KEY YELLOWCARD",  KF_YellowCard,  0},
	{"KEY REDCARD",     KF_RedCard,     0},
	{"KEY GREENCARD",   KF_GreenCard,   0},

	{"KEY BLUESKULL",   KF_BlueSkull,   0},
	{"KEY YELLOWSKULL", KF_YellowSkull, 0},
	{"KEY REDSKULL",    KF_RedSkull,    0},
	{"KEY GREENSKULL",  KF_GreenSkull,  0},

	{NULL, 0, 0}
};

const specflags_t armourtype_names[] =
{
	{"GREEN ARMOUR",  ARMOUR_Green,  0},
	{"BLUE ARMOUR",   ARMOUR_Blue,   0},
	{"YELLOW ARMOUR", ARMOUR_Yellow, 0},
	{"RED ARMOUR",    ARMOUR_Red,    0},
	{NULL, 0, 0}
};

const specflags_t powertype_names[] =
{
	{"POWERUP INVULNERABLE", PW_Invulnerable, 0},
	{"POWERUP BERSERK",      PW_Berserk,      0},
	{"POWERUP PARTINVIS",    PW_PartInvis,    0},
	{"POWERUP ACIDSUIT",     PW_AcidSuit,     0},
	{"POWERUP AUTOMAP",      PW_AllMap,       0},
	{"POWERUP LIGHTGOGGLES", PW_Infrared,     0},
	{"POWERUP JETPACK",      PW_Jetpack,      0},
	{"POWERUP NIGHTVISION",  PW_NightVision,  0},
	{"POWERUP SCUBA",        PW_Scuba,        0},
	{NULL, 0, 0}
};

const specflags_t simplecond_names[] =
{
	{"JUMPING",     COND_Jumping,    0},
	{"CROUCHING",   COND_Crouching,  0},
	{"SWIMMING",    COND_Swimming,   0},
	{"ATTACKING",   COND_Attacking,  0},
	{"RAMPAGING",   COND_Rampaging,  0},
	{"USING",       COND_Using,      0},
	{"WALKING",     COND_Walking,    0},
	{NULL, 0, 0}
};

//
// DDF_CompareName
//
// Compare two names. This is like stricmp(), except that spaces
// and `_' characters are ignored for comparison purposes.
//
// -AJA- 1999/09/11: written.
//
int DDF_CompareName(const char *A, const char *B)
{
	for (;;)
	{
		if (*A == 0 && *B == 0)
			return 0;

		if (*A == 0)
			return -1;

		if (*B == 0)
			return +1;

		if (*A == ' ' || *A == '_')
		{
			A++; continue;
		}

		if (*B == ' ' || *B == '_')
		{
			B++; continue;
		}

		if (toupper(*A) != toupper(*B))
			break;

		A++; B++;
	}

	return toupper(*A) - toupper(*B);
}

static bool ThingTryParseState(const char *field, 
									const char *contents, int index, bool is_last)
{
	int i;
	const state_starter_t *starter;
	const char *pos;

	epi::string_c labname;

	if (strncasecmp(field, "STATES(", 7) != 0)
		return false;

	// extract label name
	field += 7;

	pos = strchr(field, ')');

	if (pos == NULL || pos == field || pos > (field+64))
		return false;

	labname.Empty();
	labname.AddChars(field, 0, pos - field);

	// check for the "standard" states
	starter = NULL;

	for (i=0; thing_starters[i].label; i++)
		if (DDF_CompareName(thing_starters[i].label, labname.GetString()) == 0)
			break;

	if (thing_starters[i].label)
		starter = &thing_starters[i];

	DDF_StateReadState(contents, labname,
		&buffer_mobj.first_state, &buffer_mobj.last_state,
		starter ? starter->state_num : NULL, index, 
		is_last ? starter ? starter->last_redir : "IDLE" : NULL, 
		thing_actions, false);

	return true;
}


//
//  DDF PARSE ROUTINES
//

static bool ThingStartEntry(const char *buffer)
{
	int idx;

	epi::string_c s;
	int number = 0;
	char *pos = strchr(buffer, ':');

	s.Empty();
	if (pos)
	{
		s.AddChars(buffer, 0, pos - buffer);
		
		number = MAX(0, atoi(pos+1));
	}
	else
	{
		if (buffer && buffer[0])
			s = buffer;
	}

	idx = -1;

	if (!s.IsEmpty())
	{
		idx = mobjtypes.FindFirst(s.GetString(), mobjtypes.GetDisabledCount());
		if (idx>=0)
		{
			mobjtypes.MoveToEnd(idx);
			dynamic_mobj = mobjtypes[mobjtypes.GetSize()-1];
		}
	}

	if (idx < 0)
	{
		dynamic_mobj = new mobjtype_c;

		if (s.IsEmpty())
			dynamic_mobj->ddf.SetUniqueName("UNNAMED_THING", mobjtypes.GetSize());
		else
			dynamic_mobj->ddf.name.Set(s.GetString());

		mobjtypes.Insert(dynamic_mobj);
	}

	dynamic_mobj->ddf.number = number;

	// instantiate the static entry
	buffer_mobj.Default();

	return (idx >= 0);
}

void ThingParseField(const char *field, const char *contents,
					 int index, bool is_last)
{
#if (DEBUG_DDF)  
	L_WriteDebug("THING_PARSE: %s = %s;\n", field, contents);
#endif

	if (ddf_version < 0x128 &&
		(DDF_CompareName(field, "RESISTANCE_CLASS") == 0))
	{
		DDF_Error("%s is only available with #VERSION 1.28 or higher.\n", field);
	}

	if (ddf_version < 0x129 &&
		(DDF_CompareName(field, "PICKUP_EFFECT") == 0))
	{
		DDF_Error("%s is only available with #VERSION 1.29 or higher.\n", field);
	}

	if (DDF_MainParseField(thing_commands, field, contents))
		return;

	if (ThingTryParseState(field, contents, index, is_last))
		return;

	if (ddf_version < 0x128)
	{
		// handle properties (old crud)
		if (index == 0 && DDF_CompareName(contents, "TRUE") == 0)
		{
			L_WriteDebug("THING PROPERTY CRUD: %s = %s\n", field, contents);
			DDF_MobjGetSpecial(field, NULL);  // FIXME FOR OFFSETS
			return;
		}
	}

	DDF_WarnError2(0x128, "Unknown thing/attack command: %s\n", field);
}

static void ThingFinishEntry(void)
{
	if (buffer_mobj.first_state)
		DDF_StateFinishStates(buffer_mobj.first_state, buffer_mobj.last_state);

	// count-as-kill things are automatically monsters
	if (buffer_mobj.flags & MF_COUNTKILL)
		buffer_mobj.extendedflags |= EF_MONSTER;

	// countable items are always pick-up-able
	if (buffer_mobj.flags & MF_COUNTITEM)
		buffer_mobj.hyperflags |= HF_FORCEPICKUP;

	// check stuff...

	if (buffer_mobj.mass < 1)
	{
		DDF_WarnError2(0x128, "Bad MASS value %f in DDF.\n", buffer_mobj.mass);
		buffer_mobj.mass = 1;
	}

	if (buffer_mobj.halo.height >= 0)
	{
		if (buffer_mobj.halo.minsize < 0)
			buffer_mobj.halo.minsize = buffer_mobj.halo.size / 2;

		if (buffer_mobj.halo.maxsize < 0)
			buffer_mobj.halo.maxsize = buffer_mobj.halo.size * 2;
	}

	// check CAST stuff
	if (buffer_mobj.castorder > 0)
	{
		if (! buffer_mobj.chase_state)
			DDF_Error("Cast object must have CHASE states !\n");

		if (! buffer_mobj.death_state)
			DDF_Error("Cast object must have DEATH states !\n");
	}

	// check DAMAGE stuff
	if (buffer_mobj.explode_damage.nominal < 0)
	{
		DDF_WarnError2(0x128, "Bad EXPLODE_DAMAGE.VAL value %f in DDF.\n",
			buffer_mobj.explode_damage.nominal);
	}

	if (buffer_mobj.explode_radius < 0)
	{
		DDF_Error("Bad EXPLODE_RADIUS value %f in DDF.\n",
			buffer_mobj.explode_radius);
	}

	if (buffer_mobj.reload_shots <= 0)
	{
		DDF_Error("Bad RELOAD_SHOTS value %d in DDF.\n",
			buffer_mobj.reload_shots);
	}

	if (buffer_mobj.choke_damage.nominal < 0)
	{
		DDF_WarnError2(0x128, "Bad CHOKE_DAMAGE.VAL value %f in DDF.\n",
			buffer_mobj.choke_damage.nominal);
	}

	if (ddf_version < 0x129)
		CheckPowerupCompatibility();

	// FIXME: check more stuff

	if (!buffer_mobj.idle_state && buffer_mobj.spawn_state)
		buffer_mobj.idle_state = buffer_mobj.spawn_state;

	// transfer static entry to dynamic entry
	dynamic_mobj->CopyDetail(buffer_mobj);

	// compute CRC...
	// FIXME: Do something. :-)))
}

static void ThingClearAll(void)
{
	// not safe to delete the thing entries

	// Make all entries disabled
	mobjtypes.SetDisabledCount(mobjtypes.GetSize());
}

bool DDF_ReadThings(void *data, int size)
{
	readinfo_t things;

	things.memfile = (char*)data;
	things.memsize = size;
	things.tag = "THINGS";
	things.entries_per_dot = 6;

	if (things.memfile)
	{
		things.message = NULL;
		things.filename = NULL;
		things.lumpname = "DDFTHING";
	}
	else
	{
		things.message = "DDF_InitThings";
		things.filename = "things.ddf";
		things.lumpname = NULL;
	}

	things.start_entry  = ThingStartEntry;
	things.parse_field  = ThingParseField;
	things.finish_entry = ThingFinishEntry;
	things.clear_all    = ThingClearAll;

	return DDF_MainReadFile(&things);
}

void DDF_MobjInit(void)
{
	mobjtypes.Clear();
}

void DDF_MobjCleanUp(void)
{
	epi::array_iterator_c it;
	mobjtype_c *m;

	// lookup references
	for (it = mobjtypes.GetIterator(mobjtypes.GetDisabledCount()); it.IsValid(); it++)
	{
		m = ITERATOR_TO_TYPE(it, mobjtype_c*);

		cur_ddf_entryname.Format("[%s]  (things.ddf)", m->ddf.name.GetString());

		m->dropitem = m->dropitem_ref ? mobjtypes.Lookup(m->dropitem_ref) : NULL;
		m->blood = m->blood_ref ? mobjtypes.Lookup(m->blood_ref) : mobjtypes.Lookup("BLOOD");

		m->respawneffect = m->respawneffect_ref ? 
			mobjtypes.Lookup(m->respawneffect_ref) :
			(m->flags & MF_SPECIAL) ? mobjtypes.Lookup("ITEM RESPAWN") 
				                    : mobjtypes.Lookup("RESPAWN FLASH");

		m->spitspot = m->spitspot_ref ? mobjtypes.Lookup(m->spitspot_ref) : NULL;

		// -AJA- 1999/08/07: New SCALE & ASPECT fields.
		//       The parser placed ASPECT in xscale and SCALE in yscale.
		//       Now clean it up.
		m->xscale *= m->yscale;

		cur_ddf_entryname.Empty();
	}

	mobjtypes.Trim();
}

//
// ParseBenefitString
//
// Parses a string like "HEALTH(20:100)".  Returns the number of
// number parameters (0, 1 or 2).  If the brackets are missing, an
// error occurs.  If the numbers cannot be parsed, then 0 is returned
// and the param buffer contains the stuff in brackets (normally the
// param string will be empty).   FIXME: this interface is fucked.
//
static int ParseBenefitString(const char *info, char *name, char *param,
							  float *value, float *limit)
{
	int len = strlen(info);

	char *pos = strchr(info, '(');

	param[0] = 0;

	// do we have matched parentheses ?
	if (pos && len >= 4 && info[len - 1] == ')')
	{
		int len2 = (pos - info);

		Z_StrNCpy(name, info, len2);

		len -= len2 + 2;
		Z_StrNCpy(param, pos + 1, len);

		switch (sscanf(param, " %f : %f ", value, limit))
		{
			case 0: return 0;

			case 1: param[0] = 0; return 1;
			case 2: param[0] = 0; return 2;

			default:
				DDF_WarnError2(0x128, "Bad value in benefit string: %s\n", info);
				return -1;
		}
	}
	else if (pos)
	{
		DDF_WarnError2(0x128, "Malformed benefit string: %s\n", info);
		return -1;
	}

	strcpy(name, info);
	return 0;
}

//
//  BENEFIT TESTERS
//
//  These return true if the name matches that particular type of
//  benefit (e.g. "ROCKET" for ammo), and then adjusts the benefit
//  according to how many number values there were.  Otherwise returns
//  false.
//

static bool BenefitTryAmmo(const char *name, benefit_t *be,
								int num_vals)
{
	if (CHKF_Positive != DDF_MainCheckSpecialFlag(name, ammo_types, 
		&be->sub.type, false, false))
	{
		return false;
	}

	be->type = BENEFIT_Ammo;
	be->limit = 0;

	if ((ammotype_e)be->sub.type == AM_NoAmmo)
	{
		DDF_WarnError2(0x128, "Illegal ammo benefit: %s\n", name);
		return false;
	}

	if (num_vals < 1)
	{
		DDF_WarnError2(0x128, "Ammo benefit used, but amount is missing.\n");
		return false;
	}

	if (num_vals > 1)
	{
		DDF_WarnError2(0x128, "Ammo benefit cannot have a limit value.\n");
		return false;
	}

	return true;
}

static bool BenefitTryAmmoLimit(const char *name, benefit_t *be,
									 int num_vals)
{
	char namebuf[200];
	int len = strlen(name);

	// check for ".LIMIT" prefix

	if (len < 7 || DDF_CompareName(name+len-6, ".LIMIT") != 0)
		return false;

	len -= 6;
	Z_StrNCpy(namebuf, name, len);

	if (CHKF_Positive != DDF_MainCheckSpecialFlag(namebuf, ammo_types, 
		&be->sub.type, false, false))
	{
		return false;
	}

	be->type = BENEFIT_AmmoLimit;
	be->limit = 0;

	if (be->sub.type == AM_NoAmmo)
	{
		DDF_WarnError2(0x128, "Illegal ammolimit benefit: %s\n", name);
		return false;
	}

	if (num_vals < 1)
	{
		DDF_WarnError2(0x128, "AmmoLimit benefit used, but amount is missing.\n");
		return false;
	}

	if (num_vals > 1)
	{
		DDF_WarnError2(0x128, "AmmoLimit benefit cannot have a limit value.\n");
		return false;
	}

	return true;
}

static bool BenefitTryWeapon(const char *name, benefit_t *be,
								  int num_vals)
{
	int idx = weapondefs.FindFirst(name, weapondefs.GetDisabledCount());

	if (idx < 0)
		return false;

	be->sub.weap = weapondefs[idx];

	be->type = BENEFIT_Weapon;
	be->limit = 1.0f;

	if (num_vals < 1)
		be->amount = 1.0f;
	else if (be->amount != 0.0f && be->amount != 1.0f)
	{
		DDF_WarnError2(0x128, "Weapon benefit used, bad amount value: %1.1f\n", be->amount);
		return false;
	}

	if (num_vals > 1)
	{
		DDF_WarnError2(0x128, "Weapon benefit cannot have a limit value.\n");
		return false;
	}

	return true;
}

static bool BenefitTryKey(const char *name, benefit_t *be,
							   int num_vals)
{
	if (CHKF_Positive != DDF_MainCheckSpecialFlag(name, keytype_names, 
		&be->sub.type, false, false))
	{
		return false;
	}

	be->type = BENEFIT_Key;
	be->limit = 1.0f;

	if (num_vals < 1)
		be->amount = 1.0f;
	else if (be->amount != 0.0f && be->amount != 1.0f)
	{
		DDF_WarnError2(0x128, "Key benefit used, bad amount value: %1.1f\n", be->amount);
		return false;
	}

	if (num_vals > 1)
	{
		DDF_WarnError2(0x128, "Key benefit cannot have a limit value.\n");
		return false;
	}

	return true;
}

static bool BenefitTryHealth(const char *name, benefit_t *be,
								  int num_vals)
{
	if (DDF_CompareName(name, "HEALTH") != 0)
		return false;

	be->type = BENEFIT_Health;
	be->sub.type = 0;

	if (num_vals < 1)
	{
		DDF_WarnError2(0x128, "Health benefit used, but amount is missing.\n");
		return false;
	}

	if (num_vals < 2)
		be->limit = 100.0f;

	return true;
}

static bool BenefitTryArmour(const char *name, benefit_t *be,
								  int num_vals)
{
	if (CHKF_Positive != DDF_MainCheckSpecialFlag(name, armourtype_names, 
		&be->sub.type, false, false))
	{
		return false;
	}

	be->type = BENEFIT_Armour;

	if (num_vals < 1)
	{
		DDF_WarnError2(0x128, "Armour benefit used, but amount is missing.\n");
		return false;
	}

	if (num_vals < 2)
	{
		switch (be->sub.type)
		{
			case ARMOUR_Green:  be->limit = 100; break;
			case ARMOUR_Blue:   be->limit = 200; break;
			case ARMOUR_Yellow: be->limit = 200; break;
			case ARMOUR_Red:    be->limit = 200; break;
			default: ;
		}
	}

	return true;
}

static bool BenefitTryPowerup(const char *name, benefit_t *be,
								   int num_vals)
{
	if (CHKF_Positive != DDF_MainCheckSpecialFlag(name, powertype_names, 
		&be->sub.type, false, false))
	{
		return false;
	}

	be->type = BENEFIT_Powerup;

	if (num_vals < 1)
		be->amount = 999999.0f;

	if (num_vals < 2)
		be->limit = 999999.0f;

	return true;
}

static void BenefitAdd(benefit_t **list, benefit_t *source)
{
	benefit_t *cur, *tail;

	// check if this benefit overrides a previous one
	for (cur=(*list); cur; cur=cur->next)
	{
		if (cur->type == BENEFIT_Weapon)
			continue;

		if (cur->type == source->type && cur->sub.type == source->sub.type)
		{
			cur->amount = source->amount;
			cur->limit  = source->limit;
			return;
		}
	}

	// nope, create a new one and link it onto the _TAIL_
	cur = new benefit_t;

	cur[0] = source[0];
	cur->next = NULL;

	if ((*list) == NULL)
	{
		(*list) = cur;
		return;
	}

	for (tail = (*list); tail && tail->next; tail=tail->next)
	{ }

	tail->next = cur;
}

//
// DDF_MobjGetBenefit
//
// Parse a single benefit and update the benefit list accordingly.  If
// the type/subtype are not in the list, add a new entry, otherwise
// just modify the existing entry.
//
void DDF_MobjGetBenefit(const char *info, void *storage)
{
	char namebuf[200];
	char parambuf[200];
	int num_vals;

	benefit_t temp;

	DEV_ASSERT2(storage);

	num_vals = ParseBenefitString(info, namebuf, parambuf, &temp.amount, &temp.limit);

	// an error occurred ?
	if (num_vals < 0)
		return;

	if (BenefitTryAmmo(namebuf, &temp, num_vals) ||
		BenefitTryAmmoLimit(namebuf, &temp, num_vals) ||
		BenefitTryWeapon(namebuf, &temp, num_vals) ||
		BenefitTryKey(namebuf, &temp, num_vals) ||
		BenefitTryHealth(namebuf, &temp, num_vals) ||
		BenefitTryArmour(namebuf, &temp, num_vals) ||
		BenefitTryPowerup(namebuf, &temp, num_vals))
	{
		BenefitAdd((benefit_t **) storage, &temp);
		return;
	}

	DDF_WarnError2(0x128, "Unknown/Malformed benefit type: %s\n", namebuf);
}

pickup_effect_c::pickup_effect_c(pickup_effect_type_e _type,
	int _sub, int _slot, float _time) :
		next(NULL), type(_type), slot(_slot), time(_time)
{
	sub.type = _sub;
}

pickup_effect_c::pickup_effect_c(pickup_effect_type_e _type,
	weapondef_c *_weap, int _slot, float _time) :
		next(NULL), type(_type), slot(_slot), time(_time)
{
	sub.weap = _weap;
}

static void AddPickupEffect(pickup_effect_c **list, pickup_effect_c *cur)
{
	cur->next = NULL;

	if ((*list) == NULL)
	{
		(*list) = cur;
		return;
	}

	pickup_effect_c *tail;

	for (tail = (*list); tail && tail->next; tail=tail->next)
	{ }

	tail->next = cur;
}

void BA_ParsePowerupEffect(pickup_effect_c **list,
	int pnum, float par1, float par2, const char *word_par)
{
	int p_up = (int)par1;
	int slot = (int)par2;

	DEV_ASSERT2(0 <= p_up && p_up < NUMPOWERS);

	if (slot < 0 || slot >= NUM_FX_SLOT)
		DDF_Error("POWERUP_EFFECT: bad FX slot #%s\n", par1);

	AddPickupEffect(list, new pickup_effect_c(PUFX_PowerupEffect, p_up, slot, 0));
}

void BA_ParseScreenEffect(pickup_effect_c **list,
	int pnum, float par1, float par2, const char *word_par)
{
	int slot = (int)par1;

	if (slot < 0 || slot >= NUM_FX_SLOT)
		DDF_Error("SCREEN_EFFECT: bad FX slot #%s\n", par1);

	if (par2 <= 0)
		DDF_Error("SCREEN_EFFECT: bad time value: %1.2f\n", par2);

	AddPickupEffect(list, new pickup_effect_c(PUFX_ScreenEffect, 0, slot, par2));
}

void BA_ParseSwitchWeapon(pickup_effect_c **list,
	int pnum, float par1, float par2, const char *word_par)
{
	if (pnum != -1)
		DDF_Error("SWITCH_WEAPON: missing weapon name !\n");

	DEV_ASSERT2(word_par && word_par[0]);

	weapondef_c *weap = weapondefs.Lookup(word_par);

	AddPickupEffect(list, new pickup_effect_c(PUFX_SwitchWeapon, weap, 0, 0));
}

typedef struct pick_fx_parser_s
{
	const char *name;
	int num_pars;  // -1 means a single word
	void (* parser)(pickup_effect_c **list,
			int pnum, float par1, float par2, const char *word_par);
}
pick_fx_parser_t;

static const pick_fx_parser_t pick_fx_parsers[] =
{
	{ "SCREEN EFFECT",  2, BA_ParseScreenEffect },
	{ "SWITCH WEAPON", -1, BA_ParseSwitchWeapon },

	// that's all, folks.
	{ NULL, 0, NULL }
};

//
// DDF_MobjGetPickupEffect
//
// Parse a single effect and add it to the effect list accordingly.
// No merging is done.
//
void DDF_MobjGetPickupEffect(const char *info, void *storage)
{
	char namebuf[200];
	char parambuf[200];
	int num_vals;

	DEV_ASSERT2(storage);

	pickup_effect_c **fx_list = (pickup_effect_c **) storage;

	benefit_t temp; // FIXME kludge (write new parser method ?)

	num_vals = ParseBenefitString(info, namebuf, parambuf, &temp.amount, &temp.limit);

	// an error occurred ?
	if (num_vals < 0)
		return;

	if (parambuf[0])
		num_vals = -1;

	for (int i=0; pick_fx_parsers[i].name; i++)
	{
		if (DDF_CompareName(pick_fx_parsers[i].name, namebuf) != 0)
			continue;

		(* pick_fx_parsers[i].parser)(fx_list, num_vals, 
			temp.amount, temp.limit, parambuf);

		return;
	}

	// secondly, try the powerups
	for (int p=0; powertype_names[p].name; p++)
	{
		if (DDF_CompareName(powertype_names[p].name, namebuf) != 0)
			continue;

		BA_ParsePowerupEffect(fx_list, num_vals, p, temp.amount, parambuf);

		return;
	}

	DDF_Error("Unknown/Malformed benefit effect: %s\n", namebuf);
}

// -KM- 1998/11/25 Translucency to fractional.
// -KM- 1998/12/16 Added individual flags for all.
// -AJA- 2000/02/02: Split into two lists.

static const specflags_t normal_specials[] =
{
	{"AMBUSH", MF_AMBUSH, 0},
	{"FUZZY", MF_FUZZY, 0},
	{"SOLID", MF_SOLID, 0},
	{"ON CEILING", MF_SPAWNCEILING + MF_NOGRAVITY, 0},
	{"FLOATER", MF_FLOAT + MF_NOGRAVITY, 0},
	{"INERT", MF_NOBLOCKMAP, 0},
	{"TELEPORT TYPE", MF_NOGRAVITY, 0},
	{"LINKS", MF_NOBLOCKMAP + MF_NOSECTOR, 1},
	{"DAMAGESMOKE", MF_NOBLOOD, 0},
	{"SHOOTABLE", MF_SHOOTABLE, 0},
	{"COUNT AS KILL", MF_COUNTKILL, 0},
	{"COUNT AS ITEM", MF_COUNTITEM, 0},
	{"SPECIAL", MF_SPECIAL, 0},
	{"SECTOR", MF_NOSECTOR, 1},
	{"BLOCKMAP", MF_NOBLOCKMAP, 1},
	{"SPAWNCEILING", MF_SPAWNCEILING, 0},
	{"GRAVITY", MF_NOGRAVITY, 1},
	{"DROPOFF", MF_DROPOFF, 0},
	{"PICKUP", MF_PICKUP, 0},
	{"CLIP", MF_NOCLIP, 1},
	{"SLIDER", MF_SLIDE, 0},
	{"FLOAT", MF_FLOAT, 0},
	{"TELEPORT", MF_TELEPORT, 0},
	{"MISSILE", MF_MISSILE, 0},
	{"BARE MISSILE", MF_MISSILE, 0},
	{"DROPPED", MF_DROPPED, 0},
	{"CORPSE", MF_CORPSE, 0},
	{"STEALTH", MF_STEALTH, 0},
	{"DEATHMATCH", MF_NOTDMATCH, 1},
	{"TOUCHY", MF_TOUCHY, 0},
	{NULL, 0, 0}
};

static specflags_t extended_specials[] =
{
	{"RESPAWN", EF_NORESPAWN, 1},
	{"RESURRECT", EF_NORESURRECT, 1},
	{"DISLOYAL", EF_DISLOYALTYPE, 0},
	{"TRIGGER HAPPY", EF_TRIGGERHAPPY, 0},
	{"ATTACK HURTS", EF_OWNATTACKHURTS, 0},
	{"EXPLODE IMMUNE", EF_EXPLODEIMMUNE, 0},
	{"ALWAYS LOUD", EF_ALWAYSLOUD, 0},
	{"BOSSMAN", EF_EXPLODEIMMUNE + EF_ALWAYSLOUD, 0},
	{"NEVERTARGETED", EF_NEVERTARGET, 0},
	{"GRAV KILL", EF_NOGRAVKILL, 1},
	{"GRUDGE", EF_NOGRUDGE, 1},
	{"BOUNCE", EF_BOUNCE, 0},
	{"EDGEWALKER", EF_EDGEWALKER, 0},
	{"GRAVFALL", EF_GRAVFALL, 0},
	{"CLIMBABLE", EF_CLIMBABLE, 0},
	{"WATERWALKER", EF_WATERWALKER, 0},
	{"MONSTER", EF_MONSTER, 0},
	{"CROSSLINES", EF_CROSSLINES, 0},
	{"FRICTION", EF_NOFRICTION, 1},
	{"USABLE", EF_USABLE, 0},
	{"BLOCK SHOTS", EF_BLOCKSHOTS, 0},
	{"TUNNEL", EF_TUNNEL, 0},
	{NULL, 0, 0}
};

static specflags_t hyper_specials[] =
{
	{"FORCE PICKUP", HF_FORCEPICKUP, 0},
	{"SIDE IMMUNE", HF_SIDEIMMUNE, 0},
	{"ULTRA LOYAL", HF_ULTRALOYAL, 0},
	{NULL, 0, 0}
};

//
// DDF_MobjGetSpecial
//
// Compares info the the entries in special flag lists.  If found
// apply attribs for it to current buffer_mobj.
//
void DDF_MobjGetSpecial(const char *info, void *storage)
{
	int flag_value;

	// handle the "INVISIBLE" tag
	if (DDF_CompareName(info, "INVISIBLE") == 0)
	{
		buffer_mobj.translucency = PERCENT_MAKE(0);
		return;
	}

	// handle the "NOSHADOW" tag
	if (DDF_CompareName(info, "NOSHADOW") == 0)
	{
		buffer_mobj.shadow_trans = PERCENT_MAKE(0);
		return;
	}

	// the "MISSILE" tag needs special treatment, since it sets both
	// normal flags & extended flags.
	if (DDF_CompareName(info, "MISSILE") == 0)
	{
		buffer_mobj.flags |= MF_MISSILE;
		buffer_mobj.extendedflags |= EF_CROSSLINES | EF_NOFRICTION;
		return;
	}

	int *flag_ptr = &buffer_mobj.flags; 

	checkflag_result_e res =
		DDF_MainCheckSpecialFlag(info, normal_specials,
			&flag_value, true, false);

	if (res == CHKF_User || res == CHKF_Unknown)
	{
		// wasn't a normal special.  Try the extended ones...
		flag_ptr = &buffer_mobj.extendedflags;

		res = DDF_MainCheckSpecialFlag(info, extended_specials,
				&flag_value, true, false);
	}

	if (res == CHKF_User || res == CHKF_Unknown)
	{
		// -AJA- 2004/08/25: Try the hyper specials...
		flag_ptr = &buffer_mobj.hyperflags;

		res = DDF_MainCheckSpecialFlag(info, hyper_specials,
				&flag_value, true, false);
	}

	switch (res)
	{
		case CHKF_Positive:
			*flag_ptr |= flag_value;
			break;

		case CHKF_Negative:
			*flag_ptr &= ~flag_value;
			break;

		case CHKF_User:
		case CHKF_Unknown:
			DDF_WarnError2(0x128, "DDF_MobjGetSpecial: Unknown special '%s'", info);
			break;
	}
}

static specflags_t dlight_type_names[] =
{
	{"NONE",      DLITE_None,      0},
	{"LINEAR",    DLITE_Linear,    0},
	{"QUADRATIC", DLITE_Quadratic, 0},

	// backwards compatibility
	{"CONSTANT",  DLITE_Quadratic, 0},
	{NULL, 0, 0}
};

//
// DDF_MobjGetDLight
//
void DDF_MobjGetDLight(const char *info, void *storage)
{
	dlight_type_e *dtype = (dlight_type_e *)storage;
	int flag_value;

	DEV_ASSERT2(dtype);

	if (CHKF_Positive != DDF_MainCheckSpecialFlag(info, 
		dlight_type_names, &flag_value, false, false))
	{
		DDF_WarnError2(0x128, "Unknown dlight type '%s'", info);
		return;
	}

	(*dtype) = (dlight_type_e)flag_value;
}

//
// DDF_MobjGetExtra
//
// The "EXTRA" field. FIXME: Improve the system.
//
void DDF_MobjGetExtra(const char *info, void *storage)
{
	// If NULL is passed, then the mobj is not marked as extra. Otherwise,
	// it is (in the future, we may support a system where extras can be split
	// up into several subsets, which can be individually toggled, based
	// on the EXTRA= value).
	if (!DDF_CompareName(info, "NULL"))
	{
		buffer_mobj.extendedflags &= ~EF_EXTRA;
	}
	else
	{
		buffer_mobj.extendedflags |= EF_EXTRA;
	}
}

//
// DDF_MobjGetPlayer
//
// Reads player number and makes sure that maxplayer is large enough.
//
void DDF_MobjGetPlayer(const char *info, void *storage)
{
	int *dest = (int *)storage;

	DDF_MainGetNumeric(info, storage);

	if (*dest > MAXPLAYERS)
		DDF_Error("Bad player number `%d'.  Maximum is %d\n", *dest,
		MAXPLAYERS);
}

//
// DDF_MobjMakeAttackObj
//
// Creates an object that is tightly bound to an attack.
//
// -AJA- 2000/02/11: written.
//
mobjtype_c *DDF_MobjMakeAttackObj(mobjtype_c *info, const char *atk_name)
{
	epi::string_c s;
	mobjtype_c *result;

	s = "__ATKMOBJ_";
	s += atk_name;

	result = new mobjtype_c;
	result->CopyDetail(info[0]);
	result->ddf.name.Set(s);

	// Add to the list
	mobjtypes.Insert(result);

	return result;
}

//
//  CONDITION TESTERS
//
//  These return true if the name matches that particular type of
//  condition (e.g. "ROCKET" for ammo), and adjusts the condition
//  accodingly.  Otherwise returns false.
//

static bool ConditionTryAmmo(const char *name, const char *sub,
								  condition_check_t *cond)
{
	if (CHKF_Positive != DDF_MainCheckSpecialFlag(name, ammo_types, 
		&cond->sub.type, false, false))
	{
		return false;
	}

	if ((ammotype_e)cond->sub.type == AM_NoAmmo)
	{
		DDF_WarnError2(0x128, "Illegal ammo in condition: %s\n", name);
		return false;
	}

	if (sub[0])
		sscanf(sub, " %f ", &cond->amount);

	cond->cond_type = COND_Ammo;
	return true;
}

static bool ConditionTryWeapon(const char *name, const char *sub,
									condition_check_t *cond)
{
	int idx = weapondefs.FindFirst(name, weapondefs.GetDisabledCount());

	if (idx < 0)
		return false;

	cond->sub.weap = weapondefs[idx];

	cond->cond_type = COND_Weapon;
	return true;
}

static bool ConditionTryKey(const char *name, const char *sub,
								 condition_check_t *cond)
{
	if (CHKF_Positive != DDF_MainCheckSpecialFlag(name, keytype_names, 
		&cond->sub.type, false, false))
	{
		return false;
	}

	cond->cond_type = COND_Key;
	return true;
}

static bool ConditionTryHealth(const char *name, const char *sub,
									condition_check_t *cond)
{
	if (DDF_CompareName(name, "HEALTH") != 0)
		return false;

	if (sub[0])
		sscanf(sub, " %f ", &cond->amount);

	cond->cond_type = COND_Health;
	return true;
}

static bool ConditionTryArmour(const char *name, const char *sub,
									condition_check_t *cond)
{
	if (DDF_CompareName(name, "ARMOUR") == 0)
	{
		cond->sub.type = ARMOUR_Total;
	}
	else if (CHKF_Positive != DDF_MainCheckSpecialFlag(name, armourtype_names, 
		&cond->sub.type, false, false))
	{
		return false;
	}

	if (sub[0])
		sscanf(sub, " %f ", &cond->amount);

	cond->cond_type = COND_Armour;
	return true;
}

static bool ConditionTryPowerup(const char *name, const char *sub,
									 condition_check_t *cond)
{
	if (CHKF_Positive != DDF_MainCheckSpecialFlag(name, powertype_names, 
		&cond->sub.type, false, false))
	{
		return false;
	}

	if (sub[0])
	{
		sscanf(sub, " %f ", &cond->amount);

		cond->amount *= (float)TICRATE;
	}

	cond->cond_type = COND_Powerup;
	return true;
}

static bool ConditionTryPlayerState(const char *name, const char *sub,
										 condition_check_t *cond)
{
	return (CHKF_Positive == DDF_MainCheckSpecialFlag(name, 
		simplecond_names, (int *)&cond->cond_type, false, false));
}

static void CheckPowerupCompatibility(void)
{
	// when using BERSERK, need to switch to the FIST

	for (benefit_t *b = buffer_mobj.pickup_benefits; b; b = b->next)
	{
		if (b->type == BENEFIT_Powerup && b->sub.type == PW_Berserk)
		{
			int idx = weapondefs.FindFirst("FIST", weapondefs.GetDisabledCount());

			if (idx >= 0)
				AddPickupEffect(&buffer_mobj.pickup_effects,
					new pickup_effect_c(PUFX_SwitchWeapon, weapondefs[idx], 0, 0));
			break;
		}
	}
}

//
// DDF_MainParseCondition
//
// Returns `false' if parsing failed.
//
bool DDF_MainParseCondition(const char *info, condition_check_t *cond)
{
	char typebuf[100];
	char sub_buf[100];

	int len = strlen(info);
	int t_off;
	const char *pos;

	cond->negate = false;
	cond->cond_type = COND_NONE;
	cond->amount = 1;

	memset(&cond->sub, 0, sizeof(cond->sub));

	pos = strchr(info, '(');

	// do we have matched parentheses ?
	if (pos && pos > info && len >= 4 && info[len-1] == ')')
	{
		int len2 = (pos - info);

		Z_StrNCpy(typebuf, info, len2);

		len -= len2 + 2;
		Z_StrNCpy(sub_buf, pos + 1, len);
	}
	else if (pos || strchr(info, ')'))
	{
		DDF_WarnError2(0x128, "Malformed condition string: %s\n", info);
		return false;
	}
	else
	{
		strcpy(typebuf, info);
		sub_buf[0] = 0;
	}

	// check for negation
	t_off = 0;
	if (strnicmp(typebuf, "NOT_", 4) == 0)
	{
		cond->negate = true;
		t_off = 4;
	}

	if (ConditionTryAmmo(typebuf + t_off, sub_buf, cond) ||
		ConditionTryWeapon(typebuf + t_off, sub_buf, cond) ||
		ConditionTryKey(typebuf + t_off, sub_buf, cond) ||
		ConditionTryHealth(typebuf + t_off, sub_buf, cond) ||
		ConditionTryArmour(typebuf + t_off, sub_buf, cond) ||
		ConditionTryPowerup(typebuf + t_off, sub_buf, cond) ||
		ConditionTryPlayerState(typebuf + t_off, sub_buf, cond))
	{
		return true;
	}

	DDF_WarnError2(0x128, "Unknown/Malformed condition type: %s\n", typebuf);
	return false;
}

// ---> mobjdef class

// 
// mobjtype_c Constructor
//
mobjtype_c::mobjtype_c()
{
	Default();
}

//
// mobjtype_c Copy Constructor
//
mobjtype_c::mobjtype_c(mobjtype_c &rhs)
{
	Copy(rhs);
}

//
// mobjtype_c Destructor
//
mobjtype_c::~mobjtype_c()
{
}

//
// mobjtype_c::Copy()
//
void mobjtype_c::Copy(mobjtype_c &src)
{
	ddf = src.ddf;
	CopyDetail(src);	
}

//
// mobjtype_c::CopyDetail()
//
void mobjtype_c::CopyDetail(mobjtype_c &src)
{
	first_state = src.first_state; 
	last_state = src.last_state; 

    spawn_state = src.spawn_state; 
    idle_state = src.idle_state; 
    chase_state = src.chase_state; 
    pain_state = src.pain_state; 
    missile_state = src.missile_state; 
    melee_state = src.melee_state; 
    death_state = src.death_state; 
    overkill_state = src.overkill_state; 
    raise_state = src.raise_state; 
    res_state = src.res_state; 
    meander_state = src.meander_state; 
    bounce_state = src.bounce_state; 
    touch_state = src.touch_state; 
    jump_state = src.jump_state; 
    reload_state = src.reload_state; 
    gib_state = src.gib_state; 

    reactiontime = src.reactiontime; 
	painchance = src.painchance; 
	spawnhealth = src.spawnhealth; 
    speed = src.speed; 
	float_speed = src.float_speed; 
    radius = src.radius; 
    height = src.height; 
	step_size = src.step_size; 
	mass = src.mass; 

    flags = src.flags; 
    extendedflags = src.extendedflags; 
    hyperflags = src.hyperflags; 

	explode_damage = src.explode_damage;	
	explode_radius = src.explode_radius;

	lose_benefits = src.lose_benefits; 
	pickup_benefits = src.pickup_benefits; 
	pickup_effects = src.pickup_effects; 
	pickup_message = src.pickup_message; 
	initial_benefits = src.initial_benefits; 

    castorder = src.castorder; 
	cast_title = src.cast_title; 
	respawntime = src.respawntime; 
	translucency = src.translucency; 
	minatkchance = src.minatkchance; 
	palremap = src.palremap; 

	jump_delay = src.jump_delay; 
    jumpheight = src.jumpheight; 
    crouchheight = src.crouchheight; 
	viewheight = src.viewheight; 
	shotheight = src.shotheight; 
    maxfall = src.maxfall; 
	fast = src.fast; 
	xscale = src.xscale; 
	yscale = src.yscale; 
	bounce_speed = src.bounce_speed; 
	bounce_up = src.bounce_up; 
	sight_slope = src.sight_slope; 
	sight_angle = src.sight_angle; 
	ride_friction = src.ride_friction; 
	shadow_trans = src.shadow_trans; 

	seesound = src.seesound; 
	attacksound = src.attacksound; 
	painsound = src.painsound; 
	deathsound = src.deathsound; 
	overkill_sound = src.overkill_sound; 
	activesound = src.activesound; 
	walksound = src.walksound; 
	jump_sound = src.jump_sound; 
	noway_sound = src.noway_sound; 
	oof_sound = src.oof_sound; 
	gasp_sound = src.gasp_sound; 

    fuse = src.fuse; 
	reload_shots = src.reload_shots;
	side = src.side; 
    playernum = src.playernum; 
	lung_capacity = src.lung_capacity; 
	gasp_start = src.gasp_start; 

	// choke_damage
	choke_damage = src.choke_damage;	

	bobbing = src.bobbing; 
	immunity = src.immunity; 
	resistance = src.resistance; 

	closecombat = src.closecombat; 
	rangeattack = src.rangeattack; 
	spareattack = src.spareattack; 

	// halo info (Handled in constructor)
	halo = src.halo;

	// dynamic light info
	dlight = src.dlight;

	dropitem = src.dropitem; 
	dropitem_ref = src.dropitem_ref; 
	blood = src.blood; 
	blood_ref = src.blood_ref; 
	respawneffect = src.respawneffect; 
	respawneffect_ref = src.respawneffect_ref; 
	spitspot = src.spitspot; 
	spitspot_ref = src.spitspot_ref; 
}

//
// mobjtype_c::Default()
//
void mobjtype_c::Default()
{
	ddf.Default();

	first_state = 0;
	last_state = 0; 

    spawn_state = 0;
    idle_state = 0;
    chase_state = 0;
    pain_state = 0;
    missile_state = 0;
    melee_state = 0;
    death_state = 0;
    overkill_state = 0;
    raise_state = 0;
    res_state = 0;
    meander_state = 0;
    bounce_state = 0;
    touch_state = 0;
    jump_state = 0;
    reload_state = 0;
    gib_state = 0;

    reactiontime = 0;
	painchance = PERCENT_MAKE(0);
	spawnhealth = 1000.0f;
    speed = 0;
	float_speed = 2.0f;
    radius = 0;
    height = 0;
	step_size = 24.0f;
	mass = 100.0f;

    flags = 0;
    extendedflags = 0;
	hyperflags = 0;

	explode_damage.Default(damage_c::DEFAULT_Mobj);
	explode_radius = 0;

	lose_benefits = NULL;
	pickup_benefits = NULL;
	pickup_effects = NULL;
	pickup_message = NULL;
	initial_benefits = NULL;

    castorder = 0;
	cast_title.Clear();
	respawntime = 30 * TICRATE;
	translucency = PERCENT_MAKE(100);
	minatkchance = PERCENT_MAKE(0);
	palremap = NULL;

	jump_delay = 1 * TICRATE;
    jumpheight = 0;
    crouchheight = 0;
	viewheight = PERCENT_MAKE(75);
	shotheight = PERCENT_MAKE(64);
    maxfall = 0;
	fast = 1.0f;
	xscale = 1.0f;
	yscale = 1.0f;
	bounce_speed = 0.5f;
	bounce_up = 0.5f;
	sight_slope = 16.0f;
	sight_angle = ANG90;
	ride_friction = RIDE_FRICTION;
	shadow_trans = PERCENT_MAKE(50);

	seesound = sfx_None;
	attacksound = sfx_None;
	painsound = sfx_None;
	deathsound = sfx_None;
	overkill_sound = sfx_None;
	activesound = sfx_None;
	walksound = sfx_None;
	jump_sound = sfx_None;
	noway_sound = sfx_None;
	oof_sound = sfx_None;
	gasp_sound = sfx_None;

    fuse = 0;
	reload_shots = 5;
	side = BITSET_EMPTY;
    playernum = 0;
	lung_capacity = 20 * TICRATE;
	gasp_start = 2  * TICRATE;

	choke_damage.Default(damage_c::DEFAULT_MobjChoke);

	bobbing = PERCENT_MAKE(100);
	immunity = BITSET_EMPTY;
	resistance = BITSET_EMPTY;

	closecombat = NULL;
	rangeattack = NULL;
	spareattack = NULL;

	// halo info 
	halo.Default();

	// dynamic light info
	dlight.Default();

	dropitem = NULL;
	dropitem_ref.Clear();
	blood = NULL;
	blood_ref.Clear();
	respawneffect = NULL;
	respawneffect_ref.Clear();
	spitspot = NULL;
	spitspot_ref.Clear();
}

//
// mobjtype_c assignment operator
//
mobjtype_c& mobjtype_c::operator=(mobjtype_c &rhs)
{
	if (&rhs != this)
		Copy(rhs);
		
	return *this;
}

// --> mobjtype_container_c class

//
// mobjtype_container_c::mobjtype_container_c()
//
mobjtype_container_c::mobjtype_container_c() : epi::array_c(sizeof(mobjtype_c*))
{
	memset(lookup_cache, 0, sizeof(mobjtype_c*) * LOOKUP_CACHESIZE);
	num_disabled = 0;	
}

//
// ~mobjtype_container_c::mobjtype_container_c()
//
mobjtype_container_c::~mobjtype_container_c()
{
	Clear();					// <-- Destroy self before exiting
}

//
// mobjtype_containter_c::CleanupObject
//
void mobjtype_container_c::CleanupObject(void *obj)
{
	mobjtype_c *m = *(mobjtype_c**)obj;

	if (m)
		delete m;

	return;
}

//
// mobjtype_container_c::FindFirst
//
int mobjtype_container_c::FindFirst(const char *name, int startpos)
{
	epi::array_iterator_c it;
	mobjtype_c *m;

	if (startpos>0)
		it = GetIterator(startpos);
	else
		it = GetBaseIterator();

	while (it.IsValid())
	{
		m = ITERATOR_TO_TYPE(it, mobjtype_c*);
		if (DDF_CompareName(m->ddf.name.GetString(), name) == 0)
		{
			return it.GetPos();
		}

		it++;
	}

	return -1;
}

//
// mobjtype_container_c::FindLast
//
int mobjtype_container_c::FindLast(const char *name, int startpos)
{
	epi::array_iterator_c it;
	mobjtype_c *m;

	if (startpos>=0 && startpos<array_entries)
		it = GetIterator(startpos);
	else
		it = GetTailIterator();

	while (it.IsValid())
	{
		m = ITERATOR_TO_TYPE(it, mobjtype_c*);
		if (DDF_CompareName(m->ddf.name.GetString(), name) == 0)
		{
			return it.GetPos();
		}

		it--;
	}

	return -1;
}

//
// mobjtype_container_c::MoveToEnd
//
// Moves an entry from its current position to end of the list
//
bool mobjtype_container_c::MoveToEnd(int idx)
{
	mobjtype_c* m;

	if (idx < 0 || idx >= array_entries)
		return false;

	if (idx == (array_entries - 1))
		return true;					// Already at the end

	// Get a copy of the pointer 
	m = (*this)[idx];

	I_MoveData(&array[idx*array_block_objsize], 
		&array[(idx+1)*array_block_objsize], 
		(array_entries-(idx+1))*array_objsize);

	memcpy(&array[(array_entries-1)*array_block_objsize], (void*)&m, sizeof(mobjtype_c*));
	return true;
}

//
// const mobjtype_c* mobjtype_container_c::Lookup()
//
// Looks an mobjdef by name, returns a fatal error if it does not exist.
//
const mobjtype_c* mobjtype_container_c::Lookup(const char *refname)
{
	int idx;

	idx = FindLast(refname);

	// special rule for internal names (beginning with `_'), to allow
	// savegame files to find attack MOBJs that may have been masked by
	// a #CLEARALL.
	if (idx >= 0 && refname[0] != '_' && idx < num_disabled)
		idx = -1;

	if (idx >= 0)
		return (*this)[idx];

	if (lax_errors)
		return (*this)[0];

	// FIXME!!! Throw an epi::error_c obj
	DDF_Error("Unknown thing type: %s\n", refname);
	return NULL;
}

//
// const mobjtype_c* mobjtype_container_c::Lookup()
//
// Looks an mobjdef by number, returns a fatal error if it does not exist.
//
const mobjtype_c* mobjtype_container_c::Lookup(int id)
{
	int slot = DDF_MobjHashFunc(id);

	// check the cache
	if (lookup_cache[slot] &&
		lookup_cache[slot]->ddf.number == id)
	{
		return lookup_cache[slot];
	}

	epi::array_iterator_c it;
	mobjtype_c *m = NULL;

	for (it = GetTailIterator(); it.IsValid(); it--)
	{
		m = ITERATOR_TO_TYPE(it, mobjtype_c*);
		if (m->ddf.number == id)
		{
			break;
		}
	}

	if (!it.IsValid())
		return NULL;

	if (it.GetPos() < num_disabled)
		return NULL;

	// do a sprite check (like for weapons)
	if (! DDF_CheckSprites(m->first_state, m->last_state))
		return NULL;

	// update the cache
	lookup_cache[slot] = m;
	return m;
}

//
// mobjtype_container_c::LookupCastMember()
//
// Lookup the cast member of the one with the nearest match to the position given.
//
const mobjtype_c* mobjtype_container_c::LookupCastMember(int castpos)
{
	epi::array_iterator_c it;
	mobjtype_c* best;
	mobjtype_c* m;

	best = NULL;
	for (it = GetTailIterator(); it.IsValid() && (int)it.GetPos() >= num_disabled; it--)
	{
		m = ITERATOR_TO_TYPE(it, mobjtype_c*);
		if (m->castorder > 0)
		{
			if (m->castorder == castpos)	// Exact match
				return m;

			if (best)
			{
				if (m->castorder > castpos)
				{
					if (best->castorder > castpos)
					{
						int of1 = m->castorder - castpos;
						int of2 = best->castorder - castpos;

						if (of2 > of1)
							best = m;
					}
					else
					{
						// Our previous was before the requested
						// entry in the cast order, this is later and
						// as such always better.
						best = m;
					}
				}
				else
				{
					// We only care about updating this if the
					// best match was also prior to current 
					// entry. In this case we are looking for
					// the first entry to wrap around to.
					if (best->castorder < castpos)
					{
						int of1 = castpos - m->castorder;
						int of2 = castpos - best->castorder;

						if (of1 > of2)
							best = m;
					}
				}
			}
			else
			{
				// We don't have a best item, so this has to be our best current match
				best = m;
			}
		}
	}

	return best;
}

//
// mobjtype_container_c::LookupCastMember()
//
// Find a player thing (needed by deathmatch code).
//
const mobjtype_c* mobjtype_container_c::LookupPlayer(int playernum)
{
	epi::array_iterator_c it;

	for (it = GetTailIterator(); it.IsValid() && (int)it.GetPos() >= num_disabled; it--)
	{
		mobjtype_c *m = ITERATOR_TO_TYPE(it, mobjtype_c*);

		if (m->playernum == playernum)
			return m;
	}

	I_Error("Missing DDF entry for player number %d\n", playernum);
	return NULL; /* NOT REACHED */
}
