//----------------------------------------------------------------------------
//  EDGE Networking (New)
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

#include "i_defs.h"

#ifdef USE_HAWKNL
#include "nl.h"
#endif

#include "protocol.h"
#include "n_packet.h"
#include "n_network.h"

#include "e_input.h"
#include "e_main.h"

#include "epi/types.h"
#include "epi/endianess.h"

#include <limits.h>
#include <stdlib.h>

// only true if packets are exchanged with a server
bool netgame = false;


bool var_hogcpu = true;

extern gameflags_t default_gameflags;

#ifdef USE_HAWKNL
static int client_id;
static int game_id;
static int bots_each;

static NLaddress server;
static NLint     port;
static NLaddress local_addr;

static NLsocket  socket;
static NLint     sk_group;

static packet_c pk;
#endif


#ifdef USE_HAWKNL

static bool MakeBroadcastAddress(NLaddress *dest, const NLaddress *src)
{
	// changes the last value, e.g. 10.0.0.4 --> 10.0.0.255

	static char name_buf[NL_MAX_STRING_LENGTH];

	char *begin = nlAddrToString(src, name_buf);

	char *pos = strrchr(begin, '.');

	if (! pos)
		return false;
	
	strcpy(pos + 1, "255");

	return (nlStringToAddr(begin, dest) == NL_TRUE);
}

static bool N_DetermineLocalAddr(void)
{
	// explicitly given via command-line option?
	const char *p = M_GetParm("-local");

	if (p)
	{
		if (! nlStringToAddr(p, &local_addr))
		{
			I_Error("Bad local address '%s'\n(%s)", p, I_NetworkReturnError());
		}

		return true;
	}

	int count;
	NLaddress *all_addrs = nlGetAllLocalAddr(&count);

	if (all_addrs && count > 0)
	{
		for (int i = 0; i < count; i++)
		{
			const char *addr_name = N_GetAddrName(all_addrs +i);

			L_WriteDebug("ALL-Local[%d] = %s\n", i, addr_name);

			if (strncmp(addr_name, "127.", 4) == 0 ||
				strncmp(addr_name, "0.0.", 4) == 0 ||
				strncmp(addr_name, "255.255.", 8) == 0)
			{
				continue;
			}

			// found a valid one
			memcpy(&local_addr, all_addrs + i, sizeof(local_addr));
			return true;
		}
	}

#ifdef LINUX
	const char *local_ip = I_LocalIPAddrString("eth0");
	if (! local_ip)
		local_ip = I_LocalIPAddrString("eth1");
	if (! local_ip)
		local_ip = I_LocalIPAddrString("ppp0");

	if (local_ip)
	{
		L_WriteDebug("LINUX-Local-IP = %s\n", local_ip);

		nlStringToAddr(local_ip, &local_addr);
		return true;
	}
#endif

	return false;
}


static bool N_BroadcastToServer(NLsocket sock)
{
	pk.SetType("bd");
	pk.hd().flags = 0;
	pk.hd().data_len = 0;
	pk.hd().client = 0;

	if (! pk.Write(sock))
		I_Error("Unable to write BD packet:\n%s", I_NetworkReturnError());

	L_WriteDebug("Wrote BD packet\n"); //!!!!

	int count = 0;

	for (;;)
	{
		if (pk.Read(sock))
		{
			L_WriteDebug("READ A PACKET: [%c%c]\n", pk.hd().type[0], pk.hd().type[1]);

			if (pk.CheckType("bd"))
				break;
			else
				continue;
		}

		count++;

		if (count == 5)
			return false;

		I_Sleep(1000);
	}

	nlGetRemoteAddr(sock, &server);

	printf("Found server: %s\n", N_GetAddrName(&server));

	return true;
}

