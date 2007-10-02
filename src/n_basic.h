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

#ifndef __N_BASIC_H__
#define __N_BASIC_H__

extern bool nonet;

class net_node_c;

void I_StartupNetwork(void);
void I_ShutdownNetwork(void);

//----------------------------------------------------------------------------

bool N_CreateReliableLink(int port);
// (HOST ONLY)
// Create the link (socket) which allows clients to connect to us.
// Returns true if successful, otherwise false.

net_node_c * N_AcceptReliableConn(void);
// (HOST ONLY)
// Check to see if any clients have (requested to) connect, and
// allow them to connect.  This function should be called
// regularly to prevent the accept queue from overflowing.
//
// When successful, returns a new node.
// Returns NULL if not successful.

net_node_c * N_OpenReliableLink(void *address, int port);
// (CLIENT ONLY)
// Open a link to the Host at the given address and port.
//
// When successful, returns a new node.
// Returns NULL if an error occurred.

void N_CloseReliableLink(net_node_c *node);
// Close the link to a Client or Host.

bool N_ReliableSend(net_node_c *node, const byte *data, int len);
// Send the data to the specified node.
// Returns true if successful, false on error.
// This call is non-blocking.

int N_ReliableRecv(net_node_c *node, byte *buffer, int max_len);
// Receive upto 'max_len' of data from the specified node.
// Returns the number of bytes read, 0 for none, or -1 if
// an error occurred.  This call is non-blocking.

//----------------------------------------------------------------------------

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

#endif /* __N_BASIC_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
