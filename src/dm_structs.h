//------------------------------------------------------------------------
// STRUCT : Doom structures, raw on-disk layout
//------------------------------------------------------------------------
//
//  EDGE
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

#ifndef __DM_STRUCTS_H__
#define __DM_STRUCTS_H__


/* ----- The wad structures ---------------------- */

// wad header
typedef struct raw_wad_header_s
{
	char type[4];

	u32_t num_entries;
	u32_t dir_start;
}
raw_wad_header_t;


// directory entry
typedef struct raw_wad_entry_s
{
	u32_t start;
	u32_t length;

	char name[8];
}
raw_wad_entry_t;


// blockmap
typedef struct raw_blockmap_header_s
{
	s16_t x_origin, y_origin;
	s16_t x_blocks, y_blocks;
}
raw_blockmap_header_t;


/* ----- The level structures ---------------------- */

typedef struct raw_vertex_s
{
	s16_t x, y;
}
raw_vertex_t;

typedef struct raw_v2_vertex_s
{
	s32_t x, y;
}
raw_v2_vertex_t;


typedef struct raw_linedef_s
{
	u16_t start;     // from this vertex...
	u16_t end;       // ... to this vertex
	u16_t flags;     // linedef flags (impassible, etc)
	u16_t type;      // linedef type (0 for none, 97 for teleporter, etc)
	s16_t tag;       // this linedef activates the sector with same tag
	u16_t sidedef1;  // right sidedef
	u16_t sidedef2;  // left sidedef (only if this line adjoins 2 sectors)
}
raw_linedef_t;

typedef struct raw_hexen_linedef_s
{
	u16_t start;        // from this vertex...
	u16_t end;          // ... to this vertex
	u16_t flags;        // linedef flags (impassible, etc)
	u8_t  type;         // linedef type
	u8_t  specials[5];  // hexen specials
	u16_t sidedef1;     // right sidedef
	u16_t sidedef2;     // left sidedef
}
raw_hexen_linedef_t;


typedef struct raw_sidedef_s
{
	s16_t x_offset;  // X offset for texture
	s16_t y_offset;  // Y offset for texture

	char upper_tex[8];  // texture name for the part above
	char lower_tex[8];  // texture name for the part below
	char mid_tex[8];    // texture name for the regular part

	u16_t sector;    // adjacent sector
}
raw_sidedef_t;


typedef struct raw_sector_s
{
	s16_t floor_h;   // floor height
	s16_t ceil_h;    // ceiling height

	char floor_tex[8];  // floor texture
	char ceil_tex[8];   // ceiling texture

	u16_t light;     // light level (0-255)
	u16_t special;   // special behaviour (0 = normal, 9 = secret, ...)
	s16_t tag;       // sector activated by a linedef with same tag
}
raw_sector_t;


typedef struct raw_thing_s
{
	s16_t x, y;      // position of thing
	s16_t angle;     // angle thing faces (degrees)
	u16_t type;      // type of thing
	u16_t options;   // when appears, deaf, etc..
}
raw_thing_t;


// -JL- Hexen thing definition
typedef struct raw_hexen_thing_s
{
	s16_t tid;       // thing tag id (for scripts/specials)
	s16_t x, y;      // position
	s16_t height;    // start height above floor
	s16_t angle;     // angle thing faces
	u16_t type;      // type of thing
	u16_t options;   // when appears, deaf, dormant, etc..

	u8_t special;    // special type
	u8_t arg[5];     // special arguments
} 
raw_hexen_thing_t;


/* ----- The BSP tree structures ----------------------- */

typedef struct raw_seg_s
{
	u16_t start;     // from this vertex...
	u16_t end;       // ... to this vertex
	u16_t angle;     // angle (0 = east, 16384 = north, ...)
	u16_t linedef;   // linedef that this seg goes along
	u16_t flip;      // true if not the same direction as linedef
	u16_t dist;      // distance from starting point
}
raw_seg_t;


typedef struct raw_gl_seg_s
{
	u16_t start;      // from this vertex...
	u16_t end;        // ... to this vertex
	u16_t linedef;    // linedef that this seg goes along, or -1
	u16_t side;       // 0 if on right of linedef, 1 if on left
	u16_t partner;    // partner seg number, or -1
}
raw_gl_seg_t;


typedef struct raw_v3_seg_s
{
	u32_t start;      // from this vertex...
	u32_t end;        // ... to this vertex
	u16_t linedef;    // linedef that this seg goes along, or -1
	u16_t side;       // 0 if on right of linedef, 1 if on left
	u32_t partner;    // partner seg number, or -1
}
raw_v3_seg_t;


typedef struct raw_bbox_s
{
	s16_t maxy, miny;
	s16_t minx, maxx;
}
raw_bbox_t;


typedef struct raw_node_s
{
	s16_t x, y;         // starting point
	s16_t dx, dy;       // offset to ending point
	raw_bbox_t b1, b2;     // bounding rectangles
	u16_t right, left;  // children: Node or SSector (if high bit is set)
}
raw_node_t;


typedef struct raw_subsec_s
{
	u16_t num;     // number of Segs in this Sub-Sector
	u16_t first;   // first Seg
}
raw_subsec_t;


typedef struct raw_v3_subsec_s
{
	u32_t num;     // number of Segs in this Sub-Sector
	u32_t first;   // first Seg
}
raw_v3_subsec_t;


typedef struct raw_v5_node_s
{
	s16_t x, y;         // starting point
	s16_t dx, dy;       // offset to ending point
	raw_bbox_t b1, b2;     // bounding rectangles
	u32_t right, left;  // children: Node or SSector (if high bit is set)
}
raw_v5_node_t;


#endif /* __DM_STRUCTS_H__ */
