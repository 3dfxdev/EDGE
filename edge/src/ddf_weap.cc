//----------------------------------------------------------------------------
//  EDGE Data Definition File Code (Weapons)
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
// Player Weapons Setup and Parser Code
//
// -KM- 1998/11/25 File Written
//

#include "i_defs.h"

#include "ddf_locl.h"
#include "ddf_main.h"
#include "e_search.h"
#include "p_action.h"

#undef  DF
#define DF  DDF_CMD

static weapondef_c buffer_weapon;
static weapondef_c *dynamic_weapon;

weapondef_container_c weapondefs;
weaponkey_c weaponkey[WEAPON_KEYS];	// -KM- 1998/11/25 Always 10 weapon keys

static void DDF_WGetAmmo(const char *info, void *storage);
static void DDF_WGetUpgrade(const char *info, void *storage);
static void DDF_WGetSpecialFlags(const char *info, void *storage);

#undef  DDF_CMD_BASE
#define DDF_CMD_BASE  buffer_weapon

static const commandlist_t weapon_commands[] =
{
	DF("AMMOTYPE", ammo[0], DDF_WGetAmmo),
	DF("AMMOPERSHOT", ammopershot[0], DDF_MainGetNumeric),
	DF("CLIPSIZE", clip_size[0], DDF_MainGetNumeric),
	DF("AUTOMATIC", autofire[0], DDF_MainGetBoolean),
	DF("SEC AMMOTYPE", ammo[1], DDF_WGetAmmo),
	DF("SEC AMMOPERSHOT", ammopershot[1], DDF_MainGetNumeric),
	DF("SEC CLIPSIZE", clip_size[1], DDF_MainGetNumeric),
	DF("SEC AUTOMATIC", autofire[1], DDF_MainGetBoolean),
	DF("ATTACK", attack[0], DDF_MainRefAttack),
	DF("SECOND ATTACK", attack[1], DDF_MainRefAttack),
	DF("EJECT ATTACK", eject_attack, DDF_MainRefAttack),
	DF("FREE", autogive, DDF_MainGetBoolean),
	DF("BINDKEY", bind_key, DDF_MainGetNumeric),
	DF("PRIORITY", priority, DDF_MainGetNumeric),
	DF("DANGEROUS", dangerous, DDF_MainGetBoolean),
	DF("UPGRADES", upgrade_weap, DDF_WGetUpgrade),
	DF("IDLE SOUND", idle, DDF_MainLookupSound),
	DF("ENGAGED SOUND", engaged, DDF_MainLookupSound),
	DF("HIT SOUND", hit, DDF_MainLookupSound),
	DF("START SOUND", start, DDF_MainLookupSound),
	DF("NOTHRUST", nothrust, DDF_MainGetBoolean),
	DF("FEEDBACK", feedback, DDF_MainGetBoolean),
	DF("KICK", kick, DDF_MainGetFloat),
	DF("SPECIAL", specials[0], DDF_WGetSpecialFlags),
	DF("SEC SPECIAL", specials[1], DDF_WGetSpecialFlags),
	DF("ZOOM FOV", zoom_fov, DDF_MainGetAngle),
	DF("REFIRE INACCURATE", refire_inacc, DDF_MainGetBoolean),
	DF("SHOW CLIP", show_clip, DDF_MainGetBoolean),
	DF("BOBBING", bobbing, DDF_MainGetPercent),
	DF("SWAYING", swaying, DDF_MainGetPercent),
	DF("IDLE WAIT", idle_wait, DDF_MainGetTime),
	DF("IDLE CHANCE", idle_chance, DDF_MainGetPercent),

	// -AJA- backwards compatibility cruft...
	DF("!SOUND1", sound1, DDF_MainLookupSound),
	DF("!SOUND2", sound2, DDF_MainLookupSound),
	DF("!SOUND3", sound3, DDF_MainLookupSound),

	DDF_CMD_END
};

