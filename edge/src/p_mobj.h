//----------------------------------------------------------------------------
//  EDGE Moving Object Header
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2001  The EDGE Team.
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
//
// IMPORTANT NOTE: Altering anything within the mobj_t will most likely
//                 require changes to p_saveg.c and the save-game object
//                 (savegmobj_t); if you experience any problems with
//                 savegames, check here!
//

#ifndef __P_MOBJ__
#define __P_MOBJ__

#include "m_math.h"

#include "dm_data.h"
#include "ddf_main.h"
#include "lu_math.h"

// forward decl.
struct mobj_s;

//
// NOTES: mobj_t
//
// mobj_ts are used to tell the refresh where to draw an image,
// tell the world simulation when objects are contacted,
// and tell the sound driver how to position a sound.
//
// The refresh uses the next and prev links to follow
// lists of things in sectors as they are being drawn.
// The sprite, frame, and angle elements determine which patch_t
// is used to draw the sprite if it is visible.
// The sprite and frame values are allmost always set
// from state_t structures.
//
// The statescr.exe utility generates the states.h and states.c
// files that contain the sprite/frame numbers from the
// statescr.txt source file.
//
// The xyz origin point represents a point at the bottom middle
// of the sprite (between the feet of a biped).
// This is the default origin position for patch_ts grabbed
// with lumpy.exe.
// A walking creature will have its z equal to the floor
// it is standing on.
//
// The sound code uses the x,y, and subsector fields
// to do stereo positioning of any sound effited by the mobj_t.
//
// The play simulation uses the blocklinks, x,y,z, radius, height
// to determine when mobj_ts are touching each other,
// touching lines in the map, or hit by trace lines (gunshots,
// lines of sight, etc).
// The mobj_t->flags element has various bit flags
// used by the simulation.
//
// Every mobj_t is linked into a single sector
// based on its origin coordinates.
// The subsector_t is found with R_PointInSubsector(x,y),
// and the sector_t can be found with subsector->sector.
// The sector links are only used by the rendering code,
// the play simulation does not care about them at all.
//
// Any mobj_t that needs to be acted upon by something else
// in the play world (block movement, be shot, etc) will also
// need to be linked into the blockmap.
// If the thing has the MF_NOBLOCK flag set, it will not use
// the block links. It can still interact with other things,
// but only as the instigator (missiles will run into other
// things, but nothing can run into a missile).
// Each block in the grid is 128*128 units, and knows about
// every line_t that it contains a piece of, and every
// interactable mobj_t that has its origin contained.  
//
// A valid mobj_t is a mobj_t that has the proper subsector_t
// filled in for its xy coordinates and is linked into the
// sector from which the subsector was made, or has the
// MF_NOSECTOR flag set (the subsector_t needs to be valid
// even if MF_NOSECTOR is set), and is linked into a blockmap
// block or has the MF_NOBLOCKMAP flag set.
// Links should only be modified by the P_[Un]SetThingPosition()
// functions.
// Do not change the MF_NO? flags while a thing is valid.
//
// Any questions?
//

//
// Misc. mobj flags
//
typedef enum
{
  // Call P_TouchSpecialThing when touched.
  MF_SPECIAL = 1,

  // Blocks.
  MF_SOLID = 2,

  // Can be hit.
  MF_SHOOTABLE = 4,

  // Don't use the sector links (invisible but touchable).
  MF_NOSECTOR = 8,

  // Don't use the blocklinks (inert but displayable)
  MF_NOBLOCKMAP = 16,

  // Not to be activated by sound, deaf monster.
  MF_AMBUSH = 32,

  // Will try to attack right back.
  MF_JUSTHIT = 64,

  // Will take at least one step before attacking.
  MF_JUSTATTACKED = 128,

  // On level spawning (initial position),
  // hang from ceiling instead of stand on floor.
  MF_SPAWNCEILING = 256,

  // Don't apply gravity (every tic), that is, object will float,
  // keeping current height or changing it actively.
  MF_NOGRAVITY = 512,

  // Movement flags. This allows jumps from high places.
  MF_DROPOFF = 0x400,

  // For players, will pick up items.
  MF_PICKUP = 0x800,

  // Object is not checked when moving, no clipping is used.
  MF_NOCLIP = 0x1000,

  // Player: keep info about sliding along walls.
  MF_SLIDE = 0x2000,

  // Allow moves to any height, no gravity.
  // For active floaters, e.g. cacodemons, pain elementals.
  MF_FLOAT = 0x4000,

  // Instantly cross lines, whatever the height differences may be
  // (e.g. go from the bottom of a cliff to the top).
  // Note: nothing to do with teleporters.
  MF_TELEPORT = 0x8000,

  // Don't hit same species, explode on block.
  // Player missiles as well as fireballs of various kinds.
  MF_MISSILE = 0x10000,

  // Dropped by a demon, not level spawned.
  // E.g. ammo clips dropped by dying former humans.
  MF_DROPPED = 0x20000,

  // Use fuzzy draw (shadow demons or spectres),
  // temporary player invisibility powerup.
  MF_FUZZY = 0x40000,

  // Flag: don't bleed when shot (use puff),
  // barrels and shootable furniture shall not bleed.
  MF_NOBLOOD = 0x80000,

  // Don't stop moving halfway off a step,
  // that is, have dead bodies slide down all the way.
  MF_CORPSE = 0x100000,

  // Floating to a height for a move, ???
  // don't auto float to target's height.
  MF_INFLOAT = 0x200000,

  // On kill, count this enemy object
  // towards intermission kill total.
  // Happy gathering.
  MF_COUNTKILL = 0x400000,

  // On picking up, count this item object
  // towards intermission item total.
  MF_COUNTITEM = 0x800000,

  // Special handling: skull in flight.
  // Neither a cacodemon nor a missile.
  MF_SKULLFLY = 0x1000000,

  // Don't spawn this object
  // in death match mode (e.g. key cards).
  MF_NOTDMATCH = 0x2000000,

  // Monster grows (in)visible at certain times.
  MF_STEALTH = 0x4000000,

  // Used so bots know they have picked up their target item.
  MF_JUSTPICKEDUP = 0x8000000,

  // Object reacts to being touched (often violently :->)
  // -AJA- 1999/08/21: added this.
  MF_TOUCHY = 0x10000000
}
mobjflag_t;

