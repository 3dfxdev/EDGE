//----------------------------------------------------------------------------
//  EDGE Data Definition File Code (Main)
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2007  The EDGE Team.
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

#ifndef __DDF_MAIN__
#define __DDF_MAIN__

#include "dm_defs.h"


// ------------------------------------------------------------------
// -----------------SCREEN EFFECT DEFINITIONS------------------------
// ------------------------------------------------------------------

typedef struct tint_effect_def_s
{
	const colourmap_c *colmap;  // if NULL, use the next field
	rgbcol_t colour;

	int fade_out_time;  // in tics
	bool flashes;  // otherwise smoothly fades out
}
tint_effect_def_t;

typedef struct icon_effect_def_s
{
	lumpname_c graphic;
	int num_frames;
	float x, y;  // in 320x200 coordinates
	int alignment;  // FIXME: left/right top/bottom (etc)
	float scale, aspect;
	percent_t translucency;
}
icon_effect_def_t;

class screen_effect_def_c
{
public:
	screen_effect_def_c();
	screen_effect_def_c(screen_effect_def_c &rhs); 
	~screen_effect_def_c();

	ddf_base_c ddf;

	tint_effect_def_t tint;
	icon_effect_def_t icon;

	sfx_t *sound;
};

// ------------------------------------------------------------------
// ------------------------BENEFIT TYPES-----------------------------
// ------------------------------------------------------------------

typedef enum
{
	BENEFIT_None = 0,
	BENEFIT_Ammo,
	BENEFIT_AmmoLimit,
	BENEFIT_Weapon,
	BENEFIT_Key,
	BENEFIT_Health,
	BENEFIT_Armour,
	BENEFIT_Powerup
}
benefit_type_e;

// Ammunition types defined.
typedef enum
{
	AM_DontCare = -2,  // Only used for P_SelectNewWeapon()
	AM_NoAmmo   = -1,  // Unlimited for chainsaw / fist.
  
	AM_Bullet = 0, // Pistol / chaingun ammo.
	AM_Shell,      // Shotgun / double barreled shotgun.
	AM_Rocket,     // Missile launcher.
	AM_Cell,       // Plasma rifle, BFG.

	// New ammo types
	AM_Pellet,
	AM_Nail,
	AM_Grenade,
	AM_Gas,

	AM_9,  AM_10, AM_11, AM_12,
	AM_13, AM_14, AM_15, AM_16,

	NUMAMMO  // Total count (16)
}
ammotype_e;

typedef enum
{
	// weakest armour, saves 33% of damage
	ARMOUR_Green = 0,

    // better armour, saves 50% of damage
	ARMOUR_Blue,

	// good armour, saves 75% of damage.  (not in Doom)
	ARMOUR_Yellow,

	// the best armour, saves 90% of damage.  (not in Doom)
	ARMOUR_Red,
  
	// -AJA- Note: Savegame code relies on NUMARMOUR == 4.
	NUMARMOUR
}
armour_type_e;

#define ARMOUR_Total  (NUMARMOUR+0)  // -AJA- used for conditions

typedef short armour_set_t;  // one bit per armour

// Power up artifacts.
//
// -MH- 1998/06/17  Jet Pack Added
// -ACB- 1998/07/15 NightVision Added

typedef enum
{
	PW_Invulnerable = 0,
	PW_Berserk,
	PW_PartInvis,
	PW_AcidSuit,
	PW_AllMap,
	PW_Infrared,

	// extra powerups (not in Doom)
	PW_Jetpack,     // -MH- 1998/06/18  jetpack "fuel" counter
	PW_NightVision,
	PW_Scuba,

	PW_Unused9,
	PW_Unused10,
	PW_Unused11,

	PW_Unused12,
	PW_Unused13,
	PW_Unused14,
	PW_Unused15,

	// -AJA- Note: Savegame code relies on NUMPOWERS == 16.
	NUMPOWERS
}
power_type_e;

