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
	DF("AMMOTYPE", ammo, DDF_WGetAmmo),
	DF("AMMOPERSHOT", ammopershot, DDF_MainGetNumeric),
	DF("CLIPSIZE", clip_size, DDF_MainGetNumeric),
	DF("AUTOMATIC", autofire, DDF_MainGetBoolean),
	DF("SEC AMMOTYPE", sa_ammo, DDF_WGetAmmo),
	DF("SEC AMMOPERSHOT", sa_ammopershot, DDF_MainGetNumeric),
	DF("SEC CLIPSIZE", sa_clip_size, DDF_MainGetNumeric),
	DF("SEC AUTOMATIC", sa_autofire, DDF_MainGetBoolean),
	DF("ATTACK", attack, DDF_MainRefAttack),
	DF("SECOND ATTACK", sa_attack, DDF_MainRefAttack),
	DF("EJECT ATTACK", eject_attack, DDF_MainRefAttack),
	DF("FREE", autogive, DDF_MainGetBoolean),
	DF("BINDKEY", bind_key, DDF_MainGetNumeric),
	DF("PRIORITY", priority, DDF_MainGetNumeric),
	DF("DANGEROUS", dangerous, DDF_MainGetBoolean),
	DF("UPGRADES", upgraded_weap, DDF_WGetUpgrade),
	DF("IDLE SOUND", idle, DDF_MainLookupSound),
	DF("ENGAGED SOUND", engaged, DDF_MainLookupSound),
	DF("HIT SOUND", hit, DDF_MainLookupSound),
	DF("START SOUND", start, DDF_MainLookupSound),
	DF("NOTHRUST", nothrust, DDF_MainGetBoolean),
	DF("FEEDBACK", feedback, DDF_MainGetBoolean),
	DF("KICK", kick, DDF_MainGetFloat),
	DF("SPECIAL", special_flags, DDF_WGetSpecialFlags),
	DF("SEC SPECIAL", sa_specials, DDF_WGetSpecialFlags),
	DF("ZOOM FOV", zoom_fov, DDF_MainGetAngle),
	DF("REFIRE INACCURATE", refire_inacc, DDF_MainGetBoolean),
	DF("SHOW CLIP", show_clip, DDF_MainGetBoolean),
	DF("BOBBING", bobbing, DDF_MainGetPercent),
	DF("SWAYING", swaying, DDF_MainGetPercent),

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
	{"ATTACK",    "READY",  &buffer_weapon.attack_state},
	{"RELOAD",    "READY",  &buffer_weapon.reload_state},
	{"FLASH",     "REMOVE", &buffer_weapon.flash_state},
	{"EMPTY",     "EMPTY",  &buffer_weapon.empty_state},
	{"SECATTACK", "READY",  &buffer_weapon.sa_attack_state},
	{"SECRELOAD", "READY",  &buffer_weapon.sa_reload_state},
	{"SECFLASH",  "REMOVE", &buffer_weapon.sa_flash_state},
	{"CROSSHAIR", "REMOVE", &buffer_weapon.crosshair},
	{"ZOOM",      "ZOOM",   &buffer_weapon.zoom_state},
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
	{"SETCROSS",          A_SetCrosshair, NULL},
	{"TARGET",            A_GotTarget, NULL},
	{"LIGHT0",            A_Light0, NULL},
	{"LIGHT1",            A_Light1, NULL},
	{"LIGHT2",            A_Light2, NULL},
	{"CHECKRELOAD",       A_CheckReload, NULL},
	{"FLASH",             A_GunFlash, NULL},
	{"PLAYSOUND",         A_WeaponPlaySound, DDF_StateGetSound},
	{"KILLSOUND",         A_WeaponKillSound, NULL},
	{"JUMP",              A_WeaponJump, DDF_StateGetJump},
	{"RTS ENABLE TAGGED", A_WeaponEnableRadTrig,  DDF_StateGetInteger},
	{"RTS DISABLE TAGGED",A_WeaponDisableRadTrig, DDF_StateGetInteger},
	{"TRANS SET",         A_WeaponTransSet,  DDF_StateGetPercent},
	{"TRANS FADE",        A_WeaponTransFade, DDF_StateGetPercent},
	{"SEC SHOOT",         A_WeaponShootSA, DDF_StateGetAttack},
	{"SEC REFIRE",        A_ReFireSA, NULL},
	{"SEC NOFIRE",        A_NoFireSA, NULL},
	{"SEC NOFIRE RETURN", A_NoFireReturnSA, NULL},
	{"SEC FLASH",         A_GunFlashSA, NULL},
	{"SEC CHECKRELOAD",   A_CheckReloadSA, NULL},

	// -AJA- backwards compatibility cruft...
	{"!SOUND1",           A_SFXWeapon1, NULL},
	{"!SOUND2",           A_SFXWeapon2, NULL},
	{"!SOUND3",           A_SFXWeapon3, NULL},
	{"!RANDOMJUMP", 	  NULL, NULL},
	{NULL, NULL, NULL}
};