typedef enum
{
  // Act like a big ugly bossman (ignores certain types of damage and
  // makes start and death sound at full volume regardless of location).
  EF_BOSSMAN = 1,

  // Used when varying visibility levels
  EF_LESSVIS = 2,

  // This thing does not respawn
  EF_NORESPAWN = 4,

  // double the chance of object using range attack
  EF_NOGRAVKILL = 8,

  // This thing is not loyal to its own type, fights its own
  EF_DISLOYALTYPE = 16,

  // This thing can be hurt by another thing with same attack
  EF_OWNATTACKHURTS = 32,

  // Used for tracing (homing) projectiles, its the first time
  // this projectile has been checked for tracing if set.
  EF_FIRSTCHECK = 64,

  // This projectile can trace, but if this is set it will not.
  EF_NOTRACE = 128,

  // double the chance of object using range attack
  EF_TRIGGERHAPPY = 256,

  // not targeted by other monsters for damaging them
  EF_NEVERTARGET = 512,

  // Normally most monsters will follow a target which caused them
  // damage for a length of time, even if another object inflicted
  // pain upon them; with this enabled, they will not hold the grudge
  // and switch targets to the other object that has caused them the
  // more recent pain.
  EF_NOGRUDGE = 1024,

  // This object is dummy, used for carring a dummy set of co-ordinates for
  // use as a target.
  EF_DUMMYMOBJ = 2048,

  // Archvile cannot resurrect this monster
  EF_NORESURRECT = 4096,

  // Object bounces
  EF_BOUNCE = 8192,

  // Thing walks along the edge near large dropoffs. 
  // -AJA- 1999/07/21: added this.
  EF_EDGEWALKER = 0x4000,

  // Monster falls with gravity when walks over cliff. 
  // -AJA- 1999/07/21: added this.
  EF_GRAVFALL = 0x8000,

  // Thing can be climbed on-top-of or over. 
  // -AJA- 1999/07/21: added this.
  EF_CLIMBABLE = 0x10000,

  // Thing won't penetrate WATER extra floors. 
  // -AJA- 1999/09/25: added this.
  EF_WATERWALKER = 0x20000,

  // Thing is a monster. 
  // -AJA- 1999/10/02: added this (removed 3006 hack).
  EF_MONSTER = 0x40000,

  // Thing can cross blocking lines.
  // -AJA- 1999/10/02: added this.
  EF_CROSSLINES = 0x80000,

  // Thing is never affected by friction
  // -AJA- 1999/10/02: added this.
  EF_NOFRICTION = 0x100000,

  // Thing is optional, won't exist when -noextra is used.
  // -AJA- 1999/10/07: added this.
  EF_EXTRA = 0x200000,

  // Just bounced, won't enter bounce states until BOUNCE_REARM.
  // -AJA- 1999/10/18: added this.
  EF_JUSTBOUNCED = 0x400000,

  // Thing can be "used" (like linedefs) with the spacebar.  Thing
  // will then enter its TOUCH_STATES (when they exist).
  // -AJA- 2000/02/17: added this.
  EF_USABLE = 0x800000,

  // Thing will block bullets and missiles.  -AJA- 2000/09/29
  EF_BLOCKSHOTS = 0x1000000,

  // Player is currently crouching.  -AJA- 2000/10/19
  EF_CROUCHING = 0x2000000,

  // Missile can tunnel through enemies.  -AJA- 2000/10/23
  EF_TUNNEL = 0x4000000,

  // Marks thing as being a dynamic light.
  EF_DLIGHT = 0x8000000,

  // Thing has been gibbed.
  EF_GIBBED = 0x10000000
}
mobjextendedflag_t;

