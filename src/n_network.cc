//----------------------------------------------------------------------------
//  EDGE Networking (New)
//----------------------------------------------------------------------------
// 
//  Copyright (c) 2004  The EDGE Team.
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

#include "i_defs.h"
#include "nl.h"

#include "epi/epitype.h"
#include "epi/epiendian.h"

#include "protocol.h"
#include "n_packet.h"


static int client_id;
static int game_id;

static NLaddress server;
static NLsocket  socket;

static packet_c pk;


static void N_Preliminaries(void)
{
	nlHint(NL_REUSE_ADDRESS, NL_TRUE);

    if (nlSelectNetwork(NL_IP) == NL_FALSE)
		I_Error("nlSelectNetwork failed:\n%s", N_GetErrorStr());

	NLaddress local;

#ifdef LINUX  // !!!! FIXME: TEMP HACK
	nlStringToAddr("192.168.0.197", &local);
	nlSetLocalAddr(&local);
#endif
}

static bool N_TestServer(NLsocket sock)
{
	pk.SetType("bd");
	pk.hd().flags = 0;
	pk.hd().data_len = 0;

	if (! pk.Write(sock))
		I_Error("Unable to write BD packet:\n%s", N_GetErrorStr());

	int count = 0;

	for (;;)
	{
		sleep(1);

		if (pk.Read(sock))
			break;

		count++;

		if (count == 5)
			return false;
	}

	nlGetRemoteAddr(sock, &server);

	fprintf(stderr, "Found server: %s\n", N_GetAddrName(&server));

	return true;
}

static void N_FindServer(void)
{
	// test local computer

	NLsocket loc_sock = nlOpen(0, NL_UNRELIABLE);

    if (loc_sock == NL_INVALID)
		I_Error("Unable to create local socket:\n%s", N_GetErrorStr());

	NLaddress loc_addr;

	nlGetLocalAddr(loc_sock, &loc_addr);

	fprintf(stderr, "Local addr %s\n", N_GetAddrName(&loc_addr));

	nlSetAddrPort(&loc_addr, 26710);

	if (! nlSetRemoteAddr(loc_sock, &loc_addr))
		I_Error("Set remote address on local socket failed.\n%s", N_GetErrorStr());

	if (N_TestServer(loc_sock))
	{
		nlClose(loc_sock);
		return;
	}

	fprintf(stderr, "Server not on same computer, trying LAN...\n");

	//--------------------------------------------------------

	NLsocket bc_sock = nlOpen(26710, NL_BROADCAST);

    if (bc_sock == NL_INVALID)
		I_Error("Unable to create broadcast socket:\n%s", N_GetErrorStr());

	NLaddress remote;
	nlStringToAddr("192.168.0.255:26710", &remote);  //!!! HACK

	if (! nlSetRemoteAddr(bc_sock, &remote))
		I_Error("Set remote address on broadcast socket failed.\n%s", N_GetErrorStr());

	if (! N_TestServer(bc_sock))
		I_Error("Server not found (didn't receive reply).\n");

	nlClose(bc_sock);
}

static void N_ConnectServer(void)
{
	socket = nlOpen(0, NL_UNRELIABLE);

    if (socket == NL_INVALID)
		I_Error("Unable to create main socket:\n%s", N_GetErrorStr());

	nlSetRemoteAddr(socket, &server);

	pk.SetType("cs");
	pk.hd().flags = 0;
	pk.hd().data_len = sizeof(connect_proto_t);

	connect_proto_t& cs = pk.cs_p();

	strcpy(cs.info.name, "Flynn");

	if (! pk.Write(socket))
		I_Error("Unable to write CS packet:\n%s", N_GetErrorStr());

	sleep(1);
	sleep(1);

	if (! pk.Read(socket))
		I_Error("Failed to connect to server (no reply)\n");
	
	if (pk.CheckType("Er"))
		I_Error("Failed to connect to server:\n%s", pk.er_p().message);

	if (! pk.CheckType("Cs"))
		I_Error("Failed to connect to server: (invalid reply)\n");

	client_id = pk.hd().source;
	
	fprintf(stderr, "Connected to server: client_id %d\n", client_id);
}

static void N_NewGame(void)
{
	nlSetRemoteAddr(socket, &server);

	pk.SetType("ng");
	pk.hd().flags = 0;
	pk.hd().data_len = sizeof(new_game_proto_t);

	new_game_proto_t& ng = pk.ng_p();

	strcpy(ng.info.game_name,  "DOOM2");
	strcpy(ng.info.level_name, "MAP11");

	ng.info.mode  = 'D';
	ng.info.skill = '4';

	ng.info.min_players = 1;
	ng.info.engine_ver  = 0x129;

	ng.ByteSwap();

	if (! pk.Write(socket))
		I_Error("Unable to write NG packet:\n%s", N_GetErrorStr());

	sleep(1);
	sleep(1);

	if (! pk.Read(socket))
		I_Error("Failed to create new game (no reply)\n");
	
	if (pk.CheckType("Er"))
		I_Error("Failed to create new game:\n%s", pk.er_p().message);

	if (! pk.CheckType("Ng"))
		I_Error("Failed to create new game (invalid reply)\n");

	ng.ByteSwap();

	game_id = pk.hd().game;

	fprintf(stderr, "Created new game: %d\n", game_id);
}

static void N_Vote(void)
{
	nlSetRemoteAddr(socket, &server);

	pk.SetType("vp");
	pk.hd().flags = 0;
	pk.hd().data_len = 0;

	if (! pk.Write(socket))
		I_Error("Unable to write VP packet:\n%s", N_GetErrorStr());

	sleep(1);
	sleep(1);

	if (! pk.Read(socket))
		I_Error("Failed to vote (no reply)\n");
	
	if (pk.CheckType("Er"))
		I_Error("Failed to vote:\n%s", pk.er_p().message);

	if (! pk.CheckType("Pg"))
		I_Error("Failed to vote (invalid reply)\n");

	fprintf(stderr, "LET THE GAMES BEGIN !!!\n");
}

//
// N_InitiateGame
//
void N_InitiateGame(void)
{
	pk.Clear();

	N_Preliminaries();
	N_FindServer();
	N_ConnectServer();
	N_NewGame();
	N_Vote();
}

