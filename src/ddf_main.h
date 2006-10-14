//----------------------------------------------------------------------------
//  EDGE Data Definition File Code (Main)
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2005  The EDGE Team.
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
// $Id: ddf_main.h,v 1.171 2006/09/08 11:00:42 ajapted Exp $
//

#ifndef __DDF_MAIN__
#define __DDF_MAIN__

#include "dm_defs.h"

#include "epi/math_crc.h"
#include "epi/utility.h"

#define DEBUG_DDF  0

// Forward declaration
class atkdef_c;
class colourmap_c;
class gamedef_c;
class mapdef_c;
class mobjtype_c;
class pl_entry_c;
class weapondef_c;

struct image_s;
struct mobj_s;
struct sfx_s;

typedef struct sfx_s sfx_t;

// RGB 8:8:8
// (FIXME: use epi::colour_c)
typedef unsigned int rgbcol_t;

#define RGB_NO_VALUE  0x00FFFF  /* bright CYAN */

#define RGB_MAKE(r,g,b)  (((r) << 16) | ((g) << 8) | (b))

#define RGB_RED(rgbcol)  ((float)(((rgbcol) >> 16) & 0xFF) / 255.0f)
#define RGB_GRN(rgbcol)  ((float)(((rgbcol) >>  8) & 0xFF) / 255.0f)
#define RGB_BLU(rgbcol)  ((float)(((rgbcol)      ) & 0xFF) / 255.0f)

class ddf_base_c
{
public:
	ddf_base_c();	
	ddf_base_c(const ddf_base_c &rhs);
	~ddf_base_c();	

private:
	void Copy(const ddf_base_c &src);

public:
	void Default(void);
	void SetUniqueName(const char *prefix, int num);
	
	ddf_base_c& operator= (const ddf_base_c &rhs);

	epi::strent_c name;
	int number;	
	epi::crc32_c crc;
};

// percentage type.  Ranges from 0.0f - 1.0f
typedef float percent_t;

#define PERCENT_MAKE(val)  ((val) / 100.0f)
#define PERCENT_2_FLOAT(perc)  (perc)

// Our lumpname class
#define LUMPNAME_SIZE 10

class lumpname_c
{
public:
	lumpname_c() { Clear(); }
	lumpname_c(lumpname_c &rhs) { Set(rhs.data); }
	~lumpname_c() {};

private:
	char data[LUMPNAME_SIZE];

public:
	void Clear() { data[0] = '\0'; }

	const char* GetString() const { return data; }

	inline bool IsEmpty() const { return data[0] == '\0'; }

	void Set(const char *s) 
	{
		int i;

		for (i=0; i<(LUMPNAME_SIZE-1) && *s; i++, s++)
			data[i] = *s;

		data[i] = '\0';
	}

	lumpname_c& operator=(lumpname_c &rhs) 
	{
		if (&rhs != this) 
			Set(rhs.data);
			 
		return *this; 
	}
	
	char operator[](int idx) const { return data[idx]; }
	operator const char* () const { return data; }
};

class mobj_strref_c
{
public:
	mobj_strref_c() : name(), def(NULL) { }
	mobj_strref_c(const char *s) : name(s), def(NULL) { }
	mobj_strref_c(const mobj_strref_c &rhs) : name(rhs.name), def(NULL) { }
	~mobj_strref_c() {};

private:
	epi::strent_c name;

	const mobjtype_c *def;

public:
	const char *GetName() const { return name.GetString(); }

	const mobjtype_c *GetRef();
	// Note: this returns NULL if not found, in which case you should
	// produce an error, since future calls will do the search again.

	mobj_strref_c& operator= (mobj_strref_c &rhs) 
	{
		if (&rhs != this) 
		{
			name = rhs.name;
			def = NULL;
		}
			 
		return *this; 
	}
};


//-------------------------------------------------------------------------
//-----------------------  THING STATE STUFF   ----------------------------
//-------------------------------------------------------------------------

typedef int statenum_t;

#define S_NULL    0   // state
#define SPR_NULL  0   // sprite

// State Struct
typedef struct
{
	// sprite ref
	int sprite;

    // frame ref
	short frame;
 
	// brightness
	short bright;
 
	// duration in tics
	int tics;

	// label for state, or NULL
	const char *label;

	// routine to be performed
	void (* action)(struct mobj_s * object);

	// parameter for routine, or NULL
	void *action_par;

	// next state ref.  S_NULL means "remove me".
	int nextstate;

	// jump state ref.  S_NULL means remove.
	int jumpstate;
}
state_t;

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

// FIXME: Embed enum in dlightinfo class?
typedef enum
{
	// dynamic lighting disabled
	DLITE_None,

	// lighting is proportional to 1 / distance
	DLITE_Linear,

	// lighting is proportional to 1 / (distance^2)
	DLITE_Quadratic
}
dlight_type_e;

class dlightinfo_c
{
public:
	dlightinfo_c();
	dlightinfo_c(dlightinfo_c &rhs);
	~dlightinfo_c() {};

private:
	void Copy(dlightinfo_c &src);

public:
	void Default(void);
	dlightinfo_c& operator=(dlightinfo_c &rhs);

	dlight_type_e type;
	int intensity;
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

	// -ACB- 2003/05/15 Made dlightinfo structure external to mobjtype_c
	dlightinfo_c dlight;

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

// ------------------------------------------------------------------
// --------------------ATTACK TYPE STRUCTURES------------------------
// ------------------------------------------------------------------

// -KM- 1998/11/25 Added BFG SPRAY attack type.

// FIXME!!! Move enums into attackdef_t
typedef enum
{
	ATK_NONE = 0,
	ATK_PROJECTILE,
	ATK_SPAWNER,
	ATK_TRIPLESPAWNER,
	ATK_SPREADER,
	ATK_RANDOMSPREAD,
	ATK_SHOT,
	ATK_TRACKER,
	ATK_CLOSECOMBAT,
	ATK_SHOOTTOSPOT,
	ATK_SKULLFLY,
	ATK_SMARTPROJECTILE,
	ATK_SPRAY,
	NUMATKCLASS
}
attackstyle_e;

typedef enum
{
	AF_None            = 0,
	AF_TraceSmoke      = 1,
	AF_KillFailedSpawn = 2,
	AF_PrestepSpawn    = 4,
	AF_SpawnTelefrags  = 8,
	AF_NeedSight       = 16,
	AF_FaceTarget      = 32,
	AF_Player          = 64,
	AF_ForceAim        = 128,

	AF_AngledSpawn     = 0x0100,
	AF_NoTriggerLines  = 0x0200,
	AF_SilentToMon     = 0x0400,
	AF_NoTarget        = 0x0800
}
attackflags_e;

// attack definition class
class atkdef_c
{
public:
	atkdef_c();
	atkdef_c(atkdef_c &rhs); 
	~atkdef_c();

private:
	void Copy(atkdef_c &src);

public:
	void CopyDetail(atkdef_c &src);
	void Default();
	atkdef_c& operator=(atkdef_c &rhs);

	// Member vars
	ddf_base_c ddf;

	attackstyle_e attackstyle;
	attackflags_e flags;
	sfx_t *initsound;
	sfx_t *sound;
	float accuracy_slope;
	angle_t accuracy_angle;
	float xoffset;
	float yoffset;
	angle_t angle_offset;  // -AJA- 1999/09/10.
	float slope_offset;    //
	angle_t trace_angle; // -AJA- 2005/02/08.
	float assault_speed;
	float height;
	float range;
	int count;
	int tooclose;
	float berserk_mul;  // -AJA- 2005/08/06.
	damage_c damage;

	// class of the attack.
	bitset_t attack_class;
 
	// object init state.  The integer value only becomes valid after
	// DDF_AttackCleanUp() has been called.
	int objinitstate;
	epi::strent_c objinitstate_ref;
  
	percent_t notracechance;
	percent_t keepfirechance;

