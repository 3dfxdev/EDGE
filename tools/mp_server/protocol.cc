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
#include "epi/epitype.h"
#include "epi/epiendian.h"

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

void query_client_proto_t::ByteSwap(bool do_info)
{
	first_client  = SYS_BE_S16(first_client);
	total_clients = SYS_BE_S16(total_clients);

	if (do_info)  // FIXME: swap structures manually higher up ?
	{
		for (int c = 0; c < count; c++)
			info[c].ByteSwap();
	}
}

void game_info_t::ByteSwap()
{
	min_players = SYS_BE_S16(min_players);
	num_bots    = SYS_BE_S16(num_bots);

	features = SYS_BE_U32(features);

	wad_checksum = SYS_BE_U16(wad_checksum);
	def_checksum = SYS_BE_U16(def_checksum);

	num_players = SYS_BE_S16(num_players);
	num_votes   = SYS_BE_S16(num_votes);
}

void new_game_proto_t::ByteSwap()
{
	info.ByteSwap();

	game = SYS_BE_S16(game);
}

void query_game_proto_t::ByteSwap(bool do_info)
{
	first_game  = SYS_BE_S16(first_game);
	total_games = SYS_BE_S16(total_games);

	if (do_info)  // FIXME: swap structures manually higher up ?
	{
		for (int c = 0; c < count; c++)
			info[c].ByteSwap();
	}
}

void join_queue_proto_t::ByteSwap()
{
	game = SYS_BE_S16(game);
}

void play_game_proto_t::ByteSwap()
{
	random_seed = SYS_BE_U32(random_seed);

	for (int p = 0; p < count; p++)
		client_list[p] = SYS_BE_S16(client_list[p]);
}

void raw_ticcmd_t::ByteSwap()
{
	shorts[0] = SYS_BE_U16(shorts[0]);
	shorts[1] = SYS_BE_U16(shorts[1]);
	shorts[2] = SYS_BE_U16(shorts[2]);
	shorts[3] = SYS_BE_U16(shorts[3]);
}

void ticcmd_proto_t::ByteSwap(bool do_tics)
{
	gametic = SYS_BE_U32(gametic);
	
	if (do_tics)
	{
		for (int t = 0; t < count; t++)
			tic_cmds[t].ByteSwap();
	}
}

void tic_group_proto_t::ByteSwap(bool do_tics)
{
	gametic = SYS_BE_U32(gametic);

	if (do_tics)
	{
		for (int t = 0; t < count; t++)
			tic_cmds[t].ByteSwap();
	}
}

void tic_retransmit_proto_t::ByteSwap()
{
	gametic = SYS_BE_U32(gametic);
}

void message_proto_t::ByteSwap()
{
	dest = SYS_BE_S16(dest);
}
