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

#include "includes.h"
#include "protocol.h"

void header_proto_t::ByteSwap()
{
	flags      = SYS_BE_S16(flags);
	request_id = SYS_BE_S16(request_id);
	data_len   = SYS_BE_S16(data_len);

	source = SYS_BE_S16(source);
	dest   = SYS_BE_S16(dest);
	game   = SYS_BE_S16(game);
}

void error_proto_t::ByteSwap()
{
	type     = SYS_BE_S16(type);
	param[0] = SYS_BE_S16(param[0]);
	param[1] = SYS_BE_S16(param[1]);
}

void client_info_t::ByteSwap()
{
	game_id = SYS_BE_S16(game_id);
}

void connect_proto_t::ByteSwap()
{
	info.ByteSwap();
}

void query_client_proto_t::ByteSwap(bool do_info)
{
	first_id = SYS_BE_S16(first_id);

	if (do_info)  // FIXME: swap structures manually higher up
	{
		for (int c = 0; c < count; c++)
			info[c].ByteSwap();
	}
}

void game_info_t::ByteSwap()
{
	min_players = SYS_BE_S16(min_players);
	engine_ver  = SYS_BE_S16(engine_ver);

	num_players = SYS_BE_S16(num_players);
	num_votes   = SYS_BE_S16(num_votes);
}

void new_game_proto_t::ByteSwap()
{
	info.ByteSwap();
}

void query_game_proto_t::ByteSwap(bool do_info)
{
	first_id = SYS_BE_S16(first_id);

	if (do_info)
	{
		for (int c = 0; c < count; c++)
			info[c].ByteSwap();
	}
}

void play_game_proto_t::ByteSwap()
{
	total_players = SYS_BE_S16(total_players);
	first_player  = SYS_BE_S16(first_player);

	for (int p = 0; p < count; p++)
		client_id[p] = SYS_BE_S16(client_id[p]);
}

void ticcmd_proto_t::ByteSwap()
{
	/// tic_counter = SYS_BE_S16(tic_counter);

	/* TICCMDs are the engine's responsibility */
}

void tic_group_proto_t::ByteSwap()
{
	/// tic_counter  = SYS_BE_S16(tic_counter);
	first_player = SYS_BE_S16(first_player);

	/* TICCMDs are the engine's responsibility */
}