	// the MOBJ that is integrated with this attack, or NULL
	const mobjtype_c *atk_mobj;

	// spawned object (for spawners).  The mobjdef pointer only becomes
	// valid after DDF_AttackCleanUp().  Can be NULL.
	const mobjtype_c *spawnedobj;
	epi::strent_c spawnedobj_ref;
	int spawn_limit;
  
	// puff object.  The mobjdef pointer only becomes valid after
	// DDF_AttackCleanUp() has been called.  Can be NULL.
	const mobjtype_c *puff;
	epi::strent_c puff_ref;
};

class atkdef_container_c : public epi::array_c
{
public:
	atkdef_container_c();
	~atkdef_container_c();

private:
	void CleanupObject(void *obj);

	int num_disabled;					// Number of disabled 

public:
	// List Management
	int GetSize() {	return array_entries; } 
	int Insert(atkdef_c *a) { return InsertObject((void*)&a); }
	atkdef_c* operator[](int idx) { return *(atkdef_c**)FetchObject(idx); } 

	// Inliners
	int GetDisabledCount() { return num_disabled; }
	void SetDisabledCount(int _num_disabled) { num_disabled = _num_disabled; }

	// Search Functions
	atkdef_c* Lookup(const char* refname);
};

// ------------------------------------------------------------------
// -----------------------WEAPON HANDLING----------------------------
// ------------------------------------------------------------------

#define WEAPON_KEYS 10

// -AJA- 2000/01/12: Weapon special flags
typedef enum
{
	WPSP_None = 0x0000,

	WPSP_SilentToMon = 0x0001, // monsters cannot hear this weapon
	WPSP_Animated    = 0x0002, // raise/lower states are animated

	WPSP_SwitchAway  = 0x0010, // select new weapon when we run out of ammo

	// reload flags:
	WPSP_Trigger = 0x0100, // allow reload while holding trigger
	WPSP_Fresh   = 0x0200, // automatically reload when new ammo is avail
	WPSP_Manual  = 0x0400, // enables the manual reload key
	WPSP_Partial = 0x0800, // manual reload: allow partial refill
}
weapon_flag_e;

#define DEFAULT_WPSP  (weapon_flag_e)(WPSP_Trigger | WPSP_Manual | WPSP_SwitchAway | WPSP_Partial)

class weapondef_c
{
public:
	weapondef_c();
	weapondef_c(weapondef_c &rhs);
	~weapondef_c();
	
private:
	void Copy(weapondef_c &src);
	
public:
	void CopyDetail(weapondef_c &src);
	void Default(void);

	weapondef_c& operator=(weapondef_c &rhs);

	ddf_base_c ddf;			// Weapon's name, etc...

	atkdef_c *attack[2];	// Attack type used.
  
	ammotype_e ammo[2];		// Type of ammo this weapon uses.
	int ammopershot[2];		// Ammo used per shot.
	int clip_size[2];		// Amount of shots in a clip (if <= 1, non-clip weapon)
	bool autofire[2];		// If true, this is an automatic else it's semiauto 

	float kick;				// Amount of kick this weapon gives
  
	// range of states used
	int first_state;
	int last_state;
  
	int up_state;			// State to use when raising the weapon 
	int down_state;			// State to use when lowering the weapon (if changing weapon)
	int ready_state;		// State that the weapon is ready to fire in
	int empty_state;        // State when weapon is empty.  Usually zero
	int idle_state;			// State to use when polishing weapon

	int attack_state[2];	// State showing the weapon 'firing'
	int reload_state[2];	// State showing the weapon being reloaded
	int discard_state[2];	// State showing the weapon discarding a clip
	int warmup_state[2];	// State showing the weapon warming up
	int flash_state[2];		// State showing the muzzle flash

	int crosshair;			// Crosshair states
	int zoom_state;			// State showing viewfinder when zoomed.  Can be zero

	bool no_cheat;          // Not given for cheats (Note: set by #CLEARALL)

	bool autogive;			// The player gets this weapon on spawn.  (Fist + Pistol)
	bool feedback;			// This weapon gives feedback on hit (chainsaw)

	weapondef_c *upgrade_weap; // This weapon upgrades a previous one.
 
	// This affects how it will be selected if out of ammo.  Also
	// determines the cycling order when on the same key.  Dangerous
	// weapons are not auto-selected when out of ammo.
	int priority;
	bool dangerous;
 
  	// Attack type for the WEAPON_EJECT code pointer.

	atkdef_c *eject_attack;
  
	// Sounds.
	// Played at the start of every readystate
	sfx_t *idle;
  
  	// Played while the trigger is held (chainsaw)
	sfx_t *engaged;
  
	// Played while the trigger is held and it is pointed at a target.
	sfx_t *hit;
  
	// Played when the weapon is selected
	sfx_t *start;
  
	// Misc sounds
	sfx_t *sound1;
	sfx_t *sound2;
	sfx_t *sound3;
  
	// This close combat weapon should not push the target away (chainsaw)
	bool nothrust;
  
	// which number key this weapon is bound to, or -1 for none
	int bind_key;
  
	// -AJA- 2000/01/12: weapon special flags
	weapon_flag_e specials[2];

	// -AJA- 2000/03/18: when > 0, this weapon can zoom
	angle_t zoom_fov;

	// -AJA- 2000/05/23: weapon loses accuracy when refired.
	bool refire_inacc;

	// -AJA- 2000/10/20: show current clip in status bar (not total)
	bool show_clip;

	// controls for weapon bob (up & down) and sway (left & right).
	// Given as percentages in DDF.
	percent_t bobbing;
	percent_t swaying;

	// -AJA- 2004/11/15: idle states (polish weapon, crack knuckles)
	int idle_wait;
	percent_t idle_chance;

public:
	inline int KeyPri() const  // next/prev order value
	{
		int key = 1 + MAX(-1, MIN(10, bind_key));
		int pri = 1 + MAX(-1, MIN(900, priority));

		return key * 1000 + pri;
	}
};

class weapondef_container_c : public epi::array_c
{
public:
	weapondef_container_c();
	~weapondef_container_c();

private:
	void CleanupObject(void *obj);

	int num_disabled;					// Number of disabled 

public:
	// List Management
	int GetSize() {	return array_entries; } 
	int Insert(weapondef_c *w) { return InsertObject((void*)&w); }
	
	weapondef_c* operator[](int idx) 
	{ 
		return *(weapondef_c**)FetchObject(idx); 
	} 

	// Inliners
	int GetDisabledCount() { return num_disabled; }
	void SetDisabledCount(int _num_disabled) { num_disabled = _num_disabled; }

	// Search Functions
	int FindFirst(const char *name, int startpos = -1);
	weapondef_c* Lookup(const char* refname);
};

// -KM- 1998/11/25 Dynamic number of choices, 10 keys.
class weaponkey_c
{
public:
	weaponkey_c() { choices = NULL; numchoices = 0;	}
	~weaponkey_c() { Clear(); }
	
private:
	/* ... */
	
public:
	void Clear() 
	{
		if (choices)
			delete [] choices;
		
		choices = NULL;
		numchoices = 0;
	} 
	
	void Load(weapondef_container_c *wc);

	weapondef_c **choices;
	int numchoices;
};

// ------------------------------------------------------------------
// ---------------MAP STRUCTURES AND CONFIGURATION-------------------
// ------------------------------------------------------------------

// -KM- 1998/11/25 Added generalised Finale type.
class map_finaledef_c
{
public:
	map_finaledef_c();
	map_finaledef_c(map_finaledef_c &rhs);
	~map_finaledef_c();

private:
	void Copy(map_finaledef_c &src);
	
public:
	void Default(void);
	map_finaledef_c& operator=(map_finaledef_c &rhs);

	// Text
	epi::strent_c text;
	lumpname_c text_back;
	lumpname_c text_flat;
	float text_speed;
	unsigned int text_wait;
	const colourmap_c *text_colmap;

