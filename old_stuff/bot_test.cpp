//----------------------------------------------------------------------------
//  EDGE Experimental Bots
//----------------------------------------------------------------------------
// 
//  Copyright (c) 2005  The EDGE Team.
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

#include "p_bot.h"
#include "bot_path.h"
#include "bot_level.h"

#include "dm_defs.h"
#include "dm_state.h"
#include "e_input.h"
#include "e_ticcmd.h"
#include "g_game.h"
#include "m_bbox.h"
#include "m_random.h"
#include "p_local.h"
#include "p_weapon.h"
#include "r_state.h"
#include "s_sound.h"
#include "w_wad.h"
#include "z_zone.h"
#include "p_action.h"
#include "rad_trig.h"

#include "protocol.h"

#define ANG10  (ANG5 * 2)

#define TURN_DIST  256

#define B_SPEED  25

#define ALLOW_JUMP    (false)  // (level_flags.jumping)
#define ALLOW_CROUCH  (false)

// approx damage per second
static float DamagePerSecond(const damage_c *dam)
{
	float N = dam->nominal;

	// for a linear range, compute the average value
	if (dam->linear_max > 0)
		N = (N + dam->linear_max) / 2.0f;

	if (dam->delay > 0)
		N = N * TICRATE / float(dam->delay);
	
	if (dam->no_armour)
		N = N * 1.5f;
	
	return N;
}

//----------------------------------------------------------------------------

class global_plan_c : public path_store_base_c
{
public:
	global_plan_c() : path_store_base_c(), Achieved(), target(NULL),
		stack(), num_segs(0), first_obst(NULL)
	{ }

	virtual ~global_plan_c()
	{ }

private:
	bot_achievements_c Achieved;
	bot_achievements_c Achieved_ORIG;

public:
	fixed_target_base_c *target;

	target_group_stack_c stack;

	static const int MAX_PTS = 10000;

	seg_t *seglist[MAX_PTS];
	int num_segs;

	obstacle_base_c *first_obst;

public:
	void Prepare(fixed_target_base_c *T, keys_e cards)
	{
		target = T;

		Achieved.Clear();
		Achieved.AddKeys(cards);

		Achieved_ORIG.Clear();
		Achieved_ORIG.AddKeys(cards);

		num_segs = 0;
		first_obst = NULL;
	}

	inline bool PValid(int idx) const
	{
		if (num_segs == 0)
			return false;

		return (idx >= 0 && idx < num_segs + 2);
	}
	
	float PX(int idx) const
	{
		DEV_ASSERT2(PValid(idx));

		if (idx == 0)
			return ss_info[seglist[0]->front_sub - subsectors].mid_x;

		idx--;

		if (idx == num_segs)
			return ss_info[seglist[idx-1]->back_sub - subsectors].mid_x;

		seg_t *seg = seglist[idx];
		return (seg->v1->x + seg->v2->x) / 2.0f;
	}

	float PY(int idx) const
	{
		DEV_ASSERT2(PValid(idx));

		if (idx == 0)
			return ss_info[seglist[0]->front_sub - subsectors].mid_y;

		idx--;

		if (idx == num_segs)
			return ss_info[seglist[idx-1]->back_sub - subsectors].mid_y;

		seg_t *seg = seglist[idx];
		return (seg->v1->y + seg->v2->y) / 2.0f;
	}

public:  /* METHODS for path_store_base_c */

	virtual bool VisitSubsec(int sub)
	{
		return target->Reached(sub);
	}

