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

const char *N_GetErrorStr(void);
const char *N_GetAddrName(const NLaddress *addr);

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
	inline connect_proto_t&      cs_p() { return *((connect_proto_t *)DataPtr()); }
	inline query_client_proto_t& qc_p() { return *((query_client_proto_t *)DataPtr()); }
	inline new_game_proto_t&     ng_p() { return *((new_game_proto_t *)DataPtr()); }
	inline query_game_proto_t&   qg_p() { return *((query_game_proto_t *)DataPtr()); }
	inline play_game_proto_t&    pg_p() { return *((play_game_proto_t *)DataPtr()); }
	inline ticcmd_proto_t&       tc_p() { return *((ticcmd_proto_t *)DataPtr()); }
	inline tic_group_proto_t&    tg_p() { return *((tic_group_proto_t *)DataPtr()); }
	inline tic_retransmit_proto_t& tr_p() { return *((tic_retransmit_proto_t *)DataPtr()); }
};

#endif  // USE_HAWKNL

#endif /* __N_PACKET_H__ */