	// Pic
	epi::strlist_c pics;
	unsigned int picwait;

	// FIXME!!! Booleans should be bit-sets/flags here...
	// Cast
	bool docast;
	
	// Bunny
	bool dobunny;

	// Music
	int music;
};

// FIXME!!! Move into mapdef_c?
typedef enum
{
	MPF_None          = 0x0,
	MPF_Jumping       = 0x1,
	MPF_Mlook         = 0x2,
	MPF_Cheats        = 0x8,
	MPF_ItemRespawn   = 0x10,
	MPF_FastParm      = 0x20,     // Fast Monsters
	MPF_ResRespawn    = 0x40,     // Resurrect Monsters (else Teleport)
	MPF_StretchSky    = 0x80,
	MPF_True3D        = 0x100,    // True 3D Gameplay
	MPF_Stomp         = 0x200,    // Monsters can stomp players
	MPF_MoreBlood     = 0x400,    // Make a bloody mess
	MPF_Respawn       = 0x800,
	MPF_AutoAim       = 0x1000,
	MPF_AutoAimMlook  = 0x2000,
	MPF_ResetPlayer   = 0x4000,   // Force player back to square #1
	MPF_Extras        = 0x8000,
	MPF_LimitZoom     = 0x10000,  // Limit zoom to certain weapons
	MPF_Shadows       = 0x20000,
	MPF_Halos         = 0x40000,
	MPF_Crouching     = 0x80000,
	MPF_Kicking       = 0x100000, // Weapon recoil
	MPF_BoomCompat    = 0x200000,
	MPF_WeaponSwitch  = 0x400000,
	MPF_NoMonsters    = 0x800000  // (Only for demos!)
}
mapsettings_e;

// FIXME!!! Move into mapdef_c?
typedef enum
{
	// standard Doom shading
	LMODEL_Doom = 0,

	// Doom shading without the brighter N/S, darker E/W walls
	LMODEL_Doomish = 1,

	// flat lighting (no shading at all)
	LMODEL_Flat = 2,

	// vertex lighting
	LMODEL_Vertex = 3,

 	// Invalid (-ACB- 2003/10/06: MSVC wants the invalid value as part of the enum)
 	LMODEL_Invalid = 999
}
lighting_model_e;

// FIXME!!! Move into mapdef_c?
typedef enum
{
	// standard Doom intermission stats
	WISTYLE_Doom = 0,

	// no stats at all
	WISTYLE_None = 1
}
intermission_style_e;

class mapdef_c
{
public:
	mapdef_c();
	mapdef_c(mapdef_c &rhs);
	~mapdef_c();
	
private:
	void Copy(mapdef_c &src);
	
public:
	void CopyDetail(mapdef_c &src);
	void Default(void);
	mapdef_c& operator=(mapdef_c &rhs);

	ddf_base_c ddf;

	// next in the list
	mapdef_c *next;				// FIXME!! Gamestate information

	// level description, a reference to languages.ldf
	epi::strent_c description;
  
  	lumpname_c namegraphic;
  	lumpname_c lump;
   	lumpname_c sky;
   	lumpname_c surround;
   	
   	int music;
 
	int partime;
	epi::strent_c episode_name;			

	// flags come in two flavours: "force on" and "force off".  When not
	// forced, then the user is allowed to control it (not applicable to
	// all the flags, e.g. RESET_PLAYER).
	int force_on;
	int force_off;

	// name of the next normal level
	lumpname_c nextmapname;

	// name of the secret level
	lumpname_c secretmapname;

	// -KM- 1998/11/25 All lines with this trigger will be activated at
	// the level start. (MAP07)
	int autotag;

	// -AJA- 2000/08/23: lighting model & intermission style
	lighting_model_e lighting;
	intermission_style_e wistyle;

	// -KM- 1998/11/25 Generalised finales.
	// FIXME!!! Suboptimal - should be pointers/references
	map_finaledef_c f_pre;
	map_finaledef_c f_end;
};

// Our mapdefs container
class mapdef_container_c : public epi::array_c 
{
public:
	mapdef_container_c() : epi::array_c(sizeof(mapdef_c*)) {}
	~mapdef_container_c() { Clear(); } 

private:
	void CleanupObject(void *obj);

public:
	mapdef_c* Lookup(const char *name);
	int GetSize() {	return array_entries; } 
	int Insert(mapdef_c *m) { return InsertObject((void*)&m); }
	mapdef_c* operator[](int idx) { return *(mapdef_c**)FetchObject(idx); } 
};

// ------------------------------------------------------------------
// -----------------------GAME DEFINITIONS---------------------------
// ------------------------------------------------------------------

// FIXME!!! Replace with epi::ivec_c (epi/math_vector.h)
struct point_s
{
	int x;
	int y;
};

class wi_mapposdef_c
{
public:
	wi_mapposdef_c();
	wi_mapposdef_c(wi_mapposdef_c &rhs);
	~wi_mapposdef_c();

private:
	void Copy(wi_mapposdef_c &src);

public:
	wi_mapposdef_c& operator=(wi_mapposdef_c &rhs);

	point_s pos;
	lumpname_c name;
};

class wi_mapposdef_container_c : public epi::array_c
{
public:
	wi_mapposdef_container_c();
	wi_mapposdef_container_c(wi_mapposdef_container_c &rhs);
	~wi_mapposdef_container_c();
	
private:
	void CleanupObject(void *obj);
	void Copy(wi_mapposdef_container_c &src);

public:
	int GetSize() {	return array_entries; } 
	int Insert(wi_mapposdef_c *m) { return InsertObject((void*)&m); }
	wi_mapposdef_container_c& operator=(wi_mapposdef_container_c &rhs);
	wi_mapposdef_c* operator[](int idx) { return *(wi_mapposdef_c**)FetchObject(idx); } 
};

class wi_framedef_c
{
public:
	wi_framedef_c();	
	wi_framedef_c(wi_framedef_c &rhs); 
	~wi_framedef_c();
	
private:
	void Copy(wi_framedef_c &src);
	
public:
	void Default(void);
	wi_framedef_c& operator=(wi_framedef_c &rhs);

	int tics;						// Tics on this frame
	point_s pos;					// Position on screen where this goes
	lumpname_c pic;					// Name of pic to display.
};

class wi_framedef_container_c : public epi::array_c
{
public:
	wi_framedef_container_c();
	wi_framedef_container_c(wi_framedef_container_c &rhs);
	~wi_framedef_container_c();
		
private:
	void CleanupObject(void *obj);
	void Copy(wi_framedef_container_c &rhs);

public:
	int GetSize() {	return array_entries; } 
	int Insert(wi_framedef_c *f) { return InsertObject((void*)&f); }
	wi_framedef_container_c& operator=(wi_framedef_container_c &rhs);
	wi_framedef_c* operator[](int idx) { return *(wi_framedef_c**)FetchObject(idx); } 
};

class wi_animdef_c
{
public:
	wi_animdef_c();
	wi_animdef_c(wi_animdef_c &rhs);
	~wi_animdef_c();

 	enum animtype_e { WI_NORMAL, WI_LEVEL };
 	
private:
	void Copy(wi_animdef_c &rhs);
	
public:
	animtype_e type;
	lumpname_c level;

	wi_framedef_container_c frames;

	wi_animdef_c& operator=(wi_animdef_c &rhs);
	void Default(void);
};

class wi_animdef_container_c : public epi::array_c
{
public:
	wi_animdef_container_c();
	wi_animdef_container_c(wi_animdef_container_c &rhs);
	~wi_animdef_container_c();

private:
	void CleanupObject(void *obj);
	void Copy(wi_animdef_container_c &src);

public:
	int GetSize() {	return array_entries; } 
	int Insert(wi_animdef_c *a) { return InsertObject((void*)&a); }
	wi_animdef_container_c& operator=(wi_animdef_container_c &rhs);
	wi_animdef_c* operator[](int idx) { return *(wi_animdef_c**)FetchObject(idx); } 
};
	
// Game definition file
class gamedef_c
{
public:
	gamedef_c();
	gamedef_c(gamedef_c &rhs);
	~gamedef_c();

private:
	void Copy(gamedef_c &src);
	
public:
	void CopyDetail(gamedef_c &src);
	void Default(void);
	gamedef_c& operator=(gamedef_c &rhs);

