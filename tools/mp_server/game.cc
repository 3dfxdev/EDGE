//------------------------------------------------------------------------
//  Games
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

#include "includes.h"

#include "autolock.h"
#include "game.h"
#include "network.h"
#include "packet.h"
#include "protocol.h"
#include "ui_log.h"

//
// Game list
//
std::vector<game_c *> games;


game_c::game_c(const game_info_t *info) : state(ST_Zombie),
    game_name(info->game_name), level_name(info->level_name),
	mode(info->mode), skill(info->skill),
	min_players(info->min_players), engine_ver(info->engine_ver),
	queuers(), players(), num_players(0), num_votes(0),
	tic_counter(0), got_cmds(), tic_cmds(NULL),
	zombie_millies(0)
{
	/* nothing needed */
}

game_c::~game_c()
{
	/* nothing needed */
}

bool game_c::InQueue(int client_id) const
{
	return (std::find(queuers.begin(), queuers.end(), client_id) !=
			queuers.end());
}

bool game_c::InGame(int client_id) const
{
	return (std::find(players.begin(), players.end(), client_id) !=
			players.end());
}

int game_c::AddToQueue(int client_id)
{
	SYS_ASSERT(! InQueue(client_id));

	queuers.push_back(client_id);

	return queuers.size() - 1;
}

int game_c::AddToGame(int client_id)
{
	SYS_ASSERT(! InGame(client_id));

	players.push_back(client_id);
	num_players = players.size();

	return players.size() - 1;
}

void game_c::RemoveFromQueue(int client_id)
{
	std::vector<int>::iterator new_end =
		std::remove(queuers.begin(), queuers.end(), client_id);
	
	queuers.erase(new_end, queuers.end());
}

void game_c::RemoveFromGame(int client_id)
{
	std::vector<int>::iterator new_end =
		std::remove(players.begin(), players.end(), client_id);
	
	players.erase(new_end, players.end());

	num_players = players.size();
}

void game_c::FillGameInfo(game_info_t *info) const
{
	switch (state)
	{
		case ST_Zombie:
			info->state = game_info_t::GS_NotExist;
			return;  /* no other info */

		case ST_Queueing: info->state = game_info_t::GS_Queued;  break;
		case ST_Playing:  info->state = game_info_t::GS_Playing; break;

		default:
			AssertFail("Bad game state %d\n", info->state);
			break;
	}

	strcpy(info->game_name,  game_name.c_str());
	strcpy(info->level_name, level_name.c_str());

	info->mode  = mode;
	info->skill = skill;
	 
	info->min_players = min_players;
	info->engine_ver  = engine_ver;

	if (state == ST_Queueing)
		info->num_players = queuers.size();
	else
		info->num_players = num_players;

	info->num_votes = num_votes;
}

void game_c::InitGame()
{
///!!!	SYS_ASSERT(num_players >= 2);

	tic_counter = 0;

	got_cmds.reset();

	if (tic_cmds)
		delete[] tic_cmds;

	tic_cmds = new raw_ticcmd_t[num_players];

	state = ST_Playing;
}

void game_c::BumpTic()
{
	tic_counter++;

	got_cmds.reset();
}

//------------------------------------------------------------------------

bool GameExists(short idx)
{
	if (idx < 0 || (unsigned)idx >= games.size())
		return false;

	return games[idx] != NULL;
}

static void SV_build_play_game(game_c *GM, packet_c *pk)
{
	pk->Clear();  // paranoia

	pk->SetType("Pg");

	pk->hd().flags = 0;
	pk->hd().request_id = GM->tic_counter;

	pk->hd().data_len = sizeof(play_game_proto_t) + (GM->num_players - 1) *
		sizeof(s16_t);

	play_game_proto_t& gm = pk->pg_p();

	// Fixme: support more players

	gm.total_players = GM->num_players;
	gm.first_player  = 0;
	gm.count = GM->num_players;

	for (int i = 0; i < gm.count; i++)
	{
		gm.client_id[i] = GM->players[gm.first_player + i];
	}

	gm.ByteSwap();
}