static const state_starter_t weapon_starters[] =
{
	{"UP",        "UP",     &buffer_weapon.up_state},
	{"DOWN",      "DOWN",   &buffer_weapon.down_state},
	{"READY",     "READY",  &buffer_weapon.ready_state},
	{"EMPTY",     "EMPTY",  &buffer_weapon.empty_state},
	{"IDLE",      "READY",  &buffer_weapon.idle_state},
	{"CROSSHAIR", "CROSSHAIR", &buffer_weapon.crosshair},
	{"ZOOM",      "ZOOM",   &buffer_weapon.zoom_state},

	{"ATTACK",    "READY",  &buffer_weapon.attack_state[0]},
	{"RELOAD",    "READY",  &buffer_weapon.reload_state[0]},
	{"DISCARD",   "READY",  &buffer_weapon.discard_state[0]},
	{"WARMUP",    "ATTACK", &buffer_weapon.warmup_state[0]},
	{"FLASH",     "REMOVE", &buffer_weapon.flash_state[0]},
	{"SECATTACK", "READY",  &buffer_weapon.attack_state[1]},
	{"SECRELOAD", "READY",  &buffer_weapon.reload_state[1]},
	{"SECDISCARD","READY",  &buffer_weapon.discard_state[1]},
	{"SECWARMUP", "SECATTACK", &buffer_weapon.warmup_state[1]},
	{"SECFLASH",  "REMOVE", &buffer_weapon.flash_state[1]},
	{NULL, NULL, NULL}
};

static const actioncode_t weapon_actions[] =
{
	{"NOTHING", NULL, NULL},

	{"RAISE",             A_Raise, NULL},
	{"LOWER",             A_Lower, NULL},
	{"READY",             A_WeaponReady, NULL},
	{"EMPTY",             A_WeaponEmpty, NULL},
	{"SHOOT",             A_WeaponShoot, DDF_StateGetAttack},
	{"EJECT",             A_WeaponEject, DDF_StateGetAttack},
	{"REFIRE",            A_ReFire, NULL},
	{"NOFIRE",            A_NoFire, NULL},
	{"NOFIRE RETURN",     A_NoFireReturn, NULL},
	{"KICK",              A_WeaponKick, DDF_StateGetFloat},
	{"CHECKRELOAD",       A_CheckReload, NULL},
	{"PLAYSOUND",         A_WeaponPlaySound, DDF_StateGetSound},
	{"KILLSOUND",         A_WeaponKillSound, NULL},
	{"JUMP",              A_WeaponJump, DDF_StateGetJump},
	{"RTS ENABLE TAGGED", A_WeaponEnableRadTrig,  DDF_StateGetInteger},
	{"RTS DISABLE TAGGED",A_WeaponDisableRadTrig, DDF_StateGetInteger},
	{"SEC SHOOT",         A_WeaponShootSA, DDF_StateGetAttack},
	{"SEC REFIRE",        A_ReFireSA, NULL},
	{"SEC NOFIRE",        A_NoFireSA, NULL},
	{"SEC NOFIRE RETURN", A_NoFireReturnSA, NULL},
	{"SEC CHECKRELOAD",   A_CheckReloadSA, NULL},

	// flash-related actions
	{"FLASH",             A_GunFlash, NULL},
	{"SEC FLASH",         A_GunFlashSA, NULL},
	{"LIGHT0",            A_Light0, NULL},
	{"LIGHT1",            A_Light1, NULL},
	{"LIGHT2",            A_Light2, NULL},
	{"TRANS SET",         A_WeaponTransSet,  DDF_StateGetPercent},
	{"TRANS FADE",        A_WeaponTransFade, DDF_StateGetPercent},

	// crosshair-related actions
	{"SETCROSS",          A_SetCrosshair, DDF_StateGetFrame},
	{"TARGET JUMP",       A_TargetJump, DDF_StateGetFrame},
	{"FRIEND JUMP",       A_FriendJump, DDF_StateGetFrame},

	// -AJA- backwards compatibility cruft...
	{"!SOUND1",           A_SFXWeapon1, NULL},
	{"!SOUND2",           A_SFXWeapon2, NULL},
	{"!SOUND3",           A_SFXWeapon3, NULL},
	{"!RANDOMJUMP", 	  NULL, NULL},
	{"!TARGET",           NULL, NULL},
	{NULL, NULL, NULL}
};

const specflags_t ammo_types[] =
{
    {"NOAMMO",  AM_NoAmmo, 0},

    {"BULLETS", AM_Bullet, 0},
    {"SHELLS",  AM_Shell,  0},
    {"ROCKETS", AM_Rocket, 0},
    {"CELLS",   AM_Cell,   0},

    {"PELLETS",  AM_Pellet,  0},
    {"NAILS",    AM_Nail,    0},
    {"GRENADES", AM_Grenade, 0},
    {"GAS",      AM_Gas,     0},

    {"DAGGERS",   AM_Dagger,  0},
    {"ARROWS",    AM_Arrow,   0},
    {"TORPEDOES", AM_Torpedo, 0},
    {"CRYSTALS",  AM_Crystal, 0},

    {NULL, 0, 0}
};

