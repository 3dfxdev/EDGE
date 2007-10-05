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

#include "epi/endianess.h"

#include "n_bcast.h"


bool nonet = true;


extern void N_ChangeNonBlock(SOCKET sock, bool enable);


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


void net_address_c::FromSockAddr(const struct sockaddr_in *inaddr)
{
	port = EPI_BE_U16(inaddr->sin_port);

	addr[0] = (inaddr->sin_addr.s_addr >> 24) & 0xFF;
	addr[1] = (inaddr->sin_addr.s_addr >> 16) & 0xFF;
	addr[2] = (inaddr->sin_addr.s_addr >>  8) & 0xFF;
	addr[3] = (inaddr->sin_addr.s_addr      ) & 0xFF;
}

void net_address_c::ToSockAddr(struct sockaddr_in *inaddr) const
{
	memset(inaddr, 0, sizeof(struct sockaddr_in));

	inaddr->sin_family = AF_INET;

	inaddr->sin_port = EPI_BE_U16(port);

	inaddr->sin_addr.s_addr =
		(addr[0] << 24) || (addr[1] << 16) ||
		(addr[2] <<  8) || (addr[3]      );
}

const char * net_address_c::TempString() const
{
	static char buffer[256];

	sprintf(buffer, "%d.%d.%d.%d:%d",
			(int)addr[0], (int)addr[1],
			(int)addr[2], (int)addr[3], port);

	return buffer;
}


//----------------------------------------------------------------------------


static SOCKET host_broadcast_sock = INVALID_SOCKET;

static int host_broadcast_port = -1;

#define MAX_BCAST_ADDRS  4

static net_address_c *bcast_addr_list[MAX_BCAST_ADDRS];
 

void N_ChangeBroadcastFlag(SOCKET sock, bool enable)
{
#ifdef SO_BROADCAST
	int mode = enable ? 1 : 0;

    setsockopt(sock, SOL_SOCKET, SO_BROADCAST,
               (char*)&mode, sizeof(mode));
#endif
}

bool N_StartupBroadcastLink(int port)
{
	if (nonet)
		return false;
	
	SYS_ASSERT(port > 0);

	host_broadcast_port = port;
	host_broadcast_sock = socket(AF_INET, SOCK_DGRAM, 0);

	if (host_broadcast_sock == INVALID_SOCKET)
	{
		L_WriteDebug("N_StartupBroadcastLink: couldn't create socket!\n");
		return false;
	}

#ifdef LINUX
	// TODO: read broadcast addresses
#endif

	struct sockaddr_in sock_addr;

	memset(&sock_addr, 0, sizeof(sock_addr));

	sock_addr.sin_family = AF_INET;
	sock_addr.sin_addr.s_addr = INADDR_ANY;
	sock_addr.sin_port = EPI_BE_U16(host_broadcast_port);

	// bind the socket for listening
	if (bind(host_broadcast_sock, (struct sockaddr *)&sock_addr,
			 sizeof(sock_addr)) == SOCKET_ERROR)
	{
		I_Printf("N_StartupBroadcastLink: Couldn't bind to local port\n");
		N_ShutdownBroadcastLink();
		return false;
	}

	// get the channel host address
	net_address_c my_addr;

	my_addr.FromSockAddr(&sock_addr);
I_Printf(">>> my_addr : %s\n", my_addr.TempString());


	N_ChangeBroadcastFlag(host_broadcast_sock, 1);

	return true;
}

void N_ShutdownBroadcastLink(void)
{
	if (host_broadcast_sock != INVALID_SOCKET)
	{
		closesocket(host_broadcast_sock);

		host_broadcast_sock = INVALID_SOCKET;
		host_broadcast_port = -1;
	}
}

bool N_BroadcastSend(const net_address_c *remote, const byte *data, int len)
{
	pk->address.port = host_broadcast_port;

	if (SDLNetx_UDP_Broadcast(my_udp_socket, pk) <= 0)
		return false;

/* Win32 (at least, Win 98) seems to accept 255.255.255.255
 * as a valid broadcast address.
 * I'll live with that for now.
 */
#ifdef WIN32
	inPacket->address.host = 0xFFFFFFFF;

	int theResult = SDLNet_UDP_Send(inSocket, -1, inPacket);

	struct sockaddr_in sock_addr;

	remote->ToSockAddr(&sock_addr);

	int actual = sendto(sock->channel,
			packets[i]->data, packets[i]->len, 0,
			(struct sockaddr *)&sock_addr, sizeof(sock_addr));

	if (actual < 0) // error occurred
		...
#endif

	return true;
}


int N_BroadcastRecv(net_address_c *remote, byte *buffer, int max_len)
{
	struct sockaddr_in sock_addr;

	memset(&sock_addr, 0, sizeof(sock_addr));

	int len_var = sizeof(sock_addr);

	// clear global 'errno' var before the call
	errno = 0;
	
	int actual = recvfrom(host_broadcast_sock,
			buffer, max_len, 0 /* flags */,
			(struct sockaddr *)&sock_addr,
#ifdef USE_GUSI_SOCKETS
			(unsigned int *)&len_var);
#else
			&len_var);
#endif

	if (actual < 0) // error occurred
	{
		if (errno == EAGAIN)
			return 0;

		L_WriteDebug("N_BroadcastRecv: error %d\n", errno);
		return -1;
	}

	// set remote addr/port (output parameters)
	remote->FromSockAddr(&sock_addr);

	return actual; //OK
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
