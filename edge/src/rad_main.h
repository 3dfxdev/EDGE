//----------------------------------------------------------------------------
//  Radius Trigger Main definitions
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2000  The EDGE Team.
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

#ifndef __RAD_MAIN__
#define __RAD_MAIN__

#include "dm_type.h"
#include "ddf_main.h"
#include "e_player.h"
#include "r_defs.h"


struct rts_state_s;
struct rad_script_s;
struct rad_trigger_s;


typedef struct s_tip_s
{
  // tip text or graphic. two of these must be NULL.
  const char *tip_text;
  char *tip_ldf;
  char *tip_graphic;
  
  // display time, in ticks
  int display_time;

  // play the TINK sound ?
  boolean_t playsound;
}
s_tip_t;


//SpawnThing Function
typedef struct s_thing_s
{
  // If the object is spawned somewhere
  // else on the map.  z can be ONFLOORZ or ONCEILINGZ.
  float_t x;
  float_t y;
  float_t z;

  angle_t angle;
  float_t slope;

  // -AJA- 1999/09/11: since the RSCRIPT lump can be loaded before
  //       DDF* lumps, we can't store a pointer to a mobjinfo_t here
  //       (and the mobjinfos can move about with later additions).
  
  // thing's DDF name, or if NULL, then thing's mapnumber.
  char *thing_name;
  int thing_type;

  boolean_t ambush;
  boolean_t spawn_effect;
}
s_thing_t;


// Radius Damage Player Trigger
typedef struct
{
  float_t damage_amount;
}
s_damagep_t;


// Radius Heal Player Trigger
typedef struct
{
  float_t limit;
  float_t heal_amount;
}
s_healp_t;


// Radius GiveArmour Player Trigger
typedef struct
{
  armour_type_e type;
  float_t limit;
  float_t armour_amount;
}
s_armour_t;


// Radius Damage Monster Trigger
typedef struct s_damage_monsters_s
{
  // type of monster to damage: DDF name, or if NULL, then the
  // monster's mapnumber, or if -1 then ANY monster can be damaged.
  char *thing_name;
  int thing_type;

  // how much damage to do
  float_t damage_amount;
}
s_damage_monsters_t;


// Set Skill
typedef struct
{
  skill_t skill;
  int Respawn;
  boolean_t FastMonsters;
}
s_skill_t;


// Go to map
typedef struct
{
  char *map_name;
}
s_gotomap_t;


//PlaySound Function <SOUNDNO> {<x> <y>}
typedef struct s_sound_s
{
  mobj_t mo;  // Sound Location

  sfx_t *soundid;
}
s_sound_t;


//Sector Vertical movement
typedef struct s_sectorv_s
{
  int secnum;
  float_t increment;

  // Ceiling or Floor
  boolean_t corf;
}
s_sectorv_t;


//Sector Vertical movement
typedef struct s_sectorl_s
{
  int secnum;
  int increment;
}
s_sectorl_t;


// Enable/Disable
typedef struct s_enabler_s
{
  // script to enable/disable.  If script_name is NULL, then `tag' is
  // the tag number to enable/disable.
  char *script_name;
  int tag;

  // true to disable, false to enable
  boolean_t new_disabled;
}
s_enabler_t;


// ActivateLine
typedef struct s_lineactivator_s
{
  // line type
  int typenum;

  // sector tag
  int tag;
}
s_lineactivator_t;


// Jump
typedef struct s_jump_s
{
  // label name
  char *label;

  // state to jump to.  Initially NULL, it is looked up when needed
  // (since the label may be a future reference, we can't rely on
  // looking it up at parse time).
  struct rts_state_s *cache_state;

  // chance (from 0 -> 256 means never -> always) that the jump is
  // taken.
  int random_chance;
}
s_jump_t;


// Exit
typedef struct s_exit_s
{
  // exit time, in tics
  int exittime;
}
s_exit_t;


// SaveGame
typedef struct s_savegame_s
{
  // slot number, negative for quicksave
  int slot;
}
s_savegame_t;


// A single RTS action, not unlike the ones for DDF things.  (In fact,
// they may merge at some point in the future).
//
// -AJA- 1999/10/23: added this.

typedef struct rts_state_s
{
  // link in list of states
  struct rts_state_s *next;
  struct rts_state_s *prev;

  // duration in tics
  int tics;

  // routine to be performed
  void (*action)(struct rad_trigger_s *trig, mobj_t *actor, void *param);

  // parameter for routine, or NULL
  void *param;

  // state's label, or NULL
  char *label;
}
rts_state_t;


// ONDEATH info
typedef struct s_ondeath_s
{
  // next in link (order is unimportant)
  struct s_ondeath_s *next;

  // thing's DDF name, or if NULL, then thing's mapnumber.
  char *thing_name;
  int thing_type;

  // threshhold: number of things still alive before the trigger can
  // activate.  Defaults to zero (i.e. all of them must be dead).
  int threshhold;
  
  // mobjinfo pointer, computed the first time this ONDEATH condition
  // is tested.
  const mobjinfo_t *cached_info;
}
s_ondeath_t;


// ONHEIGHT info
typedef struct s_onheight_s
{
  // next in link (order is unimportant)
  struct s_onheight_s *next;

  // height range, trigger won't activate until sector's floor is
  // within this range (inclusive).
  float_t z1, z2;

  // sector number, < 0 means use the trigger's location
  int sec_num;

  // sector pointer, computed the first time this ONHEIGHT condition
  // is tested.
  sector_t *cached_sector;
}
s_onheight_t;


// Trigger Definition (Made up of actions)
// Start_Map & Radius_Trigger Declaration
typedef struct rad_script_s
{
  // link in list
  struct rad_script_s *next;
  struct rad_script_s *prev;

  // Which map
  char *mapid;

  // When appears
  when_appear_e appear;

  int min_players;
  int max_players;

  // Trigger size
  float_t radius;

  // Map Coordinates
  float_t x;
  float_t y;
  float_t z1;
  float_t z2;
  float_t z_mid;

  // Script name (or NULL)
  char *script_name;

  // Script tag (or 0 for none)
  int tag;

  // Initially disabled ?
  boolean_t tagged_disabled;

  // Check for use.
  boolean_t tagged_use;

  // Continues working ?
  boolean_t tagged_independent;

  // Requires no player intervention ?
  boolean_t tagged_immediate;

  // Tagged_Repeat info (normal if repeat_count < 0)
  int repeat_count;
  int repeat_delay;

  // Optional conditions...
  s_ondeath_t *boss_trig;
  s_onheight_t *height_trig;

  char *next_in_path;

  // Set of states
  rts_state_t *first_state;
  rts_state_t *last_state;
}
rad_script_t;


// Dynamic Trigger info.
// Goes away when trigger is finished.
typedef struct rad_trigger_s
{
  // link in list
  struct rad_trigger_s *next;
  struct rad_trigger_s *prev;
  
  // link for triggers with the same tag
  struct rad_trigger_s *tag_next;
  struct rad_trigger_s *tag_prev;
  
  // parent info of trigger
  rad_script_t *info;

  // is it disabled ?
  boolean_t disabled;

  // has it been activated yet?
  boolean_t activated;

  // repeat info
  int repeats_left;

  // current state info
  rts_state_t *state;
  int wait_tics;
}
rad_trigger_t;

#endif
