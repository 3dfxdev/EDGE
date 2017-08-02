/*
    Most of the logic for the node builder.
    Copyright (C) 2002-2006 Randy Heit

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#ifndef _WIN32
#include <unistd.h>
#endif

#include "zdbsp.h"
#include "nodebuild.h"
#include "templates.h"

#define Printf printf
#define STACK_ARGS

#if 0
#define D(x) x
#else
#define D(x) do{}while(0)
#endif

FNodeBuilder::FNodeBuilder (FLevel &level,
							TArray<FPolyStart> &polyspots, TArray<FPolyStart> &anchors,
							const char *name, bool makeGLnodes)
	: Level(level), SegsStuffed(0), MapName(name)
{
	VertexMap = new FVertexMap (*this, Level.MinX, Level.MinY, Level.MaxX, Level.MaxY);
	GLNodes = makeGLnodes;
	FindUsedVertices (Level.Vertices, Level.NumVertices);
	MakeSegsFromSides ();
	FindPolyContainers (polyspots, anchors);
	GroupSegPlanes ();
	BuildTree ();
}

FNodeBuilder::~FNodeBuilder()
{
	if (VertexMap != 0)
	{
		delete VertexMap;
	}
}

void FNodeBuilder::BuildTree ()
{
	fixed_t bbox[4];

	fprintf (stderr, "   BSP:   0.0%%\r");
	HackSeg = DWORD_MAX;
	HackMate = DWORD_MAX;
	CreateNode (0, Segs.Size(), bbox);
	CreateSubsectorsForReal ();
	fprintf (stderr, "   BSP: 100.0%%\n");
}

DWORD FNodeBuilder::CreateNode (DWORD set, unsigned int count, fixed_t bbox[4])
{
	node_t node;
	int skip, selstat;
	DWORD splitseg;

	// When building GL nodes, count may not be an exact count of the number of segs
	// in this set. That's okay, because we just use it to get a skip count, so an
	// estimate is fine.
	skip = int(count / MaxSegs);

	if ((selstat = SelectSplitter (set, node, splitseg, skip, true)) > 0 ||
		(skip > 0 && (selstat = SelectSplitter (set, node, splitseg, 1, true)) > 0) ||
		(selstat < 0 && (SelectSplitter (set, node, splitseg, skip, false) > 0 ||
						(skip > 0 && SelectSplitter (set, node, splitseg, 1, false)))) ||
		CheckSubsector (set, node, splitseg))
	{
		// Create a normal node
		DWORD set1, set2;
		unsigned int count1, count2;

		SplitSegs (set, node, splitseg, set1, set2, count1, count2);
		D(PrintSet (1, set1));
		D(Printf ("(%d,%d) delta (%d,%d) from seg %d\n", node.x>>16, node.y>>16, node.dx>>16, node.dy>>16, splitseg));
		D(PrintSet (2, set2));
		node.intchildren[0] = CreateNode (set1, count1, node.bbox[0]);
		node.intchildren[1] = CreateNode (set2, count2, node.bbox[1]);
		bbox[BOXTOP] = MAX (node.bbox[0][BOXTOP], node.bbox[1][BOXTOP]);
		bbox[BOXBOTTOM] = MIN (node.bbox[0][BOXBOTTOM], node.bbox[1][BOXBOTTOM]);
		bbox[BOXLEFT] = MIN (node.bbox[0][BOXLEFT], node.bbox[1][BOXLEFT]);
		bbox[BOXRIGHT] = MAX (node.bbox[0][BOXRIGHT], node.bbox[1][BOXRIGHT]);
		return (int)Nodes.Push (node);
	}
	else
	{
		return NFX_SUBSECTOR | CreateSubsector (set, bbox);
	}
}

DWORD FNodeBuilder::CreateSubsector (DWORD set, fixed_t bbox[4])
{
	int ssnum, count;

	bbox[BOXTOP] = bbox[BOXRIGHT] = INT_MIN;
	bbox[BOXBOTTOM] = bbox[BOXLEFT] = INT_MAX;

	D(Printf ("Subsector from set %d\n", set));

	assert (set != DWORD_MAX);

#if defined(_DEBUG)// || 1
	// Check for segs with duplicate start/end vertices
	DWORD s1, s2;

	for (s1 = set; s1 != DWORD_MAX; s1 = Segs[s1].next)
	{
		for (s2 = Segs[s1].next; s2 != DWORD_MAX; s2 = Segs[s2].next)
		{
			if (Segs[s1].v1 == Segs[s2].v1)
				printf ("Segs %d%c and %d%c have duplicate start vertex %d (%d, %d)\n",
				s1, Segs[s1].linedef == -1 ? '*' : ' ',
				s2, Segs[s2].linedef == -1 ? '*' : ' ',
				Segs[s1].v1,
				Vertices[Segs[s1].v1].x >> 16, Vertices[Segs[s1].v1].y >> 16);
			if (Segs[s1].v2 == Segs[s2].v2)
				printf ("Segs %d%c and %d%c have duplicate end vertex %d (%d, %d)\n",
				s1, Segs[s1].linedef == -1 ? '*' : ' ',
				s2, Segs[s2].linedef == -1 ? '*' : ' ',
				Segs[s1].v2,
				Vertices[Segs[s1].v2].x >> 16, Vertices[Segs[s1].v2].y >> 16);
		}
	}
#endif

	// We cannot actually create the subsector now because the node building
	// process might split a seg in this subsector (because all partner segs
	// must use the same pair of vertices), adding a new seg that hasn't been
	// created yet. After all the nodes are built, then we can create the
	// actual subsectors using the CreateSubsectorsForReal function below.
	ssnum = (int)SubsectorSets.Push (set);

	count = 0;
	while (set != DWORD_MAX)
	{
		AddSegToBBox (bbox, &Segs[set]);
		set = Segs[set].next;
		count++;
	}

	SegsStuffed += count;
	if ((SegsStuffed & ~63) != ((SegsStuffed - count) & ~63))
	{
		int percent = (int)(SegsStuffed * 1000.0 / Segs.Size());
		fprintf (stderr, "   BSP: %3d.%d%%\r", percent/10, percent%10);
	}

	D(Printf ("bbox (%d,%d)-(%d,%d)\n", bbox[BOXLEFT]>>16, bbox[BOXBOTTOM]>>16, bbox[BOXRIGHT]>>16, bbox[BOXTOP]>>16));

	return ssnum;
}

void FNodeBuilder::CreateSubsectorsForReal ()
{
	unsigned int i;

	for (i = 0; i < SubsectorSets.Size(); ++i)
	{
		subsector_t sub;
		DWORD set = SubsectorSets[i];

		sub.firstline = (DWORD)SegList.Size();
		while (set != DWORD_MAX)
		{
			USegPtr ptr;

			ptr.SegPtr = &Segs[set];
			SegList.Push (ptr);
			set = ptr.SegPtr->next;
		}
		sub.numlines = (DWORD)(SegList.Size() - sub.firstline);

		// Sort segs by linedef for special effects
		qsort (&SegList[sub.firstline], sub.numlines, sizeof(USegPtr), SortSegs);

		// Convert seg pointers into indices
		D(printf ("Output subsector %d:\n", Subsectors.Size()));
		if (SegList[sub.firstline].SegPtr->linedef == -1)
		{
			printf ("  Failure: Subsector %d is all minisegs!\n", Subsectors.Size());
		}
		for (unsigned int i = sub.firstline; i < SegList.Size(); ++i)
		{
			D(printf ("  Seg %5d%c%d(%5d,%5d)-%d(%5d,%5d)  [%08x,%08x]-[%08x,%08x]\n", SegList[i].SegPtr - &Segs[0],
				SegList[i].SegPtr->linedef == -1 ? '+' : ' ',
				SegList[i].SegPtr->v1,
				Vertices[SegList[i].SegPtr->v1].x>>16,
				Vertices[SegList[i].SegPtr->v1].y>>16,
				SegList[i].SegPtr->v2,
				Vertices[SegList[i].SegPtr->v2].x>>16,
				Vertices[SegList[i].SegPtr->v2].y>>16,
				Vertices[SegList[i].SegPtr->v1].x, Vertices[SegList[i].SegPtr->v1].y,
				Vertices[SegList[i].SegPtr->v2].x, Vertices[SegList[i].SegPtr->v2].y));
			SegList[i].SegNum = DWORD(SegList[i].SegPtr - &Segs[0]);
		}
		Subsectors.Push (sub);
	}
}

int STACK_ARGS FNodeBuilder::SortSegs (const void *a, const void *b)
{
	const FPrivSeg *x = ((const USegPtr *)a)->SegPtr;
	const FPrivSeg *y = ((const USegPtr *)b)->SegPtr;

	// Segs are grouped into three categories in this order:
	//
	// 1. Segs with different front and back sectors (or no back at all).
	// 2. Segs with the same front and back sectors.
	// 3. Minisegs.
	//
	// Within the first two sets, segs are also sorted by linedef.
	//
	// Note that when GL subsectors are written, the segs will be reordered
	// so that they are in clockwise order, and extra minisegs will be added
	// as needed to close the subsector. But the first seg used will still be
	// the first seg chosen here.

	int xtype, ytype;

	if (x->linedef == -1)
	{
		xtype = 2;
	}
	else if (x->frontsector == x->backsector)
	{
		xtype = 1;
	}
	else
	{
		xtype = 0;
	}

	if (y->linedef == -1)
	{
		ytype = 2;
	}
	else if (y->frontsector == y->backsector)
	{
		ytype = 1;
	}
	else
	{
		ytype = 0;
	}

	if (xtype != ytype)
	{
		return xtype - ytype;
	}
	else if (xtype < 2)
	{
		return x->linedef - y->linedef;
	}
	else
	{
		return 0;
	}
}

// Given a set of segs, checks to make sure they all belong to a single
// sector. If so, false is returned, and they become a subsector. If not,
// a splitter is synthesized, and true is returned to continue processing
// down this branch of the tree.

bool FNodeBuilder::CheckSubsector (DWORD set, node_t &node, DWORD &splitseg)
{
	int sec;
	DWORD seg;

	sec = -1;
	seg = set;

	do
	{
		D(Printf (" - seg %d%c(%d,%d)-(%d,%d) line %d front %d back %d\n", seg,
			Segs[seg].linedef == -1 ? '+' : ' ',
			Vertices[Segs[seg].v1].x>>16, Vertices[Segs[seg].v1].y>>16,
			Vertices[Segs[seg].v2].x>>16, Vertices[Segs[seg].v2].y>>16,
			Segs[seg].linedef, Segs[seg].frontsector, Segs[seg].backsector));
		if (Segs[seg].linedef != -1 &&
			Segs[seg].frontsector != sec
			// Segs with the same front and back sectors are allowed to reside
			// in a subsector with segs from a different sector, because the
			// only effect they can have on the display is to place masked
			// mid textures in the scene. Since minisegs only mark subsector
			// boundaries, their sector information is unimportant.
			//
			// Update: Lines with the same front and back sectors *can* affect
			// the display if their subsector does not match their front sector.
			/*&& Segs[seg].frontsector != Segs[seg].backsector*/)
		{
			if (sec == -1)
			{
				sec = Segs[seg].frontsector;
			}
			else
			{
				break;
			}
		}
		seg = Segs[seg].next;
	} while (seg != DWORD_MAX);

	if (seg == DWORD_MAX)
	{ // It's a valid non-GL subsector, and probably a valid GL subsector too.
		if (GLNodes)
		{
			return CheckSubsectorOverlappingSegs (set, node, splitseg);
		}
		return false;
	}

	D(Printf("Need to synthesize a splitter for set %d on seg %d\n", set, seg));
	splitseg = DWORD_MAX;

	// This is a very simple and cheap "fix" for subsectors with segs
	// from multiple sectors, and it seems ZenNode does something
	// similar. It is the only technique I could find that makes the
	// "transparent water" in nb_bmtrk.wad work properly.
	return ShoveSegBehind (set, node, seg, DWORD_MAX);
}

