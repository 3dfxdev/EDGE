//------------------------------------------------------------------------
//  RAWDEF : Doom structures, raw on-disk layout
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2007-2008 Andrew Apted
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
	char ident[4];

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



// Lump order in a map WAD: each map needs a couple of lumps
// to provide a complete scene geometry description.
enum
{
   LL_LABEL=0,   // A separator name, ExMx or MAPxx
   LL_THINGS,    // Monsters, items..
   LL_LINEDEFS,  // LineDefs, from editing
   LL_SIDEDEFS,  // SideDefs, from editing
   LL_VERTEXES,  // Vertices, edited and BSP splits generated
   LL_SEGS,      // LineSegs, from LineDefs split by BSP
   LL_SSECTORS,  // SubSectors, list of LineSegs
   LL_NODES,     // BSP nodes
   LL_SECTORS,   // Sectors, from editing
   LL_REJECT,    // LUT, sector-sector visibility 
   LL_BLOCKMAP,  // LUT, motion clipping, walls/grid element
   LL_BEHAVIOR   // Hexen scripting stuff
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
	u16_t type;     // special type (0 for none, 97 for teleporter, etc)
	s16_t tag;      // this linedef activates the sector with same tag
	u16_t right;    // right sidedef
	u16_t left;     // left sidedef (only if this line adjoins 2 sectors)
}
raw_linedef_t;

typedef struct raw_hexen_linedef_s
{
	u16_t start;      // from this vertex...
	u16_t end;        // ... to this vertex
	u16_t flags;      // linedef flags (impassible, etc)
	u8_t  type;       // special type
	u8_t  args[5];    // special arguments
	u16_t right;      // right sidedef
	u16_t left;       // left sidedef
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
	s16_t floorh;   // floor height
	s16_t ceilh;    // ceiling height

	char floor_tex[8];  // floor texture
	char ceil_tex[8];   // ceiling texture

	u16_t light;     // light level (0-255)
	u16_t type;      // special type (0 = normal, 9 = secret, ...)
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
  // solid, is an obstacle
  MLF_Blocking = 0x0001,

  // blocks monsters only
  MLF_BlockMonsters = 0x0002,

  // backside will not be present at all if not two sided
  MLF_TwoSided = 0x0004,

  // If a texture is pegged, the texture will have
  // the end exposed to air held constant at the
  // top or bottom of the texture (stairs or pulled
  // down things) and will move with a height change
  // of one of the neighbor sectors.
  //
  // Unpegged textures allways have the first row of
  // the texture at the top pixel of the line for both
  // top and bottom textures (use next to windows).

  // upper texture unpegged
  MLF_UpperUnpegged = 0x0008,

  // lower texture unpegged
  MLF_LowerUnpegged = 0x0010,

  // in AutoMap: don't map as two sided: IT'S A SECRET!
  MLF_Secret = 0x0020,

  // sound rendering: don't let sound cross two of these
  MLF_SoundBlock = 0x0040,

  // don't draw on the automap at all
  MLF_DontDraw = 0x0080,

  // set as if already seen, thus drawn in automap
  MLF_Mapped = 0x0100,

  // -AJA- this one is from Boom. Allows multiple lines to
  //       be pushed simultaneously.
  MLF_PassThru = 0x0200,

  // -AJA- these three are from XDoom
  MLF_Translucent = 0x0400,
  MLF_ShootBlock  = 0x0800,
  MLF_SightBlock  = 0x1000,
}
lineflag_e;


//
// Sector attributes.
//

typedef enum
{
	MSF_TypeMask   = 0x001F,
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

typedef enum
{
  MTF_Easy      = 1,
  MTF_Medium    = 2,
  MTF_Hard      = 4,
  MTF_Ambush    = 8,

  MTF_Not_SP    = 16,
  MTF_Not_DM    = 32,
  MTF_Not_COOP  = 64,

  MTF_Friend    = 128,
  MTF_Reserved  = 256,
}
thing_option_e;

#define MTF_EXFLOOR_MASK    0x3C00
#define MTF_EXFLOOR_SHIFT   10

#endif /* __DM_STRUCTS_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
