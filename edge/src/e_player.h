//----------------------------------------------------------------------------
//  EDGE Player Definition
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
//
//  Based on the DOOM source code, released by Id Software under the
//  following copyright:
//
//    Copyright (C) 1993-1996 by id Software, Inc.
//
//----------------------------------------------------------------------------

#ifndef __E_PLAYER_H__
#define __E_PLAYER_H__

// The player data structure depends on a number
// of other structs: items (internal inventory),
// animation states (closely tied to the sprites
// used to represent them, unfortunately).
#include "p_weapon.h"

// In addition, the player is just a special
// case of the generic moving object/actor.
#include "p_mobj.h"

// Finally, for odd reasons, the player input
// is buffered within the player data struct,
// as commands per game tick.
#include "e_ticcmd.h"

#include "ddf_main.h"  // colmap.ddf stuff

// Networking and tick handling related.
#define BACKUPTICS 12

//
// Player states.
//
typedef enum
{
  // Playing or camping.
  PST_LIVE,

  // Dead on the ground, view follows killer.
  PST_DEAD,

  // Ready to restart/respawn???
  PST_REBORN
}
playerstate_t;

//
// Player internal flags, for cheats and debug.
//
typedef enum
{
  // No clipping, walk through barriers.
  CF_NOCLIP = 1,

  // No damage, no health loss.
  CF_GODMODE = 2,

  // Not really a cheat, just a debug aid.
  CF_NOMOMENTUM = 4

}
cheat_t;

typedef struct
{
  // amount of ammo available
  int num;

  // maximum ammo carryable
  int max;
}
playerammo_t;

typedef enum
{
  // (for pending_wp only) no change is occuring
  WPSEL_NoChange = -2,

  // absolutely no weapon at all
  WPSEL_None = -1
}
weapon_selection_e;

//
// Extended player object info: player_t
//
typedef struct player_s
{
  mobj_t *mo;
  playerstate_t playerstate;
  ticcmd_t cmd;

  int pnum;

  // Determine POV,
  //  including viewpoint bobbing during movement.
  // Focal origin above r.z
  float_t viewz;

  // Base height above floor for viewz.
  float_t viewheight;

  // Bob/squat speed.
  float_t deltaviewheight;

  // bounded/scaled total momentum.
  float_t bob;

  // Kick offset for vertangle (in mobj_t)
  float_t kick_offset;
  
  // This is only used between levels,
  // mo->health is used during levels.
  float_t health;
  float_t armourpoints;

  // Armour type
  armour_type_e armourtype;

  // Power ups. invinc and invis are tic counters.
  float_t powers[NUMPOWERS];
  boolean_t cards[NUMCARDS];

  // Frags, kills of other players.
  int frags;
  int totalfrags;

  // weapons, either an index into the player->weapons[] array, or one
  // of the WPSEL_* values.
  weapon_selection_e ready_wp;
  weapon_selection_e pending_wp;

  // -KM- 1998/11/25 Dynamic allocation here.
  // -AJA- 1999/08/11: Now uses playerweapon_t.
  playerweapon_t *weapons;

  // current weapon choice for each key (1..9 and 0)
  int key_choices[10];
  
  // for status bar: which numbers (2..7) to light up
  boolean_t avail_weapons[6];
  
  // ammunition, one for each ammotype_t (except AM_NoAmmo)
  playerammo_t *ammo;

  // True if button down last tic.
  int attackdown;
  int secondatk_down;
  int usedown;

  // Bit flags, for cheats and debug.
  // See cheat_t, above.
  int cheats;

  // Refired shots are less accurate.
  int refire;

  // For intermission stats.
  int killcount;
  int itemcount;
  int secretcount;

  // For screen flashing (red or bright).
  int damagecount;
  int bonuscount;

  // Who did damage (NULL for floors/ceilings).
  mobj_t *attacker;

  // So gun flashes light up the screen.
  int extralight;
  boolean_t flash;

  // -AJA- 1999/07/10: changed for colmap.ddf.
  const colourmap_t *effect_colourmap;
  float_t effect_strength;

  // Overlay view sprites (gun, etc).
  pspdef_t psprites[NUMPSPRITES];

  // Current PSP for action
  int action_psp;

  // Implements a wait counter to prevent use jumping again
  // -ACB- 1998/08/09
  int jumpwait;

  // how many tics to grin :-)
  int grin_count;

  // used (and updated) by the status bar to gauge amount of pain
  float_t old_health;

  // how many tics player has been attacking (for rampage face)
  int attackdown_count;
  
  // status bar: used to choose which face to show
  int face_index;
  int face_count;

  // is this player currently playing ?
  boolean_t in_game;

  // -AJA- 1999/08/10: This field is the state number which is
  // remembered for WEAPON_NOFIRE_RETURN when the player lets go of
  // the button.  -1 if not yet fired or after changing weapons.
  int remember_atk1;
  int remember_atk2;

  short consistency[BACKUPTICS];
  ticcmd_t netcmds[BACKUPTICS];
  int netnode;

  // This function will be called by P_PlayerThink to initialise
  // the ticcmd_t.
  void (*thinker)(const struct player_s *, void *data, ticcmd_t *dest);
  void *data;
  void (*level_init)(const struct player_s *, void *data);

  // Linked list of in-game players.
  struct player_s *prev;
  struct player_s *next;
}
player_t;

//
// INTERMISSION
// Structure passed e.g. to WI_Start(wb)
//
typedef struct
{
  boolean_t in;  // whether the player is in game

  // Player stats, kills, collected items etc.
  int skills;
  int sitems;
  int ssecret;
  int stime;
  int frags;
  int totalfrags;
  int score;  // current score on entry, modified on return

}
wbplayerstruct_t;

typedef struct
{
  char *level;  // episode # (0-2)

  // previous and next levels, origin 0
  const mapstuff_t *last;
  const mapstuff_t *next;

  int maxkills;
  int maxitems;
  int maxsecret;
  int maxfrags;

  // the par time
  int partime;

  // index of this player in game
  int pnum;

  wbplayerstruct_t *plyr;

}
wbstartstruct_t;

// Player thinkers
void P_ConsolePlayerThinker(const player_t *p, void *data, ticcmd_t *dest);
void P_BotPlayerThinker(const player_t *p, void *data, ticcmd_t *dest);

// This is the only way to create a new player.
player_t *P_AddPlayer(int pnum);

#endif