// When creating GL nodes, we need to check for segs with the same start and
// end vertices and split them into two subsectors.

bool FNodeBuilder::CheckSubsectorOverlappingSegs (DWORD set, node_t &node, DWORD &splitseg)
{
	int v1, v2;
	DWORD seg1, seg2;

	for (seg1 = set; seg1 != DWORD_MAX; seg1 = Segs[seg1].next)
	{
		if (Segs[seg1].linedef == -1)
		{ // Do not check minisegs.
			continue;
		}
		v1 = Segs[seg1].v1;
		v2 = Segs[seg1].v2;
		for (seg2 = Segs[seg1].next; seg2 != DWORD_MAX; seg2 = Segs[seg2].next)
		{
			if (Segs[seg2].v1 == v1 && Segs[seg2].v2 == v2)
			{
				if (Segs[seg2].linedef == -1)
				{ // Do not put minisegs into a new subsector.
					swap (seg1, seg2);
				}
				D(Printf("Need to synthesize a splitter for set %d on seg %d (ov)\n", set, seg2));
				splitseg = DWORD_MAX;

				return ShoveSegBehind (set, node, seg2, seg1);
			}
		}
	}
	// It really is a good subsector.
	return false;
}

// The seg is marked to indicate that it should be forced to the
// back of the splitter. Because these segs already form a convex
// set, all the other segs will be in front of the splitter. Since
// the splitter is formed from this seg, the back of the splitter
// will have a one-dimensional subsector. SplitSegs() will add one
// or two new minisegs to close it: If mate is DWORD_MAX, then a
// new seg is created to replace this one on the front of the
// splitter. Otherwise, mate takes its place. In either case, the
// seg in front of the splitter is partnered with a new miniseg on
// the back so that the back will have two segs.

