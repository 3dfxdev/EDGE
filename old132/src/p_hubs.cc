//----------------------------------------------------------------------------
//  EDGE Hub handling
//----------------------------------------------------------------------------
// 
//  Copyright (c) 2008-2008  The EDGE Team.
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
//----------------------------------------------------------------------------

#include "i_defs.h"
#include "p_hubs.h"

#include "epi/file.h"
#include "epi/filesystem.h"
#include "epi/str_format.h"
#include "epi/path.h"

#include "g_state.h"
#include "m_strings.h"
#include "n_network.h"
#include "p_local.h"
#include "p_spec.h"
#include "sv_chunk.h"

#define MAX_HUBS  32

std::vector<hub_info_c *> active_hubs;


hub_info_c::hub_info_c(int _idx, const char * _map) : index(_idx)
{
	map_name = SV_DupString(_map);
}

hub_info_c::~hub_info_c()
{
	// map_name might be NULL, that is OK
	SV_FreeString(map_name);
}


//----------------------------------------------------------------------------

void HUB_Init(void)
{
	// nothing needed yet
}

void HUB_DeleteHubSaves(void)
{
	for (int index = 0; index < MAX_HUBS; index++)
	{
		std::string hub_name(epi::STR_Format("%s%d.%s", HUBBASE, index, SAVEGAMEEXT));

		hub_name = epi::PATH_Join(save_dir.c_str(), hub_name.c_str());
  
		if (epi::FS_Access(hub_name.c_str(), epi::file_c::ACCESS_READ))
        {
			epi::FS_Delete(hub_name.c_str());
        }
	}
}

void HUB_CopyHubsForSavegame(const char *basename)
{
	for (unsigned int j = 0; j < active_hubs.size(); j++)
	{
		hub_info_c *H = active_hubs[j];

		std::string old_name(epi::STR_Format("%s%d.%s", HUBBASE, H->index, SAVEGAMEEXT));
		std::string new_name(epi::STR_Format("%s_H%02d.%s", basename, H->index, SAVEGAMEEXT));

		old_name = epi::PATH_Join(save_dir.c_str(), old_name.c_str());

		if (! epi::FS_Copy(old_name.c_str(), new_name.c_str()) )
			I_Error("HUB Error: unable to copy file!\nsrc: %s\ndest: %s\n",
					old_name.c_str(), new_name.c_str());
	}
}

void HUB_CopyHubsForLoadgame(const char *basename)
{
	// Note: must be called _after_ savegame has been loaded,
	// since we assume 'active_hubs' is valid for new game.
	
	HUB_DeleteHubSaves();

	for (unsigned int j = 0; j < active_hubs.size(); j++)
	{
		hub_info_c *H = active_hubs[j];

		std::string old_name(epi::STR_Format("%s_H%02d.%s", basename, H->index, SAVEGAMEEXT));
		std::string new_name(epi::STR_Format("%s%d.%s", HUBBASE, H->index, SAVEGAMEEXT));

		new_name = epi::PATH_Join(save_dir.c_str(), new_name.c_str());

		if (! epi::FS_Copy(old_name.c_str(), new_name.c_str()) )
			I_Error("HUB Error: unable to copy file!\nsrc: %s\ndest: %s\n",
					old_name.c_str(), new_name.c_str());
	}
}


void HUB_DestroyAll(void)
{
	for (unsigned int j = 0; j < active_hubs.size(); j++)
	{
		delete active_hubs[j];
	}

	active_hubs.clear();
}

hub_info_c * HUB_FindMap(const char *map)
{
	for (unsigned int j = 0; j < active_hubs.size(); j++)
	{
		hub_info_c *H = active_hubs[j];

		if (stricmp(H->map_name, map) == 0)
			return H;
	}

	return NULL; // not found
}

bool HUB_AlreadyVisited(const char *map)
{
	return HUB_FindMap(map) ? true : false;
}

void HUB_AddMap(const char *map)
{
	if (HUB_FindMap(map))
		I_Error("INTERNAL ERROR: HUB_AddMap with already visited hub.\n");

	hub_info_c *H = new hub_info_c(active_hubs.size(), map);

	active_hubs.push_back(H);
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
