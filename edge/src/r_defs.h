//----------------------------------------------------------------------------
//  EDGE Rendering Definitions Header
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

#ifndef __R_DEFS__
#define __R_DEFS__

// Screenwidth.
#include "dm_defs.h"

// Some more or less basic data types
// we depend on.
#include "m_fixed.h"
#include "m_math.h"

// SECTORS do store MObjs anyway.
#include "p_mobj.h"

// -AJA- 1999/07/10: Need this for colourmap_t.
#include "ddf_main.h"

struct image_s;

// Silhouette, needed for clipping Segs (mainly)
// and sprites representing things.
#define SIL_NONE                0
#define SIL_BOTTOM              1
#define SIL_TOP                 2
#define SIL_BOTH                3


//
// INTERNAL MAP TYPES
//  used by play and refresh
//

//
// Your plain vanilla vertex.
// Note: transformed values not buffered locally, like some
// DOOM-alikes ("wt", "WebView") did.
//
typedef vec2_t vertex_t;

// Forward of LineDefs, for Sectors.
struct line_s;
struct side_s;
struct region_properties_s;


//
// Touch Node
//
// -AJA- Used for remembering things that are inside or touching
// sectors.  The idea is blatantly copied from BOOM: there are two
// lists running through each node, (a) list for things, to remember
// what sectors they are in/touch, (b) list for sectors, holding what
// things are in or touch them.
//
// NOTE: we use the same optimisation: in P_UnsetThingPos we just
// clear all the `mo' fields to NULL.  During P_SetThingPos we find
// the first NULL `mo' field (i.e. as an allocation).  The interesting
// part is that we only need to unlink the node from the sector list
// (and relink) if the sector in that node is different.  Thus saving
// work for the common case where the sector(s) don't change.
// 
// CAVEAT: this means that very little should be done in between
// P_UnsetThingPos and P_SetThingPos calls, ideally just load some new
// x/y position.  Avoid especially anything that scans the sector
// touch lists.
//
typedef struct touch_node_s
{
  struct mobj_s *mo;
  struct touch_node_s *mo_next;
  struct touch_node_s *mo_prev;

  struct sector_s *sec;
  struct touch_node_s *sec_next;
  struct touch_node_s *sec_prev;
}
touch_node_t;


//
// Region Properties
//
// Stores the properties that affect each vertical region.
//
// -AJA- 1999/10/09: added this.
//
typedef struct region_properties_s
{
  // rendering related
  int lightlevel;
  const colourmap_t *colourmap;

  // special type (e.g. damaging)
  const specialsector_t *special;

  // -KM- 1998/10/29 Added gravity + friction
  float_t gravity;
  float_t friction;
  float_t viscosity;
  float_t drag;

  // pushing sector information (normally all zero)
  vec3_t push;
}
region_properties_t;

//
// Surface
//
// Stores the texturing information about a single "surface", which is
// either a wall part or a ceiling/floor.  Doesn't include position
// info -- that is elsewhere.
// 
typedef struct surface_s
{
  const struct image_s *image;

  float_t translucency;

  // transformation matrix (usually identity)
  vec2_t x_mat;
  vec2_t y_mat;

  // current offset and scrolling deltas
  vec2_t offset;
  vec2_t scroll;

  // lighting override (as in BOOM).  Usually NULL.
  struct region_properties_s *override_p;
}
surface_t;

//
// ExtraFloor
//
// Stores information about a single extrafloor within a sector.
//
// -AJA- 2001/07/11: added this, replaces vert_region.
//
typedef struct extrafloor_s
{
  // links in chain.  These are sorted by increasing heights, using
  // bottom_h as the reference.  This is important, especially when a
  // liquid extrafloor overlaps a solid one: using this rule, the
  // liquid region will be higher than the solid one.
  // 
  struct extrafloor_s *higher;
  struct extrafloor_s *lower;

  struct sector_s *sector;

  // top and bottom heights of the extrafloor.  For non-THICK
  // extrafloors, these are the same.  These are generally the same as
  // in the dummy sector, EXCEPT during the process of moving the
  // extrafloor.
  //
  float_t top_h, bottom_h;

  // top/bottom surfaces of the extrafloor
  surface_t *top;
  surface_t *bottom;

  // properties used for stuff below us
  region_properties_t *p;

  // type of extrafloor this is.  Only NULL for unused extrafloors.
  // This value is cached pointer to ef_line->special->ef.
  //
  const extrafloor_info_t *ef_info;

  // extrafloor linedef (frontsector == control sector).  Only NULL
  // for unused extrafloors.
  //
  struct line_s *ef_line;

  // link in dummy sector's controlling list
  struct extrafloor_s *ctrl_next;
}
extrafloor_t;