	virtual int Compute_G(int sub, int next, seg_t *seg)
	{
		// marked impassable ?
		if (seg->linedef && seg->linedef->flags & ML_Blocking)
			return -1;

		float dist = BOT_BaseDistAlongSeg(sub, next, seg);
		float dist_ORIG = dist;

		obstacle_base_c *OBST;

		if (! BOT_CanTraverseSegUltra(seg, sub, 16.0f, &Achieved, &OBST))
		{
			L_WriteDebug("Unable to travel %d -> %d via seg %d (%1.0f,%1.0f) Line %d\n",
				sub, next, seg - segs,
				(seg->v1->x + seg->v2->x) / 2.0f,
				(seg->v1->y + seg->v2->y) / 2.0f,
				seg->linedef ? seg->linedef - lines : -1);

			if (! OBST)  // blocker is permanent
			{
//				L_WriteDebug("__ Permanently blocked.\n");
				return -1;
			}

			if (stack.HasObstacle(OBST))
			{
				L_WriteDebug("__ Blocked by dejavu obstacle.\n");
				return -1;
			}

			OBST->DebugDump("__ the obstacle is:");

			dist += 20000.0f * OBST->DifficultyFactor();

			delete OBST;
			OBST = NULL;
		}

		if (! seg->miniseg)
		{
			sector_t *front = seg->frontsector;
			sector_t *back  = seg->backsector;

			float step_up = ALLOW_JUMP ? 48.0f : 24.0f;

			// check for damaging sector
			if (front->props.special &&
				front->props.special->damage.nominal > 0)
			{
				float cost = DamagePerSecond(&front->props.special->damage);
				
				// FIXME: good formula ??
				cost *= dist_ORIG;

				if (cost > 50000.0f)
					cost = 50000.0f;

				dist += cost;
			}

			if (back->f_h < front->f_h - step_up)  // one-way dropoff ?
				dist += 2000.0f;

			// small penalty for going up/down
			dist += fabs(back->f_h - front->f_h) / 4.0f;
		}

#if 0
L_WriteDebug("CAN-GO: SUB %d -> %d VIA SEG %d @ (%1.0f,%1.0f) LINE %d\n",
sub, next, seg - segs, sx, sy,
seg->linedef ? seg->linedef - lines : -1);
#endif

		//  avoid popular paths
		for (int i=0; i < 30; i++)
			if (ss_info[sub].bot_visit && (1 << i))
				dist += 200.0f;

		dist *= 2.0f;
		if (dist > 1e6) dist = 1e6;

		return I_ROUND(dist);
	}

	virtual int Compute_H(int sub)
	{
		return target->Compute_H(ss_info[sub].mid_x, ss_info[sub].mid_y);
	}

	virtual bool NeedForward() const { return true; }
	virtual bool NeedTest()    const { return true; }

	virtual bool TestSubsec(int sub, int next, seg_t *seg)
	{
		if (! seg)
			return true;

		obstacle_base_c *OBST;

		if (BOT_CanTraverseSegUltra(seg, sub, 16.0f, &Achieved_ORIG, &OBST))
			return true;

		DEV_ASSERT2(OBST);  // shouldn't get here if blocker was permanent

		first_obst = OBST;

		return false;  // stop
	}

	virtual bool AddSubsec(int sub, int next, seg_t *seg)
	{
		if (! seg)
			return true;

		if (num_segs >= MAX_PTS)
			return false;

L_WriteDebug("== %1.0f %1.0f\n",
(seg->v1->x + seg->v2->x) / 2.0f, (seg->v1->y + seg->v2->y) / 2.0f);

		seglist[num_segs++] = seg;
		return true;
	}
};

//----------------------------------------------------------------------------

class bot_c
{
private:
	player_t *pl;

	enum botstate_e
	{
		SEARCHING = 0,
		FOLLOW_GOAL,
		EXECUTE_GOAL,
		WANDERING
	};

	botstate_e state;

	int ticker;
	int cur_pt;
	int use_tick;

	int last_time;
	int last_make;
	int future;

	fixed_target_base_c *goal;
	fixed_target_base_c *main_target;

	global_plan_c *path;

public:
	bot_c(player_t *player) : pl(player), state(SEARCHING),
		ticker(5), cur_pt(0), use_tick(10), last_time(0), last_make(0),
		goal(NULL), main_target(NULL), path(NULL)
	{ }

