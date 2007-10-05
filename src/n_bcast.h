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

#ifndef __N_BROADCAST_H__
#define __N_BROADCAST_H__

extern bool nonet;


class net_address_c
{
public:
	byte addr[4];

	int port;

public:
	net_address_c() : port(0)
	{
		addr[0] = addr[1] = addr[2] = addr[3] = 0;
	}

	net_address_c(const byte *_ip, int _pt = 0) : port(_pt)
	{
		addr[0] = _ip[0]; addr[1] = _ip[1];
		addr[2] = _ip[2]; addr[3] = _ip[3];
	}

	net_address_c(const net_address_c& rhs) : port(rhs.port)
	{
		addr[0] = rhs.addr[0]; addr[1] = rhs.addr[1];
		addr[2] = rhs.addr[2]; addr[3] = rhs.addr[3];
	}

	~net_address_c()
	{ }

public:
	void FromSockAddr(const struct sockaddr_in *inaddr);

	void ToSockAddr(struct sockaddr_in *inaddr) const;

	const char *TempString() const;
	// returns a string representation of the address.
	// the result is a static buffer, hence is only valid
	// temporarily (until the next call).
};


bool N_StartupBroadcastLink(int port);
// Setup the broadcast link for sending and receiving packets.
// Returns true if successful, otherwise false.

void N_ShutdownBroadcastLink(void);
// Shut down the broadcast link.

bool N_BroadcastSend(const net_address_c *remote, const byte *data, int len);
// Send a packet on the broadcast link.
// The data must be an entire packet.
// returns true if successful, otherwise false.
// This call is non-blocking.

int N_BroadcastRecv(net_address_c *remote, byte *buffer, int max_len);
// Check if any packets have been received on the broadcast link,
// and return the oldest one.  This function should be called
// regularly to prevent the packet queue from overflowing.
//
// Returns the number of bytes read, 0 for none, or -1 if
// an error occurred.  Sets the 'remote' to the address the
// packet was received from.  The result will be one entire packet.
// This call is non-blocking.

#endif /* __N_BROADCAST_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
