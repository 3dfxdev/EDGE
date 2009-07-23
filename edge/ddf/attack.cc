//----------------------------------------------------------------------------
//  EDGE Data Definition File Code (Attack Types)
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
// Attacks Setup and Parser Code
//
// 1998/10/29 -KM- Finalisation of sound code.  SmartProjectile.
//

#include "local.h"

#include "thing.h"
#include "states.h"
#include "attack.h"


#undef  DF
#define DF  DDF_CMD

static atkdef_c buffer_atk;
static atkdef_c *dynamic_atk;

// this (and buffer_mobj) logically belongs with buffer_atk:
static bool attack_has_mobj;

// kludge for backwards compatibility
#define AF_DAMAGE_KLUDGE  (1 << 24)

atkdef_container_c atkdefs;

static void DDF_AtkGetType(const char *info, void *storage);
static void DDF_AtkGetSpecial(const char *info, void *storage);
static void DDF_AtkGetLabel(const char *info, void *storage);
static void DDF_AtkGetOldDamRange(const char *info, void *storage);
static void DDF_AtkGetOldDamMulti(const char *info, void *storage);

damage_c buffer_damage;

#undef  DDF_CMD_BASE
#define DDF_CMD_BASE  buffer_damage

const commandlist_t damage_commands[] =
{
	DF("VAL", nominal,    DDF_MainGetFloat),
	DF("MAX", linear_max, DDF_MainGetFloat),
	DF("ERROR", error, DDF_MainGetFloat),
	DF("DELAY", delay, DDF_MainGetTime),

	DF("OBITUARY", obituary, DDF_MainGetString),
	DF("PAIN_STATE", pain, DDF_AtkGetLabel),
	DF("DEATH_STATE", death, DDF_AtkGetLabel),
	DF("OVERKILL_STATE", overkill, DDF_AtkGetLabel),

	DDF_CMD_END
};

// -KM- 1998/09/27 Major changes to sound handling
// -KM- 1998/11/25 Accuracy + Translucency are now fraction.  Added a spare attack for BFG.
// -KM- 1999/01/29 Merged in thing commands, so there is one list of
//  thing commands for all types of things (scenery, items, creatures + projectiles)

#undef  DDF_CMD_BASE
#define DDF_CMD_BASE  buffer_atk

static const commandlist_t attack_commands[] =
{
	// sub-commands
	DDF_SUB_LIST("DAMAGE", damage, damage_commands, buffer_damage),

	DF("ATTACKTYPE", attackstyle, DDF_AtkGetType),
	DF("ATTACK_SPECIAL", flags, DDF_AtkGetSpecial),
	DF("ACCURACY_SLOPE", accuracy_slope, DDF_MainGetSlope),
	DF("ACCURACY_ANGLE", accuracy_angle, DDF_MainGetAngle),
	DF("ATTACK_HEIGHT", height, DDF_MainGetFloat),
	DF("SHOTCOUNT", count, DDF_MainGetNumeric),
	DF("X_OFFSET", xoffset, DDF_MainGetFloat),
	DF("Y_OFFSET", yoffset, DDF_MainGetFloat),
	DF("ANGLE_OFFSET", angle_offset, DDF_MainGetAngle),
	DF("SLOPE_OFFSET", slope_offset, DDF_MainGetSlope),
	DF("ATTACKRANGE", range, DDF_MainGetFloat),
	DF("TOO_CLOSE_RANGE", tooclose, DDF_MainGetNumeric),
	DF("BERSERK_MULTIPLY", berserk_mul, DDF_MainGetFloat),
	DF("NO_TRACE_CHANCE", notracechance, DDF_MainGetPercent),
	DF("KEEP_FIRING_CHANCE", keepfirechance, DDF_MainGetPercent),
	DF("TRACE_ANGLE", trace_angle, DDF_MainGetAngle),
	DF("ASSAULT_SPEED", assault_speed, DDF_MainGetFloat),
	DF("ATTEMPT_SOUND", initsound, DDF_MainLookupSound),
	DF("ENGAGED_SOUND", sound, DDF_MainLookupSound),
	DF("SPAWNED_OBJECT", spawnedobj_ref, DDF_MainGetString),
	DF("SPAWN_OBJECT_STATE", objinitstate_ref, DDF_MainGetString),
	DF("SPAWN_LIMIT", spawn_limit, DDF_MainGetNumeric),
	DF("PUFF", puff_ref, DDF_MainGetString),
	DF("ATTACK_CLASS", attack_class, DDF_MainGetBitSet),

	// -AJA- backward compatibility cruft...
	DF("!DAMAGE", damage.nominal, DDF_MainGetFloat),
	DF("!DAMAGE_RANGE", name, DDF_AtkGetOldDamRange),
	DF("!DAMAGE_MULTI", name, DDF_AtkGetOldDamMulti),

	DDF_CMD_END
};


