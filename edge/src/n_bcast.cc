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

static int host_bcast_port = 0;
 

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

#ifdef LINUX
	// TODO: read broadcast addresses
#endif

	int enable_BC = 1;

    setsockopt(host_bcast_socket, SOL_SOCKET, SO_BROADCAST,
               (char*)&enable_BC, sizeof(enable_BC));

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
	pk->address.port = host_bcast_port;

	if (SDLNetx_UDP_Broadcast(my_udp_socket, pk) <= 0)
		return false;
/* Win32 (at least, Win 98) seems to accept 255.255.255.255
 * as a valid broadcast address.
 * I'll live with that for now.
 */
#ifdef WIN32

	inPacket->address.host = 0xffffffff;
	int theResult = SDLNet_UDP_Send(inSocket, -1, inPacket);

	struct sockaddr_in sock_addr;

	int sock_len = sizeof(sock_addr);

	sock_addr.sin_addr.s_addr = //XXX
	sock_addr.sin_port = //XXX
	sock_addr.sin_family = AF_INET;

	int actual = sendto(sock->channel,
			packets[i]->data, packets[i]->len, 0,
			(struct sockaddr *)&sock_addr,sock_len);

	if (actual < 0) // error occurred
		...
#endif

	return true;
}


int N_BroadcastRecv(byte *buffer, int max_len)
{
	struct sockaddr_in sock_addr;

	int sock_len = sizeof(sock_addr);

	// clear global 'errno' var before the call
	errno = 0;
	
	int actual = recvfrom(host_bcast_socket,
			buffer, max_len, 0 /* flags */,
			(struct sockaddr *)&sock_addr,
#ifdef USE_GUSI_SOCKETS
			(unsigned int *)&sock_len);
#else
			&sock_len);
#endif

	if (actual < 0) // error occurred
	{
		if (errno == EAGAIN)
			return 0;

		L_WriteDebug("N_BroadcastRecv: error %d\n", errno);
		return -1;
	}

	// TODO: set remote addr/port (output parameters)
	//	packet->address.host = sock_addr.sin_addr.s_addr;
	//	packet->address.port = sock_addr.sin_port;

	return actual; //OK
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