	~bot_c()
	{ }

public:
	void Think(ticcmd_t *cmd)
	{
		Z_Clear(cmd, ticcmd_t, 1);

		// temporary: don't restart game
		if (pl->playerstate == PST_DEAD)
			return;

		// E_BuildTiccmd(cmd);  // USER OVERRIDE

		future = (last_time == leveltime) ? (maketic - last_make) : 0;

		if (last_time != leveltime)
		{
			last_time = leveltime;
			last_make = maketic;
		}

		if (future > MP_SAVETICS)  // assume user is in menus
			return;

		ticker--;  // initial delay (HACK)
		if (ticker > 0)
			return;

		if (! path)
		{
			L_WriteDebug("---- Finding End Switch ----\n");

			main_target = BOT_FindLevelTarget();

			if (! main_target)
				I_Error("Cannot find EXIT switch !!\n");

			main_target->DebugDump("MAIN GOAL:");

			path = new global_plan_c();

			target_group_c *G = new target_group_c(NULL);
			G->AddSolution(main_target);

			path->stack.Push(G);

			state  = SEARCHING;
			cur_pt = 0;
		}

		switch (state)
		{
			case SEARCHING: Search(); return; /// FIXME: wander

			case WANDERING: break;

			case FOLLOW_GOAL: CheckGoal(); break;

			case EXECUTE_GOAL: ExecuteGoal(cmd); return; ////

			default:
				break;
		}

		Move(cmd);
	}
	
	void Move(ticcmd_t *cmd)
	{
		mobj_t *mo = pl->mo;

		use_tick--;
		if (use_tick <= 0)
		{
			use_tick = M_Random()/2 + 40;
			cmd->buttons |= BT_USE;
		}

	loop1:

		if (! path->PValid(cur_pt+1))
		{
			return;
		}

		float tx0 = path->PX(cur_pt);
		float ty0 = path->PY(cur_pt);
		float tz0 = R_PointInSubsector(tx0, ty0)->sector->f_h;

		float tx1 = path->PX(cur_pt+1);
		float ty1 = path->PY(cur_pt+1);
		float tz1 = R_PointInSubsector(tx1, ty1)->sector->f_h;

		float tx2, ty2, tz2;

		if (path->PValid(cur_pt+2))
		{
			tx2 = path->PX(cur_pt+2);
			ty2 = path->PY(cur_pt+2);
			tz2 = R_PointInSubsector(tx2, ty2)->sector->f_h;
		}
		else
		{
			// pretend 3rd point is on same line and slightly ahead
			tx2 = tx1 + (tx1 - tx0) / 10;
			ty2 = ty1 + (ty1 - ty0) / 10;
			tz2 = tz1;
		}

		float mx = mo->x - tx0;
		float my = mo->y - ty0;

		float dx = tx1 - tx0;
		float dy = ty1 - ty0;

		float along = (mx*dx + my*dy) / (dx*dx + dy*dy);

		// check if reached the target point
		if (along >= 1.0f)
		{
			cur_pt++;
// I_Printf("REACHED POINT, NEW TARGET (%d,%d)\n", points[cur_pt*2+0], points[cur_pt*2+1]);
			goto loop1;
		}
// I_Printf("along %1.4f\n", along);

		float leg_dist = sqrt(dx*dx + dy*dy);

		if (leg_dist > TURN_DIST)
		{
			along = 1.0f - along;
			along = along * leg_dist / float(TURN_DIST);
			along = 1.0f - along;
		}

		unsigned int frac = (along < 0.01) ? 0 : int(along * 256);
		if (frac > 256) frac = 256;

		/* compute turning change */
		{
			angle_t ang0 = R_PointToAngle(tx0, ty0, tx1, ty1);
			angle_t ang1 = R_PointToAngle(tx1, ty1, tx2, ty2);

			angle_t want_ang = ang1 - ang0;

			if (want_ang < ANG180)
				want_ang = (want_ang >> 8) * frac;
			else
				want_ang = (angle_t) ~ ((((angle_t) ~want_ang) >> 8) * frac);

			want_ang += ang0;

			angle_t diff = want_ang - mo->angle;

			if (diff < ANG180)
			{
				if (diff > ANG10)
					diff = ANG10;
			}
			else
			{
				if (diff < (angle_t)~ANG10)
					diff = ~ANG10;
			}

			cmd->angleturn += diff >> 16;
		}

		/* compute mlook change */
		{
			float dist0 = R_PointToDist(tx0, ty0, tx1, ty1);
			float dist1 = R_PointToDist(tx1, ty1, tx2, ty2);

			angle_t ang0 = R_PointToAngle(0, tz0, dist0, tz1);
			angle_t ang1 = R_PointToAngle(0, tz1, dist1, tz2);

			angle_t want_ang = ang1 - ang0;

			if (want_ang < ANG180)
				want_ang = (want_ang >> 8) * frac;
			else
				want_ang = (angle_t) ~ ((((angle_t) ~want_ang) >> 8) * frac);

			want_ang += ang0;

			angle_t diff = want_ang - mo->vertangle;
// I_Printf("frac %d : %1.2f + %1.2f --> %1.2f  CURR %1.2f diff %1.2f\n",
// frac, ANG_2_FLOAT(ang0), ANG_2_FLOAT(ang1), ANG_2_FLOAT(want_ang),
// ANG_2_FLOAT(mo->vertangle), ANG_2_FLOAT(diff));

			if (diff < ANG180)
			{
				if (diff > ANG5)
					diff = ANG5;
			}
			else
			{
				if (diff < (angle_t)~ANG5)
					diff = ~ANG5;
			}

			cmd->mlookturn += diff >> 16;
		}

		/* Set speed, taking angle change into account */
		{
			angle_t new_ang = mo->angle + (angle_t)(cmd->angleturn << 16);

			angle_t t_ang = R_PointToAngle(mo->x, mo->y, tx1, ty1);

			float want_mx = B_SPEED * -M_Sin(t_ang - new_ang);
			float want_my = B_SPEED *  M_Cos(t_ang - new_ang);

			float mom = P_ApproxDistance(mo->mom.x, mo->mom.y);

			// compensate for momentum
			if (false) // mom > 0.1)
			{
				angle_t mom_ang = R_PointToAngle(0, 0, mo->mom.x, mo->mom.y);

				mom *= 3.0; // * fabs(M_Sin(mom_ang));

				want_mx -= mom * -M_Sin(mom_ang - new_ang);
				want_my -= mom *  M_Cos(mom_ang - new_ang);
			}

			cmd->sidemove    += int(want_mx);
			cmd->forwardmove += int(want_my);

//			I_Printf("FWD %d  SIDE %d  --> SPEED %1.1f\n",
//				cmd->sidemove, cmd->forwardmove,
//				R_PointToDist(0, 0, cmd->sidemove, cmd->forwardmove));
		}
	}

