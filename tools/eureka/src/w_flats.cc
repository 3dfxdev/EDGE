//------------------------------------------------------------------------
//  FLAT LOADING
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

#include <map>
#include <algorithm>
#include <string>

#include "m_dialog.h"
#include "m_game.h"      /* yg_picture_format */
#include "r_misc.h"
#include "levels.h"
#include "w_patches.h"
#include "w_loadpic.h"
#include "w_file.h"
#include "w_io.h"
#include "w_structs.h"

#include "w_flats.h"


typedef std::map<std::string, Img *> flat_map_t;

static flat_map_t flats;


static void DeleteFlat(const flat_map_t::value_type& P)
{
	delete P.second;
}


void W_ClearFlats()
{
	std::for_each(flats.begin(), flats.end(), DeleteFlat);

	flats.clear();
}



/*
 *  flat_list_entry_match
 *  Function used by bsearch() to locate a particular 
 *  flat in the FTexture.
 */
static int flat_list_entry_match (const void *key, const void *flat_list_entry)
{
	return y_strnicmp ((const char *) key,
						((const flat_list_entry_t *) flat_list_entry)->name,
						WAD_FLAT_NAME);
}


/*
 *  load a flat into a new image.  NULL if not found.
 */

Img * Flat2Img (const char * fname)
{
	char name[WAD_FLAT_NAME + 1];
	strncpy (name, fname, WAD_FLAT_NAME);
	name[WAD_FLAT_NAME] = 0;

	flat_list_entry_t *flat = (flat_list_entry_t *)
		bsearch (name, flat_list, NumFTexture, sizeof *flat_list,
				flat_list_entry_match);

	if (! flat)  // Not found in list
		return 0;

	int width  = DOOM_FLAT_WIDTH;  // Big deal !
	int height = DOOM_FLAT_HEIGHT;

	const Wad_file0 *wadfile = flat->wadfile;
	wadfile->seek (flat->offset);

	Img *img = new Img (width, height, false);

	wadfile->read_bytes (img->wbuf (), (long) width * height);

	return img;
}



void W_LoadFlats()
{
	// FIXME
}


Img * W_GetFlat(const char *name)
{
	std::string f_str = name;

	flat_map_t::iterator P = flats.find(f_str);

	if (P != flats.end ())
		return P->second;

	// flat not in the list yet.  Add it.

	Img *result = Flat2Img(name);
	flats[f_str] = result;

	// note that a NULL return from Flat2Img is OK, it means that no
	// such flat exists.  Our renderer will revert to using a solid
	// colour.

	return result;
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