// Vertical gap between a floor & a ceiling.
// -AJA- 1999/07/19. 
//
typedef struct
{
  float_t f;  // floor
  float_t c;  // ceiling
}
vgap_t;


//
// The SECTORS record, at runtime.
//
struct subsector_s;

typedef struct sector_s
{
  // floor and ceiling heights
  float_t f_h, c_h;

  surface_t floor, ceil;

  region_properties_t props;

  int tag;

  // set of extrafloors (in the global `extrafloors' array) that this
  // sector can use.  At load time we can deduce the maximum number
  // needed for extrafloors, even if they dynamically come and go.
  //
  short exfloor_max;
  short exfloor_used;
  extrafloor_t *exfloor_first;

  // -AJA- 2001/07/11: New multiple extrafloor code.
  //
  // Now the FLOORS ARE IMPLIED.  Unlike before, the floor below an
  // extrafloor is NOT stored in each extrafloor_t -- you must scan
  // down to find them, and use the sector's floor if you hit NULL.
  //
  extrafloor_t *bottom_ef;
  extrafloor_t *top_ef;

  // Liquid extrafloors are now kept in a separate list.  For many
  // purposes (especially moving sectors) they otherwise just get in
  // the way.
  //
  extrafloor_t *bottom_liq;
  extrafloor_t *top_liq;

  // properties that are active for this sector (top-most extrafloor).
  // This may be different than the sector's actual properties (the
  // "props" field) due to flooders.
  // 
  region_properties_t *p;
 
  // linked list of extrafloors that this sector controls.  NULL means
  // that this sector is not a controller.
  //
  extrafloor_t *control_floors;
 
  // movement thinkers, for quick look-up
  struct gen_move_s *floor_move;
  struct gen_move_s *ceil_move;

  // 0 = untraversed, 1,2 = sndlines-1
  int soundtraversed;

  // player# that made a sound (starting at 0), or -1
  int sound_player;

  // mapblock bounding box for height changes
  int blockbox[4];

  // origin for any sounds played by the sector
  degenmobj_t soundorg;

  int linecount;
  struct line_s **lines;  // [linecount] size

  // touch list: objects in or touching this sector
  touch_node_t *touch_things;
    
  // sky height for GL renderer
  float_t sky_h;
 
  // keep track of vertical sight gaps within the sector.  This is
  // just a much more convenient form of the info in the extrafloor
  // list.
  // 
  short max_gaps;
  short sight_gap_num;

  vgap_t *sight_gaps;

  // if == validcount, already checked
  int validcount;

  // -AJA- 1999/07/29: Keep sectors with same tag in a list.
  struct sector_s *tag_next;
  struct sector_s *tag_prev;

  // Keep animating sectors in a linked list.
  struct sector_s *animate_next;
 
  // -AJA- 2000/03/30: Keep a list of child subsectors.
  struct subsector_s *subsectors;
}
sector_t;


typedef struct wall_tile_s
{
  // vertical extent of this tile.  The seg determines the horizontal
  // extent.
  // 
  float_t z1, z2;

  // texturing top, in world coordinates
  float_t tex_z;

  // various flags
  int flags;

  // corresponding surface.  NULL if this tile is unused.
  surface_t *surface;
}
wall_tile_t;

#define WTILF_Extra    0x0001
#define WTILF_MidMask  0x0004
#define WTILF_Sky      0x0010
#define WTILF_Slider   0x0020


