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

#ifndef __N_NETWORK_H__
#define __N_NETWORK_H__

extern bool var_hogcpu;

void N_InitNetwork(void);

// Create any new ticcmds and broadcast to other players.
// returns value of I_GetTime().
int N_NetUpdate(bool do_delay = false);

// Broadcasts special packets to other players
//  to notify of game exit
void N_QuitNetGame(void);

// returns number of ticks to run (always > 0).
// is_fresh means that game ticks can be run.
int N_TryRunTics(bool *is_fresh);

// process input and create player (and robot) ticcmds.
// returns false if couldn't hold any more.
bool N_BuildTiccmds(void);

// restart tic counters (maketic, gametic) at zero.
void N_ResetTics(void);

#endif /* __N_NETWORK_H__ */
