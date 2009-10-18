//----------------------------------------------------------------------------
//  BOT : Level analysis
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

#ifndef __BOT_LEVEL_H__
#define __BOT_LEVEL_H__

#include "ddf_main.h"
#include "r_defs.h"

#include "epi/utility.h"

class obstacle_base_c;
class target_group_c;

class traverse_bsp_base_c
{
	// This class is an Abstract Base Class used for handling the
	// results of a BSP traversal.  The virtual GotXXX functions
	// are called for each line or thing found, and should return
	// TRUE to continue searching or FALSE to stop.

public:
	traverse_bsp_base_c() { }
	virtual ~traverse_bsp_base_c() { }

	virtual bool GotLine(line_t *ld,  subsector_t *sub) = 0;
	virtual bool GotThing(mobj_t *mo, subsector_t *sub) = 0;
};

bool BOT_ThingIsAgent(mobj_t *mo);
bool BOT_ThingIsMissile(mobj_t *mo);
bool BOT_ThingIsPickup(mobj_t *mo);
bool BOT_ThingIsBlocker(mobj_t *mo);

bool BOT_TraverseBoxBSP(float lx, float ly, float hx, float hy,
	traverse_bsp_base_c *trav_info);

// bool BOT_CanTravelVector(float sx, float sy, float ex, float ey);

//----------------------------------------------------------------------------

class bot_achievements_c
{
public:
	bot_achievements_c();
	~bot_achievements_c();

private:
	keys_e owned_keys;

	epi::s32array_c sectors_opened;
	epi::s32array_c tags_opened;

public:
	void Clear();

	void AddKeys(keys_e K);
	void AddSector(int sec_num);
	void AddTag(int tag_num);

	bool HasKey(keys_e K) const;
	bool HasSector(int sec_num) const;
	bool HasTag(int tag_num) const;

	bool AllowLine(line_t *ld) const;

	void DebugDump(const char *preface = "  Achieved:");
};

float BOT_BaseDistAlongSeg(int sub, int next, seg_t *seg);

bool BOT_CanTraverseSeg(seg_t *seg, int sub, float R, bot_achievements_c *Ach);

bool BOT_CanTraverseSegUltra(seg_t *seg, int sub, float R,
	bot_achievements_c *Ach, obstacle_base_c ** OBST);

//----------------------------------------------------------------------------

///---class bot_goal_c
///---{
///---public:
///---	bot_goal_c();
///---	~bot_goal_c();
///---
///---public:
///---	bot_goal_c *goal_next;  // next in list
///---
///---	enum
///---	{
///---		TYP_INVALID = -1,
///---
///---		TYP_Obstacle = 0,
///---		TYP_Switch,
///---		TYP_Item
///---	};
///---
///---	int type;
///---
///---	/* --- obstacle info --- */
///---
///---	int ob_seg;
///---	int ob_front_subsec;
///---
///---	bool ob_switched;
///---	keys_e ob_need_keys;
///---///---	bool ob_direct_activate;
///---
///---	/* --- switch info --- */
///---
///---	int sw_line;
///---	int sw_front_subsec;
///---
///---	keys_e sw_need_keys;
///---
///---	/* --- item info --- */
///---
///---	const mobjtype_c *itm_info;
///---
///---	int itm_subsec;
///---
///---	keys_e itm_what_key;
///---
///---public:
///---	static bot_goal_c *NewObstacle(int seg, int subsec, bool switched);
///---	static bot_goal_c *NewSwitch(int line, int subsec);
///---	static bot_goal_c *NewItem(const mobjtype_c *info, int subsec);
///---};
///---
///---bool BOT_CanPassLine(seg_t *seg, int front_sub,
///---	bot_goal_c *achieved_goals, bot_goal_c **obst_list);
///---
///---bool BOT_CheckNewSwitch(seg_t *seg, int front_sub,
///---		bot_goal_c *achieved_goals, bot_goal_c **swth_list);
///---
///---bool BOT_CheckNewItem(mobj_t *mo, int sub, bot_goal_c **item_list);

//----------------------------------------------------------------------------

class fixed_target_base_c
{
	/* abstract base class */

public:
	fixed_target_base_c() { }
	virtual ~fixed_target_base_c() { }

	virtual bool Reached(int cur_sub) const = 0;

	virtual int Compute_H(float x, float y) const = 0;

	// FIXME: this only makes sense for switches!
	virtual void GetPressLocation(float *x, float *y, angle_t *ang) = 0;

	virtual bool SignificantChange() const = 0;

	virtual void DebugDump(const char *preface) = 0;
};

class bot_switch_target_c : public fixed_target_base_c
{
private:
	seg_t *sw_face_seg;
	int sw_subsec;

public:
	bot_switch_target_c(seg_t *_seg, int _sub) :
		fixed_target_base_c(),
		sw_face_seg(_seg), sw_subsec(_sub) { }

	~bot_switch_target_c() { }

public:
	void DebugDump(const char *preface);

	// FIXME !!!! Execution stuff
	void GetPressLocation(float *x, float *y, angle_t *ang);

	/* PATH-FIND methods */

	bool Reached(int cur_sub) const { return (cur_sub == sw_subsec); }

	int Compute_H(float x, float y) const
	{
		float adx = fabs((sw_face_seg->v1->x + sw_face_seg->v2->x) / 2.0f - x);
		float ady = fabs((sw_face_seg->v1->y + sw_face_seg->v2->y) / 2.0f - y);

		// Diagonal shortcut
		if (adx < ady)
			return I_ROUND(2.0f * ady + 0.8f * adx);
		else
			return I_ROUND(2.0f * adx + 0.8f * ady);
	}

	bool SignificantChange() const;
};

