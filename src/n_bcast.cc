//----------------------------------------------------------------------------
//  EDGE Networking : Broadcast Links
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
#include "i_netinc.h"

#include "n_bcast.h"


bool nonet = true;

static UDPsocket my_udp_socket = 0;

static int host_broadcast_port = 0;
 

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


bool N_OpenBroadcastLink(int port)
{
	if (nonet)
		return false;
	
	if (port > 0)
		host_broadcast_port = port;

	my_udp_socket = SDLNet_UDP_Open(port);

	if (! my_udp_socket)
	{
		// clients should try again with port=0
		I_Debugf("SDLNet_UDP_Open failed.\n");
		return false;
	}

	if (SDLNetx_EnableBroadcast(my_udp_socket) <= 0)
	{
		I_Debugf("SDLNetx_EnableBroadcast failed.\n");
		/* continue ??? */
	}

	return true;
}

void N_CloseBroadcastLink(void)
{
	if (my_udp_socket)
	{
		SDLNet_UDP_Close(my_udp_socket);
		my_udp_socket = 0;
	}
}

bool N_BroadcastSend(const byte *data, int len)
{
	UDPpacket *pk = SDLNet_AllocPacket(len);

	memcpy(pk->data, data, len);

	pk->channel = -1;
	pk->address.port = host_broadcast_port;

	if (SDLNetx_UDP_Broadcast(my_udp_socket, pk) <= 0)
		return false;

	// FIXME: free the 'pk' structure (can we do it now???)

	return true;
}

int N_BroadcastRecv(byte *buffer, int max_len)
{
	// TODO

	return -1;
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
