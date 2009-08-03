//------------------------------------------------------------------------
//  FLAT LOADING
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2009 Andrew Apted
//  Copyright (C) 1997-2003 Andr� Majorel et al
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
//  in the public domain in 1994 by Rapha�l Quinet and Brendon Wyber.
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

#include "w_wad.h"
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


void W_LoadFlats()
{
	for (int i = 0; i < (int)master_dir.size(); i++)
	{
		LogPrintf("Loading Flats from WAD #%d\n", i+1);

		Wad_file *wf = master_dir[i];

		for (int k = 0; k < (int)wf->flats.size(); k++)
		{
			Lump_c *lump = wf->GetLump(wf->flats[k]);

			DebugPrintf("  Flat %d : '%s'\n", k, lump->Name());

			// TODO: check size == 64*64

			Img *img = new Img(64, 64, false);
			
			if (! lump->Seek() ||
				! lump->Read(img->wbuf(), 64*64))
				FatalError("Error reading flat from WAD.\n");

			flats[std::string(lump->Name())] = img;
		}
	}
}


Img * W_GetFlat(const char *name)
{
	std::string f_str = name;

	flat_map_t::iterator P = flats.find(f_str);

	if (P != flats.end())
		return P->second;

	//???  flats[f_str] = NULL

	return NULL;
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
