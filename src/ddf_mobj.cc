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
#include "dm_state.h"
#include "e_search.h"
#include "m_fixed.h"
#include "m_math.h"
#include "p_action.h"
#include "p_local.h"
#include "p_mobj.h"
#include "z_zone.h"

#undef  DF
#define DF  DDF_CMD

#define DDF_MobjHashFunc(x)  (((x) + 211) % 211)

mobjinfo_t buffer_mobj;
mobjinfo_t *dynamic_mobj;

const mobjinfo_t template_mobj =
{
	DDF_BASE_NIL,  // ddf

	0,        // first_state
	0,        // last_state

	0,        // spawn_state
	0,        // idle_state
	0,        // chase_state
	0,        // pain_state
	0,        // missile_state
	0,        // melee_state
	0,        // death_state
	0,        // overkill_state
	0,        // raise_state
	0,        // res_state
	0,        // meander_state
	0,        // bounce_state
	0,        // touch_state
	0,        // jump_state
	0,        // gib_state

	0,        // reactiontime
	PERCENT_MAKE(0), // painchance
	1000.0f,  // spawnhealth
	0,        // speed
	2.0f,     // float_speed
	0,        // radius
	0,        // height
	24.0f,    // step_size
	100.0f,   // mass

	0,        // flags
	0,        // extendedflags

	// damage info
	{
		0,      // nominal
		-1,     // linear_max
		-1,     // error
		0,      // delay time
		NULL_LABEL, NULL_LABEL, NULL_LABEL,  // override labels
		false   // no_armour
	},

	NULL,     // lose_benefits
	NULL,     // pickup_benefits
	NULL,     // pickup_message
	NULL,     // initial_benefits

	0,        // castorder
	30 * TICRATE,  // respawntime
	PERCENT_MAKE(100), // translucency
	PERCENT_MAKE(0), // minatkchance
	NULL,     // palremap

	1 * TICRATE, // jump_delay
	0,        // jumpheight
	0,        // crouchheight
	PERCENT_MAKE(75), // viewheight
	PERCENT_MAKE(64), // shotheight
	0,        // maxfall
	1.0f,     // fast
	1.0f,     // xscale
	1.0f,     // yscale
	0.5f,     // bounce_speed
	0.5f,     // bounce_up
	16.0f,    // sight_slope
	ANG90,    // sight_angle
	RIDE_FRICTION,  // ride_friction
	PERCENT_MAKE(50), // shadow_trans

	sfx_None,  // seesound
	sfx_None,  // attacksound
	sfx_None,  // painsound
	sfx_None,  // deathsound
	sfx_None,  // overkill_sound
	sfx_None,  // activesound
	sfx_None,  // walksound
	sfx_None,  // jump_sound
	sfx_None,  // noway_sound
	sfx_None,  // oof_sound
	sfx_None,  // gasp_sound

	0,        // fuse
	BITSET_EMPTY, // side
	0,        // playernum
	20 * TICRATE,  // lung_capacity
	2  * TICRATE,  // gasp_start

	// choke_damage
	{
		6,    // nominal
		14,   // linear_max
		-1,   // error
		2 * TICRATE,  // delay time
		NULL_LABEL, NULL_LABEL, NULL_LABEL,  // override labels
		true   // no_armour
	},

	PERCENT_MAKE(100), // bobbing
	BITSET_EMPTY, // immunity
	BITSET_EMPTY, // resistance

	NULL,     // closecombat
	NULL,     // rangeattack
	NULL,     // spareattack

	// halo info
	{
		-1,          // height
		32, -1, -1,  // normal/min/max size
		PERCENT_MAKE(50), // translucency
		0xFFFFFF,    // colour (RGB 8:8:8)
		""           // graphic name
	},

	// dynamic light info
	{
		DLITE_None,  // type
		32,          // intensity
		0xFFFFFF,    // colour (RGB 8:8:8)
		PERCENT_MAKE(50)  // height
	},

	NULL,     // dropitem
	NULL,     // dropitem_ref
	NULL,     // blood
	NULL,     // blood_ref
	NULL,     // respawneffect
	NULL,     // respawneffect_ref
	NULL,     // spitspot
	NULL      // spitspot_ref
};

