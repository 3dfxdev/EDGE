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

volatile int total_queued_games = 0;
volatile int total_played_games = 0;


game_c::game_c(const game_info_t *info) : state(ST_Zombie),
	mode(info->mode), skill(info->skill),
	min_players(info->min_players),
	num_bots(info->num_bots),
	num_players(0), bots_each(0), num_votes(0)
{
	strcpy(engine_name, info->engine_name);
    strcpy(game_name,   info->game_name);
	strcpy(level_name,  info->level_name);
}

game_c::~game_c()
{
	/* nothing needed */
}

bool game_c::HasPlayer(int client_id) const
{
	for (int i = 0; i < num_players; i++)
		if (players[i] == client_id)
			return true;

	return false;
}

void game_c::AddPlayer(int client_id)
{
	SYS_ASSERT(! HasPlayer(client_id));
	SYS_ASSERT(num_players < MP_PLAYER_MAX);

	players[num_players++] = client_id;
}

void game_c::RemovePlayer(int client_id)
{
	// FIXME: this is utterly fucked!

	for (int i = 0; i < MP_PLAYER_MAX; i++)
		if (players[i] == client_id)
			players[i] = -1;

	// FIXME: num_players--;
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

	strcpy(info->engine_name, engine_name);
	strcpy(info->game_name,   game_name);
	strcpy(info->level_name,  level_name);

	info->mode  = mode;
	info->skill = skill;
	 
	info->min_players = min_players;
	info->num_players = num_players;

	info->num_bots    = num_bots;
	info->num_votes   = num_votes;

	//!!! FIXME: features
	//!!! FIXME: wad_checksum, def_checksum
}

void game_c::InitGame()
{
///!!!	SYS_ASSERT(num_players >= 2);

	sv_gametic = 0;
	pl_min_tic = 0;  /* no past ticcmds */

	state = ST_Playing;

	total_played_games++;
	total_queued_games--;
}

void game_c::BumpGameTic()
{
	sv_gametic++;
}

void game_c::BumpMinTic()
{
	for (int p = 0; p < num_players; p++)
	{
		client_c *CL = clients[players[p]];

		CL->tics->Clear(pl_min_tic);
	}

	pl_min_tic++;
}

int game_c::ComputeMinTic() const
{
	int new_min_tic = INT_MAX;

	for (int p = 0; p < num_players; p++)
	{
		client_c *CL = clients[players[p]];

		if (CL->pl_gametic < new_min_tic)
			new_min_tic = CL->pl_gametic;
	}

	return new_min_tic;
}

int game_c::ComputeGroupAvail() const
{
	int offset;
	for (offset = 0; offset < MP_SAVETICS; offset++)
	{
		int p;
		for (p = 0; p < num_players; p++)
		{
			client_c *CL = clients[players[p]];

			if (! CL->tics->HasGot(sv_gametic + offset))
				break;
		}

		if (p != num_players)
			break;
	}

	return offset;
}

void game_c::CalcBots()
{
	static const int BOT_MAX = 8;

	SYS_ASSERT(num_players > 0);
	SYS_ASSERT(num_players <= MP_PLAYER_MAX);

	if (num_bots + num_players >= MP_PLAYER_MAX)
		num_bots = MP_PLAYER_MAX - num_players;

	SYS_ASSERT(num_bots >= 0);

	bots_each = num_bots  / num_players;

	if (bots_each > BOT_MAX)
		bots_each = BOT_MAX;

	num_bots = bots_each * num_players;
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
	pk->hd().data_len = sizeof(play_game_proto_t) + (GM->num_players - 1) *
		sizeof(s16_t);

	play_game_proto_t& gm = pk->pg_p();

	gm.real_players = GM->num_players;
	gm.bots_each    = GM->bots_each;

	// assume RAND_MAX could be small (e.g. 0x7FFF)
	gm.random_seed  = ((rand() & 0xFF) << 24) || ((rand() & 0xFF) << 16) ||
	                  ((rand() & 0xFF) <<  8) ||  (rand() & 0xFF);

	gm.first_player = 0;
	gm.last_player  = gm.first_player + GM->num_players - 1;

	for (int i = 0; i < GM->num_players; i++)
	{
		gm.client_list[i] = GM->players[gm.first_player + i];
	}

	gm.ByteSwap();
	gm.ByteSwapPlayers(GM->num_players);
}

