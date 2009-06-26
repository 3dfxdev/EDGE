//------------------------------------------------------------------------
// STRUCT : Doom structures, raw on-disk layout
//------------------------------------------------------------------------
//
//  Copyright (c) 2007-2008  The EDGE Team.
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
	char identification[4];

	u32_t num_entries;
	u32_t dir_start;
}
raw_wad_header_t;


// directory entry
typedef struct raw_wad_entry_s
{
	u32_t pos;
	u32_t size;

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


// Lump order in a map WAD: each map needs a couple of lumps
// to provide a complete scene geometry description.
enum
{
   ML_LABEL=0,   // A separator name, ExMx or MAPxx
   ML_THINGS,    // Monsters, items..
   ML_LINEDEFS,  // LineDefs, from editing
   ML_SIDEDEFS,  // SideDefs, from editing
   ML_VERTEXES,  // Vertices, edited and BSP splits generated
   ML_SEGS,      // LineSegs, from LineDefs split by BSP
   ML_SSECTORS,  // SubSectors, list of LineSegs
   ML_NODES,     // BSP nodes
   ML_SECTORS,   // Sectors, from editing
   ML_REJECT,    // LUT, sector-sector visibility 
   ML_BLOCKMAP,  // LUT, motion clipping, walls/grid element
   ML_BEHAVIOR   // Hexen scripting stuff
};

// -AJA- 1999/12/20: Lump order from "GL-Friendly Nodes" specs.
enum
{
   ML_GL_LABEL=0,  // A separator name, GL_ExMx or GL_MAPxx
   ML_GL_VERT,     // Extra Vertices
   ML_GL_SEGS,     // Segs, from linedefs & minisegs
   ML_GL_SSECT,    // SubSectors, list of segs
   ML_GL_NODES     // GL BSP nodes
};


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
	u16_t start;    // from this vertex...
	u16_t end;      // ... to this vertex
	u16_t flags;    // linedef flags (impassible, etc)
	u16_t special;  // special type (0 for none, 97 for teleporter, etc)
	s16_t tag;      // this linedef activates the sector with same tag
	u16_t side_R;   // right sidedef
	u16_t side_L;   // left sidedef (only if this line adjoins 2 sectors)
}
raw_linedef_t;

typedef struct raw_hexen_linedef_s
{
	u16_t start;      // from this vertex...
	u16_t end;        // ... to this vertex
	u16_t flags;      // linedef flags (impassible, etc)
	u8_t  special;    // special type
	u8_t  args[5];    // special arguments
	u16_t side_R;     // right sidedef
	u16_t side_L;     // left sidedef
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
	s16_t tid;       // tag id (for scripts/specials)
	s16_t x, y;      // position
	s16_t height;    // start height above floor
	s16_t angle;     // angle thing faces
	u16_t type;      // type of thing
	u16_t options;   // when appears, deaf, dormant, etc..

	u8_t special;    // special type
	u8_t args[5];    // special arguments
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

// Indicates a GL-specific vertex
#define	SF_GL_VERTEX  (u16_t)(1 << 15)


typedef struct raw_v3_seg_s
{
	u32_t start;      // from this vertex...
	u32_t end;        // ... to this vertex
	u16_t linedef;    // linedef that this seg goes along, or -1
	u16_t side;       // 0 if on right of linedef, 1 if on left
	u32_t partner;    // partner seg number, or -1
}
raw_v3_seg_t;

// Indicates a GL-specific vertex (V3 and V5 formats)
#define	SF_GL_VERTEX_V3   (u32_t)(1 << 30)
#define	SF_GL_VERTEX_V5   (u32_t)(1 << 31)


typedef struct raw_bbox_s
{
	s16_t maxy, miny;
	s16_t minx, maxx;
}
raw_bbox_t;


typedef struct raw_node_s
{
	s16_t x, y;          // starting point
	s16_t dx, dy;        // offset to ending point
	raw_bbox_t bbox[2];  // bounding rectangles
	u16_t children[2];   // children: Node or SSector (if high bit is set)
}
raw_node_t;

// Indicate a leaf.
#define	NF_SUBSECTOR  (u16_t)(1 << 15)


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
	s16_t x, y;          // starting point
	s16_t dx, dy;        // offset to ending point
	raw_bbox_t bbox[2];  // bounding rectangles
	u32_t children[2];   // children: Node or SSector (if high bit is set)
}
raw_v5_node_t;