//
// The SideDef.
//
typedef struct side_s
{
  surface_t top;
  surface_t middle;
  surface_t bottom;

  // Sector the SideDef is facing.
  sector_t *sector;

  // set of tiles used for this side
  short tile_max;
  short tile_used;
  wall_tile_t *tiles;

  // midmasker Y offset
  float_t midmask_offset;
}
side_t;

//
// Move clipping aid for LineDefs.
//
typedef enum
{
  ST_HORIZONTAL,
  ST_VERTICAL,
  ST_POSITIVE,
  ST_NEGATIVE
}
slopetype_t;

//
// LINEDEF
//
typedef struct line_s
{
  // Vertices, from v1 to v2.
  vertex_t *v1;
  vertex_t *v2;

  // Precalculated v2 - v1 for side checking.
  float_t dx;
  float_t dy;
  float_t length;

  // Animation related.
  int flags;
  int tag;
  int count;

  const linedeftype_t *special;

  // Visual appearance: SideDefs.
  // side[1] will be NULL if one sided.
  side_t *side[2];

  // Front and back sector.
  // Note: kinda redundant (could be retrieved from sidedefs), but it
  // simplifies the code.
  sector_t *frontsector;
  sector_t *backsector;

  // Neat. Another bounding box, for the extent of the LineDef.
  float_t bbox[4];

  // To aid move clipping.
  slopetype_t slopetype;

  // if == validcount, already checked
  int validcount;

  // whether this linedef is "blocking" for rendering purposes.
  // Always true for 1s lines.  Always false when both sides of the
  // line reference the same sector.
  //
  boolean_t blocked;

  // -AJA- 1999/07/19: Extra floor support.  We now keep track of the
  // gaps between the front & back sectors here, instead of computing
  // them each time in P_LineOpening() -- which got a lot more complex
  // due to extra floors.  Now they only need to be recomputed when
  // one of the sectors changes height.  The pointer here points into
  // the single global array `vertgaps'.
  //
  short max_gaps;
  short gap_num;

  vgap_t *gaps;

  // slider thinker, normally NULL
  struct slider_move_s *slider_move;

  // Keep animating lines in a linked list.
  struct line_s *animate_next;
}
line_t;

//
// SubSector.
//
// References a Sector.
// Basically, this is a list of LineSegs, indicating the visible walls
// that define all sides of a convex BSP leaf.
//
typedef struct subsector_s
{
  // link in sector list
  struct subsector_s *sec_next;
  
  sector_t *sector;
  struct seg_s *segs;

  // list of mobjs in subsector
  mobj_t *thinglist;

  // pointer to bounding box (usually in parent node)
  float_t *bbox;

  // -- Rendering stuff (only used during rendering) --

  // link in render list (furthest to closest)
  struct subsector_s *rend_next, *rend_prev;

  // here we remember the 1D/2D occlusion buffer (for sprite
  // clipping).
  byte clip_left, clip_right;
  short x_min, x_max;
  struct Y_range_s *ranges;

  // list of floors (sorted into drawing order)
  struct drawfloor_s *floors;

  // floors sorted in height order.
  struct drawfloor_s *z_floors;

  // list of sprites to draw (unsorted).  The sprites on this list are
  // unclipped (both horizontally and vertically).  Later on in the
  // rendering process they are clipped, whereby they get moved into
  // the correct drawfloors.
  struct drawthing_s *raw_things;
}
subsector_t;