static bool N_FindServer(void)
{
	printf("Looking for server...\n");

	// set explicitly (via command line)
	const char *str = M_GetParm("-server");

	if (str)
	{
		if (nlStringToAddr(str, &server) != NL_TRUE)
			I_Error("Bad server address '%s'\n(%s)", str,
				I_NetworkReturnError());
		return true;
	}

#if 0	// test for same computer
	NLsocket same_sock = nlOpen(0, NL_UNRELIABLE);

    if (same_sock == NL_INVALID)
		I_Error("Unable to create local socket:\n%s", I_NetworkReturnError());

	NLaddress same_addr;
	memcpy(&same_addr, &local_addr, sizeof(same_addr));
	nlSetAddrPort(&same_addr, port);

	if (! nlSetRemoteAddr(same_sock, &same_addr))
	{
		I_Warning("Set remote address on broadcast socket failed.\n%s",
			I_NetworkReturnError());
	}
	else
	{
		if (N_BroadcastToServer(same_sock))
		{
			nlClose(same_sock);
			return true;
		}
	}
#endif

	//--------------------------------------------------------

	printf("Server not on same computer, trying LAN...\n");

	NLsocket bc_sock = nlOpen(port, NL_BROADCAST);

    if (bc_sock == NL_INVALID) // FIXME: don't error on this
	{
		I_Warning("Unable to create broadcast socket:\n%s", I_NetworkReturnError());
		return false;
	}

	NLaddress bc_addr;
	MakeBroadcastAddress(&bc_addr, &local_addr);
	nlSetAddrPort(&bc_addr, port);
	
	if (! nlSetRemoteAddr(bc_sock, &bc_addr))
	{
		I_Warning("Set remote address on broadcast socket failed.\n%s", I_NetworkReturnError());
		nlClose(bc_sock);
		return false;
	}

	bool result = (N_BroadcastToServer(bc_sock) == NL_TRUE);

	nlClose(bc_sock);
	return result;
}

static void N_ConnectServer(void)
{
#if 0  /// UDP : choose a random socket port
	int udp_port = 0;

	for (int tries = 0; tries < 100; tries++)
	{
		udp_port = 11000 + (rand() & 0x1FFF);

		socket = nlOpen(udp_port, NL_RELIABLE_PACKETS);

		if (socket != NL_INVALID)
			break;

		if ((tries % 10) == 9)
			I_Sleep(10 /* millisecs */);
	}
#else  /// TCP
	socket = nlOpen(0, NL_RELIABLE_PACKETS);
#endif

    if (socket == NL_INVALID)
		I_Error("Unable to create main socket:\n%s", I_NetworkReturnError());

///UDP	printf("Port %d\n", port);

	nlSetAddrPort(&server, port);

	if (nlConnect(socket, &server) != NL_TRUE)
		I_Error("Failed to connect to server: timed out\n");  // FIXME: retry a few times

	I_Sleep(1000);

///UDP	nlSetRemoteAddr(socket, &server);

	sk_group = nlGroupCreate();

	if (sk_group == NL_INVALID || ! nlGroupAddSocket(sk_group, socket))
		I_Error("Unable to create socket group (no memory)\n");

	pk.SetType("cs");
	pk.hd().flags = 0;
	pk.hd().data_len = sizeof(connect_proto_t);
	pk.hd().client = 0;

	connect_proto_t& con = pk.cs_p();

	sprintf(con.info.name, "Flynn%05d", rand()&0x7FFF);

	con.ByteSwap();

	if (! pk.Write(socket))
		I_Error("Unable to write CS packet:\n%s", I_NetworkReturnError());

	I_Sleep(1000);

	if (! pk.Read(socket))
		I_Error("Failed to connect to server (no reply)\n");
	
	if (pk.CheckType("Er"))
		I_Error("Failed to connect to server:\n%s", pk.er_p().message);

	if (! pk.CheckType("Cs"))
		I_Error("Failed to connect to server: (invalid reply)\n");

	con.ByteSwap();

	client_id = pk.hd().client;
	
	printf("Connected to server: client_id %d\n", client_id);
	printf("Server version %x.%02x  Protocol version %x.%02x\n",
		con.server_ver >> 8, con.server_ver & 0xFF,
		con.protocol_ver >> 8, con.protocol_ver & 0xFF);
}

