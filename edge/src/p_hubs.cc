//----------------------------------------------------------------------------
//  EDGE Hub handling
//----------------------------------------------------------------------------
// 
//  Copyright (c) 2007-2008  The EDGE Team.
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

#include "dm_state.h"
#include "n_network.h"
#include "p_local.h"
#include "p_spec.h"

std::vector<dormant_hub_c *> dormant_hubs;

void HUB_Init(void)
{
	// nothing needed yet
}

void HUB_DeleteHubSaves(void)
{
	// TODO
}

void HUB_CopyHubsForSavegame(const char *basename)
{
	// TODO
}

void HUB_CopyHubsForLoadgame(const char *basename)
{
	// TODO
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