	ddf_base_c ddf;

	wi_animdef_container_c anims;
	wi_mapposdef_container_c mappos;

	lumpname_c background;
	lumpname_c splatpic;
	lumpname_c yah[2];

	// -AJA- 1999/10/22: background cameras.
	char bg_camera[32];

	int music;
	sfx_t *percent;
	sfx_t *done;
	sfx_t *endmap;
	sfx_t *nextmap;
	sfx_t *accel_snd;
	sfx_t *frag_snd;

	lumpname_c firstmap;
	lumpname_c namegraphic;
	
	epi::strlist_c titlepics;
	
	int titlemusic;
	int titletics;
	int special_music;
};

class gamedef_container_c : public epi::array_c
{
public:
	gamedef_container_c();
	~gamedef_container_c();

private:
	void CleanupObject(void *obj);

public:
	// List management
	int GetSize() {	return array_entries; } 
	int Insert(gamedef_c *g) { return InsertObject((void*)&g); }
	gamedef_c* operator[](int idx) { return *(gamedef_c**)FetchObject(idx); } 
	
	// Search Functions
	gamedef_c* Lookup(const char* refname);
};

// ------------------------------------------------------------------
// ---------------------------LANGUAGES------------------------------
// ------------------------------------------------------------------

class language_c
{
public:
	language_c();
	~language_c();

private:
	epi::strbox_c choices;
	epi::strbox_c refs;
	epi::strbox_c *values;
	
	int current;

	int Find(const char *ref);

public:
	void Clear();
	
	int GetChoiceCount() { return choices.GetSize(); }
	int GetChoice() { return current; }
	
	const char* GetName(int idx = -1);
	bool IsValid() { return (current>=0 && current < choices.GetSize()); }
	bool IsValidRef(const char *refname);
	
	void LoadLanguageChoices(epi::strlist_c& langnames);
	void LoadLanguageReferences(epi::strlist_c& _refs);
	void LoadLanguageValues(int lang, epi::strlist_c& _values);
				
	bool Select(const char *name);
	bool Select(int idx);
	
	const char* operator[](const char *refname);
	
	//void Dump(void);
};


// ------------------------------------------------------------------
// ------------------------LINEDEF TYPES-----------------------------
// ------------------------------------------------------------------

#define FLO_UNUSED  ((float) 3.18081979f)

// Triggers (What the line triggers)
// NOTE TO SELF: Move to an area for linedef/rts/sector stuff
typedef enum
{
	line_none,
	line_shootable,
	line_walkable,
	line_pushable,
	line_manual,   // same as pushable, but ignore any tag
	line_Any
}
trigger_e;

// Triggers (What object types can cause the line to be triggered)
// NOTE TO SELF: Move to an area for linedef/rts/sector stuff
typedef enum
{
	trig_none    = 0,
	trig_player  = 1,
	trig_monster = 2,
	trig_other   = 4
}
trigacttype_e;

// Height Info Reference
// NOTE TO SELF: Move to an area for linedef/rts/sector stuff
typedef enum
{
	REF_Absolute = 0,  // Absolute from current position
	REF_Current,       // Measure from current sector height
	REF_Surrounding,   // Measure from surrounding heights
	REF_LowestLoTexture,

	// additive flags
	REF_MASK    = 0x00FF,
	REF_CEILING = 0x0100,   // otherwise floor
	REF_HIGHEST = 0x0200,   // otherwise lowest
	REF_NEXT    = 0x0400,   // otherwise absolute
	REF_INCLUDE = 0x0800,   // otherwise excludes self
}
heightref_e;

// Movement type
typedef enum
{
	mov_undefined = 0,
	mov_Once,
	mov_MoveWaitReturn,
	mov_Continuous,
	mov_Plat,
	mov_Stairs,
	mov_Stop,
	mov_Toggle   // -AJA- 2004/10/07: added this.
}
movetype_e;

// Security type: requires certain key
// NOTE TO SELF: Move to an area for linedef/rts/sector stuff
typedef enum
{
	KF_NONE = 0,

	// keep card/skull together, for easy SKCK check
	KF_BlueCard    = (1 << 0),
	KF_YellowCard  = (1 << 1),
	KF_RedCard     = (1 << 2),
	KF_GreenCard   = (1 << 3),

	KF_BlueSkull   = (1 << 4),
	KF_YellowSkull = (1 << 5),
	KF_RedSkull    = (1 << 6),
	KF_GreenSkull  = (1 << 7),

	// -AJA- 2001/06/30: ten new keys (these + Green ones)
	KF_GoldKey     = (1 << 8),
	KF_SilverKey   = (1 << 9),
	KF_BrassKey    = (1 << 10),
	KF_CopperKey   = (1 << 11),
	KF_SteelKey    = (1 << 12),
	KF_WoodenKey   = (1 << 13),
	KF_FireKey     = (1 << 14),
	KF_WaterKey    = (1 << 15),

	// this is a special flag value that indicates that _all_ of the
	// keys in the bitfield must be held.  Normally we require _any_ of
	// the keys in the bitfield to be held.
	//
	KF_STRICTLY_ALL = (1 << 16),

	// Boom compatibility: don't care if card or skull
	KF_BOOM_SKCK = (1 << 17),

	// mask of actual key bits
	KF_CARDS  = 0x000F,
	KF_SKULLS = 0x00F0,
	KF_MASK = 0xFFFF
}
keys_e;

#define EXPAND_KEYS(set)  ((set) |  \
    (((set) & KF_CARDS) << 4) | (((set) & KF_SKULLS) >> 4))

// NOTE TO SELF: Move to an area for linedef/rts/sector stuff
typedef enum
{
	EXIT_None = 0,
	EXIT_Normal,
	EXIT_Secret
}
exittype_e;

// -AJA- 1999/10/24: Reimplemented when_appear_e type.
// NOTE TO SELF: Move to an area for linedef/rts/sector stuff
typedef enum
{
	WNAP_None        = 0x0000,

	WNAP_SkillLevel1 = 0x0001,
	WNAP_SkillLevel2 = 0x0002,
	WNAP_SkillLevel3 = 0x0004,
	WNAP_SkillLevel4 = 0x0008,
	WNAP_SkillLevel5 = 0x0010,

	WNAP_Single      = 0x0100,
	WNAP_Coop        = 0x0200,
	WNAP_DeathMatch  = 0x0400,

	WNAP_SkillBits   = 0x001F,
	WNAP_NetBits     = 0x0700
}
when_appear_e;

#define DEFAULT_APPEAR  ((when_appear_e)(0xFFFF))

// -AJA- 1999/06/21: extra floor types
// FIXME!!! Move into to extrafloordef_c?
typedef enum
{
	EXFL_None = 0x0000,

	// keeps the value from being zero
	EXFL_Present = 0x0001,

	// floor is thick, has sides.  When clear: surface only
	EXFL_Thick = 0x0002,

	// floor is liquid, i.e. non-solid.  When clear: solid
	EXFL_Liquid = 0x0004,

	// can monsters see through this extrafloor ?
	EXFL_SeeThrough = 0x0010,

	// things with the WATERWALKER tag will not fall through.
	// Also, certain player sounds (pain, death) can be overridden when
	// in a water region.  Scope for other "waterish" effects...
	// 
	EXFL_Water = 0x0020,

	// the region properties will "flood" all lower regions (unless it
	// finds another flooder).
	// 
	EXFL_Flooder = 0x0040,

	// the properties (lighting etc..) below are not transferred from
	// the dummy sector, they'll be the same as the above region.
	// 
	EXFL_NoShade = 0x0080,

	// take the side texture for THICK floors from the upper part of the
	// sidedef where the thick floor is drawn (instead of tagging line).
	// 
	EXFL_SideUpper = 0x0100,

	// like above, but use the lower part.
	EXFL_SideLower = 0x0200,

	// this controls the Y offsets on normal THICK floors.
	EXFL_SideMidY = 0x0800,

	// Boom compatibility flag (for linetype 242)
	EXFL_BoomTex = 0x1000
}
extrafloor_type_e;

