//----------------------------------------------------------------------------
//  System Networking Basics
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
//  FIXME  acknowledge HawkNL code
//
//----------------------------------------------------------------------------

#include "i_defs.h"
#include "i_net.h"

#ifdef LINUX
#include <linux/if.h>
#include <linux/sockios.h>
#endif

#include "epi/endianess.h"

#include "m_argv.h"


bool nonet = true;

net_address_c n_local_addr;

net_address_c n_broadcast_send;
net_address_c n_broadcast_listen;


static bool GetLocalAddress(void)
{
}

#ifdef LINUX
static bool Scan_IFCONFIG(bool want_local)
{
	/* scan the IFCONFIG data to find the first broadcast-capable
	 * IP address (excluding the loop-back device 127.0.0.1).
	 */
    struct ifconf config;
    int offset = 0;

	int buf_len = 8192;
    char *buffer;

	buffer = new char[buf_len];

    config.ifc_len = buf_len;
    config.ifc_buf = buffer;

    if (ioctl(host_broadcast_sock, SIOCGIFCONF, &config) < 0)
	{
		I_Printf("WARNING: cannot find broadcast address (SIOCGIFCONF)\n");
		delete buffer;
		return;
	}

	int num_addr = 0;

	// offset marches through the return buffer to help us find subsequent
	// entries (entries can have various sizes, but always >=
	// sizeof(struct ifreq)... IP entries have minimum size).

I_Printf("offset: %d  sizeof:%d  ifc_len:%d\n",
offset, (int)sizeof(struct ifreq),  (int)config.ifc_len);

    while (num_addr < MAX_BCAST_ADDRS &&
		   offset + (int)sizeof(struct ifreq) <= (int)config.ifc_len)
	{
        struct ifreq req;

		memcpy(&req, buffer+offset, sizeof(req));

I_Printf("> name = %6.6s\n", req.ifr_name);
I_Printf("> family = %d\n", req.ifr_addr.sa_family);

        // make sure it's an entry for IP (not AppleTalk, ARP, etc)
        if (req.ifr_addr.sa_family == AF_INET)
		{
            // we don't care about the actual address, what we
			// really want is the interface's broadcast address.
			// -AJA- need to ignore loopback (127.0.0.1).

net_address_c cur_A;
cur_A.FromSockAddr((struct sockaddr_in*)&req.ifr_addr);
I_Printf(">> address = %s\n", cur_A.TempString());
	
            if (cur_A.addr[0] != 127 && cur_A.addr[0] != 0 &&
				cur_A.addr[0] != 255 &&
				ioctl(host_broadcast_sock, SIOCGIFBRDADDR, &req) >= 0)
			{
				net_address_c *addr = new net_address_c();

				addr->FromSockAddr((struct sockaddr_in*) &req.ifr_addr);

				broadcast_addresses[num_addr++] = addr;

I_Printf(">> found broadcast addr: %s\n", addr->TempString());
			}
        }

        // Increment offset to point at the next entry.
        // _SIZEOF_ADDR_IFREQ() is provided in Mac OS X and
		// accounts for entries whose address families use
        // long addresses.

#ifdef _SIZEOF_ADDR_IFREQ
        offset += _SIZEOF_ADDR_IFREQ(req);
#else
		offset += sizeof(struct ifreq);
#endif
	}

	delete buffer;
}

#endif


static bool SetupAddresses(void)
{
	bool got_local = false;

	// explicitly given via command-line option?
	const char *p = M_GetParm("-local");
	if (p)
	{
		n_local_addr.FromString(p);

		got_local = true;
	}

	if (! got_local)
		got_local = GetLocalAddress();

#ifdef WIN32
	n_broadcast_send.FromString("255.255.255.255");  // INADDR_BROADCAST

	n_broadcast_listen.FromString("0.0.0.0");  // INADDR_ANY

#else // LINUX

	if (Scan_IFCONFIG(!got_local))
		got_local = true;
	else
	{
		if (got_local)
		{
			// we will guess the correct addresses,
			// using INADDR_BROADCAST for the transmit addr
			// and a modified local address for the listen addr.

			n_broadcast_send.FromString("255.255.255.255");	

			n_broadcast_listen = n_local_addr;
			n_broadcast_listen.GuessBroadcast();
		}
	}

#endif

	if (! got_local)
	{
		I_Printf("I_StartupNetwork: unable to determine local address!\n");
		return false;
	}

	return true; // OK
}