static bool N_FindGame(game_info_t *gminfo /* out */)
{
///UDP	nlSetRemoteAddr(socket, &server);

	pk.SetType("qg");
	pk.hd().flags = 0;
	pk.hd().data_len = sizeof(query_game_proto_t);
	pk.hd().client = client_id;

	query_game_proto_t& qg = pk.qg_p();

	qg.first_game = 0;
	qg.last_game = 0;

	qg.ByteSwap();

	if (! pk.Write(socket))
		I_Error("Unable to write QG packet:\n%s", I_NetworkReturnError());

	I_Sleep(1000);

	if (! pk.Read(socket))
		I_Error("Failed to query game (no reply)\n");
	
	if (pk.CheckType("Er"))
		I_Error("Failed to query game:\n%s", pk.er_p().message);

	if (! pk.CheckType("Qg"))
		I_Error("Failed to query game (invalid reply)\n");

	qg.ByteSwap();
	qg.ByteSwapInfo(1);

	if (qg.info[0].state != game_info_t::GS_Queued)
	{
		printf("Query game #0: does not exist\n");
		return false;
	}

	game_id = 0;

	memcpy(gminfo, &qg.info, sizeof(game_info_t));

	printf("Query game #0: accepting players (QUEUING)\n");
	return true;
}

static void N_JoinGame(void)
{
///UDP	nlSetRemoteAddr(socket, &server);

	pk.SetType("jq");
	pk.hd().flags = 0;
	pk.hd().data_len = sizeof(join_queue_proto_t);
	pk.hd().client = client_id;

	join_queue_proto_t& jq = pk.jq_p();

	jq.game = game_id;

	jq.ByteSwap();

	if (! pk.Write(socket))
		I_Error("Unable to write JQ packet:\n%s", I_NetworkReturnError());

	I_Sleep(1000);
#if 0 //!!!!

	if (! pk.Read(socket))
		I_Error("Failed to join game (no reply)\n");
	
	if (pk.CheckType("Er"))
		I_Error("Failed to join game:\n%s", pk.er_p().message);

	if (! pk.CheckType("Jg"))
		I_Error("Failed to join game (invalid reply)\n");
#endif

	printf("Joined queue for game #%d\n", game_id);
}

static void N_NewGame(game_info_t *gminfo /* out */)
{
///UDP	nlSetRemoteAddr(socket, &server);

	pk.SetType("ng");
	pk.hd().flags = 0;
	pk.hd().data_len = sizeof(new_game_proto_t);
	pk.hd().client = client_id;

	new_game_proto_t& ng = pk.ng_p();

	sprintf(ng.info.engine_name, "EDGE%3x", EDGEVER);
	sprintf(ng.info.game_name,   "DOOM2");
	sprintf(ng.info.level_name,  startmap);

	ng.info.mode  = (deathmatch >= 2) ? 'A' :
	                (deathmatch == 1) ? 'D' : 'C';
	ng.info.skill = 1 + (byte)startskill;

	ng.info.min_players = startplayers;
	ng.info.num_bots = startbots;
	ng.info.gameplay = ~0;
	ng.info.features =  0;

	ng.info.wad_checksum = 0;
	ng.info.def_checksum = 0;

	memcpy(gminfo, &ng.info, sizeof(game_info_t));

	ng.ByteSwap();

	if (! pk.Write(socket))
		I_Error("Unable to write NG packet:\n%s", I_NetworkReturnError());

	I_Sleep(1000);

	if (! pk.Read(socket))
		I_Error("Failed to create new game (no reply)\n");
	
	if (pk.CheckType("Er"))
		I_Error("Failed to create new game:\n%s", pk.er_p().message);

	if (! pk.CheckType("Ng"))
		I_Error("Failed to create new game (invalid reply)\n");

	ng.ByteSwap();

	game_id = ng.game;

	printf("Created new game: %d\n", game_id);
}

