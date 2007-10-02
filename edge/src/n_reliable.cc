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
//
//  FIXME  acknowledge SDL_net code
//
//----------------------------------------------------------------------------

#include "i_defs.h"
#include "i_netinc.h"

#include "n_reliable.h"


class net_node_c
{
public:

public:
	net_node_c() : XXX
	{ }

	~net_node_c()
	{ }
};



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

net_node_c * N_OpenReliableLink(const byte *address, int port)
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
	SYS_ASSERT(len > 0);

	// keep sending data until it's sent or an error occurs
	while (len > 0)
	{
		// clear global 'errno' variable before the call
		errno = 0;

// SHIT: may block !!! FIXME
			
		int actual = send(node->sock, data, len, 0);

		if (actual <= 0) // error occurred
		{
			if (errno == EINTR)
				continue;

			if (errno != EAGAIN)
				L_WriteDebug("N_ReliableSend: error %d\n", errno);

			return false;
		}

		SYS_ASSERT(actual <= len);

		len  -= actual;
		data += actual;
	}

	return true; //OK
}

int N_ReliableRecv(net_node_c *node, byte *buffer, int max_len)
{
	int actual;

	do
	{
		// clear global 'errno' variable before the call
		errno = 0;

		actual = recv(node->sock, buffer, max_len, 0);
	}
	while (errno == EINTR);

	if (actual < 0) // error occurred
	{
		if (errno != EAGAIN)
			L_WriteDebug("N_ReliableRecv: error %d\n", errno);

		return 0;
	}

	SYS_ASSERT(actual <= max_len);

	return actual;
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
