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

#include "main.h"

#include "m_bitvec.h"
#include "m_dialog.h"
#include "m_game.h"
#include "levels.h"
#include "e_things.h"
#include "w_structs.h"
#include "w_file.h"
#include "w_io.h"
#include "w_wads.h"


int MapMaxX = -32767;   /* maximum X value of map */
int MapMaxY = -32767;   /* maximum Y value of map */
int MapMinX = 32767;    /* minimum X value of map */
int MapMinY = 32767;    /* minimum Y value of map */
bool MadeChanges;   /* made changes? */
bool MadeMapChanges;    /* made changes that need rebuilding? */

char Level_name[WAD_NAME + 1];

y_file_name_t Level_file_name;
y_file_name_t Level_file_name_saved;




/*
 *  update_level_bounds - update Map{Min,Max}{X,Y}
 */
void update_level_bounds ()
{
	MapMaxX = -32767;
	MapMaxY = -32767;
	MapMinX =  32767;
	MapMinY =  32767;

	for (obj_no_t n = 0; n < NumVertices; n++)
	{
		int x = Vertices[n]->x;
		int y = Vertices[n]->y;

		if (x < MapMinX) MapMinX = x;
		if (x > MapMaxX) MapMaxX = x;
		if (y < MapMinY) MapMinY = y;
		if (y > MapMaxY) MapMaxY = y;
	}
}


/*
 *  levelname2levelno
 *  Used to know if directory entry is ExMy or MAPxy
 *  For "ExMy" (case-insensitive),  returns 10x + y
 *  For "ExMyz" (case-insensitive), returns 100*x + 10y + z
 *  For "MAPxy" (case-insensitive), returns 1000 + 10x + y
 *  E0My, ExM0, E0Myz, ExM0z are not considered valid names.
 *  MAP00 is considered a valid name.
 *  For other names, returns 0.
 */
int levelname2levelno (const char *name)
{
	const unsigned char *s = (const unsigned char *) name;
	if (toupper (s[0]) == 'E'
			&& isdigit (s[1])
			&& s[1] != '0'
			&& toupper (s[2]) == 'M'
			&& isdigit (s[3])
			&& s[3] != '0'
			&& s[4] == '\0')
		return 10 * dectoi (s[1]) + dectoi (s[3]);
	if (yg_level_name == YGLN_E1M10
			&& toupper (s[0]) == 'E'
			&& isdigit (s[1])
			&& s[1] != '0'
			&& toupper (s[2]) == 'M'
			&& isdigit (s[3])
			&& s[3] != '0'
			&& isdigit (s[4])
			&& s[5] == '\0')
		return 100 * dectoi (s[1]) + 10 * dectoi (s[3]) + dectoi (s[4]);
	if (toupper (s[0]) == 'M'
			&& toupper (s[1]) == 'A'
			&& toupper (s[2]) == 'P'
			&& isdigit (s[3])
			&& isdigit (s[4])
			&& s[5] == '\0')
		return 1000 + 10 * dectoi (s[3]) + dectoi (s[4]);
	return 0;
}


/*
 *  levelname2rank
 *  Used to sort level names.
 *  Identical to levelname2levelno except that, for "ExMy",
 *  it returns 100x + y, so that
 *  - f("E1M10") = f("E1M9") + 1
 *  - f("E2M1")  > f("E1M99")
 *  - f("E2M1")  > f("E1M99") + 1
 *  - f("MAPxy") > f("ExMy")
 *  - f("MAPxy") > f("ExMyz")
 */
int levelname2rank (const char *name)
{
	const unsigned char *s = (const unsigned char *) name;
	if (toupper (s[0]) == 'E'
			&& isdigit (s[1])
			&& s[1] != '0'
			&& toupper (s[2]) == 'M'
			&& isdigit (s[3])
			&& s[3] != '0'
			&& s[4] == '\0')
		return 100 * dectoi (s[1]) + dectoi (s[3]);
	if (yg_level_name == YGLN_E1M10
			&& toupper (s[0]) == 'E'
			&& isdigit (s[1])
			&& s[1] != '0'
			&& toupper (s[2]) == 'M'
			&& isdigit (s[3])
			&& s[3] != '0'
			&& isdigit (s[4])
			&& s[5] == '\0')
		return 100 * dectoi (s[1]) + 10 * dectoi (s[3]) + dectoi (s[4]);
	if (toupper (s[0]) == 'M'
			&& toupper (s[1]) == 'A'
			&& toupper (s[2]) == 'P'
			&& isdigit (s[3])
			&& isdigit (s[4])
			&& s[5] == '\0')
		return 1000 + 10 * dectoi (s[3]) + dectoi (s[4]);
	return 0;
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