#define EF_DEF_THIN    ((extrafloor_type_e)(EXFL_Present | 0))
#define EF_DEF_THICK   ((extrafloor_type_e)(EXFL_Present | EXFL_Thick))
#define EF_DEF_LIQUID  ((extrafloor_type_e)(EXFL_Present | EXFL_Liquid))

// FIXME!! Move into extrafloordef_c
typedef enum
{
	// remove an extra floor
	EFCTL_None = 0,
	EFCTL_Remove
}
extrafloor_control_e;

class extrafloordef_c
{
public:
	extrafloordef_c();
	extrafloordef_c(extrafloordef_c &rhs);
	~extrafloordef_c();

private:
	void Copy(extrafloordef_c &src);

public:
	void Default(void);
	extrafloordef_c& operator=(extrafloordef_c &src);

	extrafloor_type_e type;
	extrafloor_control_e control;
};

// --> Moving plane def (Ceilings, floors and doors)
class movplanedef_c
{
public:
	movplanedef_c();
	movplanedef_c(movplanedef_c &rhs);
	~movplanedef_c();
	
	enum default_e
	{
		DEFAULT_CeilingLine,
		DEFAULT_CeilingSect,
		DEFAULT_DonutFloor,
		DEFAULT_FloorLine,
		DEFAULT_FloorSect,
		DEFAULT_Numtypes
	};
	
private:
	void Copy(movplanedef_c &src);
	
public:
	void Default(default_e def);
	movplanedef_c& operator=(movplanedef_c &rhs);

	// Type of floor: raise/lower/etc
	movetype_e type;

    // True for a ceiling, false for a floor
	bool is_ceiling;
	bool crush;

    // How fast the plane moves.
	float speed_up;
	float speed_down;

    // This refers to what the dest. height refers to.
	heightref_e destref;

	// Destination height.
	float dest;

	// -AJA- 2001/05/28: This specifies the other height used.
	heightref_e otherref;
	float other;

    // Floor texture to change to.
	lumpname_c tex;

	// PLAT/DOOR Specific: Time to wait before returning.
	int wait;
	int prewait;

    // Up/Down/Stop sfx
	sfx_t *sfxstart, *sfxup, *sfxdown, *sfxstop;

	// Scrolling. -AJA- 2000/04/16
	angle_t scroll_angle;
	float scroll_speed;
};

// --> Elevator definition class 
class elevatordef_c
{
public:
	elevatordef_c();
	elevatordef_c(elevatordef_c &rhs);
	~elevatordef_c();
	
private:
	void Copy(elevatordef_c &src);

public:
	void Default(void);
	elevatordef_c& operator=(elevatordef_c &rhs);

	// Type of floor: raise/lower/etc
	movetype_e type;
  
    // How fast the elevator moves.
	float speed_up;
	float speed_down;

    // Wait times.
	int wait;       
	int prewait;

    // Up/Down/Stop sfx
	sfx_t *sfxstart, *sfxup, *sfxdown, *sfxstop;  
};

// --> Donut definition class

// FIXME Move inside sliding_door_c?
typedef enum
{
	// not a slider
	SLIDE_None = 0,

	// door slides left (when looking at the right side)
	SLIDE_Left,

	// door slides right (when looking at the right side)
	SLIDE_Right,

	// door opens from middle
	SLIDE_Center
}
slidetype_e;

// --> Sliding Door Definition

//
// Thin Sliding Doors
//
// -AJA- 2000/08/05: added this.
//
class sliding_door_c
{
public:
	sliding_door_c();
	sliding_door_c(sliding_door_c &rhs);
	~sliding_door_c();
	
private:
	void Copy(sliding_door_c &src);
	
public:
	void Default(void);
	sliding_door_c& operator=(sliding_door_c &rhs);
	
	// type of slider, normally SLIDE_None
	slidetype_e type;

	// how fast it opens/closes
	float speed;

	// time to wait before returning (in tics).  Note: door stays open
	// after the last activation.
	int wait;

	// whether or not the texture can be seen through
	bool see_through;

	// how far it actually opens (usually 100%)
	percent_t distance;

	// sound effects.
	sfx_t *sfx_start;
	sfx_t *sfx_open;
	sfx_t *sfx_close;
	sfx_t *sfx_stop;
};

// --> Donut definition class

class donutdef_c
{
public:
	donutdef_c();
	donutdef_c(donutdef_c &rhs);
	~donutdef_c();
	
private:
	void Copy(donutdef_c &src);
	
public:
	void Default(void);
	donutdef_c& operator=(donutdef_c &rhs);
	
	// Do Donut?

	//
	// FIXME! Make the objects that use this require
	//        a pointer/ref. This becomes an
	//        therefore becomes an unnecessary entry
	//
	bool dodonut;

	// FIXME! Strip out the d_ since we're not trying to
	// to differentiate them now?
	 
	// SFX for inner donut parts
	sfx_t *d_sfxin, *d_sfxinstop;

	// SFX for outer donut parts
	sfx_t *d_sfxout, *d_sfxoutstop;
};

// -AJA- 1999/07/12: teleporter special flags.
// FIXME!! Move into teleport def class?
typedef enum
{
	TELSP_None       = 0,
	
	TELSP_Relative   = 0x0001,  // keep same relative angle
	TELSP_SameHeight = 0x0002,  // keep same height off the floor
	TELSP_SameSpeed  = 0x0004,  // keep same momentum
	TELSP_SameOffset = 0x0008,  // keep same X/Y offset along line

    TELSP_SameAbsDir = 0x0010,  // keep same _absolute_ angle (DEPRECATED)
    TELSP_Rotate     = 0x0020,  // rotate by target angle     (DEPRECATED)

	TELSP_Line       = 0x0100,  // target is a line (not a thing)
	TELSP_Flipped    = 0x0200,  // pretend target was flipped 180 degrees
	TELSP_Silent     = 0x0400   // no fog or sound
}
teleportspecial_e;

// --> Teleport def class

class teleportdef_c
{
public:
	teleportdef_c();
	teleportdef_c(teleportdef_c &rhs);
	~teleportdef_c();
	
private:
	void Copy(teleportdef_c &src);
	
public:
	void Default(void);
	teleportdef_c& operator=(teleportdef_c &rhs);

	// If true, teleport activator
	//
	// FIXME! Make the objects that use this require
	//        a pointer/ref. This
	//        therefore becomes an unnecessary entry
	//
	bool teleport;

  	// effect object spawned when going in...
	const mobjtype_c *inspawnobj;	// FIXME! Do mobjtypes.Lookup()?
	epi::strent_c inspawnobj_ref;

  	// effect object spawned when going out...
	const mobjtype_c *outspawnobj;	// FIXME! Do mobjtypes.Lookup()?
	epi::strent_c outspawnobj_ref;

  	// Teleport delay
	int delay;

	// Special flags.
	teleportspecial_e special;
};

// Light Specials
// FIXME!! Speak English(UK)/Move into lightdef_c?
typedef enum
{
	LITE_None,

	// set light to new level instantly
	LITE_Set,

	// fade light to new level over time
	LITE_Fade,

	// flicker like a fire
	LITE_FireFlicker,

	// smoothly fade between bright and dark, continously
	LITE_Glow,

	// blink randomly between bright and dark
	LITE_Flash,

	// blink between bright and dark, alternating
	LITE_Strobe
}
lighttype_e;