mobjinfo_t ** mobjinfo = NULL;
int num_mobjinfo = 0;
int num_disabled_mobjinfo = 0;

static stack_array_t mobjinfo_a;

static mobjinfo_t *mobj_lookup_cache[211];

void DDF_MobjGetBenefit(const char *info, void *storage);
void DDF_MobjGetDLight(const char *info, void *storage);

static dlightinfo_t dummy_dlight;
static haloinfo_t dummy_halo;

#undef  DDF_CMD_BASE
#define DDF_CMD_BASE  dummy_halo

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
#define DDF_CMD_BASE  dummy_dlight

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
	DDF_SUB_LIST("HALO",   halo,   halo_commands,   dummy_halo),
	DDF_SUB_LIST("DLIGHT", dlight, dlight_commands, dummy_dlight),
	DDF_SUB_LIST("EXPLODE DAMAGE", damage, damage_commands, dummy_damage),
	DDF_SUB_LIST("CHOKE DAMAGE", choke_damage, damage_commands, dummy_damage),

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

	// -AJA- backwards compatibility cruft...
	DF("!EXPLOD DAMAGE", damage.nominal, DDF_MainGetFloat),
	DF("!EXPLOSION DAMAGE", damage.nominal, DDF_MainGetFloat),
	DF("!EXPLOD DAMAGERANGE", damage.nominal, DDF_MainGetFloat),
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
	{"GIB",        "REMOVE",   &buffer_mobj.gib_state},
	{NULL, NULL, NULL}
};

// -KM- 1998/11/25 Added weapon functions.
// -AJA- 1999/08/09: Moved this here from p_action.h, and added an extra
// field `handle_arg' for things like "WEAPON_SHOOT(FIREBALL)".

static const actioncode_t thing_actions[] =
{
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
	{"PLAYER_SCREAM",     A_PlayerScream, NULL},
	{"CLOSE ATTACK",      P_ActMeleeAttack, DDF_StateGetAttack},
	{"RANGE ATTACK",      P_ActRangeAttack, DDF_StateGetAttack},
	{"SPARE ATTACK",      P_ActSpareAttack, DDF_StateGetAttack},
	{"RANGEATTEMPTSND",   P_ActMakeRangeAttemptSound, NULL},
	{"REFIRE CHECK",      P_ActRefireCheck, NULL},
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

	// miscellaneous actions
	{"NOTHING", NULL, NULL},

	// bossbrain actions
	{"BRAINSPIT", A_BrainSpit, NULL},
	{"CUBESPAWN", A_CubeSpawn, NULL},
	{"CUBETRACER", P_ActHomeToSpot, NULL},
	{"BRAINSCREAM", A_BrainScream, NULL},
	{"BRAINMISSILEEXPLODE", A_BrainMissileExplode, NULL},
	{"BRAINDIE", A_BrainDie, NULL},

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
	{"BLUESKULL",   KF_BlueSkull,   0},
	{"YELLOWSKULL", KF_YellowSkull, 0},
	{"REDSKULL",    KF_RedSkull,    0},
	{"GREENCARD",   KF_GreenCard,   0},
	{"GREENSKULL",  KF_GreenSkull,  0},

	{"GOLD KEY",    KF_GoldKey,     0},
	{"SILVER KEY",  KF_SilverKey,   0},
	{"BRASS KEY",   KF_BrassKey,    0},
	{"COPPER KEY",  KF_CopperKey,   0},
	{"STEEL KEY",   KF_SteelKey,    0},
	{"WOODEN KEY",  KF_WoodenKey,   0},
	{"FIRE KEY",    KF_FireKey,     0},
	{"WATER KEY",   KF_WaterKey,    0},

	// backwards compatibility
	{"KEY BLUECARD",    KF_BlueCard,    0},
	{"KEY YELLOWCARD",  KF_YellowCard,  0},
	{"KEY REDCARD",     KF_RedCard,     0},
	{"KEY BLUESKULL",   KF_BlueSkull,   0},
	{"KEY YELLOWSKULL", KF_YellowSkull, 0},
	{"KEY REDSKULL",    KF_RedSkull,    0},
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

	char labname[68];

	if (strncasecmp(field, "STATES(", 7) != 0)
		return false;

	// extract label name
	field += 7;

	pos = strchr(field, ')');

	if (pos == NULL || pos == field || pos > (field+64))
		return false;

	Z_StrNCpy(labname, field, pos - field);

	// check for the "standard" states
	starter = NULL;

	for (i=0; thing_starters[i].label; i++)
		if (DDF_CompareName(thing_starters[i].label, labname) == 0)
			break;

	if (thing_starters[i].label)
		starter = &thing_starters[i];

	DDF_StateReadState(contents, labname,
		&buffer_mobj.first_state, &buffer_mobj.last_state,
		starter ? starter->state_num : NULL, index, 
		is_last ? starter ? starter->last_redir : "IDLE" : NULL, 
		thing_actions);

	return true;
}


