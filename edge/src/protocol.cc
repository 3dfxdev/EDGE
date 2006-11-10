//------------------------------------------------------------------------
//  Protocol structures
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

#include "i_defs.h"

#include "protocol.h"

#include "epi/types.h"
#include "epi/endianess.h"


void ticcmd_t::ByteSwap()
{
	angleturn = EPI_BE_S16(angleturn);
	mlookturn = EPI_BE_S16(mlookturn);

	consistency = EPI_BE_S16(consistency);
}

void header_proto_t::ByteSwap()
{
	data_len = EPI_BE_S16(data_len);
	flags    = EPI_BE_S16(flags);
	request  = EPI_BE_S16(request);
}

void connect_proto_t::ByteSwap()
{
	client_ver = EPI_BE_S16(client_ver);
}

void welcome_proto_t::ByteSwap()
{
	host_ver = EPI_BE_U16(host_ver);

	gameplay    = EPI_BE_U32(gameplay);
	random_seed = EPI_BE_U32(random_seed);

	wad_checksum = EPI_BE_U16(wad_checksum);
	def_checksum = EPI_BE_U16(def_checksum);
}

void player_info_t::ByteSwap()
{
	player_flags = EPI_BE_S16(player_flags);
}

void player_list_proto_t::ByteSwapPlayers(int num_players)
{
	for (int p = 0; p < num_players; p++)
		players[p].ByteSwap();
}

void ticcmd_proto_t::ByteSwap()
{
	gametic = EPI_BE_U32(gametic);
}

void ticcmd_proto_t::ByteSwapCmds(int num_cmds)
{
	for (int t = 0; t < num_cmds; t++)
		cmds[t].ByteSwap();
}

