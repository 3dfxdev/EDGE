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

#ifndef __P_HUBS__
#define __P_HUBS__

class dormant_hub_c
{
public:
	int index;  // starts at 0

	std::string map_name;  // e.g. "MAP03"

public:
	 dormant_hub_c(int _idx, std::string& _map);
	~dormant_hub_c();
};

/* FUNCTIONS */

void HUB_Init(void);

void HUB_DeleteHubSaves(void);

void HUB_CopyHubsForSavegame(const char *basename);
void HUB_CopyHubsForLoadgame(const char *basename);


#endif /* __P_HUBS__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