//
//  DDF PARSE ROUTINES
//

static bool ThingStartEntry(const char *buffer)
{
	int i;
	bool replaces = false;

	char namebuf[200];
	int number = 0;
	char *pos = strchr(buffer, ':');

	if (pos)
	{
		Z_StrNCpy(namebuf, buffer, pos - buffer);
		number = MAX(0, atoi(pos+1));
	}
	else
	{
		strcpy(namebuf, buffer);
	}

	if (namebuf[0])
	{
		for (i=num_disabled_mobjinfo; i < num_mobjinfo; i++)
		{
			if (DDF_CompareName(mobjinfo[i]->ddf.name, namebuf) == 0)
			{
				dynamic_mobj = mobjinfo[i];
				replaces = true;
				break;
			}
		}

		// if found, adjust pointer array to keep newest entries at end
		if (replaces && i < (num_mobjinfo-1))
		{
			Z_MoveData(mobjinfo + i, mobjinfo + i + 1, mobjinfo_t *,
				num_mobjinfo - i);
			mobjinfo[num_mobjinfo - 1] = dynamic_mobj;
		}
	}

	// not found, create a new one
	if (! replaces)
	{
		Z_SetArraySize(&mobjinfo_a, ++num_mobjinfo);

		dynamic_mobj = mobjinfo[num_mobjinfo - 1];
		dynamic_mobj->ddf.name = (namebuf[0]) ? Z_StrDup(namebuf) :
		DDF_MainCreateUniqueName("UNNAMED_THING", num_mobjinfo);
	}

	dynamic_mobj->ddf.number = number;

	// instantiate the static entry
	buffer_mobj = template_mobj;

	return replaces;
}

void ThingParseField(const char *field, const char *contents,
					 int index, bool is_last)
{
#if (DEBUG_DDF)  
	L_WriteDebug("THING_PARSE: %s = %s;\n", field, contents);
#endif

	if (DDF_MainParseField(thing_commands, field, contents))
		return;

	if (ThingTryParseState(field, contents, index, is_last))
		return;

	// handle properties
	if (index == 0 && DDF_CompareName(contents, "TRUE") == 0)
	{
		DDF_MobjGetSpecial(field, NULL);  // FIXME FOR OFFSETS
		return;
	}

	DDF_WarnError("Unknown thing/attack command: %s\n", field);
}

static void ThingFinishEntry(void)
{
	ddf_base_t base;

	if (buffer_mobj.first_state)
		DDF_StateFinishStates(buffer_mobj.first_state, buffer_mobj.last_state);

	// count-as-kill things are automatically monsters
	if (buffer_mobj.flags & MF_COUNTKILL)
		buffer_mobj.extendedflags |= EF_MONSTER;

	// check stuff...

	if (buffer_mobj.mass < 1)
	{
		DDF_WarnError("Bad MASS value %f in DDF.\n", buffer_mobj.mass);
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
	if (buffer_mobj.damage.nominal < 0)
	{
		DDF_WarnError("Bad DAMAGE.VAL value %f in DDF.\n", buffer_mobj.damage.nominal);
	}

	if (buffer_mobj.choke_damage.nominal < 0)
	{
		DDF_WarnError("Bad CHOKE_DAMAGE.VAL value %f in DDF.\n",
			buffer_mobj.choke_damage.nominal);
	}

	// FIXME: check more stuff

	if (!buffer_mobj.idle_state && buffer_mobj.spawn_state)
		buffer_mobj.idle_state = buffer_mobj.spawn_state;

	// transfer static entry to dynamic entry

	base = dynamic_mobj->ddf;
	dynamic_mobj[0] = buffer_mobj;
	dynamic_mobj->ddf = base;

	// compute CRC...
	CRC32_Init(&dynamic_mobj->ddf.crc);

	// FIXME: add more stuff...

	CRC32_Done(&dynamic_mobj->ddf.crc);
}

static void ThingClearAll(void)
{
	// not safe to delete the thing entries

	num_disabled_mobjinfo = num_mobjinfo;
}


void DDF_ReadThings(void *data, int size)
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

	DDF_MainReadFile(&things);
}