typedef struct benefit_s
{
	// next in linked list
	struct benefit_s *next;

    // type of benefit (ammo, ammo-limit, weapon, key, health, armour,
    // or powerup).
	benefit_type_e type;
  
	// sub-type (specific type of ammo, weapon, key or powerup).  For
	// armour this is the class, for health it is unused.
	union
	{
		int type;
		weapondef_c *weap;
	}
	sub;

    // amount of benefit (e.g. quantity of ammo or health).  For weapons
    // and keys, this is a boolean value: 1 to give, 0 to ignore.  For
    // powerups, it is number of seconds the powerup lasts.
	float amount;

	// for health, armour and powerups, don't make the new value go
	// higher than this (if it is already higher, prefer not to pickup
	// the object).
	float limit;
}
benefit_t;

typedef enum
{
	PUFX_None = 0,
	PUFX_PowerupEffect,
	PUFX_ScreenEffect,
	PUFX_SwitchWeapon,
	PUFX_KeepPowerup
}
pickup_effect_type_e;

class pickup_effect_c
{
public:
	pickup_effect_c(pickup_effect_type_e _type, int _sub, int _slot, float _time);
	pickup_effect_c(pickup_effect_type_e _type, weapondef_c *_weap, int _slot, float _time);
	~pickup_effect_c() { }

	// next in linked list
	pickup_effect_c *next;

	// type and optional sub-type
	pickup_effect_type_e type;

	union
	{
		int type;
		weapondef_c *weap;
	}
	sub;

	// which slot to use
	int slot;

    // how long for the effect to last (in seconds).
	float time;
};

// -ACB- 2003/05/15: Made enum external to structure, caused different issues with gcc and VC.NET compile
typedef enum
{
	// dummy condition, used if parsing failed
	COND_NONE = 0,
    
    // object must have health
	COND_Health,

	// player must have armour (subtype is ARMOUR_* value)
	COND_Armour,

	// player must have a key (subtype is KF_* value).
	COND_Key,

	// player must have a weapon (subtype is slot number).
	COND_Weapon,

	// player must have a powerup (subtype is PW_* value).
	COND_Powerup,

	// player must have ammo (subtype is AM_* value)
	COND_Ammo,

	// player must be jumping
	COND_Jumping,

	// player must be crouching
	COND_Crouching,

	// object must be swimming (i.e. in water)
	COND_Swimming,

	// player must be attacking (holding fire down)
	COND_Attacking,

	// player must be rampaging (holding fire a long time)
	COND_Rampaging,

	// player must be using (holding space down)
	COND_Using,

	// player must be walking
	COND_Walking
}
condition_check_type_e;

typedef struct condition_check_s
{
	// next in linked list (order is unimportant)
	struct condition_check_s *next;

	// negate the condition
	bool negate;

	// condition typing. -ACB- 2003/05/15: Made an integer to hold condition_check_type_e enumeration
	int cond_type;

	// sub-type (specific type of ammo, weapon, key, powerup).  Not used
	// for health, jumping, crouching, etc.
	union
	{
		int type;
		weapondef_c *weap;
	}
	sub;

	// required amount of health, armour or ammo,   Not used for
	// weapon, key, powerup, jumping, crouching, etc.
	float amount;
}
condition_check_t;

// ------------------------------------------------------------------
// --------------------MOVING OBJECT INFORMATION---------------------
// ------------------------------------------------------------------

//
// -ACB- 2003/05/15: Moved this outside of damage_c. GCC and VC.NET have 
// different- and conflicting - issues with structs in structs
//
// override labels for various states, if the object being damaged
// has such a state then it is used instead of the normal ones
// (PAIN, DEATH, OVERKILL).  Defaults to NULL.
//

class label_offset_c
{
public:
	label_offset_c();
	label_offset_c(label_offset_c &rhs);
	~label_offset_c(); 

private:
	void Copy(label_offset_c &src);

public:
	void Default();
	label_offset_c& operator=(label_offset_c &rhs);

	epi::strent_c label;
	int offset;
};

class damage_c
{
public:
	damage_c();
	damage_c(damage_c &rhs);
	~damage_c();
	
