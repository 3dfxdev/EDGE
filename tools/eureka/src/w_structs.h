/*
 *  w_structs.h
 *  Wad files data structures.
 *  BW & RQ sometime in 1993 or 1994.
 *  FIXME this file should also contain the definitions for
 *  the wad header and directory entry which are currently
 *  in main.h.
 */


/*
This file is part of Yadex.

Yadex incorporates code from DEU 5.21 that was put in the public domain in
1994 by Raphaël Quinet and Brendon Wyber.

The rest of Yadex is Copyright © 1997-2003 André Majorel and others.

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation; either version 2 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program; if not, write to the Free Software Foundation, Inc., 59 Temple
Place, Suite 330, Boston, MA 02111-1307, USA.
*/


#ifndef YH_WSTRUCTS // To prevent multiple inclusion
#define YH_WSTRUCTS // To prevent multiple inclusion


// FIXME: all identifiers should start with wad_ or WAD_...


// Directory
const size_t WAD_NAME = 8;    // Length of a directory entry name
typedef char wad_name_t[WAD_NAME];
typedef struct Directory *DirPtr;
struct Directory
{
  s32_t        start;     // Offset to start of data
  s32_t        size;      // Byte size of data
  wad_name_t name;      // Name of data block
};


// Textures
const size_t WAD_TEX_NAME = 8;
typedef char wad_tex_name_t[WAD_TEX_NAME];


// Flats
const size_t WAD_FLAT_NAME = WAD_NAME;
typedef char wad_flat_name_t[WAD_FLAT_NAME];


// Pictures (sprites and patches)
const size_t WAD_PIC_NAME = WAD_NAME;
typedef char wad_pic_name_t[WAD_TEX_NAME];


// Level objects properties
typedef s16_t wad_coord_t;    // Map (X,Y) coordinates
typedef s16_t wad_ldn_t;      // Linedef#
typedef s16_t wad_sdn_t;      // Sidedef#
typedef s16_t wad_sn_t;     // Sector#
typedef s16_t wad_tag_t;      // Tag
typedef s16_t wad_tn_t;     // Thing# (theor. there might be more)
typedef s16_t wad_vn_t;     // Vertex#
typedef s16_t wad_z_t;      // Map Z coordinate


// Things
const size_t WAD_THING_BYTES       = 10;  // Size in the wad file
const size_t WAD_HEXEN_THING_BYTES = 20;  // Size in the wad file
typedef s16_t wad_ttype_t;
typedef s16_t wad_tangle_t;
typedef s16_t wad_tflags_t;
struct Thing
{
  wad_coord_t      x;    // FIXME rename to "x"
  wad_coord_t      y;    // FIXME rename to "y"
  wad_tangle_t     angle;
  wad_ttype_t      type;
  wad_tflags_t     when;    // FIXME rename to "flags"
};

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


typedef struct
{
  s16_t            tid;
  wad_coord_t      x;
  wad_coord_t      y;
  wad_z_t          height;
  wad_tangle_t     angle;
  wad_ttype_t      type;

  u16_t             options;

  byte               special;
  byte               arg1;
  byte               arg2;
  byte               arg3;
  byte               arg4;
  byte               arg5;
} wad_hexen_thing_t;
typedef struct Thing *TPtr;


// Linedefs
const size_t WAD_LINEDEF_BYTES       = 14;  // Size in the wad file
const size_t WAD_HEXEN_LINEDEF_BYTES = 16;  // Size in the wad file
typedef s16_t wad_ldflags_t;
typedef s16_t wad_ldtype_t;
struct LineDef
{
  wad_vn_t      start;      // # of start vertex
  wad_vn_t      end;      // # of end vertex
  wad_ldflags_t flags;
  wad_ldtype_t  type;
  wad_tag_t     tag;
  wad_sdn_t     sidedef1;   // # of first (right) sidedef
  wad_sdn_t     sidedef2;   // # of second (left) sidedef or 0xffff
};
typedef struct
{
  wad_vn_t      start;
  wad_vn_t      end;
  wad_ldflags_t flags;
  byte            type;
  byte            arg1;
  byte            arg2;
  byte            arg3;
  byte            arg4;
  byte            arg5;
  wad_sdn_t     sidedef1;
  wad_sdn_t     sidedef2;
} wad_hexen_linedef_t;
typedef struct LineDef *LDPtr;


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

  // -AJA- these three from XDoom
  MLF_Translucent = 0x0400,
  MLF_ShootBlock  = 0x0800,
  MLF_SightBlock  = 0x1000,
}
linedef_flag_e;



// Sidedefs
const size_t WAD_SIDEDEF_BYTES = 30;  // Size in the wad file
struct SideDef
{
  wad_coord_t    xoff;      // FIXME rename to "xofs"
  wad_coord_t    yoff;      // FIXME rename to "yofs"
  wad_tex_name_t upper;     // Name of upper texture
  wad_tex_name_t lower;     // Name of lower texture
  wad_tex_name_t middle;    // Name of middle texture
  wad_sn_t       sector;    // # of adjacent sector
};
// (it's the same for Hexen)
typedef struct SideDef *SDPtr;


// Vertices
const size_t WAD_VERTEX_BYTES = 4;  // Size in the wad file
struct Vertex
{
  wad_coord_t x;
  wad_coord_t y;
};
// (it's the same for Hexen)
typedef struct Vertex *VPtr;


// Sectors
const size_t WAD_SECTOR_BYTES = 26; // Size in the wad file
typedef s16_t wad_stype_t;
struct Sector
{
  wad_z_t         floorh;   // Floor height
  wad_z_t         ceilh;    // Ceiling height
  wad_flat_name_t floort;   // Name of floor texture
  wad_flat_name_t ceilt;    // Name of ceiling texture
  s16_t             light;    // Light level (0-255)
  wad_stype_t     special;    // FIXME rename to "type"
  wad_tag_t       tag;
};
typedef struct Sector *SPtr;

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
sector_flag_e;

#define MSF_BoomFlags  0x0FE0



// The 11 lumps that constitute a Doom/Heretic/Strife level
typedef enum
{
  WAD_LL_LABEL,
  WAD_LL_THINGS,
  WAD_LL_LINEDEFS,
  WAD_LL_SIDEDEFS,
  WAD_LL_VERTEXES,
  WAD_LL_SEGS,
  WAD_LL_SSECTORS,
  WAD_LL_NODES,
  WAD_LL_SECTORS,
  WAD_LL_REJECT,
  WAD_LL_BLOCKMAP,
          // Hexen has a BEHAVIOR lump here
  WAD_LL__
} wad_level_lump_no_t;

typedef struct
{
  const char *const name;
  size_t item_size;
} wad_level_lump_def_t;

const wad_level_lump_def_t wad_level_lump[WAD_LL__] =
{
  { 0,          0                 },  // Label -- no fixed name
  { "THINGS",   WAD_THING_BYTES   },
  { "LINEDEFS", WAD_LINEDEF_BYTES },
  { "SIDEDEFS", WAD_SIDEDEF_BYTES },
  { "VERTEXES", WAD_VERTEX_BYTES  },
  { "SEGS",     0                 },
  { "SSECTORS", 0                 },
  { "NODES",    0                 },
  { "SECTORS",  WAD_SECTOR_BYTES  },
  { "REJECT",   0                 },
  { "BLOCKMAP", 0                 }
              // Hexen has a BEHAVIOR lump here
};


#endif  /* DO NOT ADD ANYTHING AFTER THIS LINE */
//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