void DDF_MobjInit(void)
{
	Z_InitStackArray(&mobjinfo_a, (void ***)&mobjinfo, sizeof(mobjinfo_t), 0);

	// clear lookup cache
	memset(mobj_lookup_cache, 0, sizeof(mobj_lookup_cache));
}

void DDF_MobjCleanUp(void)
{
	int i;
	mobjinfo_t *mo;

	// lookup references

	for (i=num_disabled_mobjinfo; i < num_mobjinfo; i++)
	{
		mo = mobjinfo[i];

		DDF_ErrorSetEntryName("[%s]  (things.ddf)", mo->ddf.name);

		mo->dropitem = mo->dropitem_ref ? DDF_MobjLookup(mo->dropitem_ref) : NULL;

		mo->blood = mo->blood_ref ? DDF_MobjLookup(mo->blood_ref) :
		DDF_MobjLookup("BLOOD");

		mo->respawneffect = mo->respawneffect_ref ? 
			DDF_MobjLookup(mo->respawneffect_ref) :
		(mo->flags & MF_SPECIAL) ? 
			DDF_MobjLookup("ITEM RESPAWN") :
		DDF_MobjLookup("RESPAWN FLASH");

		mo->spitspot = mo->spitspot_ref ? DDF_MobjLookup(mo->spitspot_ref) : NULL;

		// -AJA- 1999/08/07: New SCALE & ASPECT fields.
		//       The parser placed ASPECT in xscale and SCALE in yscale.
		//       Now clean it up.
		mo->xscale *= mo->yscale;

		DDF_ErrorClearEntryName();
	}
}

//
// DDF_MobjLookup
//
// Looks an mobjinfo by name, returns a fatal error if it does not exist.
//
const mobjinfo_t *DDF_MobjLookup(const char *refname)
{
	int i;
	int downto;

	// special rule for internal names (beginning with `_'), to allow
	// savegame files to find attack MOBJs that may have been masked by
	// a #CLEARALL.

	downto = (refname[0] == '_') ? 0 : num_disabled_mobjinfo;

	// NOTE: we go backwards, so that later entries take priority
	for (i=num_mobjinfo-1; i >= downto; i--)
	{
		if (mobjinfo[i]->ddf.name && 
			DDF_CompareName(mobjinfo[i]->ddf.name, refname) == 0)
			return mobjinfo[i];
	}

	if (lax_errors)
		return mobjinfo[0];

	DDF_Error("Unknown thing type: %s\n", refname);
	return NULL;
}

const mobjinfo_t *DDF_MobjLookupNum(const int number)
{
	int slot = DDF_MobjHashFunc(number);
	int i;

	// check the cache
	if (mobj_lookup_cache[slot] &&
		mobj_lookup_cache[slot]->ddf.number == number)
	{
		return mobj_lookup_cache[slot];
	}

	// find mobj by number
	// NOTE: go backwards, so newer entries are found first
	for (i=num_mobjinfo-1; i >= num_disabled_mobjinfo; i--)
	{
		if (mobjinfo[i]->ddf.number == number)
			break;
	}

	if (i < num_disabled_mobjinfo)
		return NULL;

	// do a sprite check (like for weapons)
	if (! DDF_CheckSprites(mobjinfo[i]->first_state, mobjinfo[i]->last_state))
		return NULL;

	// update the cache
	mobj_lookup_cache[slot] = mobjinfo[i];

	return mobjinfo[i];
}

