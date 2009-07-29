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

// ldt <number> <ldtgroup> <shortdesc> <longdesc>
typedef struct
{
	int number;
	char ldtgroup;
	const char *shortdesc;
	const char *longdesc;
}
ldtdef_t;


// ldtgroup <ldtgroup> <description>
typedef struct
{
	char ldtgroup;
	const char *desc;
}
ldtgroup_t;


// st <number> <shortdesc> <longdesc>
typedef struct
{
	int number;
	const char *shortdesc;
	const char *longdesc;
}
stdef_t;


// thing <number> <thinggroup> <flags> <radius> <description> [<sprite>]
typedef struct
{
	int number;    // Thing number
	char thinggroup; // Thing group
	char flags;    // Flags
	int radius;    // Radius of thing
	const char *desc;  // Short description of thing
	const char *sprite;  // Root of name of sprite for thing
}
thingdef_t;


/* (1)  This is only here for speed, to avoid having to lookup
        thinggroup for each thing when drawing things */
const char THINGDEF_SPECTRAL = 0x01;


// thinggroup <thinggroup> <colour> <description>
typedef struct
{
	char thinggroup; // Thing group
	rgb_c rgb;   // RGB colour
	acolour_t acn; // Application colour#
	const char *desc;  // Description of thing group
}
thinggroup_t;


/*
 *  Global variables that contain game definition data
 */

typedef enum { YGLN__, YGLN_E1M10, YGLN_E1M1, YGLN_MAP01 } ygln_t;

extern ygln_t yg_level_name;


extern std::vector<ldtdef_t *> ldtdef;
extern std::vector<ldtgroup_t *> ldtgroup;
extern std::vector<stdef_t *> stdef;
extern std::vector<thingdef_t *> thingdef;
extern std::vector<thinggroup_t *> thinggroup;


// Shorthands to make the code more readable
#define CUR_LDTDEF     ((ldtdef_t     *)al_lptr (ldtdef    ))
#define CUR_LDTGROUP   ((ldtgroup_t   *)al_lptr (ldtgroup  ))
#define CUR_STDEF      ((stdef_t      *)al_lptr (stdef     ))
#define CUR_THINGDEF   ((thingdef_t   *)al_lptr (thingdef  ))
#define CUR_THINGGROUP ((thinggroup_t *)al_lptr (thinggroup))


void InitGameDefs(void);
void LoadGameDefs(const char *game);
void FreeGameDefs(void);


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