static void BeginGame(game_c *GM)
{
	// update number of bots (could end up as zero)
	GM->CalcBots();

	// LOCK STRUCTURES
	{
		autolock_c LOCK(&global_lock);

		// move all clients from queueing to playing state

		for (int q = 0; q < GM->num_players; q++)
		{
			int client_id = GM->players[q];

			DebugPrintf("Moving Client %d from Queue to PLAY\n", client_id);

			client_c *CL = clients[client_id];

			CL->InitGame(q, GM->bots_each);
		}

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

		nlSetRemoteAddr(main_socket, &CL->addr);

		pk.hd().client = client_id;

		pk.Write(main_socket);

		pk.hd().ByteSwap();  // FIXME: rebuild header
	}
}

static void SV_build_tic_group(game_c *GM, packet_c *pk, int tic_num, int first, int count)
{
	pk->Clear();  // paranoia

	pk->SetType("Tg");
	pk->hd().flags = 0;
	pk->hd().data_len = sizeof(tic_group_proto_t) - sizeof(raw_ticcmd_t);

	// client field is set elsewhere

	tic_group_proto_t& tg = pk->tg_p();

	tg.gametic = GM->sv_gametic;
	tg.offset  = tic_num - tg.gametic;

	tg.first_player = first;
	tg.last_player  = tg.first_player + count - 1;

	raw_ticcmd_t *raw_cmds = tg.tic_cmds;
	int band = 1 + GM->bots_each;

	for (int p = first; p < first+count; p++, raw_cmds += band)
	{
		client_c *CL = clients[GM->players[p]];

		CL->tics->Read(GM->sv_gametic, raw_cmds);

		pk->hd().data_len += band * sizeof(raw_ticcmd_t);
	}

	tg.ByteSwap();
}

static void SV_send_all_tic_groups(int one_client, game_c *GM, int tic_num,
	int first_pl, int count_pl, bool retrans)
{
	// !!!! FIXME: handle more than TICCMD_FIT
	SYS_ASSERT(GM->num_players <= tic_group_proto_t::TICCMD_FIT);

	packet_c pk;

	SV_build_tic_group(GM, &pk, tic_num, first_pl, count_pl);

	if (retrans)
		pk.hd().flags |= header_proto_t::FL_Retransmission;

	// broadcast packet to each client

	for (int p = 0; p < GM->num_players; p++)
	{
		int client_id = GM->players[p];
		client_c *CL = clients[client_id];

		if (one_client >= 0 && client_id != one_client)
			continue;

		nlSetRemoteAddr(main_socket, &CL->addr);

		pk.hd().client = client_id;

		pk.Write(main_socket);

		pk.hd().ByteSwap();  // FIXME: rebuild header each time
	}
}

//------------------------------------------------------------------------

void PK_new_game(packet_c *pk)
{
	int client_id = pk->hd().client;

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

	// ensure names are NUL-terminated
	ng.info.engine_name[game_info_t::ENGINE_STR_MAX-1] = 0;
	ng.info.game_name  [game_info_t::GAME_STR_MAX  -1] = 0;
	ng.info.level_name [game_info_t::LEVEL_STR_MAX -1] = 0;

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

		GM->AddPlayer(client_id);

		CL->state = client_c::ST_Queueing;
		CL->game_id = game_id;
	}
	// NOW UNLOCKED

	total_queued_games++;

	LogPrintf(0, "Client %d created new game %d %s %s:%s (%s)\n",
		client_id, CL->game_id, GM->engine_name,
		GM->game_name, GM->level_name,
		(GM->mode == 'C') ? "Coop" : "DM");

	// send acknowledgement

	pk->SetType("Ng");
	pk->hd().flags = 0;
	pk->hd().data_len = sizeof(new_game_proto_t) - sizeof(game_info_t);

	ng.game = CL->game_id;
	ng.ByteSwap();

	pk->Write(main_socket);
}