const specflags_t ammo_types[] =
{
    {"NOAMMO",  AM_NoAmmo, 0},
    
    {"BULLETS", AM_Bullet, 0},
    {"SHELLS",  AM_Shell,  0},
    {"ROCKETS", AM_Rocket, 0},
    {"CELLS",   AM_Cell,   0},
    
    {"PELLETS", AM_Pellet, 0},
    {"NAILS",   AM_Nail,   0},
    {"GRENADES",AM_Grenade,0},
    {"GAS",     AM_Gas,    0},

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

	if (ddf_version < 0x129)
	{
		// handle properties (old piece of crud)
		if (index == 0 && DDF_CompareName(contents, "TRUE") == 0)
		{
			DDF_WGetSpecialFlags(field, &buffer_weapon.special_flags);
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
	if (buffer_weapon.ammopershot < 0)
	{
		DDF_WarnError2(0x128, "Bad AMMOPERSHOT value for weapon: %d\n",
				buffer_weapon.ammopershot);
		buffer_weapon.ammopershot = 0;
	}
	if (buffer_weapon.sa_ammopershot < 0)
	{
		DDF_WarnError2(0x129, "Bad SEC_AMMOPERSHOT value for weapon: %d\n",
				buffer_weapon.sa_ammopershot);
		buffer_weapon.sa_ammopershot = 0;
	}

	if (buffer_weapon.clip_size < 1)
	{
		DDF_WarnError2(0x129, "Bad CLIPSIZE value for weapon: %d\n",
				buffer_weapon.clip_size);
		buffer_weapon.clip_size = 1;
	}
	if (buffer_weapon.sa_clip_size < 1)
	{
		DDF_WarnError2(0x129, "Bad SEC_CLIPSIZE value for weapon: %d\n",
				buffer_weapon.sa_clip_size);
		buffer_weapon.sa_clip_size = 1;
	}

	// zero values for ammopershot really mean infinite ammo

	if (buffer_weapon.ammopershot == 0)
		buffer_weapon.ammo = AM_NoAmmo;

	if (buffer_weapon.sa_ammopershot == 0)
		buffer_weapon.sa_ammo = AM_NoAmmo;

	// backwards compatibility (REMOVE for 1.26)
	if (buffer_weapon.priority < 0)
	{
		DDF_WarnError2(0x129, "Using PRIORITY=-1 in weapons.ddf is obsolete !\n");

		buffer_weapon.dangerous = true;
		buffer_weapon.priority = 10;
	}

	// backwards compatibility
	if (ddf_version < 0x129)
		buffer_weapon.sa_specials = (weapon_flag_e)
			(buffer_weapon.special_flags & WPSP_SilentToMon);

	// transfer static entry to dynamic entry
	dynamic_weapon->CopyDetail(buffer_weapon);

	// compute CRC...
	dynamic_weapon->ddf.crc += dynamic_weapon->ammo;
	dynamic_weapon->ddf.crc += dynamic_weapon->ammopershot;

	// FIXME: do more stuff...
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
	
#if (DEBUG_DDF)
		L_WriteDebug("DDF_Weap: CHOICES ON KEY %d:\n", key);
		for (i=0; i < wk->numchoices; i++)
		{
			L_WriteDebug("  [%s] pri=%d\n", wk->choices[i]->ddf.name.GetString(),
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
	int *dest = (int *)storage;

	*dest = weapondefs.FindFirst(info);

	if (*dest < 0)
		DDF_WarnError2(0x128, "Unknown weapon to upgrade: %s\n", info);
}

static specflags_t weapon_specials[] =
{
    {"SILENT TO MONSTERS", WPSP_SilentToMon, 0},
    {"SWITCH", WPSP_SwitchAway, 0},
	{"TRIGGER", WPSP_Trigger, 0},
	{"NEW", WPSP_New, 0},
	{"KEY", WPSP_Key, 0},
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
	attack = src.attack;		
  
	ammo = src.ammo;		
	ammopershot = src.ammopershot;	
	clip_size = src.clip_size;				
	autofire = src.autofire;		
	kick = src.kick;				
  
	sa_attack = src.sa_attack;	
	
	sa_ammo = src.sa_ammo;		
	sa_ammopershot = src.sa_ammopershot;	
	sa_clip_size = src.sa_clip_size;			
	sa_autofire = src.sa_autofire;		
  
	first_state = src.first_state;
	last_state = src.last_state;
  
	up_state = src.up_state;			
	down_state = src.down_state;		
	ready_state = src.ready_state;		
	attack_state = src.attack_state;	
	reload_state = src.reload_state;	
	flash_state = src.flash_state;		
	empty_state = src.empty_state;		

	sa_attack_state = src.sa_attack_state;	
	sa_reload_state = src.sa_reload_state;	
	sa_flash_state = src.sa_flash_state;		
	crosshair = src.crosshair;			
	zoom_state = src.zoom_state;			
	
	autogive = src.autogive;			
	feedback = src.feedback;			
	upgraded_weap = src.upgraded_weap;
 
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
  	special_flags = src.special_flags;
  	sa_specials = src.sa_specials;
	
	zoom_fov = src.zoom_fov;
	refire_inacc = src.refire_inacc;
	show_clip = src.show_clip;
	bobbing = src.bobbing;
	swaying = src.swaying;
}

//
// weapondef_c::Default()
//	
void weapondef_c::Default(void)
{
	ddf.Default();

	attack = NULL;

	ammo = AM_NoAmmo;		
	ammopershot = 0;	
	clip_size = 1;				
	autofire = false;		
	kick = 0.0f;				

	sa_attack = NULL;	
	
	sa_ammo = AM_NoAmmo;		
	sa_ammopershot = 0;	
	sa_clip_size = 1;			
	sa_autofire = false;	

	first_state = 0;
	last_state = 0;

	up_state = 0;
	down_state= 0;
	ready_state = 0;
	attack_state = 0;
	reload_state = 0;
	flash_state = 0; 
	empty_state = 0; 
	
	sa_attack_state = 0;
	sa_reload_state = 0;
	sa_flash_state = 0;
	
	crosshair = 0;
	zoom_state = 0;

	autogive = false;
	feedback = false;
	upgraded_weap = -1;
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
	special_flags = DEFAULT_WPSP;
	sa_specials = (weapon_flag_e)(DEFAULT_WPSP & ~WPSP_SwitchAway);
	zoom_fov = 0;
	refire_inacc = false;
	show_clip = false;
	bobbing = PERCENT_MAKE(100);
	swaying = PERCENT_MAKE(100);
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
	epi::array_iterator_c it;
	weapondef_c *wd;
	int i;
	
	Clear();
	
	choices = new weapondef_c*[wc->GetSize()];
	numchoices = wc->GetSize();

	for (wd = choices[0], i = 0; i < numchoices; wd++, i++)
	{
		choices[i] = (*wc)[i];
	}
}


