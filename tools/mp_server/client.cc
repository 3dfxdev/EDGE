//------------------------------------------------------------------------
//  Clients
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

#include "autolock.h"
#include "client.h"
#include "game.h"
#include "lib_util.h"
#include "mp_main.h"
#include "network.h"
#include "packet.h"
#include "protocol.h"
#include "ui_log.h"

//
// Client list
//
client_c *clients[MPS_DEF_MAX_CLIENT];

volatile int total_clients = 0;


client_c::client_c(const NLsocket *_sock, const NLaddress *_addr) :
	state(ST_Connecting),
	game_id(-1), pl_index(-1), voted(false),
	tics(NULL), die_time(1 << 30)
{
	memcpy(&sock, _sock, sizeof(sock));
	memcpy(&addr, _addr, sizeof(addr));
}

client_c::~client_c()
{
	/* nothing to do */
}

bool client_c::CheckAddr(const NLaddress *remote_addr) const
{
	return (nlAddrCompare(&addr, remote_addr) == NL_TRUE);
}

bool client_c::Verify(NLsocket WSOCK, const NLaddress *remote_addr) const
{
	if (state == ST_Going || state == ST_Gone)
		return false;

	if (sock != WSOCK)
		return false;

	return (nlAddrCompare(&addr, remote_addr) == NL_TRUE);
}

int client_c::CompareName(const char *other) const
{
	return StrCaseCmp(name, other);
}

void client_c::SetClientInfo(const client_info_t *info)
{
	strcpy(name, info->name);
}

void client_c::FillClientInfo(client_info_t *info) const
{
	switch (state)
	{
		case ST_Connecting:
		case ST_Going:
		case ST_Gone:
			info->state = client_info_t::CS_NotExist;
			return;  /* no other info */

		case ST_Browsing: info->state = client_info_t::CS_Browsing; break;
		case ST_Playing:  info->state = client_info_t::CS_Playing;  break;

		case ST_Queueing: info->state = voted ? client_info_t::CS_Voted :
						  client_info_t::CS_Queueing;
			break;

		default:
			AssertFail("Bad client state %d\n", info->state);
			break;
	}

	strcpy(info->name, name);

	info->game = game_id;
}

bool client_c::MatchDest(int dest, int game) const
{
	SYS_ASSERT(dest < 0);

	switch (dest)
	{
		case message_proto_t::D_ALL_BROWSING:
			return (state == ST_Browsing);

		case message_proto_t::D_ALL_NOT_PLAYING:
			return (state != ST_Playing);

		case message_proto_t::D_OTHER_QUEUERS:
			return (state == ST_Queueing && game_id == game);

		case message_proto_t::D_OTHER_PLAYERS:
			return (state == ST_Playing && game_id == game);

		case message_proto_t::D_ABSOLUTELY_EVERYONE:
			return true;

		default:
			// this shouldn't happen (already been checked)
			AssertFail("Bad special dest %d\n", dest);
			break;
	}

	return false;
}

void client_c::Write(packet_c *pk)
{
	pk->Write(sock);
}

void client_c::TransmitMessage(packet_c *pk)
{
///UDP	nlSetRemoteAddr(sock, &addr);

	pk->Write(sock);

	pk->hd().ByteSwap();  // for subsequent usage
}

void client_c::InitGame(int idx, int bots_each)
{
	SYS_ASSERT(state == client_c::ST_Queueing);

	pl_index = idx;
	pl_gametic = 0;

	if (tics)
		delete tics;

	tics = new tic_store_c(1 + bots_each);

	BumpRetryTime();

	state = ST_Playing;
}

void client_c::BumpRetryTime(int delta)
{
	tic_retry_time = cur_net_time + delta;
}

void client_c::BumpDieTime(int delta)
{
	die_time = cur_net_time + delta;
}

void client_c::TimeoutTic(int cl_idx)
{
// !!! FIXME: we don't fucking know our own client number

	if (state != ST_Playing)
		return;

	game_c *GM = games[game_id];
	SYS_ASSERT(GM);

	if (tic_retry_time < 0)  // OK, we already have this tic
		return;
	
	if (cur_net_time < tic_retry_time)
		return;

	// reset timer to try again
	BumpRetryTime();

	DebugPrintf("Time %d: Sending retransmit request to player %d game %d\n",
		cur_net_time, pl_index, game_id);

	packet_c pk;

	pk.Clear();  // paranoia

	pk.SetType("Tr");
	pk.hd().flags = 0;
	pk.hd().client = cl_idx;
	pk.hd().data_len = sizeof(tic_retransmit_proto_t);

	tic_retransmit_proto_t& tr = pk.tr_p();

	tr.gametic = GM->sv_gametic;
	tr.offset  = 0;
	tr.count   = 1;  // could be smarter ??

	tr.first_player = tr.last_player = pl_index;

	tr.ByteSwap();

///UDP	nlSetRemoteAddr(main_socket, &addr);

	Write(&pk);
}

