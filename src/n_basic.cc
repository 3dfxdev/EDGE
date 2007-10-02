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

#include "n_basic.h"


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

bool N_CreateReliableLink(int port);

net_node_c * N_AcceptReliableConn(void);

net_node_c * N_OpenReliableLink(void *address, int port);

void N_CloseReliableLink(net_node_c *node);

bool N_ReliableSend(net_node_c *node, const byte *data, int len);

int N_ReliableRecv(net_node_c *node, byte *buffer, int max_len);

//----------------------------------------------------------------------------

bool N_OpenBroadcastLink(int port);

void N_CloseBroadcastLink(void);

bool N_BroadcastSend(const byte *data, int len);

int N_BroadcastRecv(byte *buffer, int max_len);

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