static bool WeaponTryParseState(const char *field, 
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

	for (i=0; weapon_starters[i].label; i++)
		if (DDF_CompareName(weapon_starters[i].label, labname.GetString()) == 0)
			break;

	if (weapon_starters[i].label)
		starter = &weapon_starters[i];

	DDF_StateReadState(contents, labname,
			&buffer_weapon.first_state, &buffer_weapon.last_state,
			starter ? starter->state_num : NULL, index, 
			is_last ? starter ? starter->last_redir : "READY" : NULL, 
			weapon_actions);

	return true;
}


//
//  DDF PARSE ROUTINES
//

static bool WeaponStartEntry(const char *name)
{
	weapondef_c *existing = NULL;

	if (name && name[0])
		existing = weapondefs.Lookup(name);

	// not found, create a new one
	if (existing)
	{
		dynamic_weapon = existing;
	}
	else
	{
		dynamic_weapon = new weapondef_c;

		if (name && name[0])
			dynamic_weapon->ddf.name.Set(name);
		else
			dynamic_weapon->ddf.SetUniqueName("UNNAMED_WEAPON", weapondefs.GetSize());

		weapondefs.Insert(dynamic_weapon);
	}

	dynamic_weapon->ddf.number = 0;

	// instantiate the static entries
	buffer_weapon.Default();

	return (existing != NULL);
}

static void WeaponParseField(const char *field, const char *contents,
    int index, bool is_last)
{
#if (DEBUG_DDF)  
	L_WriteDebug("WEAPON_PARSE: %s = %s;\n", field, contents);
#endif

	if (DDF_MainParseField(weapon_commands, field, contents))
		return;

	if (WeaponTryParseState(field, contents, index, is_last))
		return;

	if (ddf_version < 0x128)
	{
		// handle properties (old piece of crud)
		if (index == 0 && DDF_CompareName(contents, "TRUE") == 0)
		{
			L_WriteDebug("WEAPON PROPERTY CRUD: %s = %s\n", field, contents);
			DDF_WGetSpecialFlags(field, &buffer_weapon.specials[0]);
			return;
		}
	}

	DDF_WarnError2(0x128, "Unknown weapons.ddf command: %s\n", field);
}

static void WeaponFinishEntry(void)
{
	if (! buffer_weapon.first_state)
		DDF_Error("Weapon `%s' has missing states.\n",
			dynamic_weapon->ddf.name.GetString());

	DDF_StateFinishStates(buffer_weapon.first_state, buffer_weapon.last_state);

	// check stuff...
	int A;
	for (A = 0; A < 2; A++)
	{
		if (buffer_weapon.ammopershot[A] < 0)
		{
			DDF_WarnError2(0x128, "Bad %sAMMOPERSHOT value for weapon: %d\n",
					A ? "SEC_" : "", buffer_weapon.ammopershot[A]);
			buffer_weapon.ammopershot[A] = 0;
		}

		// zero values for ammopershot really mean infinite ammo
		if (buffer_weapon.ammopershot[A] == 0)
			buffer_weapon.ammo[A] = AM_NoAmmo;

		if (buffer_weapon.clip_size[A] < 0)
		{
			DDF_WarnError2(0x129, "Bad %sCLIPSIZE value for weapon: %d\n",
					A ? "SEC_" : "", buffer_weapon.clip_size[A]);
			buffer_weapon.clip_size[A] = 0;
		}

		// check if clip_size + ammopershot makes sense
		if (buffer_weapon.clip_size[A] > 0 && buffer_weapon.ammo[A] != AM_NoAmmo &&
			(buffer_weapon.clip_size[A] < buffer_weapon.ammopershot[A] ||
			 buffer_weapon.clip_size[A] % buffer_weapon.ammopershot[A] != 0))
		{
			DDF_WarnError2(0x129, "%sAMMOPERSHOT=%d incompatible with %sCLIPSIZE=%d\n",
				A ? "SEC_" : "", buffer_weapon.ammopershot[A],
				A ? "SEC_" : "", buffer_weapon.clip_size[A]);
			buffer_weapon.ammopershot[A] = 1;
		}
	}

	// backwards compatibility (REMOVE for 1.26)
	if (buffer_weapon.priority < 0)
	{
		DDF_WarnError2(0x129, "Using PRIORITY=-1 in weapons.ddf is obsolete !\n");

		buffer_weapon.dangerous = true;
		buffer_weapon.priority = 10;
	}

	// backwards compatibility
	if (ddf_version < 0x129)
		buffer_weapon.specials[1] = (weapon_flag_e)
			(buffer_weapon.specials[0] & WPSP_SilentToMon);

	// transfer static entry to dynamic entry
	dynamic_weapon->CopyDetail(buffer_weapon);

	// compute CRC...
	for (A = 0; A < 2; A++)
	{
		// FIXME: attack name
		dynamic_weapon->ddf.crc += dynamic_weapon->ammo[A];
		dynamic_weapon->ddf.crc += dynamic_weapon->ammopershot[A];
		dynamic_weapon->ddf.crc += dynamic_weapon->clip_size[A];
		dynamic_weapon->ddf.crc += dynamic_weapon->autofire[A];
		dynamic_weapon->ddf.crc += dynamic_weapon->specials[A];
	}

	// FIXME: add more stuff...
}