void client_c::SendError(packet_c *pk, const char *type, const char *str, ...)
{
	SYS_ASSERT(strlen(type) == 2);

	char buffer[2048];
	
	va_list args;

	va_start(args, str);
	vsprintf(buffer, str, args);
	va_end(args);

	buffer[error_proto_t::ERR_STR_MAX] = 0;  // limit message length

	/* data */

	pk->er_p().type[0] = type[0];
	pk->er_p().type[1] = type[1];

	strcpy(pk->er_p().message, buffer);

	pk->er_p().ByteSwap();

	/* header */

	int len = strlen(buffer) + sizeof(error_proto_t);
	SYS_ASSERT(pk->CanHold(len));

	pk->SetType("Er");
	pk->hd().flags = 0;
	pk->hd().data_len = len;

	Write(pk);
}


//------------------------------------------------------------------------

bool ClientExists(short idx)
{
	if (idx < 0 || (unsigned)idx >= MPS_DEF_MAX_CLIENT)
		return false;

	return clients[idx] != NULL;
}

bool VerifyClient(short idx, NLsocket SOCK, const NLaddress *remote_addr)
{
	return ClientExists(idx) ? clients[idx]->Verify(SOCK, remote_addr) : false;
}

bool ValidatePlayerName(const char *name)
{
	// FIXME: better check
	
	if (name[0] == 0)
		return false;
	
	return true;
}

void ClientTimeouts()
{
	// lock ??

	for (int cl_idx = 0; (unsigned)cl_idx < MPS_DEF_MAX_CLIENT; cl_idx++)
	{
		client_c *CL = clients[cl_idx];

		if (! CL)
			continue;

		if (cur_net_time >= CL->die_time)
		{
			// FIXME: remove client !!
			continue;
		}

		CL->TimeoutTic(cl_idx);
	}
}

//------------------------------------------------------------------------

void PK_connect_to_server(packet_c *pk, NLsocket SOCK)
{
	// find client using socket

	int client_id;

	for (client_id = 0; client_id < MPS_DEF_MAX_CLIENT; client_id++)
		if (clients[client_id] && clients[client_id]->sock == SOCK)
			break;
	
	if (client_id >= MPS_DEF_MAX_CLIENT)
	{
		DebugPrintf("PK_connect_to_server: no such client (SOCK 0x%x)\n", SOCK);
		return;
	}

	client_c *CL = clients[client_id];

	connect_proto_t& con = pk->cs_p();

	// FIXME: check if too many games

	// FIXME: check data_len

	con.ByteSwap();

	con.info.name[client_info_t::NAME_LEN-1] = 0;  // ensure NUL-terminated

	if (! ValidatePlayerName(con.info.name))
	{
		CL->SendError(pk, "bn", "Invalid name !");
		LogPrintf(1, "Client %d tried to connect with invalid name.\n", client_id);
		return;
	}

	// check if name already used
	for (int c2 = 0; (unsigned)c2 < MPS_DEF_MAX_CLIENT; c2++)
	{
		client_c *CL = clients[c2];

		if (! CL || c2 == client_id)
			continue;

		if (CL->CompareName(con.info.name) == 0)
		{
			CL->SendError(pk, "en", "Name already exists !");
			LogPrintf(2, "Client %d tried to connect, name was already used.\n", client_id);
			return;
		}
	}

	// successful!
	CL->state = client_c::ST_Browsing;

	CL->SetClientInfo(&con.info);

	pk->SetType("Cs");

	pk->hd().flags = 0;
	pk->hd().data_len = sizeof(connect_proto_t) - sizeof(client_info_t);
	pk->hd().client = client_id;

	con.server_ver   = MPSERVER_VER_HEX;
	con.protocol_ver = MP_PROTOCOL_VER;

	con.ByteSwap();

	clients[client_id]->Write(pk);

	LogPrintf(0, "Client %d connected: name '%s'\n", client_id, con.info.name);
}

void PK_leave_server(packet_c *pk)
{
	client_c *CL = clients[pk->hd().client];

	// client actually removed during timeout handling
	CL->state = client_c::ST_Going;
	LogPrintf(0, "Client %d has disconnected.\n", pk->hd().client);

	// FIXME: !!!! remove from queue/game

	pk->SetType("Ls");

	pk->hd().flags = 0;
	pk->hd().data_len = 0;

	CL->Write(pk);
}

void PK_keep_alive(packet_c *pk)
{
	DebugPrintf("Time %d: keep alive from client %d\n", cur_net_time,
		pk->hd().client);
}