static void BeginGame(game_c *GM)
{
	// LOCK STRUCTURES
	{
		autolock_c LOCK(&global_lock);

		// move all clients from queueing to playing state

		SYS_ASSERT(GM->queuers.size() == (unsigned)GM->num_players);
		SYS_ASSERT(GM->players.size() == 0);

		GM->players.reserve(GM->num_players);

		for (int q = 0; q < GM->num_players; q++)
		{
			int client_id = GM->queuers[q];

			DebugPrintf("Moving Client %d from Queue to PLAY\n", client_id);

			client_c *CL = clients[client_id];
			SYS_ASSERT(CL->state == client_c::ST_Queueing);

			CL->state = client_c::ST_Playing;

			GM->RemoveFromQueue(client_id);
			CL->player_id = GM->AddToGame(client_id);
		}

		SYS_ASSERT(GM->queuers.size() == 0);

		GM->InitGame();
	}
	// NOW UNLOCKED

	// send PLAY packets !!

	packet_c pk;

	SV_build_play_game(GM, &pk);

	for (int p = 0; p < GM->num_players; p++)
	{
		int client_id = GM->players[p];

		client_c *CL = clients[client_id];

		pk.hd().source = client_id;
		pk.hd().dest   = client_id;
		pk.hd().game   = CL->game_id;

		nlSetRemoteAddr(main_socket, &CL->addr);

		pk.Write(main_socket);

		pk.hd().ByteSwap();  // FIXME: rebuild header
	}
}

static void SV_build_tic_group(game_c *GM, packet_c *pk, int first, int count)
{
	pk->Clear();  // paranoia

	pk->SetType("Tg");

	pk->hd().flags = 0;
	pk->hd().request_id = GM->tic_counter;

	pk->hd().data_len = sizeof(tic_group_proto_t) + (count - 1) *
		sizeof(raw_ticcmd_t);

	// source, dest, and game are set elsewhere

	tic_group_proto_t& tg = pk->tg_p();

	tg.first_player = first;
	tg.count = count;

	memcpy(tg.tic_cmds, GM->tic_cmds + first, count * sizeof(raw_ticcmd_t));

	tg.ByteSwap();
}

static void SV_send_all_tic_groups(game_c *GM)
{
	// !!!! FIXME: handle more than TICCMD_FIT
	SYS_ASSERT(GM->num_players <= tic_group_proto_t::TICCMD_FIT);

	packet_c pk;

	SV_build_tic_group(GM, &pk, 0, GM->num_players);

	// broadcast packet to each client

	for (int p = 0; p < GM->num_players; p++)
	{
		int client_id = GM->players[p];

		client_c *CL = clients[client_id];

		pk.hd().source = client_id;
		pk.hd().dest   = client_id;
		pk.hd().game   = CL->game_id;

		nlSetRemoteAddr(main_socket, &CL->addr);

		pk.Write(main_socket);

		pk.hd().ByteSwap();  // FIXME: rebuild header each time
	}
}

//------------------------------------------------------------------------

void PK_new_game(packet_c *pk)
{
	int client_id = pk->hd().source;

	client_c *CL = clients[client_id];

	if (CL->state != client_c::ST_Browsing)
	{
		// FIXME: log and/or error packet
		return;
	}

	// FIXME: check if too many games

	new_game_proto_t& ng = pk->ng_p();

	ng.ByteSwap();

	// !!! FIXME: validate game info

	game_c *GM;

	// LOCK STRUCTURES
	{
		autolock_c LOCK(&global_lock);

		// find free slot
		int game_id;

		for (game_id = 0; (unsigned)game_id < games.size(); game_id++)
			if (games[game_id] == NULL)
				break;
		
		GM = new game_c(&ng.info);

		if ((unsigned)game_id == games.size())
			games.push_back(GM);
		else
			games[game_id] = GM;

		GM->AddToQueue(client_id);
		GM->num_players++;

		CL->state = client_c::ST_Queueing;
		CL->game_id = game_id;
	}
	// NOW UNLOCKED

	LogPrintf(0, "Client %d created new game %d %s:%s (%s)\n",
		client_id, CL->game_id, GM->game_name.c_str(),
		GM->level_name.c_str(), (GM->mode == 'C') ? "Coop" : "DM");

	// send acknowledgement

	pk->SetType("Ng");
	pk->hd().flags = 0;
	pk->hd().data_len = 0;
	pk->hd().game = CL->game_id;

	pk->Write(main_socket);
}

void PK_join_queue(packet_c *pk)
{
	int client_id = pk->hd().source;

	client_c *CL = clients[client_id];

	int game_id = pk->hd().game;

	if (! GameExists(game_id))
	{
		// FIXME: log and/or error packet
		return;
	}

	if (CL->state != client_c::ST_Browsing)
	{
		// FIXME: log and/or error packet
		return;
	}

	// FIXME: check if queue is FULL

	game_c *GM;

	// LOCK STRUCTURES
	{
		autolock_c LOCK(&global_lock);

		GM = games[CL->game_id];

		GM->AddToQueue(client_id);
		GM->num_players++;

		CL->state = client_c::ST_Queueing;
		CL->game_id = game_id;
	}
	// NOW UNLOCKED

	LogPrintf(0, "Client %d joined game %d's queue (%d/%d).\n", client_id,
		game_id, GM->num_players, GM->min_players);

	// FIXME send acknowledgement
}

