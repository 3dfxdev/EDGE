//----------------------------------------------------------------------------
//  EDGE Rendering Definitions Header
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

// -AJA- Don't like imposing this limit...
#define MAXOPENGAPS   12

//
// INTERNAL MAP TYPES
//  used by play and refresh
//

//
// Your plain vanilla vertex.
// Note: transformed values not buffered locally, like some
// DOOM-alikes ("wt", "WebView") did.
//
typedef struct
{
  float_t x, y;
}
vertex_t;

// Forward of LineDefs, for Sectors.
struct line_s;
struct side_s;
struct region_properties_s;

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

  // pushing sector information (normally 0)
  float_t x_push, y_push, z_push;

  // ... thinker stuff (for glowing lights) ...
}
region_properties_t;

// Plane Info
//
// Stores information about a single plane, and is used for floors and
// ceilings of sectors.
//
// -AJA- 1999/10/09: added this.
//
typedef struct plane_info_s
{
  float_t h;
  const struct image_s *image;

  float_t x_offset;
  float_t y_offset;
  float_t translucency;

  // current scrolling deltas (normally 0)
  float_t x_scroll, y_scroll;

  // texture scale.  Normal is 64.0, 32.0 is half the size, 128.0 is
  // twice the normal size (i.e. alignment on 128x128 blocks).
  float_t scale;
 
  // texture angle (normally 0)
  angle_t angle;
  
  // lighting override (as in BOOM).  Usually NULL.
  struct region_properties_s *override_p;

  // -ACB- 2001/01/29 Added
  void *movedata;
}
plane_info_t;

// Side Part
//
// Stores information about a single part of a sidedef/wall (lower,
// middle or upper).
//
// -AJA- 1999/10/09: added this.
//
typedef struct sidepart_s
{
  // Texture image
  // We do not maintain names here. 
  const struct image_s *image;

  // offsets (horizontal and vertical)
  float_t x_offset;
  float_t y_offset;
  float_t translucency;
  
  // current scrolling deltas (normally 0)
  float_t x_scroll, y_scroll;

  // scaling: 1.0 is normal, 0.5 is half size, 2.0 is twice size.
  float_t scale;

  // skewing: 0.0 is normal, 1.0 is 45 degrees up, -1.0 is down.
  float_t skew;
}
sidepart_t;

// Vertical Region
//
// Stores information about a single vertical region within a sector
// (a single "storey" if you will).
//
// -AJA- 1999/10/09: added this.
//
typedef struct vert_region_s
{
  // links in chain
  // (sorted in order of increasing height)
  struct vert_region_s *sec_next;
  struct vert_region_s *sec_prev;

  // floor and ceiling of the region.  When the containing sector is
  // bogus (having floor height >= ceiling height), then there will
  // also be a single bogus region, otherwise all regions have
  // floor->h <= ceil->h.  NOTE WELL: an empty region is allowed, this
  // can occur when (for example) a thick extrafloor sits exactly on
  // the sector's floor.

  plane_info_t *floor;
  plane_info_t *ceil;

  region_properties_t *p;

  // heights: floor->h, ceil->h, and sec_next->floor->h.
  float_t f_h, c_h, top_h;
  
  // extra floor details
  // (NULL if this region is part of the normal sector)
  const extrafloor_t *extrafloor;

  // line for nominal extra side 
  // (between this region's CEILING and the next region's FLOOR)
  struct line_s *extraline;
}
vert_region_t;

// Record of an extrafloor used in a sector.
// Used to construct the region list.
//
// -AJA- 1999/11/23: added this.
//
typedef struct extrafloor_record_s
{
  // link in list
  struct extrafloor_record_s *next;

  // priority, higher values occur later.  We need this since the
  // order that extrafloors get added is significant, for example
  // liquid floors must be adder after solid floors.
  int priority;

  // type of extra floor
  const extrafloor_t *info;

  // linedef responsible (frontsector == control sector)
  struct line_s *line;
}
extrafloor_record_t;

//
// The SECTORS record, at runtime.
//
struct subsector_s;