static void WeaponClearAll(void)
{
	// not safe to delete weapons, there are (integer) references
	weapondefs.SetDisabledCount(weapondefs.GetSize());
}


//
// DDF_ReadWeapons
//
bool DDF_ReadWeapons(void *data, int size)
{
	readinfo_t weapons;

	weapons.memfile = (char*)data;
	weapons.memsize = size;
	weapons.tag = "WEAPONS";
	weapons.entries_per_dot = 1;

	if (weapons.memfile)
	{
		weapons.message = NULL;
		weapons.filename = NULL;
		weapons.lumpname = "DDFWEAP";
	}
	else
	{
		weapons.message = "DDF_InitWeapons";
		weapons.filename = "weapons.ddf";
		weapons.lumpname = NULL;
	}

	weapons.start_entry  = WeaponStartEntry;
	weapons.parse_field  = WeaponParseField;
	weapons.finish_entry = WeaponFinishEntry;
	weapons.clear_all    = WeaponClearAll;

	return DDF_MainReadFile(&weapons);
}

void DDF_WeaponInit(void)
{
	weapondefs.Clear();
}

void DDF_WeaponCleanUp(void)
{
	// compute the weaponkey array
	epi::array_iterator_c it;
	int key;
	weapondef_container_c tmplist;
	weapondef_c *wd;
	weaponkey_c *wk;
	
	for (key=0; key < WEAPON_KEYS; key++)
	{
		wk = &weaponkey[key];
	
		// Clear the list without destroying the contents
		tmplist.ZeroiseCount();	

		for (it = weapondefs.GetIterator(weapondefs.GetDisabledCount());
			it.IsValid(); it++)
		{
			wd = ITERATOR_TO_TYPE(it, weapondef_c*);
			if (!wd)
				continue;
				
			if (wd->bind_key != key)
				continue;
				
			tmplist.Insert(wd);
		}
		
		wk->Load(&tmplist);
	
#if 1 // (DEBUG_DDF)
		L_WriteDebug("DDF_Weap: CHOICES ON KEY %d:\n", key);
		for (int i=0; i < wk->numchoices; i++)
		{
			L_WriteDebug("  %p [%s] pri=%d\n", wk->choices[i], wk->choices[i]->ddf.name.GetString(),
					wk->choices[i]->priority);
		}
#endif

		if (wk->numchoices < 2)
			continue;

		// sort choices based on weapon priority
#define CMP(a, b)  ((a)->priority < (b)->priority)
		QSORT(weapondef_c *, wk->choices, wk->numchoices, CUTOFF);
#undef CMP
	}
	
	//
	// Clear the list without destroying the contents
	// otherwise it'll destroy the weapondefs still
	// in the tmplist on exit.
	//
	tmplist.ZeroiseCount();	
	
	// Trim down the required to size
	weapondefs.Trim();
}