void PK_leave_game(packet_c *pk)
{
	int client_id = pk->hd().source;

	client_c *CL = clients[client_id];

	if (CL->state != client_c::ST_Queueing && CL->state != client_c::ST_Playing)
	{
		// FIXME: log and/or error packet
		return;
	}

	LogPrintf(0, "Client %d leaving game %d's queue.\n", client_id, CL->game_id);

	// LOCK STRUCTURES
	{
		autolock_c LOCK(&global_lock);

		SYS_ASSERT(GameExists(CL->game_id));
		game_c *GM = games[CL->game_id];

		if (CL->state == client_c::ST_Queueing)
		{
			GM->RemoveFromQueue(client_id);

			if (CL->voted)
				GM->num_votes--;
		}
		else if (CL->state == client_c::ST_Playing)
		{
			GM->RemoveFromGame(client_id);

			// FIXME: notify other players
		}

		GM->num_players--;

		CL->state = client_c::ST_Browsing;
		CL->game_id = -1;
		CL->player_id = -1;
		CL->voted = false;
	}
	// NOW UNLOCKED

	// FIXME send acknowledgement
}

void PK_vote_to_play(packet_c *pk)
{
	client_c *CL = clients[pk->hd().source];

	if (CL->state != client_c::ST_Queueing)
	{
		// FIXME: log and/or error packet
		return;
	}

	if (CL->voted)  // assume resend
		return;

	CL->voted = true;

	// FIXME: send acknowledge

	SYS_ASSERT(GameExists(CL->game_id));
	game_c *GM = games[CL->game_id];

	GM->num_votes++;

	LogPrintf(0, "Client %d voted on game %d's queue (%d/%d).\n", pk->hd().source,
		CL->game_id, GM->num_votes, GM->num_players);

	if (GM->num_votes == GM->num_players &&
		GM->num_players >= GM->min_players)
	{
		LogPrintf(0, "GAME %d HAS BEGUN !!\n", CL->game_id);

		BeginGame(GM);
	}
}

//------------------------------------------------------------------------

void PK_query_game(packet_c *pk)
{
	query_game_proto_t& qg = pk->qg_p();

	qg.ByteSwap(false);

	// FIXME: check data_len

	short cur_id = qg.first_id;
	byte  total  = qg.count;

	if (total == 0)
		return;

	while (total > 0)
	{
		qg.first_id = cur_id;
		qg.count = MIN(total, query_game_proto_t::GAME_FIT);
		
		total -= qg.count;

		for (byte i = 0; i < qg.count; i++)
		{
			if (! GameExists(qg.first_id + i))
			{
				qg.info[i].state = game_info_t::GS_NotExist;
				continue;
			}

			game_c *GM = games[qg.first_id + i];
			SYS_NULL_CHECK(GM);

			GM->FillGameInfo(qg.info + i);
		}

		qg.ByteSwap(true);

		pk->SetType("Qg");

		pk->hd().flags = 0;
		pk->hd().data_len = sizeof(query_game_proto_t) +
			(qg.count - 1) * sizeof(game_info_t);

		pk->Write(main_socket);

		pk->hd().ByteSwap();  // FIXME: rebuild header each time
	}
}

void PK_ticcmd(packet_c *pk)
{
	ticcmd_proto_t& tc = pk->tc_p();

	tc.ByteSwap();

	// FIXME: check data_len

	client_c *CL = clients[pk->hd().source];
	SYS_ASSERT(GameExists(CL->game_id));

	game_c *GM = games[CL->game_id];
	SYS_ASSERT(GM->InGame(pk->hd().source));

	int plyr_id = CL->player_id;

	SYS_ASSERT(0 <= plyr_id && plyr_id < GM->num_players);
	SYS_ASSERT(GM->players[plyr_id] == pk->hd().source);

	// NOTE: Not LOCKING since tic-stuff is not touched by UI thread

	if (pk->hd().request_id != GM->tic_counter)
	{
		LogPrintf(1, "Ticcmd from client %d, bad tic # (%d != %d)\n",
			pk->hd().source, pk->hd().request_id, GM->tic_counter);
		return;
	}

	if (GM->got_cmds.test(plyr_id))
	{
		LogPrintf(1, "Ticcmd from client %d, already received\n",
			pk->hd().source);
		return;
	}

	memcpy(GM->tic_cmds + plyr_id, &tc.tic_cmd, sizeof(raw_ticcmd_t));

	GM->got_cmds.set(plyr_id);

	if (GM->got_cmds.count() == (unsigned)GM->num_players)
	{
		SV_send_all_tic_groups(GM);

		GM->BumpTic();
	}
}