	void Search()
	{
		if (path->stack.IsEmpty())
		{
			// nothing left to do -- just wander
			state = WANDERING;
			return;
		}

		// PATH FIND from current location to current target, treating all
		//   obstacles as passable EXCEPT any obstacle associated with
		//   a target in the stack.  Remember the first obstacle of this
		//   path in variable OBST.

		path->Prepare(path->stack.CurTarget(), pl->cards);

		int source = pl->mo->subsector - subsectors;

		bool found_path = BOT_FindPath(source, path);

		if (! found_path)
		{
			obstacle_base_c *top_obst = path->stack.CurObstacle();

			if (! path->stack.Bump())
			{
				if (top_obst)
					top_obst->DebugDump("! No path for obstacle:");
				else
					L_WriteDebug("! No path to MAIN GOAL.\n");
			}
			return;
		}

		if (! path->first_obst)
		{
			// success!
			obstacle_base_c *top_obst = path->stack.CurObstacle();

			if (top_obst)
				top_obst->DebugDump("Found path for obstacle:");
			else
				L_WriteDebug("Found path to MAIN GOAL.\n");

			goal = path->target;
			goal->DebugDump("__ New goal:");

			path->stack.Pop();
			if (goal->SignificantChange())
			{
				// pop all targets except first one.
				while (path->stack.GetSize() > 1)
					path->stack.Pop();
			}

			state = FOLLOW_GOAL;
			return;
		}

		path->first_obst->DebugDump("Hit obstacle:");

		// An obstacle got in the way -- solve it!
		// FIND all solutions to obstacle in OBST.

		target_group_c *T = new target_group_c(path->first_obst);

		int count = path->first_obst->FindSolutions(T, pl->cards);

		if (count == 0)
		{
			obstacle_base_c *top_obst = path->stack.CurObstacle();

			if (! path->stack.Bump())
			{
				if (top_obst)
					top_obst->DebugDump("!!! No solution for obstacle:");
				else
					L_WriteDebug("!!! No solution for MAIN GOAL.\n");
			}
			return;
		}

		for (int snum = 0; snum < count; snum++)
			T->GetSolution(snum)->DebugDump("  Solution:");

		path->stack.Push(T);
	}