	enum default_e
	{
		DEFAULT_Attack,
		DEFAULT_Mobj,
		DEFAULT_MobjChoke,
		DEFAULT_Sector,
		DEFAULT_Numtypes
	};
	
private:
	void Copy(damage_c &src);

public:
	void Default(default_e def);
	damage_c& operator= (damage_c &rhs);

	// nominal damage amount (required)
	float nominal;

	// used for DAMAGE.MAX: when this is > 0, the damage is random
	// between nominal and linear_max, where each value has equal
	// probability.
	float linear_max;
  
	// used for DAMAGE.ERROR: when this is > 0, the damage is the
	// nominal value +/- this error amount, with a bell-shaped
	// distribution (values near the nominal are much more likely than
	// values at the outer extreme).
	float error;

	// delay (in terms of tics) between damage application, e.g. 34
	// would be once every second.  Only used for slime/crush damage.
	int delay;

	// override labels for various states, if the object being damaged
	// has such a state then it is used instead of the normal ones
	// (PAIN, DEATH, OVERKILL).  Defaults to NULL.
	label_offset_c pain, death, overkill;

	// this flag says that the damage is unaffected by the player's
	// armour -- and vice versa.
	bool no_armour;
};

// a bitset is a set of named bits, from `A' to `Z'.
typedef int bitset_t;

#define BITSET_EMPTY  0
#define BITSET_FULL   0x7FFFFFFF
#define BITSET_MAKE(ch)  (1 << ((ch) - 'A'))

// dynamic light info

typedef enum
{
	// dynamic lighting disabled
	DLITE_None,

	// light texture is modulated with wall texture
	DLITE_Modulate,

	// light texture is simply added to wall
	DLITE_Add,
}
dlight_type_e;

class dlight_info_c
{
public:
	dlight_info_c();
	dlight_info_c(dlight_info_c &rhs);
	~dlight_info_c() {};

private:
	void Copy(dlight_info_c &src);

public:
	void Default(void);
	dlight_info_c& operator=(dlight_info_c &rhs);

	dlight_type_e type;
	epi::strent_c shape;  // IMAGES.DDF reference
	float radius;
	rgbcol_t colour;
	percent_t height;
};

// halo information (height < 0 disables halo)
class haloinfo_c
{
public:
	haloinfo_c();
	haloinfo_c(haloinfo_c &rhs);
	~haloinfo_c() {};

private:
	void Copy(haloinfo_c &src);

public:
	void Default(void);
	haloinfo_c& operator=(haloinfo_c &rhs);

	float height;
	float size, minsize, maxsize;
	percent_t translucency;
	rgbcol_t colour;
	lumpname_c graphic;
};

// mobjdef class
class mobjtype_c
{
public:
	mobjtype_c();
	mobjtype_c(mobjtype_c &rhs);
	~mobjtype_c();

private:
	void Copy(mobjtype_c &src);

public:
	void CopyDetail(mobjtype_c &src);
	void Default();
	
	mobjtype_c& operator=(mobjtype_c &rhs);

	// DDF Id
	ddf_base_c ddf;

	// range of states used
	int first_state;
	int last_state;
  
	int spawn_state;
	int idle_state;
	int chase_state;
	int pain_state;
	int missile_state;
	int melee_state;
	int death_state;
	int overkill_state;
	int raise_state;
	int res_state;
	int meander_state;
	int bounce_state;
	int touch_state;
	int jump_state;
	int gib_state;
	int reload_state;

	int reactiontime;
	percent_t painchance;
	float spawnhealth;
	float speed;
	float float_speed;
	float radius;
	float height;
	float step_size;
	float mass;

	int flags;
	int extendedflags;
	int hyperflags;

	damage_c explode_damage;
	float explode_radius;  // normally zero (radius == damage)

	// linked list of losing benefits, or NULL
	benefit_t *lose_benefits;
  
	// linked list of pickup benefits, or NULL
	benefit_t *pickup_benefits;

	// linked list of pickup effects, or NULL
	pickup_effect_c *pickup_effects;

	// pickup message, a reference to languages.ldf
	char *pickup_message;

	// linked list of initial benefits for players, or NULL if none
	benefit_t *initial_benefits;