//
// DDF_MobjLookupCast
//
const mobjinfo_t *DDF_MobjLookupCast(int castnum)
{
	int i;

	// search for a matching cast
	for (i=num_mobjinfo-1; i >= num_disabled_mobjinfo; i--)
	{
		if (mobjinfo[i]->castorder == castnum)
			return mobjinfo[i];
	}

	// not found, try castnumber #1

	if (castnum == 1)
		I_Error("Must have thing with castorder of 1 !\n");

	return DDF_MobjLookupCast(1);
}

//
// ParseBenefitString
//
// Parses a string like "HEALTH(20:100)".  Returns the number of
// number parameters (0, 1 or 2).  Causes an error if it cannot parse
// the string.
//
static int ParseBenefitString(const char *info, char *name,
							  float *value, float *limit)
{
	int len = strlen(info);

	char *pos = strchr(info, '(');
	char buffer[200];

	// do we have matched parentheses ?
	if (pos && len >= 4 && info[len - 1] == ')')
	{
		int len2 = (pos - info);

		Z_StrNCpy(name, info, len2);

		len -= len2 + 2;
		Z_StrNCpy(buffer, pos + 1, len);

		switch (sscanf(buffer, " %f : %f ", value, limit))
		{
			case 1: return 1;
			case 2: return 2;

			default:
				DDF_WarnError("Missing values in benefit string: %s\n", info);
				return -1;
		}
	}
	else if (pos)
	{
		DDF_WarnError("Malformed benefit string: %s\n", info);
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
		&be->subtype, false, false))
	{
		return false;
	}

	be->type = BENEFIT_Ammo;
	be->limit = 0;

	if ((ammotype_e)be->subtype == AM_NoAmmo)
	{
		DDF_WarnError("Illegal ammo benefit: %s\n", name);
		return false;
	}

	if (num_vals < 1)
	{
		DDF_WarnError("Ammo benefit used, but amount is missing.\n");
		return false;
	}

	if (num_vals > 1)
	{
		DDF_WarnError("Ammo benefit cannot have a limit value.\n");
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
		&be->subtype, false, false))
	{
		return false;
	}

	be->type = BENEFIT_AmmoLimit;
	be->limit = 0;

	if (be->subtype == AM_NoAmmo)
	{
		DDF_WarnError("Illegal ammolimit benefit: %s\n", name);
		return false;
	}

	if (num_vals < 1)
	{
		DDF_WarnError("AmmoLimit benefit used, but amount is missing.\n");
		return false;
	}

	if (num_vals > 1)
	{
		DDF_WarnError("AmmoLimit benefit cannot have a limit value.\n");
		return false;
	}

	return true;
}

static bool BenefitTryWeapon(const char *name, benefit_t *be,
								  int num_vals)
{
	be->subtype = DDF_WeaponLookup(name);

	if (be->subtype < 0)
		return false;

	be->type = BENEFIT_Weapon;
	be->limit = 1.0f;

	if (num_vals < 1)
		be->amount = 1.0f;
	else if (be->amount != 0.0f && be->amount != 1.0f)
	{
		DDF_WarnError("Weapon benefit used, bad amount value: %1.1f\n", be->amount);
		return false;
	}

	if (num_vals > 1)
	{
		DDF_WarnError("Weapon benefit cannot have a limit value.\n");
		return false;
	}

	return true;
}