// --> Light information class
class lightdef_c
{
public:
	lightdef_c();
	lightdef_c(lightdef_c &rhs);
	~lightdef_c();
	
private:
	void Copy(lightdef_c &src);
	
public:
	void Default(void);
	lightdef_c& operator=(lightdef_c &rhs);
	
	lighttype_e type;

	// light level to change to (for SET and FADE)
	int level;

	// chance value for FLASH type
	percent_t chance;
  
	// time remaining dark and bright, in tics
	int darktime;
	int brighttime;

	// synchronisation time, in tics
	int sync;

	// stepping used for FADE and GLOW types
	int step;
};

// --> Ladder definition class (Probably somewhat OTT)
 
class ladderdef_c
{
public:
	ladderdef_c();
	ladderdef_c(ladderdef_c &rhs);
	~ladderdef_c();
	
private:
	void Copy(ladderdef_c &src);

public:
	void Default(void);
	ladderdef_c& operator=(ladderdef_c &rhs);

	// height of ladder itself.  Zero or negative disables.  Bottom of
	// ladder comes from Y_OFFSET on the linedef.
	float height;
};

// FIXME!!! Move inside linetype_c
typedef enum
{
	LINEFX_None = 0x0000,

	// make tagged lines (inclusive) 50% translucent
	LINEFX_Translucency = 0x0001,

	// make tagged walls (inclusive) scroll using vector
	LINEFX_VectorScroll = 0x0002,

	// make source line scroll using sidedef offsets
	LINEFX_OffsetScroll = 0x0004,

	// experimental: tagged walls (inclusive) scaling & skewing
	LINEFX_Scale = 0x0010,
	LINEFX_Skew  = 0x0020,

	// experimental: transfer properties to tagged walls (incl)
	LINEFX_LightWall = 0x0040,

	// experimental: make tagged lines (exclusive) non-blocking
	LINEFX_UnblockThings = 0x0100,

	// experimental: make tagged lines (incl) block bullets/missiles
	LINEFX_BlockShots = 0x0200,

	// experimental: make tagged lines (incl) block monster sight
	LINEFX_BlockSight = 0x0400
}
line_effect_type_e;

// FIXME!!! Move inside linetype_c
typedef enum
{
	SECTFX_None = 0x0000,

	// transfer sector lighting to tagged floors/ceilings
	SECTFX_LightFloor   = 0x0001,
	SECTFX_LightCeiling = 0x0002,

	// make tagged floors/ceilings scroll
	SECTFX_ScrollFloor   = 0x0004,
	SECTFX_ScrollCeiling = 0x0008,

	// push things on tagged floor
	SECTFX_PushThings = 0x0010,

	// restore light/scroll/push in tagged floors/ceilings
	SECTFX_ResetFloor   = 0x0040,
	SECTFX_ResetCeiling = 0x0080,

	// experimental: set floor/ceiling texture scale
	SECTFX_ScaleFloor   = 0x0100,
	SECTFX_ScaleCeiling = 0x0200,

	// experimental: align floor/ceiling texture to line
	SECTFX_AlignFloor   = 0x0400,
	SECTFX_AlignCeiling = 0x0800,

	// set various force parameters
	SECTFX_SetFriction  = 0x1000,
	SECTFX_WindForce    = 0x2000,
	SECTFX_CurrentForce = 0x4000,
	SECTFX_PointForce   = 0x8000

}
sector_effect_type_e;

// -AJA- 1999/10/12: Generalised scrolling parts of walls.
// FIXME!!! Move inside linetype_c
typedef enum
{
	SCPT_None  = 0x0000,

	SCPT_RightUpper  = 0x0001,
	SCPT_RightMiddle = 0x0002,
	SCPT_RightLower  = 0x0004,

	SCPT_LeftUpper  = 0x0010,
	SCPT_LeftMiddle = 0x0020,
	SCPT_LeftLower  = 0x0040,

	SCPT_LeftRevX   = 0x0100,
	SCPT_LeftRevY   = 0x0200
}
scroll_part_e;

#define SCPT_RIGHT  ((scroll_part_e)(SCPT_RightUpper | SCPT_RightMiddle | SCPT_RightLower))
#define SCPT_LEFT   ((scroll_part_e)(SCPT_LeftUpper | SCPT_LeftMiddle | SCPT_LeftLower))

// -AJA- 1999/12/07: Linedef special flags
// FIXME!!! Move inside linetype_c
typedef enum
{
	LINSP_None = 0x0000,

	// player must be able to vertically reach this linedef to press it
	LINSP_MustReach = 0x0001,

	// don't change the texture on other linedefs with the same tag
	LINSP_SwitchSeparate = 0x0002
}
line_special_e;

// --> Line definition type class

class linetype_c
{
public:
	linetype_c();
	linetype_c(linetype_c &rhs);
	~linetype_c();
	
private:
	void Copy(linetype_c &src);

public:
	void CopyDetail(linetype_c &src);
	void Default(void);
	linetype_c& operator=(linetype_c &rhs);
	
	ddf_base_c ddf;
	
    // Linedef will change to this.
	int newtrignum;

	// Determines whether line is shootable/walkable/pushable
	trigger_e type;

	// Determines whether line is acted on by monsters/players/projectiles
	trigacttype_e obj;

	// Keys required to use
	keys_e keys;

	// Number of times this line can be triggered. -1 = Any amount
	int count;

	// Special sector type to change to.  Used to turn off acid
	int specialtype;

	// Crush.  If true, players will be crushed.  If false, obj will stop(/return)
	bool crush;

	// Floor - FIXME!!! Pointer/reference to table?
	movplanedef_c f;

	// Ceiling - FIXME!!! Pointer/reference to table?
	movplanedef_c c;

	// Elevator (moving sector) -ACB- 2001/01/11 - FIXME!!! Pointer/reference to table?
	elevatordef_c e;

	// Donut - FIXME!!! Pointer/reference to table?
	donutdef_c d;

	// Slider - FIXME!!! Pointer/reference to table?
	sliding_door_c s;

	// -AJA- 2001/03/10: ladder linetypes - FIXME!!! Pointer/reference to table?
	ladderdef_c ladder;

	// Teleport - FIXME!!! Pointer/reference to table?
	teleportdef_c t;

	// LIGHT SPECIFIC - FIXME!!! Pointer/reference to table?
	// Things may be added here; start strobing/flashing glowing lights.
	lightdef_c l;

    // EXIT SPECIFIC
	exittype_e e_exit;

	// SCROLLER SPECIFIC
	float s_xspeed;
	float s_yspeed;
	scroll_part_e scroll_parts;

	// -ACB- 1998/09/11 Message handling
	epi::strent_c failedmessage;

	// Colourmap changing
	// -AJA- 1999/07/09: Now uses colmap.ddf
	const colourmap_c *use_colourmap;

	// Property Transfers (FLO_UNUSED if unset)
	float gravity;
	float friction;
	float viscosity;
	float drag;

    // Ambient sound transfer
	sfx_t *ambient_sfx;

	// Activation sound (overrides the switch sound)
	sfx_t *activate_sfx;

	int music;

    // Automatically trigger this line at level start ?
	bool autoline;

	// Activation only possible from right side of line
	bool singlesided;

	// -AJA- 1999/06/21: Extra floor handling
	extrafloordef_c ef;

	// -AJA- 1999/06/30: TRANSLUCENT MID-TEXTURES
	percent_t translucency;

	// -AJA- 1999/10/24: Appearance control.
	when_appear_e appear;

	// -AJA- 1999/12/07: line special flags
	line_special_e special_flags;

	// -AJA- 2000/01/09: enable (if +1) or disable (if -1) all radius
	//       triggers with the same tag as the linedef.
	int trigger_effect;