typedef struct sector_s
{
  // basic plane info.  Note that the region list should be used for
  // most things (esp. rendering).
  plane_info_t floor;
  plane_info_t ceil;

  // properties
  region_properties_t p;

  // tag
  int tag;

  // -AJA- 1999/10/09: Multiple extrafloor code.
  // There is always at least one region, even when the sector is
  // bogus and has floor height >= ceiling height.
  vert_region_t *regions;

  // does this sector control extrafloors in other sectors ?
  boolean_t controller;

  extrafloor_record_t *extrafloor_records;

  // 0 = untraversed, 1,2 = sndlines-1
  int soundtraversed;

  // thing that made a sound (or null)
  mobj_t *soundtarget;

  // mapblock bounding box for height changes
  int blockbox[4];

  // origin for any sounds played by the sector
  degenmobj_t soundorg;

/*
  //
  // thinker_t for reversable actions
  // -KM- 1998/10/29 Separate thinkers for floor + ceiling
  //
  // -AJA- FIXME: ick, clean this up (i.e. move thinkers into
  //       plane_info_t).
  //
  // -ACB- Increased this to 3 ([2] is used for elevators). Agreed that
  //       this is not a pleasent hack.
  //
  void *specialdata[3];
*/

  int linecount;
  struct line_s **lines;  // [linecount] size

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

#define FLOOR    0
#define CEILING  1

//
// The SideDef.
//

typedef struct side_s
{
  sidepart_t top;
  sidepart_t middle;
  sidepart_t bottom;

  // Sector the SideDef is facing.
  sector_t *sector;
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

// Vertical gap between a floor & a ceiling.
// -AJA- 1999/07/19. 

typedef struct
{
  float_t f;  // floor
  float_t c;  // ceiling
}
vgap_t;

typedef struct line_s
{
  // Vertices, from v1 to v2.
  vertex_t *v1;
  vertex_t *v2;

  // Precalculated v2 - v1 for side checking.
  float_t dx;
  float_t dy;

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

  // -AJA- 1999/07/19: Extra floor support.  We now keep track of the
  // gaps between the front & back sectors here, instead of computing
  // them each time in P_LineOpening() -- which got a lot more complex
  // due to extra floors.  Now they only need to be recomputed when
  // one of the sectors changes height.

  int gap_num;
  vgap_t gaps[MAXOPENGAPS];

  // -AJA- 2000/10/01: Sight gaps.
  int s_gap_num;
  vgap_t sight_gaps[MAXOPENGAPS];

  // slider thinker, normally NULL
  void *slider_special;

  // Keep animating lines in a linked list.
  struct line_s *animate_next;
}
line_t;

//
// A SubSector.
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
// The LineSeg.
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

struct visplane_s;

// Drawsegs.
//
// These remember where a wall was drawn on the screen, so that sprites
// can be clipped correctly.  Also stores info for drawing mid-textures
// on 2S linedefs.
//
typedef struct drawseg_s
{
  seg_t *curline;
  int x1;
  int x2;

  float_t scale1;
  float_t scale2;
  float_t scalestep;

  // -ES- 1999/03/24 Added These.
  float_t light1;
  float_t lightstep;

  // 0=none, 1=bottom, 2=top, 3=both
  int silhouette;

  // do not clip sprites above this
  float_t bsilheight;

  // do not clip sprites below this
  float_t tsilheight;

  // texture to use for masked mid.
  sidepart_t *part;
 
  // openings index to lists for sprite clipping,
  //  all three adjusted so [x1] is first value.
  // maskedtexturecol is >=0 when there is a masked mid texture.

  int sprtopclip;
  int sprbottomclip;
  int maskedtexturecol;
}
drawseg_t;

// Patches.
//
// A patch holds one or more columns.
// Patches are used for sprites and all masked pictures,
// and we compose textures from the TEXTURE1/2 lists
// of patches.
//
typedef struct patch_s
{
  short width;  // bounding box size 

  short height;
  short leftoffset;  // pixels to the left of origin 

  short topoffset;  // pixels below the origin 

  int columnofs[8];  // only [width] used

  // the [0] is &columnofs[width] 
}
patch_t;

// A vissprite_t is a thing
//  that will be drawn during a refresh.
// I.e. a sprite object that is partly visible.
// -KM- 1989/10/29 Removed sorting stuff, added colourmap stuff
typedef struct vissprite_s
{
  int x1;
  int x2;

  // for line side calculation
  float_t gx;
  float_t gy;

  // global bottom / top for silhouette clipping
  float_t gz;
  float_t gzt;

  // horizontal position of x1
  float_t startfrac;

  // -ES- 1999/03/22 Renamed to yscale, it is no longer used for anything else
  float_t yscale;

  // negative if flipped
  float_t xiscale;

  // -ES- 1999/03/22 Separate distance variable, for zsorting & light calculations
  float_t distance;

  // -AJA- 1999/08/xx: Scale based on distance, used for clipping against
  //       drawsegs. Not affected by SPRITE_SCALE settings in mobjinfo_t.
  float_t dist_scale;

  float_t texturemid;
  int patch;

  // for colour translation and shadow draw,
  //  maxbright frames as well
  // -AJA- 1999/07/10: Now uses colmap.ddf.
  const coltable_t *coltable;

  int mobjflags;
  const byte *trans_table;

  float_t visibility;

  // -AJA- 1999/06/21: for clipping against extra floors.
  sector_t *sector;
}
vissprite_t;

//      
// Sprites are patches with a special naming convention
//  so they can be recognized by R_InitSprites.
// The base name is NNNNFx or NNNNFxFx, with
//  x indicating the rotation, x = 0, 1-7.
// The sprite and frame specified by a thing_t
//  is range checked at run time.
// A sprite is a patch_t that is assumed to represent
//  a three dimensional object and may have multiple
//  rotations pre drawn.
// Horizontal flipping is used to save space,
//  thus NNNNF2F5 defines a mirrored patch.
// Some sprites will only have one picture used
// for all views: NNNNF0
//
typedef struct
{
  // If false use 0 for any position.
  // Note: as eight entries are available,
  //  we might as well insert the same name eight times.
  boolean_t rotate;

  // Lump to use for view angles 0-7.
  short lump[8];

  // Flip bit (1 = flip) to use for view angles 0-7.
  byte flip[8];
}
spriteframe_t;

//
// A sprite definition: a number of animation frames.
//
typedef struct
{
  int numframes;
  spriteframe_t *spriteframes;
}
spritedef_t;

//
// Visplanes.
//
// These are horizontal planes that will be drawn during a refresh.
// Multiple floors (or ceilings) can be merged into the same visplane
// (if they shared the same height, texture, lighting, etc...).
// Conversely, a single sector may require multiple visplanes.
// 
typedef struct visplane_s
{
  float_t height;
  int picnum;
  int lightlevel;

  // -KM- 1998/09/27 Dynamic colourmaps
  // -AJA- 1999/07/08: Now uses colmap.ddf.
  const colourmap_t *colourmap;

  float_t xoffset;
  float_t yoffset;

  int minx;
  int maxx;

  unsigned short *top;
  unsigned short *bottom;
}
visplane_t;

#endif
