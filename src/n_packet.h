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

#ifndef __N_PACKET_H__
#define __N_PACKET_H__

#ifdef USE_HAWKNL

#include "protocol.h"

const char *N_GetAddrName(const NLaddress *addr);
// utility method: return string for address (Note: static buffer)

class packet_c
{
public:
	packet_c();
	~packet_c();

private:
	static const int MAX_PACKET = 512;
	static const int READ_GAP = 4;   // used to detect too-long packets
	static const int HEADER_LEN = sizeof(header_proto_t);

	char raw[MAX_PACKET];

public:
	inline int Capacity() const { return MAX_PACKET - HEADER_LEN; }
	inline bool CanHold(int length) const { return length <= Capacity(); }

	inline header_proto_t& hd() { return *((header_proto_t *)raw); }
	inline char *DataPtr() { return raw + HEADER_LEN; }

	void Clear();
	void ClearData();

	void SetType(const char *str);
	bool CheckType(const char *str);

	bool Read (NLsocket sock);
	bool Write(NLsocket sock);

	/* access to different packet types */

	inline error_proto_t&        er_p() { return *((error_proto_t *)DataPtr()); }
	inline connect_proto_t&      cn_p() { return *((connect_proto_t *)DataPtr()); }
	inline welcome_proto_t&      we_p() { return *((welcome_proto_t *)DataPtr()); }
	inline player_list_proto_t&  pl_p() { return *((player_list_proto_t *)DataPtr()); }
	inline ticcmd_proto_t&       tc_p() { return *((ticcmd_proto_t *)DataPtr()); }

	// "no" NO-OPERATION
	// "bd" BROADCAST-DISCOVERY
	// "dc" DISCONNECT
	// "sg" START-GAME
};

#endif  // USE_HAWKNL

#endif /* __N_PACKET_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