    // -AJA- 2000/09/28: BOOM compatibility fields (and more !).
	line_effect_type_e line_effect;
	scroll_part_e line_parts;
	sector_effect_type_e sector_effect;
};

// --> Linetype container class

class linetype_container_c : public epi::array_c
{
public:
	linetype_container_c();
	~linetype_container_c();

private:
	void CleanupObject(void *obj);

	linetype_c* lookup_cache[LOOKUP_CACHESIZE];

public:
	// List Management
	int GetSize() {	return array_entries; } 
	int Insert(linetype_c *l) { return InsertObject((void*)&l); }
	linetype_c* Lookup(int num);
	linetype_c* operator[](int idx) { return *(linetype_c**)FetchObject(idx); }
	void Reset();
};

// ------------------------------------------------------------------
// -------------------------SECTOR TYPES-----------------------------
// ------------------------------------------------------------------

// -AJA- 1999/11/25: Sector special flags
typedef enum
{
	SECSP_None = 0x0000,

	// apply damage whenever in whole region (not just touching floor)
	SECSP_WholeRegion = 0x0001,

	// goes with above: damage is proportional to how deep you're in
	// Also affects pushing sectors.
	SECSP_Proportional = 0x0002,

	// push _all_ things, including NOGRAVITY ones
	SECSP_PushAll = 0x0008,

	// the push force is constant, regardless of the mass
	SECSP_PushConstant = 0x0010,

	// breathing support: this sector contains no air.
	SECSP_AirLess = 0x0020,

	// player can swim in this sector
	SECSP_Swimming = 0x0040
}
sector_flag_e;

class sectortype_c
{
public:
	sectortype_c();
	sectortype_c(sectortype_c &rhs);
	~sectortype_c();
	
private:
	void Copy(sectortype_c &src);
	
public:
	void CopyDetail(sectortype_c &src);
	void Default(void);
	sectortype_c& operator=(sectortype_c &rhs);
	
	// Sector's name, number, etc...
	ddf_base_c ddf;

    // This sector gives you secret count
	bool secret;
	bool crush;

	// Gravity
	float gravity;
	float friction;
	float viscosity;
	float drag;

	// Movement
	movplanedef_c f, c;

	// Elevator: Added -ACB- 2001/01/11
	elevatordef_c e;

	// Lighting
	lightdef_c l;

	// Slime
	damage_c damage;

	// -AJA- 1999/11/25: sector special flags
	sector_flag_e special_flags;

	// Exit.  Also disables god mode.
	exittype_e e_exit;

	// Colourmap changing
	// -AJA- 1999/07/09: Now uses colmap.ddf
	const colourmap_c *use_colourmap;

    // SFX
	sfx_t *ambient_sfx;

	// -AJA- 1999/10/24: Appearance control.
	when_appear_e appear;

	// -AJA- 2000/01/02: DDF-itisation of crushers.
	int crush_time;
	float crush_damage;

    // -AJA- 2000/04/16: Pushing (fixed direction).
	float push_speed;
	float push_zspeed;
	angle_t push_angle;
};

// --> Sectortype container class

class sectortype_container_c : public epi::array_c
{
public:
	sectortype_container_c();
	~sectortype_container_c();

private:
	void CleanupObject(void *obj);

	sectortype_c* lookup_cache[LOOKUP_CACHESIZE];

public:
	// List Management
	int GetSize() {	return array_entries; } 
	int Insert(sectortype_c *s) { return InsertObject((void*)&s); }
	sectortype_c* Lookup(int num);
	sectortype_c* operator[](int idx) { return *(sectortype_c**)FetchObject(idx); }
	void Reset();
};

// Misc playsim constants
#define CEILSPEED   		1.0f
#define FLOORSPEED  		1.0f

#define GRAVITY     		8.0f
#define FRICTION    		0.9063f
#define VISCOSITY   		0.0f
#define DRAG        		0.99f
#define RIDE_FRICTION    	0.7f
#define LADDER_FRICTION  	0.8f

#define STOPSPEED   		0.15f
#define OOF_SPEED   		20.0f

// ----------------------------------------------------------------
// -------------------------MUSIC PLAYLIST-------------------------
// ----------------------------------------------------------------

// Playlist entry class

// FIXME: Move enums in pl_entry_c class?
typedef enum
{
	MUS_UNKNOWN   = 0,
	MUS_CD        = 1,
	MUS_MIDI      = 2,
	MUS_MUS       = 3,
	MUS_OGG       = 4,
	MUS_MP3       = 5,
	ENDOFMUSTYPES = 6
}
musictype_t;

typedef enum
{
	MUSINF_UNKNOWN   = 0,
	MUSINF_TRACK     = 1,
	MUSINF_LUMP      = 2,
	MUSINF_FILE      = 3,
	ENDOFMUSINFTYPES = 4
}
musicinftype_e;

class pl_entry_c
{
public:
	pl_entry_c();
	pl_entry_c(pl_entry_c &rhs);
	~pl_entry_c();

private:
	void Copy(pl_entry_c &src);

public:
	void CopyDetail(pl_entry_c &src);
	void Default(void);
	pl_entry_c& operator=(pl_entry_c &rhs);

	ddf_base_c ddf;
	musictype_t type;
	musicinftype_e infotype;
	epi::strent_c info;
};

// Our playlist entry container
class pl_entry_container_c : public epi::array_c 
{
public:
	pl_entry_container_c() : epi::array_c(sizeof(pl_entry_c*)) {}
	~pl_entry_container_c() { Clear(); } 

private:
	void CleanupObject(void *obj);

public:
	pl_entry_c* Find(int number);
	int GetSize() {	return array_entries; } 
	int Insert(pl_entry_c *p) { return InsertObject((void*)&p); }
	pl_entry_c* operator[](int idx) { return *(pl_entry_c**)FetchObject(idx); } 
};

// ----------------------------------------------------------------
// ------------------------ SOUND EFFECTS -------------------------
// ----------------------------------------------------------------

#define S_CLOSE_DIST    160.0f
#define S_CLIPPING_DIST 1600.0f

// -KM- 1998/10/29
typedef struct sfx_s
{
	int num;
	int sounds[1]; // -ACB- 1999/11/06 Zero based array is not ANSI compliant
	// -AJA- I'm also relying on the [1] within sfxdef_c.
}
sfx_t;

// Bastard SFX are sounds that are hardcoded into the
// code.  They should be removed if at all possible

typedef struct
{
	sfx_t *s;
	char name[8];
}
bastard_sfx_t;

extern bastard_sfx_t bastard_sfx[];

#define sfx_swtchn bastard_sfx[0].s
#define sfx_tink bastard_sfx[1].s
#define sfx_radio bastard_sfx[2].s
#define sfx_oof bastard_sfx[3].s
#define sfx_pstop bastard_sfx[4].s
#define sfx_stnmov bastard_sfx[5].s
#define sfx_pistol bastard_sfx[6].s
#define sfx_swtchx bastard_sfx[7].s
#define sfx_jpmove bastard_sfx[8].s
#define sfx_jpidle bastard_sfx[9].s
#define sfx_jprise bastard_sfx[10].s
#define sfx_jpdown bastard_sfx[11].s
#define sfx_jpflow bastard_sfx[12].s

#define sfx_None (sfx_t*) NULL

// Sound Effect Definition Class
class sfxdef_c
{
public:
	sfxdef_c();
	sfxdef_c(sfxdef_c &rhs);
	~sfxdef_c();
	
private:
	void Copy(sfxdef_c &src);
	
public:
	void CopyDetail(sfxdef_c &src);
	void Default(void);
	sfxdef_c& operator=(sfxdef_c &rhs);
	
	// sound's name, etc..
	ddf_base_c ddf;

    // full sound lump name
	lumpname_c lump_name;

