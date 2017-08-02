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




// -AJA- 1999/12/20: Lump order from "GL-Friendly Nodes" specs.
enum
{
   ML_GL_LABEL=0,  // A separator name, GL_ExMx or GL_MAPxx
   ML_GL_VERT,     // Extra Vertices
   ML_GL_SEGS,     // Segs, from linedefs & minisegs
   ML_GL_SSECT,    // SubSectors, list of segs
   ML_GL_NODES     // GL BSP nodes
};

/// WALLS - Walls are stored in the WAD between the two labels "WALLSTRT" and
/// "WALLSTOP".  The format of each wall is a 4,096 block of data with no
/// header.  The bitmaps are grabbed in vertical posts so that drawing in
/// modex is more straight format.  All walls must be 64 x 64. The walls must
/// be the first few lumps in the WAD.

// Patches.
//
// A patch holds one or more columns.
//
typedef struct patch_s
{
	// bounding box size 
	short width;
	short height;

	// pixels to the left of origin 
	short leftoffset;

	// pixels below the origin 
	short topoffset;

	int columnofs[1];  // only [width] used
}
patch_t;

/// SKYS, FLOORS and CEILINGS - Skys are larger than the screen and are made
/// up of two 256X200 grabs in posts similar to the walls.  The first grab
/// represents the bottom part of the sky and the second part the top of the
/// sky.  The skys are denoted by the labels SKYSTRT and SKYSTOP.  Floors and
/// ceilings use the following structure as desribed below. This will eventually
/// be converted to use 3DGE's internal system for flats and skys.

/// !!! ///
typedef struct
{
   short     width,height;
   short     orgx,orgy;
   byte     data;
} lpic_t;

///They can be found between the labels UPDNSTRT and UPDNSTOP.

#endif // __ROTTDATA__