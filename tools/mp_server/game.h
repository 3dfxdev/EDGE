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

#ifndef __GAME_H__
#define __GAME_H__

#include "protocol.h"
#include "client.h"

class packet_c;

class game_c
{
public:
	game_c(const game_info_t *info);
	~game_c();

private:
	enum
	{
		ST_Queueing,
		ST_Playing,
		ST_Zombie  // no players or queuers
	};

	int state;

public: //FIXME
	char engine_name[game_info_t::ENGINE_STR_MAX];  // EDGE129
	char game_name  [game_info_t::GAME_STR_MAX];    // DOOM2 (etc)
	char level_name [game_info_t::LEVEL_STR_MAX];   // MAP01 (etc)

	char mode;
	byte skill;

	int features; // Yada yada

	int min_players;
	int num_bots;

	int num_players;
	int bots_each;

	int num_votes;

	int players[MP_PLAYER_MAX]; // queued or in the game

	// ---- tic handlig ----
	// {
		int sv_gametic;
		int pl_min_tic;  // MIN(all pl_gametic)
	// }

public:
	bool HasPlayer(int client_id) const;

	void AddPlayer(int client_id);
	void RemovePlayer(int client_id);

	void CalcBots();
	void FillGameInfo(game_info_t *info) const;

	void InitGame();
	void BumpGameTic();
	void BumpMinTic();
	void TryRunTics();

	int ComputeMinTic() const;
	int ComputeGroupAvail() const;
};

extern std::vector<game_c *> games;

extern volatile int total_queued_games;
extern volatile int total_played_games;

void GameTimeouts();

void PK_new_game(packet_c *pk);
void PK_join_queue(packet_c *pk);
void PK_leave_game(packet_c *pk);
void PK_vote_to_play(packet_c *pk);

void PK_query_game(packet_c *pk);
void PK_ticcmd(packet_c *pk);
void PK_tic_retransmit(packet_c *pk);

#endif /* __GAME_H__ */