	int castorder;
	epi::strent_c cast_title;
	int respawntime;
	percent_t translucency;
	percent_t minatkchance;
	const colourmap_c *palremap;

	int jump_delay;
	float jumpheight;
	float crouchheight;
	percent_t viewheight;
	percent_t shotheight;
	float maxfall;
	float fast;
	float xscale;
	float yscale;
	float bounce_speed;
	float bounce_up;
	float sight_slope;
	angle_t sight_angle;
	float ride_friction;
	percent_t shadow_trans;

	sfx_t *seesound;
	sfx_t *attacksound;
	sfx_t *painsound;
	sfx_t *deathsound;
	sfx_t *overkill_sound;
	sfx_t *activesound;
	sfx_t *walksound;
	sfx_t *jump_sound;
	sfx_t *noway_sound;
	sfx_t *oof_sound;
	sfx_t *gasp_sound;

	int fuse;
	int reload_shots;

	bitset_t side;
	int playernum;

	// breathing support: lung_capacity is how many tics we can last
	// underwater.  gasp_start is how long underwater before we gasp
	// when leaving it.  Damage and choking interval is in choke_damage.
	int lung_capacity;
	int gasp_start;
	damage_c choke_damage;

	// controls how much the player bobs when walking.
	percent_t bobbing;

	// what attack classes we are immune/resistant to (usually none).
	bitset_t immunity;
	bitset_t resistance;
	bitset_t ghost;  // pass through us

	const atkdef_c *closecombat;
	const atkdef_c *rangeattack;
	const atkdef_c *spareattack;

	// -ACB- 2003/05/15 Made haloinfo structure external to mobjtype_c
	haloinfo_c halo;

	// -ACB- 2003/05/15 Made dlight_info structure external to mobjtype_c
	dlight_info_c dlight0;
	dlight_info_c dlight1;

	// item to drop (or NULL).  The mobjdef pointer is only valid after
	// DDF_MobjCleanUp() has been called.
	const mobjtype_c *dropitem;
	epi::strent_c dropitem_ref;

	// blood object (or NULL).  The mobjdef pointer is only valid after
	// DDF_MobjCleanUp() has been called.
	const mobjtype_c *blood;
	epi::strent_c blood_ref;
  
	// respawn effect object (or NULL).  The mobjdef pointer is only
	// valid after DDF_MobjCleanUp() has been called.
	const mobjtype_c *respawneffect;
	epi::strent_c respawneffect_ref;
  
	// spot type for the `SHOOT_TO_SPOT' attack (or NULL).  The mobjdef
	// pointer is only valid after DDF_MobjCleanUp() has been called.
	const mobjtype_c *spitspot;
	epi::strent_c spitspot_ref;
};

// Our mobjdef container
#define LOOKUP_CACHESIZE 211

class mobjtype_container_c : public epi::array_c
{
public:
	mobjtype_container_c();
	~mobjtype_container_c();

private:
	void CleanupObject(void *obj);

	mobjtype_c* lookup_cache[LOOKUP_CACHESIZE];
	int num_disabled;					// Number of disabled 

public:
	// List Management
	int GetSize() {	return array_entries; } 
	int Insert(mobjtype_c *m) { return InsertObject((void*)&m); }
	mobjtype_c* operator[](int idx) { return *(mobjtype_c**)FetchObject(idx); } 
	bool MoveToEnd(int idx);

	// Inliners
	int GetDisabledCount() { return num_disabled; }
	void SetDisabledCount(int _num_disabled) { num_disabled = _num_disabled; }

	// Search Functions
	int FindFirst(const char *name, int startpos = -1);
	int FindLast(const char *name, int startpos = -1);
	const mobjtype_c* Lookup(const char* refname);
	const mobjtype_c* Lookup(int id);

	// FIXME!!! Move to a more app location
	const mobjtype_c* LookupCastMember(int castpos);
	const mobjtype_c *LookupPlayer(int playernum);
};


// -------EXTERNALISATIONS-------

extern mobjtype_container_c mobjtypes;

void DDF_MobjGetBenefit(const char *info, void *storage);

bool DDF_ReadThings(void *data, int size);

#endif // __DDF_MAIN__

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
