//------------------------------------------------------------------------
//  Packets
//------------------------------------------------------------------------
//
//  Edge MultiPlayer Server (C) 2005  Andrew Apted
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
//------------------------------------------------------------------------

#include "includes.h"

#include "buffer.h"
#include "network.h"
#include "packet.h"
#include "ui_log.h"


int volatile total_in_packets = 0;
int volatile total_in_bytes = 0;

int volatile total_out_packets = 0;
int volatile total_out_bytes = 0;


packet_c::packet_c()
{
	Clear();
}

packet_c::~packet_c()
{
	/* nothing needed */
}

void packet_c::Clear()
{
	memset(raw, 0, sizeof(raw));
}

void packet_c::ClearData()
{
	memset(raw + HEADER_LEN, 0, sizeof(raw) - HEADER_LEN);
}


///---char *packet_c::DataPtr()
///---{
///---	return raw + HEADER_LEN;
///---}
///---
///---bool packet_c::DataCapacity(int length) const
///---{
///---	assert(length >= 0);
///---
///---	return length <= (MAX_PACKET - HEADER_LEN);
///---}

bool packet_c::Read(NLsocket sock)
{
	int len = nlRead(sock, raw, MAX_PACKET + READ_GAP);

	if (len == 0)  // no packet
		return false;

	if (len < 0)
	{
		LogPrintf(2, "PacketRead failed: %s\n", GetNLErrorStr());
		return false;
	}

	total_in_packets++;
	total_in_bytes += len;

	if (len < HEADER_LEN)
	{
		LogPrintf(1, "PacketRead: short packet received (%d bytes)\n", len);
		return false;
	}

	if (len > MAX_PACKET)
	{
		LogPrintf(1, "PacketRead: long packet received (%d bytes)\n", len);
		return false;
	}

	if (hd().type[0] == 0 || hd().type[1] == 0)
	{
		LogPrintf(1, "PacketRead: illegal packet type found (0x%2x%2x)\n",
			hd().type[0], hd().type[1]);
		return false;
	}
	
	hd().ByteSwap();

	if (hd().data_len < 0 || ! CanHold(hd().data_len))
	{
		LogPrintf(1, "PacketRead: illegal data length found (%d)\n", hd().data_len);
		return false;
	}

	if (hd().data_len != len - HEADER_LEN)
	{
		LogPrintf(1, "PacketRead: data length does not match packet (%d != %d)\n",
			hd().data_len, len - HEADER_LEN);
		return false;
	}
	
	return true;
}

void packet_c::Write(NLsocket sock)
{
	SYS_ASSERT(hd().type[0] != 0 && hd().type[1] != 0);
	SYS_ASSERT(hd().data_len >= 0 && CanHold(hd().data_len));

	int len = HEADER_LEN + hd().data_len;

	hd().ByteSwap();

	int wrote = nlWrite(sock, raw, len); 

	if (wrote == 0)
	{
		// unable to write now, need to buffer it and try later
		BufferPacket(sock, raw, len);
		return;
	}
	else if (wrote < 0)
	{
		LogPrintf(2, "PacketWrite failed: %s\n", GetNLErrorStr());
		return;
	}

	total_out_packets++;
	total_out_bytes += len;

	if (wrote != len)
	{
		LogPrintf(1, "PacketWrite: wrote less data (%d < %d)\n", wrote, len);
	}
}

void packet_c::SetType(const char *str)
{
	SYS_ASSERT(strlen(str) == 2);

	hd().type[0] = str[0];
	hd().type[1] = str[1];
}

bool packet_c::CheckType(const char *str)
{
	SYS_ASSERT(strlen(str) == 2);

	return (hd().type[0] == str[0] && hd().type[1] == str[1]);
}