//
//  DDF PARSE ROUTINES
//

static void AttackStartEntry(const char *name)
{
	atkdef_c *existing = NULL;

	if (!name || !name[0])
	{
		DDF_WarnError("New attack entry is missing a name!");
		name = "!NO_NAME_ATTACK!";
	}

	existing = atkdefs.Lookup(name);

	// not found, create a new one
	if (existing)
	{
		dynamic_atk = existing;
	}
	else
	{
		dynamic_atk = new atkdef_c;

		dynamic_atk->name = name;

		atkdefs.Insert(dynamic_atk);
	}

	attack_has_mobj = false;

	// instantiate the static entries
	buffer_atk.Default();

	buffer_mobj.states.clear();
	buffer_mobj.Default();
}

static void AttackParseField(const char *field, const char *contents,
							 int index, bool is_last)
{
#if (DEBUG_DDF)  
	I_Debugf("ATTACK_PARSE: %s = %s;\n", field, contents);
#endif

	// first, check attack commands
	if (DDF_MainParseField(attack_commands, field, contents))
		return;

	// we need to create an MOBJ for this attack
	attack_has_mobj = true;

	ThingParseField(field, contents, index, is_last);
}

static void AttackFinishEntry(void)
{
	// check DAMAGE stuff
	if (buffer_atk.damage.nominal < 0)
	{
		DDF_WarnError2(128, "Bad DAMAGE.VAL value %f in DDF.\n", buffer_atk.damage.nominal);
	}

	// FIXME: check more stuff...

	// handle attacks that have mobjs
	if (attack_has_mobj)
	{
		DDF_StateFinishStates(buffer_mobj.states); 

		// check MOBJ stuff

		if (buffer_mobj.explode_damage.nominal < 0)
		{
			DDF_WarnError2(131, "Bad EXPLODE_DAMAGE.VAL value %f in DDF.\n",
				buffer_mobj.explode_damage.nominal);
		}

		if (buffer_mobj.explode_radius < 0)
		{
			DDF_WarnError2(131, "Bad EXPLODE_RADIUS value %f in DDF.\n",
				buffer_mobj.explode_radius);
		}

		if (buffer_mobj.model_skin < 0 || buffer_mobj.model_skin > 9)
			DDF_Error("Bad MODEL_SKIN value %d in DDF (must be 0-9).\n",
				buffer_mobj.model_skin);

		if (buffer_mobj.dlight[0].radius > 512)
			DDF_Warning("DLIGHT RADIUS value %1.1f too large (over 512).\n",
				buffer_mobj.dlight[0].radius);

		buffer_atk.atk_mobj = DDF_MobjMakeAttackObj(&buffer_mobj,
											dynamic_atk->name.c_str());
	}
	else
		buffer_atk.atk_mobj = NULL;

	// compute an attack class, if none specified
	if (buffer_atk.attack_class == BITSET_EMPTY)
	{
		buffer_atk.attack_class = attack_has_mobj ? BITSET_MAKE('M') : 
			(buffer_atk.attackstyle == ATK_CLOSECOMBAT ||
			 buffer_atk.attackstyle == ATK_SKULLFLY) ? 
			BITSET_MAKE('C') : BITSET_MAKE('B');
	}

	// -AJA- 2009: Backwards compatibility
	if (buffer_atk.flags & AF_DAMAGE_KLUDGE)
	{
		if (buffer_atk.damage.linear_max < 1.0f)
			buffer_atk.damage.linear_max = 1.0f;

		buffer_atk.damage.linear_max *= buffer_atk.damage.nominal;
		buffer_atk.flags = (attackflags_e)(buffer_atk.flags & ~AF_DAMAGE_KLUDGE);
	}

	// -AJA- 2005/08/06: Berserk backwards compatibility
	if (DDF_CompareName(dynamic_atk->name.c_str(), "PLAYER_PUNCH") == 0
		&& buffer_atk.berserk_mul == 1.0f)
	{
		buffer_atk.berserk_mul = 10.0f;
	}

	// transfer static entry to dynamic entry
  
	dynamic_atk->CopyDetail(buffer_atk);

	// FIXME!! Compute the CRC value
}

static void AttackClearAll(void)
{
return; //!!!! -AJA- FIXME: temp hack to get castle.wad working
	// not safe to delete attacks -- mark as disabled
	atkdefs.SetDisabledCount(atkdefs.GetSize());
}