static void N_Vote(const game_info_t *gminfo, newgame_params_c *params)
{
///UDP	nlSetRemoteAddr(socket, &server);

	pk.SetType("vp");
	pk.hd().flags = 0;
	pk.hd().data_len = 0;
	pk.hd().client = client_id;

	if (! pk.Write(socket))
		I_Error("Unable to write VP packet:\n%s", I_NetworkReturnError());

	int loop_num;
	for (loop_num = 60; loop_num > 0; loop_num -= 2)
	{
		I_Sleep(1000);
		I_Sleep(1000);

		if (pk.Read(socket))
			break;

		printf("Waiting for game-start packet (%d)\n", loop_num);
	}

	if (loop_num <= 0)
		I_Error("Timeout waiting for game-start packet from server.\n");

	if (pk.CheckType("Er"))
		I_Error("Failed to vote:\n%s", pk.er_p().message);

	// FIXME: will normally get "Vp" (acknowledge)

	if (! pk.CheckType("Pg"))
		I_Error("Failed to get game-start packet (different reply)\n");

	play_game_proto_t& pg = pk.pg_p();

	pg.ByteSwap();
	pg.ByteSwapPlayers(pg.real_players);

	/* ---- decode parameters ---- */

	printf("Setting up game parameters\n");

	bots_each = pg.bots_each;

	params->total_players = pg.real_players * (1 + bots_each);

	for (int RP = 0; RP < pg.real_players; RP++)
	{
		int R_client = pg.client_list[RP];

		printf("Real player %d --> client %d\n", RP, R_client);

		for (int BT = 0; BT < (1 + bots_each); BT++)
		{
			int pnum = RP * (1 + bots_each) + BT;

			params->players[pnum] = (BT == 0) ? PFL_Zero : PFL_Bot;

			if (R_client != client_id)
				params->players[pnum] = (playerflag_e)
					(params->players[pnum] | PFL_Network);

			printf("  Game player %d --> 0x%x\n",
				pnum, params->players[pnum]);
		}
	}

	printf("Random seed: 0x%x\n", (unsigned int) pg.random_seed);
	printf("Skill %d  Game Mode '%c'\n", gminfo->skill, gminfo->mode);
	printf("Level: '%s'\n", gminfo->level_name);

	params->random_seed = pg.random_seed;

	params->skill = (skill_t)(gminfo->skill - 1);
	params->deathmatch = (gminfo->mode == 'D') ? 1 : (gminfo->mode == 'A') ? 2 : 0;

	params->map = G_LookupMap(gminfo->level_name);

	if (! params->map)
		I_Error("Net_start: no such level '%s'\n", gminfo->level_name);

	params->game = gamedefs.Lookup(params->map->episode_name);
	if (! params->game)
		I_Error("Net_start: no gamedef for level '%s'\n", gminfo->level_name);

	printf("LET THE GAMES BEGIN !!!\n");
}

//
// N_InitiateGame
//
void N_InitiateNetGame(void)
{
	if (nonet)
		I_Error("N_InitiateNetGame: no network!\n");

	pk.Clear();

	if (N_FindServer())
		I_Printf("Network: server address is %s\n", N_GetAddrName(&server));
	else
		I_Error("Failed to find server!\nPlease specify manually using -server");

	N_ConnectServer();

	bool found_game = false;

	game_info_t gminfo;

	if (N_FindGame(&gminfo))
	{
		found_game = true;
		N_JoinGame();
	}
	else
		N_NewGame(&gminfo);

	newgame_params_c params;

	N_Vote(&gminfo, &params);

	if (! G_DeferredInitNew(params, true /* compat_check */))
		I_Error("N_InitiateNetGame: init_new failed\n");
}

#else // USE_HAWKNL

void N_InitiateNetGame(void)
{
	I_Error("Networking not supported (hawk has flown)\n");
}

#endif // USE_HAWKNL

//----------------------------------------------------------------------------
//  TIC HANDLING
//----------------------------------------------------------------------------

static int last_update_tic;
static int last_tryrun_tic;

void N_InitNetwork(void)
{
	srand(I_PureRandom());

	N_ResetTics();

#ifdef USE_HAWKNL
	if (nonet)
		return;

	if (N_DetermineLocalAddr())
	{
		I_Printf("Network: local address is %s\n", N_GetAddrName(&local_addr));
		nlSetLocalAddr(&local_addr);
	}
	else
	{
		I_Printf("Network: FAILED to determine local address.\n");
		nonet = true;
	}

	port = 26710;

	const char *str = M_GetParm("-port");
	if (str)
		port = atoi(str);
	
	I_Printf("Network: server port is %d\n", port);
#endif
}