//
// The LineSeg
//
// Defines part of a wall that faces inwards on a convex BSP leaf.
//
typedef struct seg_s
{
  vertex_t *v1;
  vertex_t *v2;

  angle_t angle;
  float_t length;

  // link in subsector list.
  // (NOTE: sorted in clockwise order)
  struct seg_s *sub_next;
  
  // -AJA- 1999/12/20: Reference to partner seg, or NULL if the seg
  //       lies along a one-sided line.
  struct seg_s *partner;

  // -AJA- 1999/09/23: Reference to subsector on each side of seg,
  //       back_sub is NULL for one-sided segs.
  //       (Addendum: back_sub is obsolete with new `partner' field)
  subsector_t *front_sub;
  subsector_t *back_sub;
  
  // -AJA- 1999/09/23: For "True BSP rendering", we keep track of the
  //       `minisegs' which define all the non-wall borders of the
  //       subsector.  Thus all the segs (normal + mini) define a
  //       closed convex polygon.  When the `miniseg' field is true,
  //       all the fields below it are unused.
  //
  boolean_t miniseg;

  float_t offset;

  side_t *sidedef;
  line_t *linedef;

  // Sector references.
  // backsector is NULL for one sided lines

  sector_t *frontsector;
  sector_t *backsector;

  // -- Rendering stuff (only used during rendering) --

  boolean_t visible;
  boolean_t back;

  unsigned short x1, x2;
  angle_t angle1, angle2;
  float_t scale1, scale2;
  float_t rw_distance, rw_offset;

  // translated coords
  float_t tx1, tz1;
  float_t tx2, tz2;
  
  // orientation.  (Used for sprite clipping)
  //    0 : not needed for clipping (e.g. parallel to viewplane)
  //   +1 : right side (on screen) faces containing subsector
  //   -1 : left  side (on screen) faces containing subsector
  int orientation;
}
seg_t;

// Partition line.
typedef struct divline_s
{
  float_t x;
  float_t y;
  float_t dx;
  float_t dy;
}
divline_t;

//
// BSP node.
//
typedef struct node_s
{
  divline_t div;
  float_t div_len;

  // If NF_SUBSECTOR its a subsector.
  unsigned short children[2];

  // Bounding boxes for this node.
  float_t bbox[2][4];
}
node_t;

// posts are runs of non masked source pixels
typedef struct
{
  // -1 is the last post in a column
  byte topdelta;

  // length data bytes follows
  byte length;  // length data bytes follows
}
post_t;

// column_t is a list of 0 or more post_t, (byte)-1 terminated
typedef post_t column_t;


//
// OTHER TYPES
//

// Lighttables.
//
// Each value ranges from 0 to 255, from totally black to fully lighted.
// The values get scaled to fit the colourmap being used (e.g. divided
// by 8 for the 32-level COLORMAP lump).
//
typedef byte lighttable_t;

// Coltables.
//
// Each coltable is 256 bytes from some colourmap lump, which maps the
// index colour to a palette colour.  In 16-bit mode however, it is 256
// shorts which are the pixel values for the screen.
//
typedef byte coltable_t;

//      
// Sprites are patches with a special naming convention so they can be
// recognized by R_InitSprites.  The base name is NNNNFx or NNNNFxFx,
// with x indicating the rotation, x = 0, 1-15.
//
// Horizontal flipping is used to save space, thus NNNNF2F5 defines a
// mirrored patch (F5 is the mirrored one).
//
// Some sprites will only have one picture used for all views: NNNNF0.
// In that case, the `rotated' field is false.
//
typedef struct spriteframe_s
{
  // whether this frame has been completed.  Completed frames cannot
  // be replaced by sprite lumps in older wad files.
  // 
  byte finished;
  
  // if not rotated, we don't have to determine the angle for the
  // sprite.  This is an optimisation.
  // 
  byte rotated;
  
  // Flip bits (1 = flip) to use for view angles 0-15.
  byte flip[16];
  
  // normally zero, will be 1 if the [9ABCDEFG] rotations are used.
  byte extended;
    
  // Images for each view angle 0-15.  Never NULL.
  const struct image_s *images[16];
}
spriteframe_t;

// utility macro.
#define ANG_2_ROT(angle)  ((angle_t)(angle) >> (ANGLEBITS-4))

//
// A sprite definition: a number of animation frames.
//
typedef struct spritedef_s
{
  // four letter sprite name (e.g. "TROO").
  char name[6];
  
  // total number of frames.  Zero for missing sprites.
  int numframes;

  // sprite frames.
  spriteframe_t *frames;
}
spritedef_t;

#endif  // __R_DEFS__