readinfo_t attack_readinfo =
{
	"DDF_InitAttacks",  // message
	"ATTACKS",  // tag

	AttackStartEntry,
	AttackParseField,
	AttackFinishEntry,
	AttackClearAll
};


void DDF_AttackInit(void)
{
	atkdefs.Clear();
}

void DDF_AttackCleanUp(void)
{
	epi::array_iterator_c it;
	atkdef_c *a;

	for (it = atkdefs.GetIterator(atkdefs.GetDisabledCount()); it.IsValid(); it++)
	{
		a = ITERATOR_TO_TYPE(it, atkdef_c*);

		cur_ddf_entryname = epi::STR_Format("[%s]  (attacks.ddf)", a->name.c_str());

		// lookup thing references

		a->puff = a->puff_ref.empty() ?  NULL :
			mobjtypes.Lookup(a->puff_ref.c_str());

		a->spawnedobj = a->spawnedobj_ref.empty() ?  NULL :
			mobjtypes.Lookup(a->spawnedobj_ref.c_str());
      
		if (a->spawnedobj)
		{
			if (a->objinitstate_ref.empty())
				a->objinitstate = a->spawnedobj->spawn_state;
			else
				a->objinitstate = DDF_MainLookupDirector(a->spawnedobj, a->objinitstate_ref.c_str());
		}

		cur_ddf_entryname.clear();
	}

	atkdefs.Trim();
}

static const specflags_t attack_specials[] =
{
    {"SMOKING_TRACER", AF_TraceSmoke, 0},
    {"KILL_FAILED_SPAWN", AF_KillFailedSpawn, 0},
    {"REMOVE_FAILED_SPAWN", AF_KillFailedSpawn, 1},
    {"PRESTEP_SPAWN", AF_PrestepSpawn, 0},
    {"SPAWN_TELEFRAGS", AF_SpawnTelefrags, 0},
    {"NEED_SIGHT", AF_NeedSight, 0},
    {"FACE_TARGET", AF_FaceTarget, 0},

    {"FORCE_AIM", AF_ForceAim, 0},
    {"ANGLED_SPAWN", AF_AngledSpawn, 0},
    {"PLAYER_ATTACK", AF_Player, 0},
    {"TRIGGER_LINES", AF_NoTriggerLines, 1},
    {"SILENT_TO_MONSTERS", AF_SilentToMon, 0},
    {"TARGET", AF_NoTarget, 1},
    {"VAMPIRE", AF_Vampire, 0},

    // -AJA- backwards compatibility cruft...
    {"!NOAMMO", AF_None, 0},

    {NULL, AF_None, 0}
};

static void DDF_AtkGetSpecial(const char *info, void *storage)
{
	attackflags_e *flags = (attackflags_e *)storage;

	int value;

	switch (DDF_MainCheckSpecialFlag(info, attack_specials, &value, true))
	{
		case CHKF_Positive:
			*flags = (attackflags_e)(*flags | value);
			break;
    
		case CHKF_Negative:
			*flags = (attackflags_e)(*flags & ~value);
			break;

		default:
			DDF_WarnError2(128, "DDF_AtkGetSpecials: Unknown Attack Special: %s\n", info);
			break;
	}
}

// -KM- 1998/11/25 Added new attack type for BFG: Spray
static const char *attack_class[NUMATKCLASS] =
{
    "NONE",
    "PROJECTILE",
    "SPAWNER",
    "TRIPLE_SPAWNER",
    "FIXED_SPREADER",
    "RANDOM_SPREADER",
    "SHOT",
    "TRACKER",
    "CLOSECOMBAT",
    "SHOOTTOSPOT",
    "SKULLFLY",
    "SMARTPROJECTILE",
    "SPRAY"
};

static void DDF_AtkGetType(const char *info, void *storage)
{
	attackstyle_e *atkst = (attackstyle_e *)storage;

	int i = 0;

	while (i != NUMATKCLASS && DDF_CompareName(info, attack_class[i]) != 0)
		i++;

	if (i == NUMATKCLASS)
	{
		DDF_WarnError2(128, "DDF_AtkGetType: No such attack type '%s'\n", info);
		*atkst = ATK_SHOT;
		return;
	}
  
	*atkst = (attackstyle_e)i;
}

static void DDF_AtkGetLabel(const char *info, void *storage)
{
	label_offset_c *lab = (label_offset_c *)storage;

	// check for `:' in string
	const char *div = strchr(info, ':');

	int i = div ? (div - info) : (int)strlen(info);

	if (i <= 0)
		DDF_Error("Bad State `%s'.\n", info);

	lab->label.Set(info, i);
	lab->offset = div ? MAX(0, atoi(div+1) - 1) : 0;
}