	void CheckGoal()
	{
		if (! path->PValid(cur_pt+1))
		{
			if (goal)
				state = EXECUTE_GOAL;
			else
				state = WANDERING;
		}
	}

	void ExecuteGoal(ticcmd_t *cmd)
	{
		if (! goal) I_Error("ExecuteGoal: missing goal!\n");

		mobj_t *mo = pl->mo;

		float t_x, t_y;
		angle_t t_ang;

		// FIXME !!!! Assumes SWITCH target
		goal->GetPressLocation(&t_x, &t_y, &t_ang);

		bool reached_spot = true;

		// change horizontal angle
		{
			angle_t diff = t_ang - mo->angle;

			if (diff < ANG180)
			{
				if (diff > ANG5)
					diff = ANG5, reached_spot = false;
			}
			else
			{
				if (diff < (angle_t)~ANG5)
					diff = ~ANG5, reached_spot = false;
			}

			cmd->angleturn += diff >> 16;
		}

		// change mlook angle
		{
			angle_t diff = ~mo->vertangle;

			if (diff < ANG180)
			{
				if (diff > ANG5/2)
					diff = ANG5/2, reached_spot = false;
			}
			else
			{
				if (diff < (angle_t)~(ANG5/2))
					diff = ~(ANG5/2), reached_spot = false;
			}

			cmd->mlookturn += diff >> 16;
		}

		// home to spot
		float dist = R_PointToDist(mo->x, mo->y, t_x, t_y);
		t_ang = R_PointToAngle(mo->x, mo->y, t_x, t_y);

		if (dist > 6)
			reached_spot = false;

		float speed = 5; //!!!! dist / 4;

		if (speed < 5)  speed = 5;
		if (speed > 50) speed = 50;

L_WriteDebug("WANT TO GET TO: (%1.0f,%1.0f)  now @ (%1.0f,%1.0f)\n",
t_x, t_y, mo->x, mo->y);

		{
			angle_t new_ang = mo->angle + (angle_t)(cmd->angleturn << 16);

			// FIXME !!! this is fucked up (take current momentum into account)
			float want_mx = speed * -M_Sin(t_ang - new_ang);
			float want_my = speed *  M_Cos(t_ang - new_ang);

			cmd->sidemove    += int(want_mx);
			cmd->forwardmove += int(want_my);
		}

		if (reached_spot)
		{
			cmd->buttons |= BT_USE;
			state = SEARCHING;
			cur_pt = 0;
			ticker = TICRATE;
		}
	}

	void NewLevel()
	{
		delete path;
		path = NULL;

		state = SEARCHING;
		cur_pt = 0;

		ticker = (M_Random() & 127) + 15;
	}
};

void P_BotPlayerBuilder(const player_t *p, void *data, ticcmd_t *cmd)
{
	bot_c *bot = (bot_c *)data;

	if (gamestate != GS_LEVEL)
		return;

	bot->Think(cmd);
}

//
// P_BotCreate
//
// Converts the player (which should be empty, i.e. neither a network
// or console player) to a bot.  Recreate is true for bot players
// loaded from a savegame.
//
void P_BotCreate(player_t *p, bool recreate)
{
	bot_c *bot = new bot_c(p);

	p->builder = P_BotPlayerBuilder;
	p->build_data = (void *)bot;
	p->playerflags |= PFL_Bot;

	if (! recreate)
		sprintf(p->playername, "Bot%d", p->pnum + 1);
}

//
// BOT_BeginLevel
//
void BOT_BeginLevel(void)
{
	BOT_CreateSubInfo();

	for (int pnum = 0; pnum < MAXPLAYERS; pnum++)
	{
		player_t *p = players[pnum];

		if (! p || ! (p->playerflags & PFL_Bot))
			continue;

		if (! p->build_data)
			continue;

		bot_c *bot = (bot_c *) p->build_data;

		bot->NewLevel();
	}
}

//
// BOT_EndLevel
//
void BOT_EndLevel(void)
{
	BOT_FreeSubInfo();
}

