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
//
//  FIXME  acknowledge SDL_net code
//
//  FIXME  acknowledge SDLNetx code
//
//----------------------------------------------------------------------------

#include "i_defs.h"
#include "i_netinc.h"

#include "n_bcast.h"


bool nonet = true;


void I_StartupNetwork(void)
{
#ifdef WIN32
	// start up the Windows networking
	WORD version_wanted = MAKEWORD(1,1);
	WSADATA wsaData;

	if (WSAStartup(version_wanted, &wsaData) != 0)
	{
		I_Printf("I_StartupNetwork: Couldn't initialize Winsock 1.1\n");
		return;
	}
#endif

	nonet = false;

	I_Printf("I_StartupNetwork: SDL_net Initialised OK.\n");
}


void I_ShutdownNetwork(void)
{
	if (! nonet)
	{
		nonet = true;

#ifdef WIN32
		// clean up Windows networking
		if (WSACleanup() == SOCKET_ERROR )
		{
			if (WSAGetLastError() == WSAEINPROGRESS)
			{
				WSACancelBlockingCall();
				WSACleanup();
			}
		}
#endif
	}
}


//----------------------------------------------------------------------------


static SOCKET host_bcast_socket = INVALID_SOCKET;

static int host_broadcast_port = 0;
 

bool N_StartupBroadcastLink(int port)
{
	if (nonet)
		return false;
	
	SYS_ASSERT(port > 0);

	host_bcast_port = port;

	host_bcast_socket = ...

	if (host_bcast_socket == INVALID_SOCKET)
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

void N_ShutdownBroadcastLink(void)
{
	if (my_udp_socket)
	{
		SDLNet_UDP_Close(my_udp_socket);
		my_udp_socket = 0;
	}
}

bool N_BroadcastSend(const byte *data, int len)
{
	pk->address.port = host_broadcast_port;

	if (SDLNetx_UDP_Broadcast(my_udp_socket, pk) <= 0)
		return false;



	return true;
}


int N_BroadcastRecv(byte *buffer, int max_len)
{
	// if (SocketReady(host_bcast_socket)

	int sock_len;
	struct sockaddr_in sock_addr;

	{
		UDPpacket *packet;

		packet = packets[numrecv];

		int sock_len = sizeof(sock_addr);

		int actual = recvfrom(host_bcast_socket,
				buffer, max_len, 0 /* flags */,
				(struct sockaddr *)&sock_addr,
#ifdef USE_GUSI_SOCKETS
				(unsigned int *)&sock_len);
#else
				&sock_len);
#endif

		if ( packet->status >= 0 )
		{
			packet->len = packet->status;
			packet->address.host = sock_addr.sin_addr.s_addr;
			packet->address.port = sock_addr.sin_port;
		}

	return -1;
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
