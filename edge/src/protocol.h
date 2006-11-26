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
//
//  NOTE: the 'header_proto_s' and other struct names are really
//        redundant, however GCC fucks up if you remove them, as
//        the compiled .o file will lack the ByteSwap() methods!
//  

#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

#define MP_PROTOCOL_VER  0x080  /* 0.80 */

#define MP_SAVETICS  6  // past and future

//
// common packet header
//
// types are an alphanumeric pair, e.g. "cn" for connect packet.
// Data length does not include this header, e.g. sizeof(connect_proto_t).
// 
// Sequence values are used to detect missing packets:
//
//  -  seq_out is the current packet number (always +1 from previous
//     packet sent)
//
//  -  seq_rcv is the lowest numbered packet we haven't received yet
//     (i.e. the packet number we expect to get next).
// 
typedef struct header_proto_s
{
	char type[2];

	s16_t data_len;
	s16_t flags;

	u16_t seq_out;
	u16_t seq_recv;

	enum // flags
	{
		FL_Retransmit = 0x4000,
	};

	void ByteSwap();
}
header_proto_t;


/* NO-OPERATION ("no") has no data */


//
// ERROR ("er")
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
typedef struct error_proto_s
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
typedef struct connect_proto_s
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
typedef struct welcome_proto_s
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
		GP_FastMon    = (1 << 1),

		GP_True3D     = (1 << 3),
		GP_Jumping    = (1 << 4),
		GP_Crouching  = (1 << 5),

		GP_AutoAim    = (1 << 7),
		GP_MLook      = (1 << 8),
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


typedef struct player_info_s
{
	u16_t player_flags;  // PFL_xxx values

	char name[16];

	byte reserved[10];

	void ByteSwap();
}
player_info_t;

//
// PLAYER-LIST ("pl")
//
typedef struct player_list_proto_s
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


//
// TICCMD ("tc")
//
// Holds a group of ticcmds, both CL->SV and SV->CL.
//
typedef struct ticcmd_s
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

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