static void GetPackets(bool do_delay)
{
	if (! netgame)
	{
		// -AJA- This can make everything a bit "jerky" :-(
		if (do_delay && ! var_hogcpu)
			I_Sleep(10 /* millis */);

		return;
	}

#ifdef USE_HAWKNL

	NLsocket socks[4];  // only one in the group

	int delay = (do_delay && ! var_hogcpu) ? 10 /* millis */ : 0;
	int num = nlPollGroup(sk_group, NL_READ_STATUS, socks, 4, delay);

	if (num < 1)
		return;

	if (! pk.Read(socks[0]))
		return;

	L_WriteDebug("GOT PACKET [%c%c] len = %d\n", pk.hd().type[0], pk.hd().type[1],
		pk.hd().data_len);

	if (! pk.CheckType("Tg"))
		return;

	// FIXME: validate packet, check size

	tic_group_proto_t& tg = pk.tg_p();

	int count = tg.last_player - tg.first_player + 1;

	tg.ByteSwap();
	tg.ByteSwapCmds((1 + bots_each) * count);

	// !!! FIXME: handle tg.count

	player_t *p = players[consoleplayer]; //!!!!

	int got_tic = tg.gametic + tg.offset;

	L_WriteDebug("-- Got TG: counter = %d  in_tic = %d\n", got_tic, p->in_tic);

	if (got_tic != p->in_tic)
	{
		L_WriteDebug("GOT TIC %d != EXPECTED %d\n", got_tic, p->in_tic);

		// !!! FIXME: handle "future" tics (save them, set bit in mask)
		//            (send retransmission request for gametic NOW).
		return;
	}

	ticcmd_t *raw_cmd = tg.tic_cmds;

	for (int pnum = 0; pnum < MAXPLAYERS; pnum++)
	{
		player_t *p = players[pnum];

		if (! p) continue;

		memcpy(p->in_cmds + (got_tic % (MP_SAVETICS*2)), raw_cmd, sizeof(ticcmd_t));

		raw_cmd++;

		p->in_tic++;
	}

//	DEV_ASSERT2((raw_cmd - tg.tic_cmds) == (1 + bots_each));

#endif  // USE_HAWKNL
}

static void DoSendTiccmds(int tic)
{
#ifdef USE_HAWKNL
	pk.Clear();  // FIXME: TESTING ONLY

	pk.SetType("tc");
	pk.hd().flags = 0;
	pk.hd().data_len = sizeof(ticcmd_proto_t) - sizeof(ticcmd_t);
	pk.hd().client = client_id;

    ticcmd_proto_t& tc = pk.tc_p();

	DEV_ASSERT2(tic >= gametic);

	tc.gametic = gametic;
	tc.offset  = tic - gametic;
	tc.count   = 1;

	ticcmd_t *raw_cmd = tc.tic_cmds;

	for (int pnum = 0; pnum < MAXPLAYERS; pnum++)
	{
		player_t *p = players[pnum];

		if (! p || ! p->builder) continue;

		memcpy(raw_cmd, p->out_cmds + (tic % (MP_SAVETICS*2)), sizeof(ticcmd_t));

		raw_cmd++;

		pk.hd().data_len += sizeof(ticcmd_t);
	}

	DEV_ASSERT2((raw_cmd - tc.tic_cmds) == (1 + bots_each));

    tc.ByteSwap();
    tc.ByteSwapCmds((1 + bots_each) * tc.count);

	L_WriteDebug("Writing ticcmd for tic %d\n", tic);

	if (! pk.Write(socket))
		L_WriteDebug("Failed to write packet (tic %d)\n", tic);
	
	// FIXME: need an 'out_tic' for resends....

#endif  // USE_HAWKNL
}

