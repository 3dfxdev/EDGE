//----------------------------------------------------------------------------
//  EDGE Networking : Reliable Links
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

#include "n_reliable.h"



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


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
