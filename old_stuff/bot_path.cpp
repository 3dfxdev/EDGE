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

#include "i_defs.h"
#include "bot_path.h"

#include "r_main.h"
#include "r_state.h"
#include "z_zone.h"


sub_extra_info_c *ss_info;

priority_heap_c *ss_heap;


sub_extra_info_c::sub_extra_info_c() : state(SUB_Unknown),
	parent(0), via_seg(NULL), G_score(0), H_score(0),
	bot_visit(0)
{
}

sub_extra_info_c::~sub_extra_info_c()
{
}

void sub_extra_info_c::ComputeMid(subsector_t *sub)
{
	double tmp_mid_x = 0;
	double tmp_mid_y = 0;

	int count = 0;

	for (seg_t *seg = sub->segs; seg; seg = seg->sub_next)
	{
		tmp_mid_x += double(seg->v1->x);
		tmp_mid_y += double(seg->v1->y);

		count++;
	}

	if (count > 0)
	{
		mid_x = float(tmp_mid_x / count);
		mid_y = float(tmp_mid_y / count);
	}
	else
	{
		mid_x = mid_y = 0;
	}
}

//----------------------------------------------------------------------------
//  PRIORITY HEAP ADT
//----------------------------------------------------------------------------

priority_heap_c::priority_heap_c(int _size)
{
	total_size = _size;

	heap = new int[total_size];
	map  = new int[total_size];

	Clear();
}

priority_heap_c::~priority_heap_c()
{
	delete[] heap;
	delete[] map;
}

void priority_heap_c::Clear()
{
	heap_size = 0;

	Z_Clear(map, int, total_size);
}

bool priority_heap_c::IsMapped(int node)
{
	return map[node] != 0;
}

void priority_heap_c::Insert(int node)
{
	DEV_ASSERT2(heap_size < total_size);  // overflow ?
	DEV_ASSERT2(! IsMapped(node));

	heap[heap_size] = node;
	map[node] = heap_size + 1;

	heap_size++;

	Adjust(heap_size-1);

#ifdef TEST_HEAP
	Validate(0);
#endif
}

int priority_heap_c::RemoveHead()
{
	DEV_ASSERT2(heap_size > 0);

	int node = heap[0];

	DEV_ASSERT2(IsMapped(node));

	if (heap_size > 1)
	{
		// move last entry to top, then adjust (it trickles down)
		Swap(0, heap_size-1);

		heap_size--;

		Adjust(0);
	}
	else
	{
		heap_size--;
	}

#ifdef TEST_HEAP
	Validate(0);
#endif

	map[node] = 0;

	return node;
}

void priority_heap_c::ChangeScore(int node)
{
	DEV_ASSERT2(IsMapped(node));

	Adjust(map[node] - 1);

#ifdef TEST_HEAP
	Validate(0);
#endif
}

void priority_heap_c::Adjust(int h_idx)
{
	for (;;)
	{
		// check for trickle up
		if (h_idx > 0)
		{
			if (ScoreAt(h_idx) > ScoreAt(Par(h_idx)))
			{
				Swap(h_idx, Par(h_idx));

				h_idx = Par(h_idx);
				continue;
			}
		}

		// check for trickle down
		if (HasLeft(h_idx))
		{
			int score = ScoreAt(h_idx);

			int left_sc = ScoreAt(Left(h_idx));

			if (HasRight(h_idx))
			{
				int right_sc = ScoreAt(Right(h_idx));

				if (score < right_sc && right_sc >= left_sc)
				{
					Swap(h_idx, Right(h_idx));

					h_idx = Right(h_idx);
					continue;
				}
			}

			if (score < left_sc)
			{
				Swap(h_idx, Left(h_idx));

				h_idx = Left(h_idx);
				continue;
			}
		}

		// everything was OK
		break;
	}
}

void priority_heap_c::DebugDump(int h_idx, int level)
{
	if (h_idx == 0 && IsEmpty())
	{
		L_WriteDebug("HEAP EMPTY\n");
		return;
	}

	static char buf[2048];

	buf[0] = 0;

	for (int k = 0; k < level-1; k++)
		strcat(buf, "|   ");

	if (level > 0)
		strcat(buf, "|__ ");

	L_WriteDebug("%s%d (node %d)\n", buf, ScoreAt(h_idx), heap[h_idx]);

	if (HasLeft(h_idx))
		DebugDump(Left(h_idx), level+1);

	if (HasRight(h_idx))
		DebugDump(Right(h_idx), level+1);
}

void priority_heap_c::Validate(int h_idx)
{
	if (h_idx == 0 && IsEmpty())
		return;
	
	DEV_ASSERT2(h_idx < heap_size);

	if (HasLeft(h_idx))
	{
		if (ScoreAt(Left(h_idx)) > ScoreAt(h_idx))
		{
			DebugDump();
			I_Error("priority_heap_c: Validate failed at %d (< L)\n", h_idx); 
		}

		Validate(Left(h_idx));
	}

	if (HasRight(h_idx))
	{
		if (ScoreAt(Right(h_idx)) > ScoreAt(h_idx))
		{
			DebugDump();
			I_Error("priority_heap_c: Validate failed at %d (< R)\n", h_idx); 
		}

		Validate(Right(h_idx));
	}
}