bool FNodeBuilder::ShoveSegBehind (DWORD set, node_t &node, DWORD seg, DWORD mate)
{
	SetNodeFromSeg (node, &Segs[seg]);
	HackSeg = seg;
	HackMate = mate;
	if (!Segs[seg].planefront)
	{
		node.x += node.dx;
		node.y += node.dy;
		node.dx = -node.dx;
		node.dy = -node.dy;
	}
	return Heuristic (node, set, false) > 0;
}

// Splitters are chosen to coincide with segs in the given set. To reduce the
// number of segs that need to be considered as splitters, segs are grouped into
// according to the planes that they lie on. Because one seg on the plane is just
// as good as any other seg on the plane at defining a split, only one seg from
// each unique plane needs to be considered as a splitter. A result of 0 means
// this set is a convex region. A result of -1 means that there were possible
// splitters, but they all split segs we want to keep intact.
int FNodeBuilder::SelectSplitter (DWORD set, node_t &node, DWORD &splitseg, int step, bool nosplit)
{
	int stepleft;
	int bestvalue;
	DWORD bestseg;
	DWORD seg;
	bool nosplitters = false;

	bestvalue = 0;
	bestseg = DWORD_MAX;

	seg = set;
	stepleft = 0;

	memset (&PlaneChecked[0], 0, PlaneChecked.Size());

	D(printf("Processing set %d\n", set));

	while (seg != DWORD_MAX)
	{
		FPrivSeg *pseg = &Segs[seg];

		if (--stepleft <= 0)
		{
			int l = pseg->planenum >> 3;
			int r = 1 << (pseg->planenum & 7);

			if (l < 0 || (PlaneChecked[l] & r) == 0)
			{
				if (l >= 0)
				{
					PlaneChecked[l] |= r;
				}

				stepleft = step;
				SetNodeFromSeg (node, pseg);

				int value = Heuristic (node, set, nosplit);

				D(Printf ("Seg %5d, ld %d (%5d,%5d)-(%5d,%5d) scores %d\n", seg,
					Segs[seg].linedef,
					node.x>>16, node.y>>16,
					(node.x+node.dx)>>16, (node.y+node.dy)>>16, value));

				if (value > bestvalue)
				{
					bestvalue = value;
					bestseg = seg;
				}
				else if (value < 0)
				{
					nosplitters = true;
				}
			}
			else
			{
				pseg = pseg;
			}
		}

		seg = pseg->next;
	}

	if (bestseg == DWORD_MAX)
	{ // No lines split any others into two sets, so this is a convex region.
	D(Printf ("set %d, step %d, nosplit %d has no good splitter (%d)\n", set, step, nosplit, nosplitters));
		return nosplitters ? -1 : 0;
	}

	D(Printf ("split seg %u in set %u, score %d, step %d, nosplit %d\n", bestseg, set, bestvalue, step, nosplit));

	splitseg = bestseg;
	SetNodeFromSeg (node, &Segs[bestseg]);
	return 1;
}

