//----------------------------------------------------------------------------
//  EDGE Data Definition File Code (Main)
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

#ifndef __DDF_WEAPON_H__
#define __DDF_WEAPON_H__

#include "epi/utility.h"

#include "types.h"
#include "states.h"


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
	~weapondef_c();
	
public:
	void Default(void);
	void CopyDetail(weapondef_c &src);

	// Weapon's name, etc...
	epi::strent_c name;

	atkdef_c *attack[2];	// Attack type used.
  
	ammotype_e ammo[2];		// Type of ammo this weapon uses.
	int ammopershot[2];		// Ammo used per shot.
	int clip_size[2];		// Amount of shots in a clip (if <= 1, non-clip weapon)
	bool autofire[2];		// If true, this is an automatic else it's semiauto 

	float kick;				// Amount of kick this weapon gives
  
	// range of states used
	state_group_t state_grp;
  
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
	struct sfx_s *idle;
  
  	// Played while the trigger is held (chainsaw)
	struct sfx_s *engaged;
  
	// Played while the trigger is held and it is pointed at a target.
	struct sfx_s *hit;
  
	// Played when the weapon is selected
	struct sfx_s *start;
  
	// Misc sounds
	struct sfx_s *sound1;
	struct sfx_s *sound2;
	struct sfx_s *sound3;
  
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

	// -AJA- 2007/11/12: clip is shared between 1st/2nd attacks.
	bool shared_clip;

	// controls for weapon bob (up & down) and sway (left & right).
	// Given as percentages in DDF.
	percent_t bobbing;
	percent_t swaying;

	// -AJA- 2004/11/15: idle states (polish weapon, crack knuckles)
	int idle_wait;
	percent_t idle_chance;

	int model_skin;  // -AJA- 2007/10/16: MD2 model support
	float model_aspect;
	float model_bias;
	float model_forward;
	float model_side;

public:
	inline int KeyPri(int idx) const  // next/prev order value
	{
		int key = 1 + MAX(-1, MIN(10, bind_key));
		int pri = 1 + MAX(-1, MIN(900, priority));

		return (pri * 20 + key) * 100 + idx;
	}

private:
	// disable copy construct and assignment operator
	explicit weapondef_c(weapondef_c &rhs) { }
	weapondef_c& operator= (weapondef_c &rhs) { return *this; }
};


class weapondef_container_c : public epi::array_c
{
public:
	weapondef_container_c();
	~weapondef_container_c();

private:
	void CleanupObject(void *obj);

public:
	// List Management
	int GetSize() {	return array_entries; } 
	int Insert(weapondef_c *w) { return InsertObject((void*)&w); }
	
	weapondef_c* operator[](int idx) 
	{ 
		return *(weapondef_c**)FetchObject(idx); 
	} 

	// Search Functions
	int FindFirst(const char *name, int startpos = -1);
	weapondef_c* Lookup(const char* refname);
};


// -------EXTERNALISATIONS-------

extern weapondef_container_c weapondefs;	// -ACB- 2004/07/14 Implemented

bool DDF_ReadWeapons(void *data, int size);

#endif // __DDF_WEAPON_H__

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