void PK_join_queue(packet_c *pk)
{
	int client_id = pk->hd().client;

	client_c *CL = clients[client_id];

	join_queue_proto_t& jq = pk->jq_p();

	jq.ByteSwap();

	int game_id = jq.game;

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

		GM->AddPlayer(client_id);

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
	int client_id = pk->hd().client;

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
			GM->RemovePlayer(client_id);

			if (CL->voted)
				GM->num_votes--;
		}
		else if (CL->state == client_c::ST_Playing)
		{
			GM->RemovePlayer(client_id);

			// FIXME: notify other players (ETC !)
		}

		CL->state = client_c::ST_Browsing;
		CL->game_id = -1;
		CL->pl_index = -1;
		CL->voted = false;
	}
	// NOW UNLOCKED

	// FIXME send acknowledgement
}

void PK_vote_to_play(packet_c *pk)
{
	client_c *CL = clients[pk->hd().client];

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

	LogPrintf(0, "Client %d voted on game %d's queue (%d/%d).\n", pk->hd().client,
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

	qg.ByteSwap();

	// FIXME: check data_len

	short cur_id = qg.first_game;
	short total  = qg.last_game - qg.first_game + 1;

	if (total <= 0)  // FIXME: allow zero (get just the total)
		return;

	while (total > 0)
	{
		int count = MIN(total, query_game_proto_t::GAME_FIT);

		qg.total_games = games.size();
		qg.first_game = cur_id;
		qg.last_game = qg.first_game + count - 1;
		
		total -= count;

		for (byte i = 0; i < count; i++)
		{
			if (! GameExists(qg.first_game + i))
			{
				qg.info[i].state = game_info_t::GS_NotExist;
				continue;
			}

			game_c *GM = games[qg.first_game + i];
			SYS_NULL_CHECK(GM);

			GM->FillGameInfo(qg.info + i);
		}

		qg.ByteSwap();
		qg.ByteSwapInfo(count);

		pk->SetType("Qg");

		pk->hd().flags = 0;
		pk->hd().data_len = sizeof(query_game_proto_t) +
			(count - 1) * sizeof(game_info_t);

		pk->Write(main_socket);

		pk->hd().ByteSwap();  // FIXME: rebuild header each time
	}
}

void PK_ticcmd(packet_c *pk)
{
	int client_id = pk->hd().client;

	ticcmd_proto_t& tc = pk->tc_p();

	tc.ByteSwap();

	// FIXME: check data_len

	client_c *CL = clients[client_id];
	SYS_ASSERT(GameExists(CL->game_id));

	game_c *GM = games[CL->game_id];
	SYS_ASSERT(GM->HasPlayer(client_id));

	int pl_id = CL->pl_index;

	SYS_ASSERT(0 <= pl_id && pl_id < GM->num_players);
	SYS_ASSERT(GM->players[pl_id] == client_id);

	// NOTE: We are not LOCKING since tic-stuff is not touched by UI thread

	int cl_gametic = (int)tc.gametic; // FIXME: validate this

	if (cl_gametic > CL->pl_gametic)
		CL->pl_gametic = cl_gametic;

	int min_tic = GM->ComputeMinTic();

	while (GM->pl_min_tic < min_tic)
		GM->BumpMinTic();

	int got_tic = cl_gametic + tc.offset;
	int count = tc.count;

	const raw_ticcmd_t *raw_cmds = tc.tic_cmds;
	int band = 1 + GM->bots_each;

	int new_count = 0;

	for (; count > 0; got_tic++, raw_cmds += band, count--)
	{
		// in the past ?  ignore it.
		if (got_tic < GM->sv_gametic)
		{
			DebugPrintf("Ticcmd from client %d in past (%d < %d)\n",
				client_id, got_tic, GM->sv_gametic);
			continue;
		}

		// too far in the future ?  shouldn't happen (FIXME: send error)
		if (got_tic >= GM->sv_gametic + MP_SAVETICS)
		{
			LogPrintf(1, "Ticcmd from client %d too far ahead (%d >> %d)\n",
				client_id, got_tic, GM->sv_gametic);
			break; // skip remaining
		}

		if (CL->tics->HasGot(got_tic))
		{
			DebugPrintf("Old ticcmd #%d from client %d\n", got_tic, client_id);
			continue;
		}

		DebugPrintf("New ticcmd #%d from client %d\n", got_tic, client_id);

		CL->tics->Write(got_tic, raw_cmds);
		new_count++;
	}

	// check for missing packet (future write)
	if (new_count > 0 && ! CL->tics->HasGot(GM->sv_gametic))
	{
		// FIXME: speed up retransmit timer......
	}

	int saved_tics = GM->sv_gametic - GM->pl_min_tic;

	SYS_ASSERT(saved_tics >= 0);
	SYS_ASSERT(saved_tics <= MP_SAVETICS);

	int avail_tics = GM->ComputeGroupAvail();

	DebugPrintf("=== gametic %d avail %d saved %d\n", GM->sv_gametic,
		avail_tics, saved_tics);

	// when the save area is full, the server cannot advance
	if (avail_tics > (MP_SAVETICS - saved_tics))
		avail_tics = (MP_SAVETICS - saved_tics);

	for (; avail_tics > 0; avail_tics--)
	{
		SV_send_all_tic_groups(-1, GM, GM->sv_gametic, 0, GM->num_players, false);

		GM->BumpGameTic();
	}
}

void PK_tic_retransmit(packet_c *pk)
{
	int client_id = pk->hd().client;

	tic_retransmit_proto_t& tr = pk->tr_p();

	tr.ByteSwap();

	// FIXME: check data_len

	client_c *CL = clients[client_id];
	SYS_ASSERT(GameExists(CL->game_id));

	game_c *GM = games[CL->game_id];
	SYS_ASSERT(GM->HasPlayer(client_id));

	int pl_id = CL->pl_index;

	SYS_ASSERT(0 <= pl_id && pl_id < GM->num_players);
	SYS_ASSERT(GM->players[pl_id] == client_id);

	// NOTE: We are not LOCKING since tic-stuff is not touched by UI thread

	int cl_gametic = (int)tr.gametic; // FIXME: validate this

	if (cl_gametic > CL->pl_gametic)
		CL->pl_gametic = cl_gametic;

	int min_tic = GM->ComputeMinTic();

	while (GM->pl_min_tic < min_tic)
		GM->BumpMinTic();

	// FIXME: check player range
	int player_count = tr.last_player - tr.first_player + 1;

	DebugPrintf("Client %d tic retransmit req: tic %u (gametic %d)\n",
		client_id, tr.gametic + tr.offset, cl_gametic);

	int tic_num = tr.gametic + tr.offset;
	int count   = tr.count;

	for (; count > 0; tic_num++, count--)
	{
		// request too far in past ?  shouldn't happen (FIXME: send error)
		if (tic_num < GM->pl_min_tic)
		{
			LogPrintf(1, "Retrans-Req from client %d too far behind (%d < %d)\n",
				client_id, tic_num, GM->pl_min_tic);
			continue;
		}

		// request too far in future ?  FIXME: can this happen legit ???
		if (tic_num >= GM->sv_gametic)
		{
			LogPrintf(1, "Retrans-Req from client %d too far ahead (%d >= %d)\n",
				client_id, tic_num, GM->sv_gametic);
			break;
		}

		SV_send_all_tic_groups(client_id, GM, GM->sv_gametic, tr.first_player, player_count, true);
	}

	// !!! FIXME: the BumpMinTic above may mean the server can move on
}

