//------------------------------------------------------------------------
// STRUCT : Doom structures, raw on-disk layout
//------------------------------------------------------------------------
//
//  LevNamer (C) 2001 Andrew Apted
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
//------------------------------------------------------------------------

#ifndef __LEVNAMER_STRUCTS_H__
#define __LEVNAMER_STRUCTS_H__

#include "system.h"


/* ----- The wad structures ---------------------- */

// wad header

typedef struct raw_wad_header_s
{
  char type[4];

  uint32_g num_entries;
  uint32_g dir_start;
}
raw_wad_header_t;


// directory entry

typedef struct raw_wad_entry_s
{
  uint32_g start;
  uint32_g length;

  char name[8];
}
raw_wad_entry_t;


// blockmap

typedef struct raw_blockmap_header_s
{
  sint16_g x_origin, y_origin;
  sint16_g x_blocks, y_blocks;
}
raw_blockmap_header_t;


/* ----- The level structures ---------------------- */

typedef struct raw_vertex_s
{
  sint16_g x, y;
}
raw_vertex_t;

typedef struct raw_v2_vertex_s
{
  sint32_g x, y;
}
raw_v2_vertex_t;


typedef struct raw_linedef_s
{
  uint16_g start;     // from this vertex...
  uint16_g end;       // ... to this vertex
  uint16_g flags;     // linedef flags (impassible, etc)
  uint16_g type;      // linedef type (0 for none, 97 for teleporter, etc)
  sint16_g tag;       // this linedef activates the sector with same tag
  uint16_g sidedef1;  // right sidedef
  uint16_g sidedef2;  // left sidedef (only if this line adjoins 2 sectors)
}
raw_linedef_t;

typedef struct raw_hexen_linedef_s
{
  uint16_g start;        // from this vertex...
  uint16_g end;          // ... to this vertex
  uint16_g flags;        // linedef flags (impassible, etc)
  uint8_g  type;         // linedef type
  uint8_g  specials[5];  // hexen specials
  uint16_g sidedef1;     // right sidedef
  uint16_g sidedef2;     // left sidedef
}
raw_hexen_linedef_t;

#define LINEFLAG_TWO_SIDED  4

#define HEXTYPE_POLY_START     1
#define HEXTYPE_POLY_EXPLICIT  5


typedef struct raw_sidedef_s
{
  sint16_g x_offset;  // X offset for texture
  sint16_g y_offset;  // Y offset for texture

  char upper_tex[8];  // texture name for the part above
  char lower_tex[8];  // texture name for the part below
  char mid_tex[8];    // texture name for the regular part

  uint16_g sector;    // adjacent sector
}
raw_sidedef_t;


typedef struct raw_sector_s
{
  sint16_g floor_h;   // floor height
  sint16_g ceil_h;    // ceiling height

  char floor_tex[8];  // floor texture
  char ceil_tex[8];   // ceiling texture

  uint16_g light;     // light level (0-255)
  uint16_g special;   // special behaviour (0 = normal, 9 = secret, ...)
  sint16_g tag;       // sector activated by a linedef with same tag
}
raw_sector_t;


/* ----- The BSP tree structures ----------------------- */

typedef struct raw_seg_s
{
  uint16_g start;     // from this vertex...
  uint16_g end;       // ... to this vertex
  uint16_g angle;     // angle (0 = east, 16384 = north, ...)
  uint16_g linedef;   // linedef that this seg goes along
  uint16_g flip;      // true if not the same direction as linedef
  uint16_g dist;      // distance from starting point
}
raw_seg_t;


typedef struct raw_gl_seg_s
{
  uint16_g start;      // from this vertex...
  uint16_g end;        // ... to this vertex
  uint16_g linedef;    // linedef that this seg goes along, or -1
  uint16_g side;       // 0 if on right of linedef, 1 if on left
  uint16_g partner;    // partner seg number, or -1
}
raw_gl_seg_t;


typedef struct raw_bbox_s
{
  sint16_g maxy, miny;
  sint16_g minx, maxx;
}
raw_bbox_t;


typedef struct raw_node_s
{
  sint16_g x, y;         // starting point
  sint16_g dx, dy;       // offset to ending point
  raw_bbox_t b1, b2;     // bounding rectangles
  uint16_g right, left;  // children: Node or SSector (if high bit is set)
}
raw_node_t;


typedef struct raw_subsec_s
{
  uint16_g num;     // number of Segs in this Sub-Sector
  uint16_g first;   // first Seg
}
raw_subsec_t;


#endif /* __LEVNAMER_STRUCTS_H__ */