static void DDF_WGetAmmo(const char *info, void *storage)
{
	int *ammo = (int *)storage;
	int flag_value;

	switch (DDF_MainCheckSpecialFlag(info, ammo_types, &flag_value,
				false, false))
	{
		case CHKF_Positive:
		case CHKF_Negative:
			(*ammo) = flag_value;
			break;

		case CHKF_User:
		case CHKF_Unknown:
			DDF_WarnError2(0x128, "Unknown Ammo type '%s'\n", info);
			break;
	}
}


static void DDF_WGetUpgrade(const char *info, void *storage)
{
	weapondef_c **dest = (weapondef_c **)storage;

	*dest = weapondefs.Lookup(info);
}

static specflags_t weapon_specials[] =
{
    {"SILENT TO MONSTERS", WPSP_SilentToMon, 0},
    {"ANIMATED",  WPSP_Animated, 0},
    {"SWITCH",  WPSP_SwitchAway, 0},
	{"TRIGGER", WPSP_Trigger, 0},
	{"FRESH",   WPSP_Fresh, 0},
	{"MANUAL",  WPSP_Manual, 0},
	{"PARTIAL", WPSP_Partial, 0},
    {NULL, WPSP_None, 0}
};

//
// DDF_WGetSpecialFlags
//
static void DDF_WGetSpecialFlags(const char *info, void *storage)
{
	int flag_value;

	weapon_flag_e *dest = (weapon_flag_e *) storage;

	switch (DDF_MainCheckSpecialFlag(info, weapon_specials, &flag_value,
				true, false))
	{
		case CHKF_Positive:
			*dest = (weapon_flag_e)(*dest | flag_value);
			break;

		case CHKF_Negative:
			*dest = (weapon_flag_e)(*dest & ~flag_value);
			break;

		case CHKF_User:
		case CHKF_Unknown:
			DDF_WarnError2(0x128, "DDF_WGetSpecialFlags: Unknown Special: %s", info);
			return;
	}

	if (ddf_version < 0x129 && (flag_value & 0xFFFE) != 0)
		DDF_Error("New weapon specials require #VERSION 1.29 or later.\n");
}

//
// DDF_WeaponIsUpgrade
//
// Checks whether first weapon is an upgrade of second one,
// including indirectly (e.g. an upgrade of an upgrade).
//
bool DDF_WeaponIsUpgrade(weapondef_c *weap, weapondef_c *old)
{
	if (!weap || !old || weap == old)
		return false;
	
	for (int loop = 0; loop < 10; loop++)
	{
		if (! weap->upgrade_weap)
			return false;

		if (weap->upgrade_weap == old)
			return true;
	
		weap = weap->upgrade_weap;
	}

	return false;
}

// --> Weapon Definition

//
// weapondef_c Constructor
//
weapondef_c::weapondef_c()
{
	Default();
}

//
// weapondef_c Copy constructor
//
weapondef_c::weapondef_c(weapondef_c &rhs)
{
	Copy(rhs);
}

//
// weapondef_c Destructor
//
weapondef_c::~weapondef_c()
{
}

//
// weapondef_c::Copy()
//
void weapondef_c::Copy(weapondef_c &src)
{
	ddf = src.ddf;
	CopyDetail(src);
}

//
// weapondef_c::CopyDetail()
//
void weapondef_c::CopyDetail(weapondef_c &src)
{
	for (int ATK = 0; ATK < 2; ATK++)
	{
		attack[ATK] = src.attack[ATK];
		ammo[ATK]   = src.ammo[ATK];
		ammopershot[ATK] = src.ammopershot[ATK];
		autofire[ATK]  = src.autofire[ATK];
		clip_size[ATK] = src.clip_size[ATK];
		specials[ATK]  = src.specials[ATK];

		attack_state[ATK]  = src.attack_state[ATK];
		reload_state[ATK]  = src.reload_state[ATK];
		discard_state[ATK] = src.discard_state[ATK];
		warmup_state[ATK]  = src.warmup_state[ATK];
		flash_state[ATK]   = src.flash_state[ATK];
	}

	kick = src.kick;

	first_state = src.first_state;
	last_state  = src.last_state;

	up_state    = src.up_state;
	down_state  = src.down_state;
	ready_state = src.ready_state;
	empty_state = src.empty_state;
	idle_state  = src.idle_state;
	crosshair   = src.crosshair;
	zoom_state  = src.zoom_state;

	autogive = src.autogive;
	feedback = src.feedback;
	upgrade_weap = src.upgrade_weap;

	priority = src.priority;
	dangerous = src.dangerous;

	eject_attack = src.eject_attack;

	idle = src.idle;
	engaged = src.engaged;
	hit = src.hit;
	start = src.start;

	sound1 = src.sound1;
	sound2 = src.sound2;
	sound3 = src.sound3;

	nothrust = src.nothrust;

  	bind_key = src.bind_key;

	zoom_fov = src.zoom_fov;
	refire_inacc = src.refire_inacc;
	show_clip = src.show_clip;
	bobbing = src.bobbing;
	swaying = src.swaying;
	idle_wait = src.idle_wait;
	idle_chance = src.idle_chance;
}

