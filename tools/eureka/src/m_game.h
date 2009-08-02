//------------------------------------------------------------------------
//  GAME HANDLING
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


#include "im_color.h"


extern const char ygd_file_magic[];


/*
 *  Data structures for game definition data
 */

// linegroup <letter> <description>
typedef struct
{
	char group;
	const char *desc;
}
linegroup_t;


// line <number> <group> <shortdesc> <longdesc>
typedef struct
{
	char group;
	const char *desc;
}
linetype_t;


// sector <number> <description>
typedef struct
{
	const char *desc;
}
sectortype_t;


// thinggroup <letter> <colour> <description>
typedef struct
{
	char group;        // group letter
	pcolour_t color;   // RGB colour
	const char *desc;  // Description of thing group
}
thinggroup_t;


// thing <number> <group> <flags> <radius> <description> [<sprite>]
typedef struct
{
	char group; // Thing group
	char flags;    // Flags
	int radius;    // Radius of thing
	const char *desc;  // Short description of thing
	const char *sprite;  // Root of name of sprite for thing
}
thingtype_t;


/* (1)  This is only here for speed, to avoid having to lookup
        thinggroup for each thing when drawing things */
const char THINGDEF_SPECTRAL = 0x01;


/*
 *  Global variables that contain game definition data
 */

typedef enum { YGLN__, YGLN_E1M10, YGLN_E1M1, YGLN_MAP01 } ygln_t;

extern ygln_t yg_level_name;


void InitGameDefs();
void LoadGameDefs(const char *game);
void FreeGameDefs();


const sectortype_t * M_GetSectorType(int type);
const linetype_t   * M_GetLineType(int type);
const thingtype_t  * M_GetThingType(int type);


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