void I_StartupNetwork(void)
{
	if (M_CheckParm("-nonet"))
		return;

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

	if (! SetupAddresses())
		return;

	nonet = false;

	I_Printf("I_StartupNetwork: Initialised OK.\n");
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


net_address_c::net_address_c(const byte *_ip, int _pt = 0) : port(_pt)
{
	addr[0] = _ip[0];
	addr[1] = _ip[1];
	addr[2] = _ip[2];
	addr[3] = _ip[3];
}

net_address_c::net_address_c(const net_address_c& rhs) : port(rhs.port)
{
	addr[0] = rhs.addr[0];
	addr[1] = rhs.addr[1];
	addr[2] = rhs.addr[2];
	addr[3] = rhs.addr[3];
}

void net_address_c::FromSockAddr(const struct sockaddr_in *inaddr)
{
	port = ntohs(inaddr->sin_port);

	const byte *source = (const byte*) &inaddr->sin_addr.s_addr;

	addr[0] = source[0];
	addr[1] = source[1];
	addr[2] = source[2];
	addr[3] = source[3];
}

void net_address_c::ToSockAddr(struct sockaddr_in *inaddr) const
{
	memset(inaddr, 0, sizeof(struct sockaddr_in));

	inaddr->sin_family = AF_INET;
	inaddr->sin_port   = htons(port);

	byte *dest = (byte *) &inaddr->sin_addr.s_addr;

	dest[0] = addr[0];
	dest[1] = addr[1];
	dest[2] = addr[2];
	dest[3] = addr[3];
}

const char * net_address_c::TempString() const
{
	static char buffer[256];

	sprintf(buffer, "%d.%d.%d.%d:%d",
			(int)addr[0],
			(int)addr[1],
			(int)addr[2],
			(int)addr[3], port);

	return buffer;
}

bool net_address_c::FromString(const char *str)
{
	if (strchr(str, ':') != NULL)
		port = atoi(strchr(str, ':') + 1);

	int tmp_addr[4];

	if (4 != sscanf(str, " %d.%d.%d.%d  ",
			   temp_addr+0, temp_addr+1, temp_addr+2, temp_addr+3))
		return false;

	addr[0] = tmp_addr[0];
	addr[1] = tmp_addr[1];
	addr[2] = tmp_addr[2];
	addr[3] = tmp_addr[3];

	return true;
}

void net_address_c::GuessBroadcast(void)
{
	// class A subnet mask = 255.0.0.0
	// class B subnet mask = 255.255.0.0
	// class C subnet mask = 255.255.255.0

	if (addr[0] < 128) addr[1] = 255;
	if (addr[0] < 192) addr[2] = 255;

	addr[3] = 255;
}


//----------------------------------------------------------------------------


static SOCKET host_broadcast_sock = INVALID_SOCKET;

static int host_broadcast_port = -1;


#define MAX_BCAST_ADDRS  4

static net_address_c *broadcast_addresses[MAX_BCAST_ADDRS];
 

static void FindBroadcastAddresses(void)
{
	memset(broadcast_addresses, 0, sizeof(broadcast_addresses));

/* Win32 (at least, Win 98) seems to accept 255.255.255.255
 * as a valid broadcast address.
 * I'll live with that for now.
 */
#ifdef WIN32
	static const bc_win32[4] = { 255,255,255,255 };

	broadcast_addresses[0] = new net_address_c(bc_win32, host_broadcast_port);

#else // LINUX


	if (num_addr == 0)
	{
		I_Printf("WARNING: cannot find any broadcast addresses!\n");
	}

#endif // WIN32 vs LINUX
}


void I_SetNonBlock(SOCKET sock, bool enable)
{
#ifdef WIN32
	unsigned long mode = enable ? 1 : 0;

	ioctlsocket(sock, FIONBIO, &mode);

#elif defined(O_NONBLOCK)
	fcntl(sock, F_SETFL, enable ? O_NONBLOCK : 0);
#endif
}

void I_SetNoDelay(SOCKET sock, bool enable)
{
#ifdef TCP_NODELAY
	int mode = enable ? 1 : 0;

	setsockopt(sock, IPPROTO_TCP, TCP_NODELAY,
			(char*)&mode, sizeof(mode));
#endif
}

void I_SetBroadcast(SOCKET sock, bool enable)
{
#ifdef SO_BROADCAST
	int mode = enable ? 1 : 0;

    setsockopt(sock, SOL_SOCKET, SO_BROADCAST,
               (char*)&mode, sizeof(mode));
#endif
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