// Given a splitter (node), returns a score based on how "good" the resulting
// split in a set of segs is. Higher scores are better. -1 means this splitter
// splits something it shouldn't and will only be returned if honorNoSplit is
// true. A score of 0 means that the splitter does not split any of the segs
// in the set.

int FNodeBuilder::Heuristic (node_t &node, DWORD set, bool honorNoSplit)
{
	// Set the initial score above 0 so that near vertex anti-weighting is less likely to produce a negative score.
	int score = 1000000;
	int segsInSet = 0;
	int counts[2] = { 0, 0 };
	int realSegs[2] = { 0, 0 };
	int specialSegs[2] = { 0, 0 };
	DWORD i = set;
	int sidev[2];
	int side;
	bool splitter = false;
	unsigned int max, m2, p, q;
	double frac;

	Touched.Clear ();
	Colinear.Clear ();

	while (i != DWORD_MAX)
	{
		const FPrivSeg *test = &Segs[i];

		if (HackSeg == i)
		{
			side = 1;
		}
		else
		{
			side = ClassifyLine (node, &Vertices[test->v1], &Vertices[test->v2], sidev);
		}

		switch (side)
		{
		case 0:	// Seg is on only one side of the partition
		case 1:
			// If we don't split this line, but it abuts the splitter, also reject it.
			// The "right" thing to do in this case is to only reject it if there is
			// another nosplit seg from the same sector at this vertex. Note that a line
			// that lies exactly on top of the splitter is okay.
			if (test->loopnum && honorNoSplit && (sidev[0] == 0 || sidev[1] == 0))
			{
				if ((sidev[0] | sidev[1]) != 0)
				{
					max = Touched.Size();
					for (p = 0; p < max; ++p)
					{
						if (Touched[p] == test->loopnum)
						{
							break;
						}
					}
					if (p == max)
					{
						Touched.Push (test->loopnum);
					}
				}
				else
				{
					max = Colinear.Size();
					for (p = 0; p < max; ++p)
					{
						if (Colinear[p] == test->loopnum)
						{
							break;
						}
					}
					if (p == max)
					{
						Colinear.Push (test->loopnum);
					}
				}
			}

			counts[side]++;
			if (test->linedef != -1)
			{
				realSegs[side]++;
				if (test->frontsector == test->backsector)
				{
					specialSegs[side]++;
				}
				// Add some weight to the score for unsplit lines
				score += SplitCost;	
			}
			else
			{
				// Minisegs don't count quite as much for nosplitting
				score += SplitCost / 4;
			}
			break;

		default:	// Seg is cut by the partition
			// If we are not allowed to split this seg, reject this splitter
			if (test->loopnum)
			{
				if (honorNoSplit)
				{
					D(Printf ("Splits seg %d\n", i));
					return -1;
				}
				else
				{
					splitter = true;
				}
			}

			// Splitters that are too close to a vertex are bad.
			frac = InterceptVector (node, *test);
			if (frac < 0.001 || frac > 0.999)
			{
				FPrivVert *v1 = &Vertices[test->v1];
				FPrivVert *v2 = &Vertices[test->v2];
				double x = v1->x, y = v1->y;
				x += frac * (v2->x - x);
				y += frac * (v2->y - y);
				if (fabs(x - v1->x) < VERTEX_EPSILON+1 && fabs(y - v1->y) < VERTEX_EPSILON+1)
				{
					D(Printf("Splitter will produce same start vertex as seg %d\n", i));
					return -1;
				}
				if (fabs(x - v2->x) < VERTEX_EPSILON+1 && fabs(y - v2->y) < VERTEX_EPSILON+1)
				{
					D(Printf("Splitter will produce same end vertex as seg %d\n", i));
					return -1;
				}
				if (frac > 0.999)
				{
					frac = 1 - frac;
				}
				int penalty = int(1 / frac);
				score = MAX(score - penalty, 1);
				D(Printf ("Penalized splitter by %d for being near endpt of seg %d (%f).\n", penalty, i, frac));
			}

			counts[0]++;
			counts[1]++;
			if (test->linedef != -1)
			{
				realSegs[0]++;
				realSegs[1]++;
				if (test->frontsector == test->backsector)
				{
					specialSegs[0]++;
					specialSegs[1]++;
				}
			}
			break;
		}

		segsInSet++;
		i = test->next;
	}

	// If this line is outside all the others, return a special score
	if (counts[0] == 0 || counts[1] == 0)
	{
		return 0;
	}

	// A splitter must have at least one real seg on each side.
	// Otherwise, a subsector could be left without any way to easily
	// determine which sector it lies inside.
	if (realSegs[0] == 0 || realSegs[1] == 0)
	{
		D(Printf ("Leaves a side with only mini segs\n"));
		return -1;
	}

	// Try to avoid splits that leave only "special" segs, so that the generated
	// subsectors have a better chance of choosing the correct sector. This situation
	// is not neccesarily bad, just undesirable.
	if (honorNoSplit && (specialSegs[0] == realSegs[0] || specialSegs[1] == realSegs[1]))
	{
		D(Printf ("Leaves a side with only special segs\n"));
		return -1;
	}

	// If this splitter intersects any vertices of segs that should not be split,
	// check if it is also colinear with another seg from the same sector. If it
	// is, the splitter is okay. If not, it should be rejected. Why? Assuming that
	// polyobject containers are convex (which they should be), a splitter that
	// is colinear with one of the sector's segs and crosses the vertex of another
	// seg of that sector must be crossing the container's corner and does not
	// actually split the container.

	max = Touched.Size ();
	m2 = Colinear.Size ();

	// If honorNoSplit is false, then both these lists will be empty.

	// If the splitter touches some vertices without being colinear to any, we
	// can skip further checks and reject this right away.
	if (m2 == 0 && max > 0)
	{
		return -1;
	}

	for (p = 0; p < max; ++p)
	{
		int look = Touched[p];
		for (q = 0; q < m2; ++q)
		{
			if (look == Colinear[q])
			{
				break;
			}
		}
		if (q == m2)
		{ // Not a good one
			return -1;
		}
	}

	// Doom maps are primarily axis-aligned lines, so it's usually a good
	// idea to prefer axis-aligned splitters over diagonal ones. Doom originally
	// had special-casing for orthogonal lines, so they performed better. ZDoom
	// does not care about the line's direction, so this is merely a choice to
	// try and improve the final tree.

	if ((node.dx == 0) || (node.dy == 0))
	{
		// If we have to split a seg we would prefer to keep unsplit, give
		// extra precedence to orthogonal lines so that the polyobjects
		// outside the entrance to MAP06 in Hexen MAP02 display properly.
		if (splitter)
		{
			score += segsInSet*8;
		}
		else
		{
			score += segsInSet/AAPreference;
		}
	}

	score += (counts[0] + counts[1]) - abs(counts[0] - counts[1]);

	return score;
}

