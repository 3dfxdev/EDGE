//------------------------------------------------------------------------
//  Clients
//------------------------------------------------------------------------
//
//  Edge MultiPlayer Server (C) 2004  Andrew Apted
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

#ifndef __CLIENT_H__
#define __CLIENT_H__

#include "protocol.h"
#include "ticstore.h"

class packet_c;

class client_c
{
public:
	client_c(const client_info_t *info, const NLaddress *_addr);
	~client_c();

public: //!!!!  private:
	enum
	{
		ST_Browsing = 0,
		ST_Queueing,
		ST_Playing,
		ST_Going,   // still exists in game lists
		ST_Gone
	};

	int state;

	char name[client_info_t::NAME_LEN];
	NLaddress addr;

	int game_id;
	int pl_index;

	bool voted;

	// --- tic handling ---
	// {
		int pl_gametic;

		tic_store_c *tics;

		int tic_wait;    // units are 1/100 second.
		int tic_wait_total;
	// }

public:
	bool CheckAddr(const NLaddress *remote_addr) const;
	bool Verify(const NLaddress *remote_addr) const;
	
	int CompareName(const char *other) const;
	void FillClientInfo(client_info_t *info) const;

	bool MatchDest(int dest, int game) const;
	void TransmitMessage(packet_c *pk);

	void InitGame(int idx, int bots_each);
};

extern std::vector<client_c *> clients;
extern volatile int total_clients;

bool VerifyClient(short idx, const NLaddress *remote_addr);

void SV_send_error(packet_c *pk, const char *type, const char *str, ...);

void PK_connect_to_server(packet_c *pk, NLaddress *remote_addr);
void PK_leave_server(packet_c *pk);
void PK_broadcast_discovery(packet_c *pk, NLaddress *remote_addr);
void PK_keep_alive(packet_c *pk);
void PK_query_client(packet_c *pk);
void PK_message(packet_c *pk);

#endif /* __CLIENT_H__ */
