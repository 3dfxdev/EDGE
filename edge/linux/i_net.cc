//----------------------------------------------------------------------------
//  EDGE Linux Networking Code
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2005  The EDGE Team.
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

#include "../i_defs.h"
#include "i_sysinc.h"

#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <sys/ioctl.h>

// -AJA- 2005/04/01: Yet another work-around for the huge pile of shit called C++
#define ntohl  ntohl_FUCK
#define ntohs  ntohs_FUCK
#define htonl  htonl_FUCK
#define htons  htons_FUCK

#include <arpa/inet.h>
#include <linux/netdevice.h> 

// possible: <linux/if.h>
//           <linux/sockios.h>

#include <netinet/if_ether.h>
#include <netinet/in.h>

//
// I_InitNetwork
//
void I_InitNetwork(void)
{
}

//
// I_NetCmd
//
// Sends/Receives a packet.
//
void I_NetCmd(void)
{
}

//
// Determine IP address of local host via ethernet adapter.
// Result will be in dotted notation (like 11.22.33.44) or NULL
// if something went wrong (invalid interface).
//
const char * I_LocalIPAddrString(const char *eth_name)
{
	int fd;

	fd = socket(AF_INET, SOCK_DGRAM, 0);

	if (fd < 0)
	{
		// perror("socket");
		return NULL;
	}

	struct ifreq ifr;

	strcpy(ifr.ifr_name, eth_name);

	if (ioctl(fd, SIOCGIFADDR, &ifr) < 0)
	{
		close(fd);
		return NULL;
	}

	close(fd);

	struct sockaddr_in *skt = (struct sockaddr_in*) &ifr.ifr_addr;

	if (! skt->sin_addr.s_addr)
		return NULL;

	static char buffer[1024];

	strcpy(buffer, inet_ntoa(skt->sin_addr));
	return buffer;
}