void FNodeBuilder::SplitSegs (DWORD set, node_t &node, DWORD splitseg, DWORD &outset0, DWORD &outset1, unsigned int &count0, unsigned int &count1)
{
	unsigned int _count0 = 0;
	unsigned int _count1 = 0;
	outset0 = DWORD_MAX;
	outset1 = DWORD_MAX;

	Events.DeleteAll ();
	SplitSharers.Clear ();

	while (set != DWORD_MAX)
	{
		bool hack;
		FPrivSeg *seg = &Segs[set];
		int next = seg->next;

		int sidev[2], side;

		if (HackSeg == set)
		{
			HackSeg = DWORD_MAX;
			side = 1;
			sidev[0] = sidev[1] = 0;
			hack = true;
		}
		else
		{
			side = ClassifyLine (node, &Vertices[seg->v1], &Vertices[seg->v2], sidev);
			hack = false;
		}

		switch (side)
		{
		case 0: // seg is entirely in front
			seg->next = outset0;
			//Printf ("%u in front\n", set);
			outset0 = set;
			_count0++;
			break;

		case 1: // seg is entirely in back
			seg->next = outset1;
			//Printf ("%u in back\n", set);
			outset1 = set;
			_count1++;
			break;

		default: // seg needs to be split
			double frac;
			FPrivVert newvert;
			unsigned int vertnum;
			int seg2;

			//Printf ("%u is cut\n", set);
			if (seg->loopnum)
			{
				Printf ("   Split seg %lu (%d,%d)-(%d,%d) of sector %d on line %d\n",
					(unsigned long)set,
					Vertices[seg->v1].x>>16, Vertices[seg->v1].y>>16,
					Vertices[seg->v2].x>>16, Vertices[seg->v2].y>>16,
					seg->frontsector, seg->linedef);
			}

			frac = InterceptVector (node, *seg);
			newvert.x = Vertices[seg->v1].x;
			newvert.y = Vertices[seg->v1].y;
			newvert.x += fixed_t(frac * double(Vertices[seg->v2].x - newvert.x));
			newvert.y += fixed_t(frac * double(Vertices[seg->v2].y - newvert.y));
			newvert.index = 0;
			vertnum = VertexMap->SelectVertexClose (newvert);

			if ((int)vertnum == seg->v1 || (int)vertnum == seg->v2)
			{
				Printf("SelectVertexClose selected endpoint of seg %u\n", (unsigned int)set);
			}

			seg2 = SplitSeg (set, vertnum, sidev[0]);

			Segs[seg2].next = outset0;
			outset0 = seg2;
			Segs[set].next = outset1;
			outset1 = set;
			_count0++;
			_count1++;

			// Also split the seg on the back side
			if (Segs[set].partner != DWORD_MAX)
			{
				int partner1 = Segs[set].partner;
				int partner2 = SplitSeg (partner1, vertnum, sidev[1]);
				// The newly created seg stays in the same set as the
				// back seg because it has not been considered for splitting
				// yet. If it had been, then the front seg would have already
				// been split, and we would not be in this default case.
				// Moreover, the back seg may not even be in the set being
				// split, so we must not move its pieces into the out sets.
				Segs[partner1].next = partner2;
				Segs[partner2].partner = seg2;
				Segs[seg2].partner = partner2;

				assert (Segs[partner2].v1 == Segs[seg2].v2);
				assert (Segs[partner2].v2 == Segs[seg2].v1);
				assert (Segs[partner1].v1 == Segs[set].v2);
				assert (Segs[partner1].v2 == Segs[set].v1);
			}

			if (GLNodes)
			{
				AddIntersection (node, vertnum);
			}

			break;
		}
		if (side >= 0 && GLNodes)
		{
			if (sidev[0] == 0)
			{
				double dist1 = AddIntersection (node, seg->v1);
				if (sidev[1] == 0)
				{
					double dist2 = AddIntersection (node, seg->v2);
					FSplitSharer share = { dist1, set, dist2 > dist1 };
					SplitSharers.Push (share);
				}
			}
			else if (sidev[1] == 0)
			{
				AddIntersection (node, seg->v2);
			}
		}
		if (hack && GLNodes)
		{
			DWORD newback, newfront;

			newback = AddMiniseg (seg->v2, seg->v1, DWORD_MAX, set, splitseg);
			if (HackMate == DWORD_MAX)
			{
				newfront = AddMiniseg (Segs[set].v1, Segs[set].v2, newback, set, splitseg);
				Segs[newfront].next = outset0;
				outset0 = newfront;
			}
			else
			{
				newfront = HackMate;
				Segs[newfront].partner = newback;
				Segs[newback].partner = newfront;
			}
			Segs[newback].frontsector = Segs[newback].backsector =
				Segs[newfront].frontsector = Segs[newfront].backsector =
				Segs[set].frontsector;

			Segs[newback].next = outset1;
			outset1 = newback;
		}
		set = next;
	}
	FixSplitSharers ();
	if (GLNodes)
	{
		AddMinisegs (node, splitseg, outset0, outset1);
	}
	count0 = _count0;
	count1 = _count1;
}