// Directions
typedef enum
{
  DI_EAST,
  DI_NORTHEAST,
  DI_NORTH,
  DI_NORTHWEST,
  DI_WEST,
  DI_SOUTHWEST,
  DI_SOUTH,
  DI_SOUTHEAST,
  DI_NODIR,
  NUMDIRS
}
dirtype_t;

// Each sector has a degenmobj_t in its center for sound origin
// purposes.
typedef struct
{
  float_t x, y, z;
}
degenmobj_t;

typedef struct
{
  // location on the map.  `z' can take the special values ONFLOORZ
  // and ONCEILINGZ.
  float_t x, y, z;

  // direction thing faces
  angle_t angle;
  float_t slope;

  // type of thing
  const mobjinfo_t *info;

  // certain flags (mainly MF_AMBUSH).
  int flags;
}
spawnpoint_t;

// Map Object definition.
typedef struct mobj_s mobj_t;

struct mobj_s
{
  // Info for drawing: position.
  // NOTE: these three fields must be first, so mobj_t can be used
  // anywhere that degenmobj_t is expected.
  float_t x, y, z;

  // More drawing info: to determine current sprite.
  angle_t angle;  // orientation

  // used to find patch_t and flip value
  spritenum_t sprite;

  // frame and brightness
  short frame, bright;

  // current subsector
  struct subsector_s *subsector;

  // properties from extrafloor the thing is in
  struct region_properties_s *props;

  // The closest interval over all contacted Sectors.
  float_t floorz;
  float_t ceilingz;
  float_t dropoffz;

  // For movement checking.
  float_t radius;
  float_t height;

  // Momentum, used to update position.
  vec3_t mom;

  // Thing's health level
  float_t health;

  // This is the current speed of the object.
  // if fastparm, it is already calculated.
  float_t speed;
  int fuse;

  // If == validcount, already checked.
  int validcount;

  const mobjinfo_t *info;

  // state tic counter
  int tics;
  int tic_skip;

  const state_t *state;
  const state_t *next_state;

  // flags (Old and New)
  int flags;
  int extendedflags;

  // Movement direction, movement generation (zig-zagging).
  dirtype_t movedir;  // 0-7

  // when 0, select a new dir
  int movecount;

  // Reaction time: if non 0, don't attack yet.
  // Used by player to freeze a bit after teleporting.
  int reactiontime;

  // If >0, the target will be chased
  // no matter what (even if shot)
  int threshold;

  // Additional info record for player avatars only.
  struct player_s *player;

  // Player number last looked for.
  int lastlook;

  // For respawning.
  spawnpoint_t spawnpoint;

  float_t origheight;

  // current visibility and target visibility
  float_t visibility;
  float_t vis_target;

  // looking up or down.....
  float_t vertangle;

  // current attack to be made
  const attacktype_t *currentattack;

  // spread count for Ordered spreaders
  int spreadcount;

  // -ES- 1999/10/25 Reference Count. DO NOT TOUCH.
  // All the following mobj references should be set only
  // through P_MobjSetX, where X is the field name. This is useful because
  // it sets the pointer to NULL if the mobj is removed, this protects us
  // from a crash.
  int refcount;

  // source of the mobj, used for projectiles (i.e. the shooter)
  mobj_t * source;

  // target of the mobj
  mobj_t * target;

  // current spawned fire of the mobj
  mobj_t * tracer;

  // if exists, we are supporting/helping this object
  mobj_t * supportobj;
  int side;

  // objects that is above and below this one.  If there were several,
  // then the closest one (in Z) is chosen.  We are riding the below
  // object if the head height == our foot height.  We are being
  // ridden if our head == the above object's foot height.
  //
  mobj_t * above_mo;
  mobj_t * below_mo;

  // these delta values give what position from the ride_em thing's
  // center that we are sitting on.
  float_t ride_dx, ride_dy;

  // -AJA- 1999/09/25: Path support.
  struct rad_script_s *path_trigger;

  // if we're on a ladder, this is the linedef #, otherwise -1.
  int on_ladder;
  
  float_t dlight_qty;
  float_t dlight_target;

  // hash values for TUNNEL missiles
  unsigned long tunnel_hash[2];

  // touch list: sectors this thing is in or touches
  struct touch_node_s *touch_sectors;

  // linked list (mobjlisthead)
  mobj_t *next, *prev;

  // Interaction info, by BLOCKMAP.
  // Links in blocks (if needed).
  mobj_t *bnext, *bprev;

  // More list: links in subsector (if needed)
  mobj_t *snext, *sprev;

  // One more: link in dynamic light blockmap
  mobj_t *dlnext, *dlprev;
};

// Item-in-Respawn-que Structure -ACB- 1998/07/30
typedef struct iteminque_s
{
  spawnpoint_t spawnpoint;
  int time;
  struct iteminque_s *next;
  struct iteminque_s *prev;
}
iteminque_t;

#endif  // __P_MOBJ__