class bot_walk_target_c : public fixed_target_base_c
{
private:
	seg_t *wk_seg;
	int wk_front_sub;
	int wk_back_sub;
	bool wk_one_way;

public:
	bot_walk_target_c(seg_t *_seg, int _front, int _back, bool _oneway) :
		fixed_target_base_c(),
		wk_seg(_seg), wk_front_sub(_front), wk_back_sub(_back),
		wk_one_way(_oneway) { }

	~bot_walk_target_c() { }

public:
	void DebugDump(const char *preface);

	void GetPressLocation(float *x, float *y, angle_t *ang);

	/* PATH-FIND methods */

	bool Reached(int cur_sub) const
	{
		// usually allow either subsector to be reached
		if (! wk_one_way && cur_sub == wk_back_sub)
			return true;

		return (cur_sub == wk_front_sub);
	}

	int Compute_H(float x, float y) const
	{
		float adx = fabs((wk_seg->v1->x + wk_seg->v2->x) / 2.0f - x);
		float ady = fabs((wk_seg->v1->y + wk_seg->v2->y) / 2.0f - y);

		// Diagonal shortcut
		if (adx < ady)
			return I_ROUND(2.0f * ady + 0.8f * adx);
		else
			return I_ROUND(2.0f * adx + 0.8f * ady);
	}

	bool SignificantChange() const;
};

class bot_item_target_c : public fixed_target_base_c
{
private:
	keys_e ky_type;
	int ky_subsec;
	float ky_pos_x;
	float ky_pos_y;

public:
	bot_item_target_c(keys_e _type, int _sub, float _posx, float _posy) :
		fixed_target_base_c(),
		ky_type(_type), ky_subsec(_sub),
		ky_pos_x(_posx), ky_pos_y(_posy)
	{ }

	~bot_item_target_c() { }

public:
	void DebugDump(const char *preface);

	void GetPressLocation(float *x, float *y, angle_t *ang);

	/* PATH-FIND methods */

	bool Reached(int cur_sub) const { return (cur_sub == ky_subsec); }

	int Compute_H(float x, float y) const
	{
		float adx = fabs(ky_pos_x - x);
		float ady = fabs(ky_pos_y - y);

		// Diagonal shortcut
		if (adx < ady)
			return I_ROUND(2.0f * ady + 0.8f * adx);
		else
			return I_ROUND(2.0f * adx + 0.8f * ady);
	}

	bool SignificantChange() const { return true; }
};

fixed_target_base_c *BOT_FindLevelTarget(void);

//----------------------------------------------------------------------------

class obstacle_base_c
{
	/* abstract base class */

public:
	obstacle_base_c() { }
	virtual ~obstacle_base_c() { }

	bool Match(const obstacle_base_c *rhs) const
	{
		// UGH !!
		int lhs_data[4];  RawMatchData(lhs_data);
		int rhs_data[4];  rhs->RawMatchData(rhs_data);

		return (lhs_data[0] == rhs_data[0]) && (lhs_data[1] == rhs_data[1]) &&
		       (lhs_data[2] == rhs_data[2]) && (lhs_data[3] == rhs_data[3]);
	}

	virtual void DebugDump(const char *preface) = 0;

	virtual float DifficultyFactor() const = 0;
	virtual void MakeAchieved(bot_achievements_c *Ach) const = 0;

	virtual int FindSolutions(target_group_c *T, keys_e owned_keys) = 0;
	// returns number of solutions.

protected:
	virtual void RawMatchData(int *D) const = 0;
};

class sector_obstacle_c : public obstacle_base_c
{
	/* includes DOORS, LIFTS, MOVING FLOORS.
	 * can be self-activated or remote-activated.
     */
private:
	int sector;
	line_t *line;  // activator line

public:
	sector_obstacle_c(int _sector, line_t *ld);
	~sector_obstacle_c();

	void DebugDump(const char *preface);

	float DifficultyFactor() const { return 1.0f; }
	void MakeAchieved(bot_achievements_c *Ach) const;

	int FindSolutions(target_group_c *T, keys_e owned_keys);

protected:
	void RawMatchData(int *D) const
	{
		D[0] = 0x53656374;  // type marker: 'Sect'
		D[1] = sector;
		D[2] = 0;
		D[3] = 0;
	}
};

//----------------------------------------------------------------------------

class target_group_c
{
private:
	// the obstacle we are trying to pass
	obstacle_base_c *obst;

	static const int MAX_SOL = 20;  // FIXME: inefficient

	// the set of solutions (as targets) for the obstacle.
	fixed_target_base_c *solutions[MAX_SOL];
	int num_sols;

public:
	target_group_c(obstacle_base_c *_Obst);
	~target_group_c();

	void AddSolution(fixed_target_base_c *_Sol);

	inline int NumSolutions() const { return num_sols; }
	
	obstacle_base_c *GetObstacle() const;
	fixed_target_base_c *GetSolution(int idx) const;
};

class target_group_stack_c
{
private:
	static const int MAX_STK = 100;

	target_group_c *groups[MAX_STK];
	int cur_size;

	int solut_indexes[MAX_STK];

public:
	target_group_stack_c();
	~target_group_stack_c();

	inline int GetSize()  const { return cur_size; }
	inline bool IsEmpty() const { return cur_size == 0; }

	void Push(target_group_c *_Group);
	void Pop();

	bool Bump();
	// if there is another alternative, moves to it and returns true.
	// Otherwise performs Pop() and returns false.

	bool HasObstacle(obstacle_base_c *test) const;

	obstacle_base_c *CurObstacle() const;
	fixed_target_base_c *CurTarget() const;
};

#endif  /* __BOT_LEVEL_H__ */