bool N_BuildTiccmds(void)
{
	I_ControlGetEvents();
	E_ProcessEvents();

	if (numplayers == 0)
	{
		E_UpdateKeyState();
		return false;
	}

	if (maketic >= gametic + MP_SAVETICS)
	{
		E_UpdateKeyState();
		return false;  // can't hold any more
	}

	// build ticcmds
	for (int pnum = 0; pnum < MAXPLAYERS; pnum++)
	{
		player_t *p = players[pnum];
		if (! p) continue;

		if (p->builder)
		{
			ticcmd_t *cmd;

///     L_WriteDebug("N_BuildTiccmds: pnum %d netgame %c\n", pnum, netgame ? 'Y' : 'n');

			if (netgame)
				cmd = &p->out_cmds[maketic % (MP_SAVETICS*2)];
			else
				cmd = &p->in_cmds[maketic % (MP_SAVETICS*2)];

			p->builder(p, p->build_data, cmd);
			
			cmd->consistency = p->consistency[maketic % (MP_SAVETICS*2)];
		}
	}

	E_UpdateKeyState();

	if (netgame)
		DoSendTiccmds(maketic);

	maketic++;
	return true;
}

int N_NetUpdate(bool do_delay)
{
	int nowtime = I_GetTime();

	if (singletics)  // singletic update is syncronous
		return nowtime;

	int newtics = nowtime - last_update_tic;
	last_update_tic = nowtime;

	if (newtics > 0)
	{
		// build and send new ticcmds for local players
		int t;
		for (t = 0; t < newtics; t++)
		{
			if (! N_BuildTiccmds())
				break;
		}

		if (t != newtics && numplayers > 0)
			L_WriteDebug("N_NetUpdate: lost tics: %d\n", newtics - t);
	}

	GetPackets(do_delay);

	return nowtime;
}

int DetermineLowTic(void)
{
	if (! netgame)
		return maketic;

	int lowtic = INT_MAX;

	for (int pnum = 0; pnum < MAXPLAYERS; pnum++)
	{
		player_t *p = players[pnum];
		if (! p) continue;

		// ignore bots
		if (p->playerflags & PFL_Bot)
			continue;

		lowtic = MIN(lowtic, p->in_tic);
	}

	return lowtic;
}

int N_TryRunTics(bool *is_fresh)
{
	*is_fresh = false;

	if (singletics)
	{
		N_BuildTiccmds();

		if (numplayers > 0)
			*is_fresh = true;

		return 1;
	}

	int nowtime = N_NetUpdate();
	int realtics = nowtime - last_tryrun_tic;
	last_tryrun_tic = nowtime;

#if 0
L_WriteDebug("N_TryRunTics: now %d last_tryrun %d --> real %d\n",
nowtime, nowtime - realtics, realtics);
#endif

	// simpler handling when no game in progress
	if (numplayers == 0)
	{
		while (realtics <= 0)
		{
			nowtime = N_NetUpdate(true);
			realtics = nowtime - last_tryrun_tic;
			last_tryrun_tic = nowtime;
		}

		if (realtics > MP_SAVETICS)
			realtics = MP_SAVETICS;

		return realtics;
	}

	int lowtic = DetermineLowTic();
	int availabletics = lowtic - gametic;

	// this shouldn't happen, since we can only run a gametic when
	// the ticcmds for _all_ players have arrived (hence lowtic >= gametic);
	DEV_ASSERT2(availabletics >= 0);

	// decide how many tics to run
	int counts;

	if (realtics + 1 < availabletics)
		counts = realtics + 1;
	else
		counts = MIN(realtics, availabletics);

#if 0
	L_WriteDebug("=== lowtic %d gametic %d | real %d avail %d raw-counts %d\n",
		lowtic, gametic, realtics, availabletics, counts);
#endif

	if (counts < 1)
		counts = 1;

	// wait for new tics if needed
	while (lowtic < gametic + counts)
	{
		int wait_tics = N_NetUpdate(true) - last_tryrun_tic;

		lowtic = DetermineLowTic();

		DEV_ASSERT2(lowtic >= gametic);

		// don't stay in here forever -- give the menu a chance to work
		if (wait_tics > TICRATE/2)
		{
			L_WriteDebug("Waited %d tics IN VAIN !\n", wait_tics);
			return 3;
		}
	}

	*is_fresh = true;

	return counts;
}

void N_ResetTics(void)
{
	maketic = gametic = 0;

	last_update_tic = last_tryrun_tic = I_GetTime();
}

void N_QuitNetGame(void)
{
	// !!!! FIXME: N_QuitNetGame
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
