//----------------------------------------------------------------------------
//  EDGE Packet handling
//----------------------------------------------------------------------------
// 
//  Copyright (c) 2004-2005  The EDGE Team.
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

#ifdef USE_HAWKNL  // applies to whole file

#include "i_defs.h"
#include "nl.h"

#include "epi/types.h"
#include "epi/endianess.h"

#include "n_packet.h"

//
// N_GetAddrName
//
const char *N_GetAddrName(const NLaddress *addr)
{
	static char name_buf[NL_MAX_STRING_LENGTH];

	return nlAddrToString(addr, name_buf);
}

//----------------------------------------------------------------------------

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


bool packet_c::Read(NLsocket sock)
{
	int len = nlRead(sock, raw, MAX_PACKET + READ_GAP);

	if (len == 0)  // no packet
		return false;

	if (len < 0)
	{
		L_WriteDebug("PacketRead failed: %s\n", I_NetworkReturnError());
		return false;
	}

	if (len < HEADER_LEN)
	{
		L_WriteDebug("PacketRead: short packet received (%d bytes)\n", len);
		return false;
	}

	if (len > MAX_PACKET)
	{
		L_WriteDebug("PacketRead: long packet received (%d bytes)\n", len);
		return false;
	}

	if (hd().type[0] == 0 || hd().type[1] == 0)
	{
		L_WriteDebug("PacketRead: illegal packet type found (0x%2x%2x)\n",
			hd().type[0], hd().type[1]);
		return false;
	}

	hd().ByteSwap();

	if (hd().data_len < 0 || ! CanHold(hd().data_len))
	{
		L_WriteDebug("PacketRead: illegal data length found (%d)\n", hd().data_len);
		return false;
	}

	if (hd().data_len != len - HEADER_LEN)
	{
		L_WriteDebug("PacketRead: data length does not match packet (%d != %d)\n",
			hd().data_len, len - HEADER_LEN);
		return false;
	}
	
	return true;
}

bool packet_c::Write(NLsocket sock)
{
	DEV_ASSERT2(hd().type[0] != 0 && hd().type[1] != 0);
	DEV_ASSERT2(hd().data_len >= 0 && CanHold(hd().data_len));

	int len = HEADER_LEN + hd().data_len;

	hd().ByteSwap();

	int wrote = nlWrite(sock, raw, len); 

	if (wrote == 0)
	{
		// FIXME: try to write later ???
		L_WriteDebug("PacketWrite failed -- no space in buffers.\n");
		return false;
	}
	else if (wrote < 0)
	{
		L_WriteDebug("PacketWrite failed: %s\n", I_NetworkReturnError());
		return false;
	}
	else if (wrote != len)
	{
		L_WriteDebug("PacketWrite: wrote less data (%d < %d)\n", wrote, len);
		return false;
	}

	return true;
}

void packet_c::SetType(const char *str)
{
	DEV_ASSERT2(strlen(str) == 2);

	hd().type[0] = str[0];
	hd().type[1] = str[1];
}

bool packet_c::CheckType(const char *str)
{
	DEV_ASSERT2(strlen(str) == 2);

	return (hd().type[0] == str[0] && hd().type[1] == str[1]);
}

#endif  // USE_HAWKNL
