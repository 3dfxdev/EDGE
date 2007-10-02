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

#include "sdl_netx.h"

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

static TCPsocket host_conn_socket = 0;

bool N_CreateReliableLink(int port)
{
	IPaddress addr;

	addr.host = INADDR_ANY;
	addr.port = port;
	
	host_conn_socket = SDLNet_TCP_Open(&addr);

	if (! host_conn_socket)
		return false;

	return true;
}

net_node_c * N_AcceptReliableConn(void)
{
	// TODO
	
	return NULL;
}

net_node_c * N_OpenReliableLink(void *address, int port)
{
	// TODO
	
	return NULL;
}

void N_CloseReliableLink(net_node_c *node)
{
	// TODO
}

bool N_ReliableSend(net_node_c *node, const byte *data, int len)
{
	// Intentional Const Override (dumb SDL fuckers)
	int actual = SDLNet_TCP_Send(node->socket, (void*)data, len);

	if (actual < len)  // FIXME: mark node as gone
		return false;

	return true;
}

int N_ReliableRecv(net_node_c *node, byte *buffer, int max_len)
{
	int actual = SDLNet_TCP_Recv(node->socket, buffer, max_len);

	if (actual <= 0)
		return -1;  // error

	return actual;
}

//----------------------------------------------------------------------------

static UDPsocket my_udp_socket = 0;

static int host_broadcast_port = 0;

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