void FNodeBuilder::SetNodeFromSeg (node_t &node, const FPrivSeg *pseg) const
{
	if (pseg->planenum >= 0)
	{
		FSimpleLine *pline = &Planes[pseg->planenum];
		node.x = pline->x;
		node.y = pline->y;
		node.dx = pline->dx;
		node.dy = pline->dy;
	}
	else
	{
		node.x = Vertices[pseg->v1].x;
		node.y = Vertices[pseg->v1].y;
		node.dx = Vertices[pseg->v2].x - node.x;
		node.dy = Vertices[pseg->v2].y - node.y;
	}
}

DWORD FNodeBuilder::SplitSeg (DWORD segnum, int splitvert, int v1InFront)
{
	double dx, dy;
	FPrivSeg newseg;
	int newnum = (int)Segs.Size();

	newseg = Segs[segnum];
	dx = double(Vertices[splitvert].x - Vertices[newseg.v1].x);
	dy = double(Vertices[splitvert].y - Vertices[newseg.v1].y);
	if (v1InFront > 0)
	{
		newseg.offset += fixed_t (sqrt (dx*dx + dy*dy));

		newseg.v1 = splitvert;
		Segs[segnum].v2 = splitvert;

		RemoveSegFromVert2 (segnum, newseg.v2);

		newseg.nextforvert = Vertices[splitvert].segs;
		Vertices[splitvert].segs = newnum;

		newseg.nextforvert2 = Vertices[newseg.v2].segs2;
		Vertices[newseg.v2].segs2 = newnum;

		Segs[segnum].nextforvert2 = Vertices[splitvert].segs2;
		Vertices[splitvert].segs2 = segnum;
	}
	else
	{
		Segs[segnum].offset += fixed_t (sqrt (dx*dx + dy*dy));

		Segs[segnum].v1 = splitvert;
		newseg.v2 = splitvert;

		RemoveSegFromVert1 (segnum, newseg.v1);

		newseg.nextforvert = Vertices[newseg.v1].segs;
		Vertices[newseg.v1].segs = newnum;

		newseg.nextforvert2 = Vertices[splitvert].segs2;
		Vertices[splitvert].segs2 = newnum;

		Segs[segnum].nextforvert = Vertices[splitvert].segs;
		Vertices[splitvert].segs = segnum;
	}

	Segs.Push (newseg);

	D(Printf("Split seg %d to get seg %d\n", segnum, newnum));

	return newnum;
}