#ifdef TEST_HEAP
void BOT_TestHeap(void)
{
	srand(123);

	L_WriteDebug("========================================\n");
	L_WriteDebug("  TEST HEAP\n");
	L_WriteDebug("========================================\n");

	L_WriteDebug("Insertion test:\n");

	priority_heap_c *heap = new priority_heap_c(1024);

	for (int i=0; i < 32; i++)
	{
		int node = rand() & 1023;

		if (heap->IsMapped(node))
			continue;

		L_WriteDebug("  Adding #%i, node %d, score %d\n", i, node, heap->Score(node));

		heap->Insert(node);
	}

	L_WriteDebug("\nDump:\n");

	heap->DebugDump();

	L_WriteDebug("\nRemoval test:\n");

	while (! heap->IsEmpty())
	{
		int node = heap->RemoveHead();

		L_WriteDebug("  Removed node %d, score %d\n", node, heap->Score(node));
	}

	L_WriteDebug("\nTest completely successfully !\n");
	L_WriteDebug("========================================\n");

	delete heap;
}
#endif

//----------------------------------------------------------------------------
//   A* SEARCH ALGORITHM
//----------------------------------------------------------------------------

bool BOT_FindPath(int source, path_store_base_c *path_func)
{
	for (int j = 0; j < numsubsectors; j++)
		ss_info[j].state = sub_extra_info_c::SUB_Unknown;

	ss_heap->Clear();

	L_WriteDebug("---- Generating Path ----\n");

	ss_info[source].state = sub_extra_info_c::SUB_Open;
	ss_info[source].parent = -1;
	ss_info[source].via_seg = NULL;
	ss_info[source].G_score = 0;
	ss_info[source].H_score = 0;

	ss_heap->Insert(source);

	int target = -1;

	// iterate until target found or all avenues are exhausted
	for (;;)
	{
		if (ss_heap->IsEmpty())
		{
			L_WriteDebug("NO PATH DUDE!!\n");
			break;
		}

		int cur = ss_heap->RemoveHead();

// L_WriteDebug("  Visiting %d : (%1.1f, %1.1f)\n", cur, mx, my);

		ss_info[cur].state = sub_extra_info_c::SUB_Closed;

		if (path_func->VisitSubsec(cur))
		{
			L_WriteDebug("<<<<< FOUND >>>>>\n");
			target = cur;
			break;
		}

		for (seg_t *seg = subsectors[cur].segs; seg; seg = seg->sub_next)
		{
			if (! seg->back_sub)  // one-sided ?
				continue;

			int back = seg->back_sub - subsectors;

			if (ss_info[back].state == sub_extra_info_c::SUB_Closed)
				continue;

			int G_score = path_func->Compute_G(cur, back, seg);

			if (G_score < 0)  // impassable
				continue;

			// compute total distance for path
			G_score += ss_info[cur].G_score;

			if (! ss_heap->IsMapped(back))
			{
				ss_info[back].state = sub_extra_info_c::SUB_Open;
				ss_info[back].parent = cur;
				ss_info[back].via_seg = seg;
				ss_info[back].G_score = G_score;
				ss_info[back].H_score = path_func->Compute_H(cur);

				ss_heap->Insert(back);
			}
			else if (G_score < ss_info[back].G_score)
			{
///--- L_WriteDebug("Changing score: NODE %d: old %d --> new %d\n", back, ss_info[back].G_score, G_score);
				ss_info[back].parent = cur;
				ss_info[back].via_seg = seg;
				ss_info[back].G_score = G_score;

				ss_heap->ChangeScore(back);
			}
		}
	}

	if (target <= 0)
		return false;

	L_WriteDebug("---- Transforming point array ----\n");

	if (path_func->NeedForward())
	{
		// reverse the path (update parent pointers)
		// We need to update the 'via_seg' fields too...

		int last = -1;
		seg_t *last_seg = NULL;

		int next;
		seg_t *cur_seg;

		for (int cur = target; cur >= 0; cur = next)
		{
			next    = ss_info[cur].parent;
			cur_seg = ss_info[cur].via_seg;

			ss_info[cur].parent  = last;
			ss_info[cur].via_seg = last_seg;

			last     = cur;
			last_seg = cur_seg;
		}

		target = last;
	}

	if (path_func->NeedTest())
	{
		for (int cur = target; cur >= 0; cur = ss_info[cur].parent)
		{
			if (! path_func->TestSubsec(cur, ss_info[cur].parent, ss_info[cur].via_seg))
				return true;
		}
	}

	for (int cur = target; cur >= 0; cur = ss_info[cur].parent)
	{
		if (! path_func->AddSubsec(cur, ss_info[cur].parent, ss_info[cur].via_seg))
			break;
	}

	return true;
}

//----------------------------------------------------------------------------

//
// BOT_CreateSubInfo
//
void BOT_CreateSubInfo(void)
{
	ss_info = new sub_extra_info_c[numsubsectors];
	
	for (int i = 0; i < numsubsectors; i++)
		ss_info[i].ComputeMid(subsectors + i);

	ss_heap = new priority_heap_c(numsubsectors);
}

//
// BOT_FreeSubInfo
//
void BOT_FreeSubInfo(void)
{
	delete[] ss_info;
	ss_info = NULL;

	delete ss_heap;
	ss_heap = NULL;
}

