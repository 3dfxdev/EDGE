//------------------------------------------------------------------------
//  Protocol structures
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

#ifndef EDGEVER
#include "includes.h"
#else
/* EDGE */
#include "epi/types.h"
#include "epi/endianess.h"

#define SYS_BE_S16  EPI_BE_S16
#define SYS_BE_U16  EPI_BE_U16
#define SYS_BE_S32  EPI_BE_S32
#define SYS_BE_U32  EPI_BE_U32
#endif

#include "protocol.h"

void header_proto_t::ByteSwap()
{
	flags    = SYS_BE_S16(flags);
	request  = SYS_BE_S16(request);
	client   = SYS_BE_S16(client);
	data_len = SYS_BE_S16(data_len);
}

void error_proto_t::ByteSwap()
{
	/* nothing needed -- yet */
}

void client_info_t::ByteSwap()
{
	game = SYS_BE_S16(game);
}

void connect_proto_t::ByteSwap()
{
	server_ver   = SYS_BE_S16(server_ver);
	protocol_ver = SYS_BE_S16(protocol_ver);

	info.ByteSwap();
}

void query_client_proto_t::ByteSwap()
{
	first_client  = SYS_BE_S16(first_client);
	last_client   = SYS_BE_S16(last_client);
	total_clients = SYS_BE_S16(total_clients);
}

void query_client_proto_t::ByteSwapInfo(int num_info)
{
	for (int i = 0; i < num_info; i++)
		info[i].ByteSwap();
}

void game_info_t::ByteSwap()
{
	features = SYS_BE_U32(features);

	wad_checksum = SYS_BE_U16(wad_checksum);
	def_checksum = SYS_BE_U16(def_checksum);
}

void new_game_proto_t::ByteSwap()
{
	info.ByteSwap();

	game = SYS_BE_S16(game);
}

void query_game_proto_t::ByteSwap()
{
	first_game  = SYS_BE_S16(first_game);
	last_game   = SYS_BE_S16(last_game);
	total_games = SYS_BE_S16(total_games);
}

void query_game_proto_t::ByteSwapInfo(int num_info)
{
	for (int i = 0; i < num_info; i++)
		info[i].ByteSwap();
}

void join_queue_proto_t::ByteSwap()
{
	game = SYS_BE_S16(game);
}

void play_game_proto_t::ByteSwap()
{
	random_seed = SYS_BE_U32(random_seed);
}

void play_game_proto_t::ByteSwapPlayers(int num_players)
{
	for (int p = 0; p < num_players; p++)
		client_list[p] = SYS_BE_S16(client_list[p]);
}

void raw_ticcmd_t::ByteSwap()
{
	shorts[0] = SYS_BE_U16(shorts[0]);
	shorts[1] = SYS_BE_U16(shorts[1]);
	shorts[2] = SYS_BE_U16(shorts[2]);
	shorts[3] = SYS_BE_U16(shorts[3]);
}

void ticcmd_proto_t::ByteSwap()
{
	gametic = SYS_BE_U32(gametic);
}

void ticcmd_proto_t::ByteSwapCmds(int num_cmds)
{
	for (int t = 0; t < num_cmds; t++)
		tic_cmds[t].ByteSwap();
}

void tic_group_proto_t::ByteSwap()
{
	gametic = SYS_BE_U32(gametic);
}

void tic_group_proto_t::ByteSwapCmds(int num_cmds)
{
	for (int t = 0; t < num_cmds; t++)
		tic_cmds[t].ByteSwap();
}

void tic_retransmit_proto_t::ByteSwap()
{
	gametic = SYS_BE_U32(gametic);
}

void message_proto_t::ByteSwap()
{
	dest = SYS_BE_S16(dest);
}