void FNodeBuilder::RemoveSegFromVert1 (DWORD segnum, int vertnum)
{
	FPrivVert *v = &Vertices[vertnum];

	if (v->segs == segnum)
	{
		v->segs = Segs[segnum].nextforvert;
	}
	else
	{
		DWORD prev, curr;
		prev = 0;
		curr = v->segs;
		while (curr != DWORD_MAX && curr != segnum)
		{
			prev = curr;
			curr = Segs[curr].nextforvert;
		}
		if (curr == segnum)
		{
			Segs[prev].nextforvert = Segs[curr].nextforvert;
		}
	}
}

void FNodeBuilder::RemoveSegFromVert2 (DWORD segnum, int vertnum)
{
	FPrivVert *v = &Vertices[vertnum];

	if (v->segs2 == segnum)
	{
		v->segs2 = Segs[segnum].nextforvert2;
	}
	else
	{
		DWORD prev, curr;
		prev = 0;
		curr = v->segs2;
		while (curr != DWORD_MAX && curr != segnum)
		{
			prev = curr;
			curr = Segs[curr].nextforvert2;
		}
		if (curr == segnum)
		{
			Segs[prev].nextforvert2 = Segs[curr].nextforvert2;
		}
	}
}

double FNodeBuilder::InterceptVector (const node_t &splitter, const FPrivSeg &seg)
{
	double v2x = (double)Vertices[seg.v1].x;
	double v2y = (double)Vertices[seg.v1].y;
	double v2dx = (double)Vertices[seg.v2].x - v2x;
	double v2dy = (double)Vertices[seg.v2].y - v2y;
	double v1dx = (double)splitter.dx;
	double v1dy = (double)splitter.dy;

	double den = v1dy*v2dx - v1dx*v2dy;

	if (den == 0.0)
		return 0;		// parallel

	double v1x = (double)splitter.x;
	double v1y = (double)splitter.y;

	double num = (v1x - v2x)*v1dy + (v2y - v1y)*v1dx;
	return num / den;
}