//
// weapondef_c::Default()
//
void weapondef_c::Default(void)
{
	ddf.Default();

	for (int ATK = 0; ATK < 2; ATK++)
	{
		attack[ATK] = NULL;
		ammo[ATK]   = AM_NoAmmo;
		ammopershot[ATK] = 0;
		clip_size[ATK]   = 0;
		autofire[ATK]    = false;

		attack_state[ATK]  = 0;
		reload_state[ATK]  = 0;
		discard_state[ATK] = 0;
		warmup_state[ATK]  = 0;
		flash_state[ATK]   = 0;
	}

	specials[0] = DEFAULT_WPSP;
	specials[1] = (weapon_flag_e)(DEFAULT_WPSP & ~WPSP_SwitchAway);

	kick = 0.0f;

	first_state = 0;
	last_state = 0;

	up_state = 0;
	down_state= 0;
	ready_state = 0;
	empty_state = 0;
	idle_state = 0;

	crosshair = 0;
	zoom_state = 0;

	autogive = false;
	feedback = false;
	upgrade_weap = NULL;
	priority = 0;
	dangerous = false;

	eject_attack = NULL;
	idle = NULL;
	engaged = NULL;
	hit = NULL;
	start = NULL;

	sound1 = NULL;
	sound2 = NULL;
	sound3 = NULL;

	nothrust = false;
	bind_key = -1;
	zoom_fov = 0;
	refire_inacc = false;
	show_clip = false;
	bobbing = PERCENT_MAKE(100);
	swaying = PERCENT_MAKE(100);
	idle_wait = 15 * TICRATE;
	idle_chance = PERCENT_MAKE(12);
}

//
// weapondef_c assignment operator
//	
weapondef_c& weapondef_c::operator=(weapondef_c &rhs)
{
	if (&rhs != this)
		Copy(rhs);
	
	return *this;
}

// --> Weapon Definition Container

//
// weapondef_container_c Constructor
//
weapondef_container_c::weapondef_container_c() 
	: epi::array_c(sizeof(weapondef_c*))
{
	num_disabled = 0;
}

//
// weapondef_container_c Destructor
//
weapondef_container_c::~weapondef_container_c()
{
	Clear();
}

//
// weapondef_container_c::CleanupObject()
//
void weapondef_container_c::CleanupObject(void *obj)
{
	weapondef_c *w = *(weapondef_c**)obj;

	if (w)
		delete w;

	return;
}

//
// weapondef_container_c::FindFirst()
//
int weapondef_container_c::FindFirst(const char *name, int startpos)
{
	epi::array_iterator_c it;
	weapondef_c *w;

	if (startpos>0)
		it = GetIterator(startpos);
	else
		it = GetBaseIterator();

	while (it.IsValid())
	{
		w = ITERATOR_TO_TYPE(it, weapondef_c*);
		if (DDF_CompareName(w->ddf.name.GetString(), name) == 0)
		{
			return it.GetPos();
		}

		it++;
	}

	return -1;
}

//
// weapondef_container_c::Lookup()
//
weapondef_c* weapondef_container_c::Lookup(const char* refname)
{
	int idx = FindFirst(refname, num_disabled);
	if (idx >= 0)
		return (*this)[idx];

	// FIXME!!! Throw an epi::error_c obj
	//DDF_Error("Unknown weapon type: %s\n", refname);
	return NULL;
}

//
// weaponkey_c::Load()
//
void weaponkey_c::Load(weapondef_container_c *wc)
{
	Clear();
	
	choices = new weapondef_c*[wc->GetSize()];
	numchoices = wc->GetSize();

	for (int i = 0; i < numchoices; i++)
	{
		choices[i] = (*wc)[i];
	}
}