static void DDF_AtkGetOldDamRange(const char *info, void *storage)
{
	atkdef_c *atk = (atkdef_c *) storage;

	atk->flags = (attackflags_e)(atk->flags | AF_DAMAGE_KLUDGE);
	atk->damage.nominal = atof(info);
}

static void DDF_AtkGetOldDamMulti(const char *info, void *storage)
{
	atkdef_c *atk = (atkdef_c *) storage;

	atk->flags = (attackflags_e)(atk->flags | AF_DAMAGE_KLUDGE);
	atk->damage.linear_max = atof(info);
}


// Attack definition class --->

atkdef_c::atkdef_c() : name()
{
	Default();
}

atkdef_c::~atkdef_c()
{
}


void atkdef_c::Default()
{
	atk_mobj = NULL;

	attackstyle = ATK_NONE;
	flags = AF_None;
	initsound = NULL;
	sound = NULL;
	accuracy_slope = 0;
	accuracy_angle = 0;
	xoffset = 0;
	yoffset = 0;
	angle_offset = 0;
	slope_offset = 0;
	trace_angle = (ANG270 / 16);
	assault_speed = 0;
	height = 0;
	range = 0;
	count = 0;
	tooclose = 0;
	berserk_mul = 1.0f;

	damage.Default(damage_c::DEFAULT_Attack);

	attack_class = BITSET_EMPTY; 
	objinitstate = 0;
	objinitstate_ref.clear();
	notracechance = PERCENT_MAKE(0); 
	keepfirechance = PERCENT_MAKE(0);
	spawnedobj = NULL;
	spawnedobj_ref.clear();
	spawn_limit = 0;  // unlimited
	puff = NULL;
	puff_ref.clear();
}

void atkdef_c::CopyDetail(atkdef_c &src)
{
	atk_mobj = src.atk_mobj; //FIXME !!!!!! PROBABLY NOT RIGHT !

	attackstyle = src.attackstyle;
	flags = src.flags;
	initsound = src.initsound;
	sound = src.sound;
	accuracy_slope = src.accuracy_slope;
	accuracy_angle = src.accuracy_angle;
	xoffset = src.xoffset;
	yoffset = src.yoffset;
	angle_offset = src.angle_offset;
	slope_offset = src.slope_offset;
	trace_angle  = src.trace_angle;
	assault_speed = src.assault_speed;
	height = src.height;
	range = src.range;
	count = src.count;
	tooclose = src.tooclose;
	berserk_mul = src.berserk_mul;

	damage = src.damage;

	attack_class = src.attack_class;
	objinitstate = src.objinitstate;
	objinitstate_ref = src.objinitstate_ref;
	notracechance = src.notracechance;
	keepfirechance = src.keepfirechance;
	spawnedobj = src.spawnedobj;
	spawnedobj_ref = src.spawnedobj_ref;
	spawn_limit = src.spawn_limit;
	puff = src.puff;
	puff_ref = src.puff_ref;
}


// --> atkdef_container_c class

//
// atkdef_container_c::atkdef_container_c()
//
atkdef_container_c::atkdef_container_c() : epi::array_c(sizeof(atkdef_c*))
{
	num_disabled = 0;	
}

//
// ~atkdef_container_c::atkdef_container_c()
//
atkdef_container_c::~atkdef_container_c()
{
	Clear();					// <-- Destroy self before exiting
}

//
// atkdef_container_c::CleanupObject()
//
void atkdef_container_c::CleanupObject(void *obj)
{
	atkdef_c *a = *(atkdef_c**)obj;

	if (a)
		delete a;

	return;
}

//
// atkdef_c* atkdef_container_c::Lookup()
//
// Looks an atkdef by name, returns a fatal error if it does not exist.
//
atkdef_c* atkdef_container_c::Lookup(const char *refname)
{
	epi::array_iterator_c it;
	atkdef_c *a;

	if (!refname || !refname[0])
		return NULL;

	for (it = GetIterator(num_disabled); it.IsValid(); it++)
	{
		a = ITERATOR_TO_TYPE(it, atkdef_c*);
		if (DDF_CompareName(a->name.c_str(), refname) == 0)
			return a;
	}

	return NULL;
}


atkdef_c * DDF_AttackForMobjtype(const mobjtype_c *info)
{
	SYS_ASSERT(info);

	epi::array_iterator_c it;
	for (it = atkdefs.GetIterator(atkdefs.num_disabled); it.IsValid(); it++)
	{
		atkdef_c * a = ITERATOR_TO_TYPE(it, atkdef_c*);

		if (a->atk_mobj == info)
			return a;
	}

	return NULL;
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