void FNodeBuilder::PrintSet (int l, DWORD set)
{
	Printf ("set %d:\n", l);
	for (; set != DWORD_MAX; set = Segs[set].next)
	{
		Printf ("\t%5lu(%d)%c%d(%d,%d)-%d(%d,%d)\n", (unsigned long)set,
			Segs[set].frontsector,
			Segs[set].linedef == -1 ? '+' : ':',
			Segs[set].v1,
			Vertices[Segs[set].v1].x>>16, Vertices[Segs[set].v1].y>>16,
			Segs[set].v2,
			Vertices[Segs[set].v2].x>>16, Vertices[Segs[set].v2].y>>16);
	}
	Printf ("*\n");
}

#ifdef BACKPATCH
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <sys/mman.h>
#include <limits.h>
#endif

#ifdef __GNUC__
extern "C" int ClassifyLineBackpatch (node_t &node, const FSimpleVert *v1, const FSimpleVert *v2, int sidev[2])
#else
static int *CallerOffset;
int ClassifyLineBackpatchC (node_t &node, const FSimpleVert *v1, const FSimpleVert *v2, int sidev[2])
#endif
{
	// Select the routine based on SSELevel and patch the caller so that
	// they call that routine directly next time instead of going through here.
	int *calleroffset;
	int diff;
	int (*func)(node_t &, const FSimpleVert *, const FSimpleVert *, int[2]);
	DWORD oldprotect;

#ifdef __GNUC__
	calleroffset = (int *)__builtin_return_address(0);
#else
	calleroffset = CallerOffset;
#endif
//	printf ("Patching for SSE %d @ %p %d\n", SSELevel, calleroffset, *calleroffset);

	if (SSELevel == 2)
	{
		func = ClassifyLineSSE2;
		diff = (char *)ClassifyLineSSE2 - (char *)calleroffset;
	}
	else if (SSELevel == 1)
	{
		func = ClassifyLineSSE1;
		diff = (char *)ClassifyLineSSE1 - (char *)calleroffset;
	}
	else
	{
		func = ClassifyLine2;
		diff = (char *)ClassifyLine2 - (char *)calleroffset;
	}

	// Patch the caller.
	calleroffset--;
#ifdef _WIN32
	if (VirtualProtect (calleroffset, sizeof(void*), PAGE_EXECUTE_READWRITE, &oldprotect))
#else
	// must make this page-aligned for mprotect
	long pagesize = sysconf(_SC_PAGESIZE);
	char *callerpage = (char *)((intptr_t)calleroffset & ~(pagesize - 1));
	size_t protectlen = (intptr_t)calleroffset + sizeof(void*) - (intptr_t)callerpage;
	int ptect;
	if (!(ptect = mprotect(callerpage, protectlen, PROT_READ|PROT_WRITE|PROT_EXEC)))
#endif
	{
		*calleroffset = diff;
#ifdef _WIN32
		VirtualProtect (calleroffset, sizeof(void*), oldprotect, &oldprotect);
#else
		mprotect(callerpage, protectlen, PROT_READ|PROT_EXEC);
#endif
	}

	// And return by calling the real function.
	return func (node, v1, v2, sidev);
}

#ifndef __GNUC__
// The ClassifyLineBackpatch() function here is a stub that uses inline assembly and nakedness
// to retrieve the return address of the stack before sending control to the real
// ClassifyLineBackpatchC() function. Since BACKPATCH shouldn't be defined on 64-bit builds,
// we're okay that VC++ can't do inline assembly on that target.

extern "C" __declspec(noinline) __declspec(naked) int ClassifyLineBackpatch (node_t &node, const FSimpleVert *v1, const FSimpleVert *v2, int sidev[2])
{
	// We store the return address in a global, so as not to need to mess with the parameter list.
	__asm
	{
		mov eax, [esp]
		mov CallerOffset, eax
		jmp ClassifyLineBackpatchC
	}
}
#endif
#endif
