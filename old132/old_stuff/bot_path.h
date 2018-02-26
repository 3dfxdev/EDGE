//----------------------------------------------------------------------------
//  BOT : Pathing Algorithms
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

#ifndef __BOT_PATH_H__
#define __BOT_PATH_H__

#include "r_defs.h"

/* #define TEST_HEAP 1 */

class sub_extra_info_c
{
public:
	sub_extra_info_c();
	~sub_extra_info_c();

	enum
	{
		SUB_Unknown = 0,
		SUB_Open,
		SUB_Closed
	};

	int state;
	int parent;
	seg_t *via_seg;  // seg in PARENT's subsector used to get HERE

	int G_score;  
	int H_score;  

	float mid_x, mid_y;

	int bot_visit;  // bitflags of bots passing through here

	inline int F_score() const { return G_score + H_score; }

	void ComputeMid(subsector_t *sub);
};

extern sub_extra_info_c *ss_info;

//----------------------------------------------------------------------------

// priority heap for the A* open list

class priority_heap_c
{
public:
	priority_heap_c(int _size);
	~priority_heap_c();

private:
	int total_size;

	int *heap;
	int heap_size;

	int *map;  // maps node index --> (heap index + 1), zero => not present

public:
	void Clear();

	inline bool IsEmpty() { return (heap_size == 0); }

	bool IsMapped(int node);

	void Insert(int node);
	// insert a node into the heap.  The node must not be present
	// already.

	int RemoveHead();
	// removes the top-most node and returns it.  Must not be called
	// on an empty heap.

	void ChangeScore(int node);
	// change the score for a node.  Causes heap to be re-adjusted.

	void DebugDump(int h_idx = 0, int level = 0);

private:
	inline int Par(int h_idx) const
	{
		return (h_idx+1) / 2 - 1;
	}

	inline int Left(int h_idx) const
	{
		return (h_idx+1) * 2 - 1;
	}

	inline int Right(int h_idx) const
	{
		return (h_idx+1) * 2;
	}

	inline bool HasLeft(int h_idx) const
	{
		return Left(h_idx) < heap_size;
	}

	inline bool HasRight(int h_idx) const
	{
		return Right(h_idx) < heap_size;
	}

	void Adjust(int h_idx);
	// handles trickle up and down.

	void Validate(int h_idx);
	// debugging support -- check if heap is valid.

	inline void Swap(int i, int j)
	{
		int tmp = heap[i];
		heap[i] = heap[j];
		heap[j] = tmp;

		map[heap[i]] = i+1;
		map[heap[j]] = j+1;
	}

public:
	/*  NODE SCORING FUNCTION
	 */
	inline int Score(int node)
	{
#ifdef TEST_HEAP
		return (node * 37) & 255;
#else
		// negate it, since we want the LOWEST score
		return 0 - ss_info[node].F_score();
#endif
	}

	inline int ScoreAt(int h_idx) { return Score(heap[h_idx]); }
};

//----------------------------------------------------------------------------

class path_store_base_c
{
	// this is an Abstract Base Class used to compute the scores
    // for the A* path finding algorithm.  It is also used to store
	// the generated path.

public:
	path_store_base_c() { }
	virtual ~path_store_base_c() { }

	virtual bool VisitSubsec(int sub) = 0;
	// called when a subsector is visited (i.e. becomes CLOSED in
	// the A* algorithm).  Normally returns FALSE, and should
	// return TRUE if the path-finding should stop (i.e. the
	// given subsector is the target).

	virtual int Compute_G(int sub, int next, seg_t *seg) = 0;
	// compute the cost from crossing the current subsector to
	// the 'next' subsector via the given seg.  This value should
	// roughly be the distance * 100, with added penalties for more
	// difficult terrain (e.g. slime), waiting for doors, etc..
	// Return a NEGATIVE value for an unpassable wall.

	virtual int Compute_H(int sub) = 0;
	// compute the heuristic value for the current subsector.
	// Generally this estimates the "G" cost to reach the target
	// subsector (e.g. straight line dist * typical G cost).
	// Returning ZERO gives a pure breadth-first search (i.e.
	// Dijkstra's algorithm).

	virtual bool NeedForward() const = 0;
	// TRUE if the subsectors must be added in the forward
	// direction (source to target).  When FALSE, the subsectors
	// will be added in reverse order (target to source).

	virtual bool NeedTest() const = 0;
	// TRUE if the testing phase is needed.

	virtual bool TestSubsec(int sub, int next, seg_t *seg) = 0;
	// Test a single element of the found path.  Return true if
	// o.k. or FALSE if path was unsuitable (in which case we
	// stop here -- no more TestSubsec calls are made and no
	// calls to AddSubsec are made).  Note: the path finding is
	// still considered successful, hence you should mark the
	// testing failure internally in your class.

	virtual bool AddSubsec(int sub, int next, seg_t *seg) = 0;
	// store a single subsector as part of the found path.
	// Return true to continue, or FALSE to stop right here.
	// 'next' is NEGATIVE for the last subsector in the chain.
	//
	// It is assumed that this object is prepared to receive
	// these subsectors (e.g. any previous path was cleared
	// before starting the path finding algorithm).
};

void BOT_CreateSubInfo(void);
void BOT_FreeSubInfo(void);

bool BOT_FindPath(int source, path_store_base_c *path_func);

#endif  /* __BOT_PATH_H__ */
