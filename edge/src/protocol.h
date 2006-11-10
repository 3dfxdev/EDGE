//------------------------------------------------------------------------
//  Protocol structures
//------------------------------------------------------------------------
//
//  Copyright (c) 2005-2006  The EDGE Team.
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

#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

#define MP_PROTOCOL_VER  0x080  /* 0.80 */

#define MP_SAVETICS  6  // past and future

//
// common packet header
//
// types are an alphanumeric pair, generally Verb+Noun.  They should
// start with lowercase for packets from the client (e.g. "cs"), and
// uppercase for ones from the server (e.g. "Er").  Data length does
// NOT include the header, e.g. data_len = sizeof(connect_proto_t).
//
typedef struct
{
	char type[2];

	s16_t data_len;
	s16_t flags;
	s16_t request;   // use to match requests with replies

	enum // flags
	{
		FL_Retransmit = 0x4000,
	};

	void ByteSwap();
}
header_proto_t;


//
// ERROR ("Er")
//
// Error types are a lowercase alphanumeric pair.
// Current error codes:
//
//   ac : Already Connected
//   lc : Late Connection (game in progress)
//   bc : Bad Client info
//   bg : Bad Game info
//   ps : Packet too Short (data_len)
//   pl : Packet too Long  (data_len)
//
typedef struct
{
	char type[2];

	enum
	{
		ERR_STR_MAX = 200
	};
	
	char message[1];  // as big as needed (upto ERR_STR_MAX)
}
error_proto_t;


/* BROADCAST-DISCOVERY-UDP ("bd") */


//
// CONNECT ("cn")
//
typedef struct
{
	u16_t client_ver;  // MP_PROTOCOL_VER from client

	char player_name[16];

	byte reserved[32];

	void ByteSwap();
}
connect_proto_t;


//
// WELCOME ("we")
//
typedef struct
{
	u16_t host_ver;  // MP_PROTOCOL_VER from host

	enum
	{
		GAME_STR_MAX = 32
	};

	char game_name[GAME_STR_MAX];     // e.g. HELL_ON_EARTH

	enum
	{
		LEVEL_STR_MAX = 8
	};

	char level_name[LEVEL_STR_MAX];   // e.g. MAP01

	enum  // mode values:
	{
		MODE_Coop       = 'C',
		MODE_New_DM     = 'N',
		MODE_Old_DM     = 'O',  // weapons stay on map (etc)
		MODE_LastMan    = 'L',
		MODE_CTF        = 'F',
	};

	byte mode;
	byte skill;  // 1 to 5

	byte bots;
	byte teams;  // 0 = no teams

	enum  // gameplay bitflags:
	{
		GP_NoMonsters = (1 << 0),

		GP_Jumping    = (1 << 1),
		GP_Crouching  = (1 << 2),
		GP_Zooming    = (1 << 3),
		GP_MLook      = (1 << 4),
		GP_AutoAim    = (1 << 5),
	};

	u32_t gameplay;
	u32_t random_seed;

	u16_t wad_checksum;  // checksum over all loaded wads
	u16_t def_checksum;  // checksum over all definitions

	byte reserved[16];  // (future expansion)

	void ByteSwap();
}
welcome_proto_t;


/* DISCONNECT ("dc") has no data */


typedef struct
{
	u16_t player_flags;  // PFL_xxx values

	char name[16];

	byte reserved[6];

	void ByteSwap();
}
player_info_t;

//
// PLAYER-LIST ("pl")
//
typedef struct
{
	byte real_players;
	byte total_players;

	byte first_player;
	byte last_player;

	byte reserved[4];

	static const int PLAYER_FIT = 8;

	player_info_t players[1];  // upto PLAYER_FIT entries

//	void ByteSwap();
	void ByteSwapPlayers(int num_players);
}
player_list_proto_t;


/* START-GAME ("sg") has no data */


///--- //
///--- // raw TICCMD
///--- //
///--- typedef struct
///--- {
///--- 	u16_t shorts[4];
///--- 	u8_t  bytes [8];
///--- 
///--- 	void ByteSwap();
///--- }
///--- raw_ticcmd_t;


//
// TICCMD ("tc")
//
// Holds a group of ticcmds, both CL->SV and SV->CL.
//
typedef struct
{
	u32_t gametic;
	byte  offset;  // start_tic = gametic + offset
	byte  count;   // end_tic = start_tic + count - 1

	byte first_player;
	byte last_player;

	byte reserved[2];

	static const int TICCMD_FIT = 24;

	ticcmd_t cmds[1];  // upto TICCMD_FIT commands

	void ByteSwap();
	void ByteSwapCmds(int num_cmds);
}
ticcmd_proto_t;


#endif /* __PROTOCOL_H__ */