	// sfxinfo ID number
	// -AJA- Changed to a sfx_t.  It serves two purposes: (a) hold the
	//       sound ID, like before, (b) better memory usage, as we don't
	//       need to allocate a new sfx_t for non-wildcard sounds.
	sfx_t normal;

    // Sfx singularity (only one at a time), or 0 if not singular
	int singularity;

	// Sfx priority
	int priority;

	// volume adjustment (100% is normal, lower is quieter)
	percent_t volume;

	// -KM- 1998/09/01  Looping: for non NULL origins
	bool looping;

	// -AJA- 2000/04/19: Prefer to play the whole sound rather than
	//       chopping it off with a new sound.
	bool precious;
 
    // distance limit, if the hearer is further away than `max_distance'
    // then the this sound won't be played at all.
	float max_distance;
 };

// Our sound effect definition container
class sfxdef_container_c : public epi::array_c 
{
public:
	sfxdef_container_c() : epi::array_c(sizeof(sfxdef_c*)) { num_disabled = 0; }
	~sfxdef_container_c() { Clear(); } 

private:
	void CleanupObject(void *obj);
	
	int num_disabled;					// Number of disabled 

public:
	// List management
	int GetSize() { return array_entries; } 
	int Insert(sfxdef_c *s) { return InsertObject((void*)&s); }
	sfxdef_c* operator[](int idx) { return *(sfxdef_c**)FetchObject(idx); } 
	
	// Inliners
	int GetDisabledCount() { return num_disabled; }
	void SetDisabledCount(int _num_disabled) { num_disabled = _num_disabled; }
	
	// Lookup functions
	sfx_t* GetEffect(const char *name, bool error = true);
	sfxdef_c* Lookup(const char *name);
};

// ------------------------------------------------------------------
// -------------------------EXTERNALISATIONS-------------------------
// ------------------------------------------------------------------

extern state_t *states;
extern int num_states;

extern atkdef_container_c atkdefs;			// -ACB- 2004/06/09 Implemented
extern gamedef_container_c gamedefs;		// -ACB- 2004/06/21 Implemented
extern linetype_container_c linetypes;		// -ACB- 2004/07/05 Implemented
extern mapdef_container_c mapdefs;			// -ACB- 2004/06/29 Implemented
extern mobjtype_container_c mobjtypes;		// -ACB- 2004/06/07 Implemented
extern pl_entry_container_c playlist;		// -ACB- 2004/06/04 Implemented
extern sectortype_container_c sectortypes;	// -ACB- 2004/07/05 Implemented
extern sfxdef_container_c sfxdefs;			// -ACB- 2004/07/25 Implemented
extern weapondef_container_c weapondefs;	// -ACB- 2004/07/14 Implemented

extern language_c language;				// -ACB- 2004/06/27 Implemented

// -KM- 1998/11/25 Dynamic number of weapons, always 10 weapon keys.
extern weaponkey_c weaponkey[WEAPON_KEYS];	

void DDF_Init(void);
void DDF_CleanUp(void);
bool DDF_MainParseCondition(const char *str, condition_check_t *cond);
void DDF_MainGetWhenAppear(const char *info, void *storage);
void DDF_MainGetRGB(const char *info, void *storage);
bool DDF_MainDecodeBrackets(const char *info, char *outer, char *inner, int buf_len);
const char *DDF_MainDecodeList(const char *info, char divider, bool simple);
void DDF_GetLumpNameForFile(const char *filename, char *lumpname);

int DDF_CompareName(const char *A, const char *B);

bool DDF_CheckSprites(int st_low, int st_high);
void DDF_MobjGetBenefit(const char *info, void *storage);
bool DDF_WeaponIsUpgrade(weapondef_c *weap, weapondef_c *old);

bool DDF_IsBoomLineType(int num);
bool DDF_IsBoomSectorType(int num);
void DDF_BoomClearGenTypes(void);
linetype_c *DDF_BoomGetGenLine(int number);
sectortype_c *DDF_BoomGetGenSector(int number);

// -KM- 1998/12/16 If you have a ddf file to add, call
//  this, passing the data and the size of the data in
//  memory.  Returns false for files which cannot be found.
bool DDF_ReadAtks(void *data, int size);
bool DDF_ReadGames(void *data, int size);
bool DDF_ReadLangs(void *data, int size);
bool DDF_ReadLevels(void *data, int size);
bool DDF_ReadLines(void *data, int size);
bool DDF_ReadMusicPlaylist(void *data, int size);
bool DDF_ReadSectors(void *data, int size);
bool DDF_ReadSFX(void *data, int size);
bool DDF_ReadThings(void *data, int size);
bool DDF_ReadWeapons(void *data, int size);

#endif

//
// $Log: ddf_main.h,v $
// Revision 1.171  2006/09/08 11:00:42  ajapted
// Preliminaries for 'out of the box' BOOM compatibility, including
// removing the Edge/Boom auto-detection crud, removing the compatibility
// menu option, renumbering EDGE sector types, adding an '-ecompat'
// cmd-line option for old mods, and initial implementation for some
// BOOM features (Wind/Current/Point pushers).
//
// Revision 1.170  2006/08/03 15:50:39  ajapted
// New SPAWN_LIMIT command for attacks.ddf, used to limit the
// number of lost souls produced by pain elementals.
//
// Revision 1.169  2006/08/02 04:01:23  ajapted
// Changed #includes from <epi/xxx.h> to "epi/xxx.h"
// (same goes for glbsp and humidity).
//
// Revision 1.168  2006/08/01 12:15:12  ajapted
// Fixes for 64-bit systems (orig by Darren Salt).
//
// Revision 1.167  2005/11/13 15:12:39  darkknight
// Rejig of headers for configure script
//
// Revision 1.166  2005/10/22 21:32:24  darkknight
// Treat epi, glbsp & humidity headers as if in the global space
//
// Revision 1.165  2005/08/23 13:07:57  ajapted
// Implemented new NOTARGET attack special.
//
// Revision 1.164  2005/08/06 06:33:07  ajapted
// DDF control over whether the Berserk powerup lasts the entire level,
// via new PICKUP_EFFECT called "KEEP_POWERUP(BERSERK)".
//
// Revision 1.163  2005/08/06 04:00:26  ajapted
// Fixed problem of Berserk powerup applying to all melee weapons,
// with new Attacks.DDF command BERSERK_MULTIPLY.
//
// Revision 1.162  2005/06/27 08:51:08  darkknight
// Initial new-SFX system commit
//
// Revision 1.161  2005/05/15 13:58:35  ajapted
// Added new HOVER thing special, and support for "Ghosts"
// (certain attacks passing straight through an object).
//
// Revision 1.160  2005/05/14 06:38:39  ajapted
// Added support for loading DDF files specified directly on the
// command line (with the extension ".ddf" or ".ldf").
// For TNT and Plutonia iwads, implemented loading their specific
// language lumps (TNTLANG and PLUTLANG, stored in edge.wad).
//
// Revision 1.159  2005/04/07 03:07:59  ajapted
// Make #CLEARALL in Weapons.ddf affect what you get by cheating.
//
// Revision 1.158  2005/04/02 20:41:47  darkknight
// Removed transluc flag
//
// Revision 1.157  2005/03/12 01:42:37  ajapted
// Replaced new ammo names with plain old AMMO9, AMMO10 ... AMMO16.
//
// Revision 1.156  2005/03/11 03:01:16  ajapted
// Added four new ammo types (total now 16).
//
// Revision 1.155  2005/03/07 11:59:02  darkknight
// Copyright header update
//
// Revision 1.154  2005/03/04 09:13:25  ajapted
// Fixed demos when using -nomonsters (the flag wasn't written).
//
// Revision 1.153  2005/02/26 11:32:31  ajapted
// Comment update.
//
// Revision 1.152  2005/02/19 08:12:17  darkknight
// crc & md5 files now math_ prefixed
//
// Revision 1.151  2005/02/18 16:28:21  darkknight
// id/log test 2
//
//
