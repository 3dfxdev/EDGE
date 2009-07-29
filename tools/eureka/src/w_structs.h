//------------------------------------------------------------------------
//  RAW WAD STRUCTURES
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2009 Andrew Apted
//  Copyright (C) 1997-2003 André Majorel et al
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
//
//  Based on Yadex which incorporated code from DEU 5.21 that was put
//  in the public domain in 1994 by Raphaël Quinet and Brendon Wyber.
//
//------------------------------------------------------------------------

#ifndef YH_WSTRUCTS // To prevent multiple inclusion
#define YH_WSTRUCTS // To prevent multiple inclusion



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


/*
 *  Directory
 */
/* The wad file pointer structure is used for holding the information
   on the wad files in a linked list.
   The first wad file is the main wad file. The rest are patches. */
class Wad_file;

/* The master directory structure is used to build a complete directory
   of all the data blocks from all the various wad files. */
typedef struct MasterDirectory *MDirPtr;
struct MasterDirectory
   {
   MDirPtr next;    // Next in list
   const Wad_file *wadfile; // File of origin
   struct Directory dir;  // Directory data
   };


// Defined in wads.cc
extern MDirPtr   MasterDir; // The master directory


/* Lump location : enough information to load a lump without
   having to do a directory lookup. */
struct Lump_loc
   {
   Lump_loc () { wad = 0; }
   Lump_loc (const Wad_file *w, s32_t o, s32_t l) { wad = w; ofs = o; len = l; }
   bool operator == (const Lump_loc& other) const
     { return wad == other.wad && ofs == other.ofs && len == other.len; }
   const Wad_file *wad;
   s32_t ofs;
   s32_t len;
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



// Things
typedef s16_t wad_ttype_t;
typedef s16_t wad_tflags_t;



MDirPtr FindMasterDir (MDirPtr, const char *);
MDirPtr FindMasterDir (MDirPtr, const char *, const char *);


#endif  /* DO NOT ADD ANYTHING AFTER THIS LINE */
//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