// Indicate a leaf.
#define	NF_V5_SUBSECTOR  (u32_t)(1 << 31)


/* ----- Graphical structures ---------------------- */

typedef struct
{
	s16_t x_origin;
	s16_t y_origin;

	u16_t pname;    // index into PNAMES
	u16_t stepdir;  // NOT USED
	u16_t colormap; // NOT USED
}
raw_patchdef_t;


// Texture definition.
//
// Each texture is composed of one or more patches,
// with patches being lumps stored in the WAD.
//
typedef struct
{
	char name[8];

	u32_t masked;      // NOT USED
	u16_t width;
	u16_t height;
	u32_t column_dir;  // NOT USED
	u16_t patch_count;

	raw_patchdef_t patches[1];
}
raw_texture_t;


// Patches.
//
// A patch holds one or more columns.
// Patches are used for sprites and all masked pictures,
// and we compose textures from the TEXTURE1/2 lists
// of patches.
//
typedef struct patch_s
{
	// bounding box size 
	s16_t width;
	s16_t height;

	// pixels to the left of origin 
	s16_t leftoffset;

	// pixels below the origin 
	s16_t topoffset;

	u32_t columnofs[1];  // only [width] used
}
patch_t;


//
// LineDef attributes.
//

typedef enum
{
	// Solid, is an obstacle.
	MLF_Blocking = 0x0001,

	// Blocks monsters only.
	MLF_BlockMonsters = 0x0002,

	// Backside will not be present at all if not two sided.
	MLF_TwoSided = 0x0004,

	// If a texture is pegged, the texture will have
	// the end exposed to air held constant at the
	// top or bottom of the texture (stairs or pulled
	// down things) and will move with a height change
	// of one of the neighbor sectors.
	// Unpegged textures allways have the first row of
	// the texture at the top pixel of the line for both
	// top and bottom textures (use next to windows).

	// upper texture unpegged
	MLF_UpperUnpegged = 0x0008,

	// lower texture unpegged
	MLF_LowerUnpegged = 0x0010,

	// In AutoMap: don't map as two sided: IT'S A SECRET!
	MLF_Secret = 0x0020,

	// Sound rendering: don't let sound cross two of these.
	MLF_SoundBlock = 0x0040,

	// Don't draw on the automap at all.
	MLF_DontDraw = 0x0080,

	// Set if already seen, thus drawn in automap.
	MLF_Mapped = 0x0100,

	// -AJA- 1999/08/16: This one is from Boom. Allows multiple lines to
	//       be pushed simultaneously.
	MLF_PassThru = 0x0200,

	// -AJA- These two from XDoom.
	MLF_ShootBlock  = 0x0800,
	MLF_SightBlock  = 0x1000,

	// ----- internal flags -----
	MLF_Mirror = (1 << 16),
}
lineflag_e;


//
// Sector attributes.
//

typedef enum
{
	MSF_DamageMask = 0x0060,
	MSF_Secret     = 0x0080,
	MSF_Friction   = 0x0100,
	MSF_Push       = 0x0200,
	MSF_NoSounds   = 0x0400,
	MSF_QuietPlane = 0x0800
}
sectorflag_e;

#define MSF_BoomFlags  0x0FE0


//
// Thing attributes.
//

// Skill flags.
#define MTF_EASY         1
#define MTF_NORMAL       2
#define MTF_HARD         4

// Deaf monsters/do not react to sound.
#define MTF_AMBUSH       8

// Multiplayer only.
#define MTF_NOT_SINGLE  16

// -AJA- 1999/09/22: Boom compatibility.
#define MTF_NOT_DM      32
#define MTF_NOT_COOP    64

// -AJA- 2000/07/31: Friend flag, from MBF
#define MTF_FRIEND      128 

// -AJA- 2004/11/04: This bit should be zero (otherwise old WAD).
#define MTF_RESERVED    256

// -AJA- 2008/03/08: Extrafloor placement
#define MTF_EXFLOOR_MASK    0x3C00
#define MTF_EXFLOOR_SHIFT   10

#endif /* __DM_STRUCTS_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
