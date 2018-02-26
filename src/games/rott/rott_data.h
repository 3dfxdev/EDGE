//----------------------------------------------------------------------------
//  Rise of the Triad Data
//----------------------------------------------------------------------------
// 
//  Copyright (c) 2016 Isotope SoftWorks
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
//
//----------------------------------------------------------------------------
//
// DESCRIPTION:
//  all external data is defined here
//  most of the data is loaded into different structures at run time
//  some internal structures shared by many modules are here
//
#ifndef __ROTT_DATA_H__
#define __ROTT_DATA_H__

/// TODO: Define lump order in ROTT WAD, define FL_ flags here as well (translated to 3DGE's MPF_ flags)

/// WALLS - Walls are stored in the WAD between the two labels "WALLSTRT" and
/// "WALLSTOP".  The format of each wall is a 4,096 block of data with no
/// header.  The bitmaps are grabbed in vertical posts so that drawing in
/// modex is more straight format.  All walls must be 64 x 64. The walls must
/// be the first few lumps in the WAD.


/// SKYS, FLOORS and CEILINGS - Skys are larger than the screen and are made
/// up of two 256X200 grabs in posts similar to the walls.  The first grab
/// represents the bottom part of the sky and the second part the top of the
/// sky.  The skys are denoted by the labels SKYSTRT and SKYSTOP.  Floors and
/// ceilings use the following structure as desribed below. This will eventually
/// be converted to use 3DGE's internal system for flats and skys.

/// !!! ///
typedef struct {
    byte width, height;
    byte data;
} pic_t;

#define CONVERT_ENDIAN_pic_t(pp) { }

typedef struct {
    short width, height;
    short orgx, orgy;
    byte data;
} lpic_t;

#define CONVERT_ENDIAN_lpic_t(lp)            \
    {                                        \
        SwapIntelShort(&lp->width);          \
        SwapIntelShort(&lp->height);         \
        SwapIntelShort(&lp->orgx);           \
        SwapIntelShort(&lp->orgy);           \
    }

typedef struct {
    short height;
    char width[256];
    short charofs[256];
    byte data;			// as much as required
} font_t;

#define CONVERT_ENDIAN_font_t(fp)            \
    {                                        \
        int i;                               \
        SwapIntelShort(&fp->height);         \
        for (i = 0; i < 256; i++) {          \
            SwapIntelShort(&fp->charofs[i]); \
        }                                    \
    }

typedef struct {
    short width;
    short height;
    byte palette[768];
    byte data;
} lbm_t;

#define CONVERT_ENDIAN_lbm_t(lp)             \
    {                                        \
        SwapIntelShort(&lp->width);          \
        SwapIntelShort(&lp->height);         \
    }

typedef struct {
    short origsize;		// the orig size of "grabbed" gfx
    short width;		// bounding box size
    short height;
    short leftoffset;		// pixels to the left of origin
    short topoffset;		// pixels above the origin
    unsigned short collumnofs[320];	// only [width] used, the [0] is &collumnofs[width]
} rottpatch_t;

#define CONVERT_ENDIAN_patch_t(pp)           \
    {                                        \
        int i;                               \
        SwapIntelShort(&pp->origsize);       \
        SwapIntelShort(&pp->width);          \
        SwapIntelShort(&pp->height);         \
        SwapIntelShort(&pp->leftoffset);     \
        SwapIntelShort(&pp->topoffset);      \
        for (i = 0; i < pp->width; i++) {          \
            SwapIntelShort((short*)&pp->collumnofs[i]); \
        }                                    \
    }

typedef struct {
    short origsize;		// the orig size of "grabbed" gfx
    short width;		// bounding box size
    short height;
    short leftoffset;		// pixels to the left of origin
    short topoffset;		// pixels above the origin
    short translevel;
    short collumnofs[320];	// only [width] used, the [0] is &collumnofs[width]
} transpatch_t;

#define CONVERT_ENDIAN_transpatch_t(pp)      \
    {                                        \
        int i;                               \
        SwapIntelShort(&pp->origsize);       \
        SwapIntelShort(&pp->width);          \
        SwapIntelShort(&pp->height);         \
        SwapIntelShort(&pp->leftoffset);     \
        SwapIntelShort(&pp->topoffset);      \
        SwapIntelShort(&pp->translevel);     \
        for (i = 0; i < pp->width; i++) {          \
            SwapIntelShort((short*)&pp->collumnofs[i]); \
        }                                    \
    }

typedef struct {
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
        SwapIntelShort(&pp->height);         \
        for (i = 0; i < 256; i++) {          \
            SwapIntelShort(&pp->charofs[i]); \
        }                                    \
    }


///They can be found between the labels UPDNSTRT and UPDNSTOP.

#endif // __ROTTDATA__