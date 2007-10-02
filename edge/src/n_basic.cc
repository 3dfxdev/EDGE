//----------------------------------------------------------------------------
//  EDGE Networking Primitives
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2007  The EDGE Team.
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
#include "i_sdlinc.h"


bool nonet = true;
 

void I_StartupNetwork(void)
{
	if (SDLNet_Init() != 0)
	{
		I_Printf("I_StartupNetwork: SDL_net Initialisation FAILED.\n");
		return;
	}

	nonet = false;

	I_Printf("I_StartupNetwork: SDL_net Initialised OK.\n");
}


void I_ShutdownNetwork(void)
{
	if (! nonet)
	{
		nonet = true;
		SDLNet_Quit();
	}
}


//----------------------------------------------------------------------------


N_CreateReliableLink (host only)
N_AcceptReliableConn (host only)

N_OpenReliableLink   (client only)
N_CloseReliableLink  (client only)

N_ReliableSend
N_ReliableRecv


//----------------------------------------------------------------------------


N_OpenBroadcastLink
N_CloseBroadcastLink

N_BroadcastSend
N_BroadcastRecv


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
