//----------------------------------------------------------------------------
//  EDGE2 Data
//----------------------------------------------------------------------------
//
//  Copyright (c) 1999-2009  The EDGE2 Team.
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
// DESCRIPTION:
//  all external data is defined here
//  most of the data is loaded into different structures at run time
//  some internal structures shared by many modules are here
//

#ifndef __DOOMDATA__
#define __DOOMDATA__

// Some global defines, that configure the game.
#include "dm_defs.h"

//
// Map level types.
// The following data structures define the persistent format
// used in the lumps of the WAD files.
//
#include "dm_structs.h"

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

  // ML_PORTALCONNECT = 0x80000000	// for internal use only: This line connects to a sector with a linked portal (used to speed up sight checks.)
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


// Patches.
//
// A patch holds one or more columns.
// Patches are used for sprites and all masked pictures,
// and we compose textures from the TEXTURE1/2 lists
// of patches.
//
typedef struct patch_s
{
	//short origsize; //for ROTT

	// bounding box size
	short width;
	short height;

	// pixels to the left of origin
	short leftoffset;

	// pixels below the origin
	short topoffset;

	int columnofs[1];  // only [width] used, the [1] is &columnofs[width]
}
patch_t;

// ROTT Picture Format (rottpic)

typedef struct pic_s
{
	byte width, height;
	byte data;
} 
pic_t;


#define CONVERT_ENDIAN_pic_t(pp) { }

//Rise of the Triad FLATS FORMAT
typedef struct 
{
	short width, height;
	short orgx, orgy;
	byte data;
} lpic_t;

#define CONVERT_ENDIAN_lpic_t(lp)            \
    {                                        \
        EPI_LE_S16(&lp->width);          \
        EPI_LE_S16(&lp->height);         \
        EPI_LE_S16(&lp->orgx);           \
        EPI_LE_S16(&lp->orgy);           \
    }

//ROTT Fonts
typedef struct 
{
	short height;
	char width[256];
	short charofs[256];
	byte data;			// as much as required
} font_t;

#define CONVERT_ENDIAN_font_t(fp)            \
    {                                        \
        int i;                               \
        EPI_LE_S16(&fp->height);         \
        for (i = 0; i < 256; i++) {          \
            EPI_LE_S16(&fp->charofs[i]); \
        }                                    \
    }
// LBM ROTT 320x200 images (only used a few times in Darkwar)
typedef struct 
{
	short width;
	short height;
	byte palette[768];
	byte data;
} lbm_t;

#define CONVERT_ENDIAN_lbm_t(lp)             \
    {                                        \
        EPI_LE_S16(&lp->width);          \
        EPI_LE_S16(&lp->height);         \
    }
// Rise of the Triad Patches
typedef struct rottpatch_s
{
	short origsize;		// the orig size of "grabbed" gfx
	short width;		// bounding box size
	short height;
	short leftoffset;		// pixels to the left of origin
	short topoffset;		// pixels above the origin
	unsigned short columnofs[320];	// only [width] used, the [0] is &collumnofs[width]
} rottpatch_t;

#define CONVERT_ENDIAN_rottpatch_t(pp)           \
    {                                        \
        int i;                               \
        EPI_LE_S16(&pp->origsize);       \
        EPI_LE_S16(&pp->width);          \
        EPI_LE_S16(&pp->height);         \
        EPI_LE_S16(&pp->leftoffset);     \
        EPI_LE_S16(&pp->topoffset);      \
        for (i = 0; i < pp->width; i++) {          \
            EPI_LE_S16((short*)&pp->columnofs[i]); \
        }                                    \
    }
// Rise of the Triad Masked Patches
typedef struct
{
	short origsize;		// the orig size of "grabbed" gfx
	short width;		// bounding box size
	short height;
	short leftoffset;		// pixels to the left of origin
	short topoffset;		// pixels above the origin
	short translevel;
	short columnofs[320];	// only [width] used, the [0] is &collumnofs[width]
} transpatch_t;

#define CONVERT_ENDIAN_transpatch_t(pp)      \
    {                                        \
        int i;                               \
        EPI_LE_S16(&pp->origsize);       \
        EPI_LE_S16(&pp->width);          \
        EPI_LE_S16(&pp->height);         \
        EPI_LE_S16(&pp->leftoffset);     \
        EPI_LE_S16(&pp->topoffset);      \
        EPI_LE_S16(&pp->translevel);     \
        for (i = 0; i < pp->width; i++) {          \
            EPI_LE_S16((short*)&pp->collumnofs[i]); \
        }                                    \
    }
//ROTT Fonts
typedef struct 
{
	byte color;
	short height;
	char width[256];
	short charofs[256];
	byte pal[0x300];
	byte data;			// as much as required
} cfont_t;

#define CONVERT_ENDIAN_cfont_t(pp)           \
    {                                        \
        int i;                               \
        EPI_LE_S16(&pp->height);         \
        for (i = 0; i < 256; i++) {          \
            EPI_LE_S16(&pp->charofs[i]); \
        }                                    \
    }



#endif // __DOOMDATA__

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
