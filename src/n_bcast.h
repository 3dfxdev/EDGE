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


bool N_OpenBroadcastLink(int port);
// Setup the broadcast link for sending and receiving packets.
// Returns true if successful, otherwise false.

void N_CloseBroadcastLink(void);
// Shut down the broadcast link.

bool N_BroadcastSend(const byte *data, int len);
// Send a packet on the broadcast link.
// The data must be an entire packet.
// returns true if successful, otherwise false.
// This call is non-blocking.

int N_BroadcastRecv(byte *buffer, int max_len);
// Check if any packets have been received on the broadcast link,
// and return the oldest one.  This function should be called
// regularly to prevent the packet queue from overflowing.
//
// Returns the number of bytes read, 0 for none, or -1 if
// an error occurred.  This call is non-blocking.
// The result will be an entire packet.

#endif /* __N_BROADCAST_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
