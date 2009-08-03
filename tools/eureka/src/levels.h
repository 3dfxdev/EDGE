//------------------------------------------------------------------------
//  LEVEL LOADING ETC
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


#ifndef YH_LEVELS  /* Prevent multiple inclusion */
#define YH_LEVELS  /* Prevent multiple inclusion */


#include "w_name.h"
#include "e_things.h"


// FIXME should be somewhere else
extern int   NumWTexture; /* number of wall textures */
extern char  **WTexture;  /* array of wall texture names */


extern int   MapMaxX;   /* maximum X value of map */
extern int   MapMaxY;   /* maximum Y value of map */
extern int   MapMinX;   /* minimum X value of map */
extern int   MapMinY;   /* minimum Y value of map */

extern bool  MadeChanges; /* made changes? */
extern bool  MadeMapChanges;  /* made changes that need rebuilding? */

extern char Level_name[WAD_NAME + 1]; /* The name of the level (E.G.
           "MAP01" or "E1M1"), followed by a
           NUL. If the Level has been created as
           the result of a "c" command with no
           argument, an empty string. The name
           is not necesarily in upper case but
           it always a valid lump name, not a
           command line shortcut like "17". */

extern y_file_name_t Level_file_name;  /* The name of the file in which
           the level would be saved. If the
           level has been created as the result
           of a "c" command, with or without
           argument, an empty string. */

extern y_file_name_t Level_file_name_saved;  /* The name of the file in
           which the level was last saved. If
           the Level has never been saved yet,
           an empty string. */

void update_level_bounds ();


extern Wad_name sky_flat;
extern int sky_color;


/*
 *  is_sky - is this flat a sky
 */
inline bool is_sky (Wad_name flat)
{
  return sky_flat == flat;
}


int levelname2levelno (const char *name);
int levelname2rank (const char *name);


void ReadWTextureNames (void);
void ForgetFTextureNames (void);
int is_flat_name_in_list (const char *name);

void ReadFTextureNames (void);
void ForgetWTextureNames (void);


#endif  /* DO NOT ADD ANYTHING AFTER THIS LINE */
//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
