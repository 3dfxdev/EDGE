//------------------------------------------------------------------------
// WAD : WAD read/write functions.
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

#ifndef __LEVNAMER_WAD_H__
#define __LEVNAMER_WAD_H__

#include "structs.h"
#include "system.h"


struct lump_s;


// options for whole wad

extern int load_all;
extern int hexen_mode;


// wad header

typedef struct wad_s
{
  // kind of wad file
  enum { IWAD, PWAD } kind;

  // number of entries in directory
  int num_entries;

  // offset to start of directory
  int dir_start;

  // current directory entries
  struct lump_s *dir_head;
  struct lump_s *dir_tail;
}
wad_t;


// directory entry

typedef struct lump_s
{
  // link in list
  struct lump_s *next;
  struct lump_s *prev;

  // name of lump
  char *name;

  // offset to start of lump
  int start;
  int new_start;

  // length of lump
  int length;
  int space;

  // various flags
  int flags;
 
  // data of lump
  void *data;
}
lump_t;

// this lump should be copied from the input wad
#define LUMP_COPY_ME      0x0004

// this lump shouldn't be written to the output wad
#define LUMP_IGNORE_ME    0x0008

// this lump needs to be loaded
#define LUMP_READ_ME      0x0100


extern wad_t wad;


/* ----- function prototypes --------------------- */

// check if the filename has the given extension.  Returns 1 if yes,
// otherwise zero.
//
int CheckExtension(const char *filename, const char *ext);

// remove any extension from the given filename, and add the given
// extension, and return the newly creating filename.
//
char *ReplaceExtension(const char *filename, const char *ext);

// open the input wad file and read the contents into memory.  When
// `load_all' is true, lump data will be loaded into memory too
// instead of being copied.
//
void ReadWadFile(char *filename);

// open the output wad file and write the contents.  Any lumps marked
// as copyable will be copied from the input file instead of from
// memory.  Lumps marked as ignorable will be skipped.
//
void WriteWadFile(char *filename);

// close all wad files and free any memory.
void CloseWads(void);


/* ----- conversion macros ----------------------- */


// -AJA- I wanted this to simply be `BIG_ENDIAN', but some
//       system header already defines it.  Grrrr !!
#ifdef CPU_BIG_ENDIAN

#define UINT16(x)  \
  ( ((uint16_g)(x) >> 8) | ((uint16_g)(x) << 8) )

#define UINT32(x)  \
  ( ((uint32_g)(x) >> 24) | (((uint32_g)(x) >> 8) & 0xff00) |  \
    (((uint32_g)(x) << 8) & 0xff0000) | ((uint32_g)(x) << 24) )

#else
#define UINT16(x)  ((uint16_g) (x))
#define UINT32(x)  ((uint32_g) (x))
#endif

#define UINT8(x)   ((uint8_g) (x))
#define SINT8(x)   ((sint8_g) (x))

#define SINT16(x)  ((sint16_g) UINT16(x))
#define SINT32(x)  ((sint32_g) UINT32(x))


#endif /* __LEVNAMER_WAD_H__ */