void PK_query_client(packet_c *pk)
{
	client_c *CL = clients[pk->hd().client];

	query_client_proto_t& qc = pk->qc_p();

	qc.ByteSwap();

	// FIXME: check data_len

	short cur_id = qc.first_client;
	short total  = qc.last_client - qc.first_client + 1;

	if (total <= 0) // FIXME: allow zero (just get total field)
		return;

	while (total > 0)
	{
		short count = MIN(total, query_client_proto_t::CLIENT_FIT);

		qc.total_clients = total_clients;
		qc.first_client = cur_id;
		qc.last_client = qc.first_client + count - 1;

		total -= count;

		for (byte i = 0; i < count; i++)
		{
			if (! ClientExists(qc.first_client + i))
			{
				qc.info[i].state = client_info_t::CS_NotExist;
				continue;
			}

			client_c *QCL = clients[qc.first_client + i];
			SYS_NULL_CHECK(QCL);

			QCL->FillClientInfo(qc.info + i);
		}

		qc.ByteSwap();
		qc.ByteSwapInfo(count);

		// prepare packet header
		pk->SetType("Qc");

		pk->hd().flags = 0;
		pk->hd().data_len = sizeof(query_client_proto_t) +
			(count - 1) * sizeof(client_info_t);

		CL->Write(pk);

		pk->hd().ByteSwap();  // FIXME: rebuild header each time
	}
}

void PK_message(packet_c *pk)
{
	int client_id = pk->hd().client;

	message_proto_t& mp = pk->ms_p();

	int dest_id = SYS_BE_S16(mp.dest);

	pk->SetType("Ms");

	if (dest_id >= 0)
	{
		if (ClientExists(dest_id))
		{
			clients[dest_id]->TransmitMessage(pk);
			return;
		}
		else
		{
			// !!! FIXME: no-such-client error
		}
		return;
	}

	int game_id = clients[client_id]->game_id;

	switch (dest_id)
	{
		case message_proto_t::D_ALL_BROWSING:
		case message_proto_t::D_ALL_NOT_PLAYING:
		case message_proto_t::D_OTHER_QUEUERS:
		case message_proto_t::D_OTHER_PLAYERS:
		case message_proto_t::D_ABSOLUTELY_EVERYONE:
		{
			for (int c = 0; (unsigned)c < MPS_DEF_MAX_CLIENT; c++)
			{
				client_c *CL = clients[c];
				
				if (! CL)
					continue;

				// Fixme: have a "not me too" flag
				// if (c == client_id && ! me_too) continue;

				// handle special destinations
				if (dest_id < 0 && ! CL->MatchDest(dest_id, game_id))
					continue;

				CL->TransmitMessage(pk);
			}
		}
		break;

		default:
			// FIXME: log and/or error packet
			break;
	}
}

//------------------------------------------------------------------------

void PK_broadcast_discovery_UDP(packet_c *pk, NLaddress *remote_addr)
{
	// very simple: just send it back!
	// (client will get our address and port)

	LogPrintf(0, "Broadcast discovery from addr %s\n", GetAddrName(remote_addr));

	/* send reply back to originator */
	nlSetRemoteAddr(bcast_socket, remote_addr);

	pk->SetType("Bd");
	pk->hd().flags = 0;
	pk->hd().client = 0;
	pk->hd().data_len = 0;

	pk->Write(bcast_socket);
}

//------------------------------------------------------------------------

void CreateNewClient(NLsocket SOCK)
{
	NLaddress remote_addr;

	nlGetRemoteAddr(SOCK, &remote_addr);

	if (total_clients >= MPS_DEF_MAX_CLIENT)
	{
		LogPrintf(2, "Connection rejected, too many clients (%d)\n", total_clients);
		nlClose(SOCK);
		return;
	}
	
	int client_id;

	// check if already connected
	// Note: don't need to LOCK here, since other thread never modifies
	//       the data (only copies it).

	for (client_id = 0; (unsigned)client_id < MPS_DEF_MAX_CLIENT; client_id++)
	{
		client_c *CL = clients[client_id];

		if (CL && CL->CheckAddr(&remote_addr))
		{
			// CL->SendError(pk, "ac", "Already connected !");
			LogPrintf(2, "Client %d was already connected.\n", client_id);
			return;
		}
	}

	// determine client ID#
	for (client_id = 0; (unsigned)client_id < MPS_DEF_MAX_CLIENT; client_id++)
	{
		if (clients[client_id] == NULL)
			break;
	}

	SYS_ASSERT((unsigned)client_id < MPS_DEF_MAX_CLIENT);

	// LOCK STRUCTURES
	{
		autolock_c LOCK(&global_lock);

		clients[client_id] = new client_c(&SOCK, &remote_addr);

		total_clients++;
	}
	// NOW UNLOCKED

	nlGroupAddSocket(main_group, SOCK);
}