static bool BenefitTryKey(const char *name, benefit_t *be,
							   int num_vals)
{
	if (CHKF_Positive != DDF_MainCheckSpecialFlag(name, keytype_names, 
		&be->subtype, false, false))
	{
		return false;
	}

	be->type = BENEFIT_Key;
	be->limit = 1.0f;

	if (num_vals < 1)
		be->amount = 1.0f;
	else if (be->amount != 0.0f && be->amount != 1.0f)
	{
		DDF_WarnError("Key benefit used, bad amount value: %1.1f\n", be->amount);
		return false;
	}

	if (num_vals > 1)
	{
		DDF_WarnError("Key benefit cannot have a limit value.\n");
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
	be->subtype = 0;

	if (num_vals < 1)
	{
		DDF_WarnError("Health benefit used, but amount is missing.\n");
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
		&be->subtype, false, false))
	{
		return false;
	}

	be->type = BENEFIT_Armour;

	if (num_vals < 1)
	{
		DDF_WarnError("Armour benefit used, but amount is missing.\n");
		return false;
	}

	if (num_vals < 2)
	{
		switch (be->subtype)
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
		&be->subtype, false, false))
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
		if (cur->type == source->type && cur->subtype == source->subtype)
		{
			cur->amount = source->amount;
			cur->limit  = source->limit;
			return;
		}
	}

	// nope, create a new one and link it onto the _TAIL_
	cur = Z_New(benefit_t, 1);

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
	int num_vals;

	benefit_t temp;

	DEV_ASSERT2(storage);

	num_vals = ParseBenefitString(info, namebuf, &temp.amount, &temp.limit);

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

	DDF_WarnError("Unknown/Malformed benefit type: %s\n", namebuf);
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
	{"BOSSMAN", EF_BOSSMAN, 0},
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

	switch (DDF_MainCheckSpecialFlag(info, normal_specials,
		&flag_value, true, false))
	{
	case CHKF_Positive:
		buffer_mobj.flags |= flag_value;
		break;

	case CHKF_Negative:
		buffer_mobj.flags &= ~flag_value;
		break;

	case CHKF_User:
	case CHKF_Unknown:

		// wasn't a normal special.  Try the extended ones...

		switch (DDF_MainCheckSpecialFlag(info, extended_specials,
			&flag_value, true, false))
		{
		case CHKF_Positive:
			buffer_mobj.extendedflags |= flag_value;
			break;

		case CHKF_Negative:
			buffer_mobj.extendedflags &= ~flag_value;
			break;

		case CHKF_User:
		case CHKF_Unknown:
			DDF_WarnError("DDF_MobjGetSpecial: Unknown special '%s'", info);
			break;
		}
		break;
	}
}

static specflags_t dlight_type_names[] =
{
	{"NONE",      DLITE_None,      0},
	{"CONSTANT",  DLITE_Constant,  0},
	{"LINEAR",    DLITE_Linear,    0},
	{"QUADRATIC", DLITE_Quadratic, 0},
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
		DDF_WarnError("Unknown dlight type '%s'", info);
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
mobjinfo_t *DDF_MobjMakeAttackObj(mobjinfo_t *info, const char *atk_name)
{
	char namebuf[400];

	mobjinfo_t *result;

	sprintf(namebuf, "__ATKMOBJ_%s", atk_name);

	// add it to array
	Z_SetArraySize(&mobjinfo_a, ++num_mobjinfo);

	result = mobjinfo[num_mobjinfo-1];

	result[0] = info[0];

	result->ddf.name = Z_StrDup(namebuf);
	result->ddf.number = 0;
	result->ddf.crc = 0;

	return result;
}

const mobjinfo_t *DDF_MobjLookupPlayer(int playernum)
{
	int i;

	for (i=num_mobjinfo-1; i >= num_disabled_mobjinfo; i--)
		if (mobjinfo[i]->playernum == playernum)
			return mobjinfo[i];

	I_Error("Player type %d%s missing in DDF !", playernum+1,
		(playernum <= -2) ? " (DEATHBOT)" : "");
	return NULL;
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
		&cond->subtype, false, false))
	{
		return false;
	}

	if ((ammotype_e)cond->subtype == AM_NoAmmo)
	{
		DDF_WarnError("Illegal ammo in condition: %s\n", name);
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
	cond->subtype = DDF_WeaponLookup(name);

	if (cond->subtype < 0)
		return false;

	cond->cond_type = COND_Weapon;
	return true;
}

static bool ConditionTryKey(const char *name, const char *sub,
								 condition_check_t *cond)
{
	if (CHKF_Positive != DDF_MainCheckSpecialFlag(name, keytype_names, 
		&cond->subtype, false, false))
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
		cond->subtype = ARMOUR_Total;
	}
	else if (CHKF_Positive != DDF_MainCheckSpecialFlag(name, armourtype_names, 
		&cond->subtype, false, false))
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
		&cond->subtype, false, false))
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
	cond->subtype = 0;
	cond->amount = 1;

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
		DDF_WarnError("Malformed condition string: %s\n", info);
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

	DDF_WarnError("Unknown/Malformed condition type: %s\n", typebuf);
	return false;
}

