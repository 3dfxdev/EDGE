//----------------------------------------------------------------------------
//  EDGE2 OpenGL Rendering (Occlusion testing)
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2009  The EDGE2 Team.
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

#ifndef __RGL_OCCLUDE_H__
#define __RGL_OCCLUDE_H__

#include "../epi/types.h"
#include "dm_structs.h"
#include "r_misc.h"
#include "r_defs.h"

#define ANGLE_MAX ANG_MAX
#define ANGLE_90 ANG90
#define ANGLE_180 ANG180
#define ANGLE_270 ANG270

//clipper
bool gld_clipper_SafeCheckRange(angle_t startAngle, angle_t endAngle);
void gld_clipper_SafeAddClipRange(angle_t startangle, angle_t endangle);
void gld_clipper_SafeAddClipRangeRealAngles(angle_t startangle, angle_t endangle);
void gld_clipper_Clear(void);
angle_t gld_FrustumAngle(void);
void gld_FrustrumSetup(void);
bool gld_SphereInFrustum(float x, float y, float z, float radius);

#if 0
class ClipNode
{
	friend class Clipper;
	friend class ClipNodesFreer;

	ClipNode *prev, *next;
	angle_t start, end;
	static ClipNode * freelist;

	bool operator== (const ClipNode &other)
	{
		return other.start == start && other.end == end;
	}

	void Free()
	{
		next = freelist;
		freelist = this;
	}

	static ClipNode * GetNew()
	{
		if (freelist)
		{
			ClipNode * p = freelist;
			freelist = p->next;
			return p;
		}
		else return new ClipNode;
	}

	static ClipNode * NewRange(angle_t start, angle_t end)
	{
		ClipNode * c = GetNew();

		c->start = start;
		c->end = end;
		c->next = c->prev = NULL;
		return c;
	}

};


class Clipper
{
	ClipNode * clipnodes;
	ClipNode * cliphead;
	ClipNode * silhouette;	// will be preserved even when RemoveClipRange is called

	static angle_t AngleToPseudo(angle_t ang);
	bool IsRangeVisible(angle_t startangle, angle_t endangle);
	void RemoveRange(ClipNode * cn);
	void AddClipRange(angle_t startangle, angle_t endangle);
	void RemoveClipRange(angle_t startangle, angle_t endangle);
	void DoRemoveClipRange(angle_t start, angle_t end);

public:

	static int anglecache;

	Clipper()
	{
		clipnodes = cliphead = NULL;
	}

	~Clipper();

	void Clear();


	void SetSilhouette();

	bool SafeCheckRange(angle_t startAngle, angle_t endAngle)
	{
		if (startAngle > endAngle)
		{
			return (IsRangeVisible(startAngle, ANGLE_MAX) || IsRangeVisible(0, endAngle));
		}

		return IsRangeVisible(startAngle, endAngle);
	}

	void SafeAddClipRange(angle_t startangle, angle_t endangle)
	{
		if (startangle > endangle)
		{
			// The range has to added in two parts.
			AddClipRange(startangle, ANGLE_MAX);
			AddClipRange(0, endangle);
		}
		else
		{
			// Add the range as usual.
			AddClipRange(startangle, endangle);
		}
	}

	void SafeAddClipRangeRealAngles(angle_t startangle, angle_t endangle)
	{
		SafeAddClipRange(AngleToPseudo(startangle), AngleToPseudo(endangle));
	}


	void SafeRemoveClipRange(angle_t startangle, angle_t endangle)
	{
		if (startangle > endangle)
		{
			// The range has to added in two parts.
			RemoveClipRange(startangle, ANGLE_MAX);
			RemoveClipRange(0, endangle);
		}
		else
		{
			// Add the range as usual.
			RemoveClipRange(startangle, endangle);
		}
	}

	void SafeRemoveClipRangeRealAngles(angle_t startangle, angle_t endangle)
	{
		SafeRemoveClipRange(AngleToPseudo(startangle), AngleToPseudo(endangle));
	}

	bool CheckBox(const float *bspcoord);
};


extern Clipper clipper;

angle_t R_PointToPseudoAngle(float x, float y);
void R_SetView();

// Used to speed up angle calculations during clipping
inline angle_t vec2_s::GetClipAngle()
{
	return angletime == Clipper::anglecache ? viewangle : (angletime = Clipper::anglecache, viewangle = R_PointToPseudoAngle(x, y));
}
#endif // 0


#endif

#if 0
void RGL_1DOcclusionClear(void);
void RGL_1DOcclusionSet(angle_t low, angle_t high);
bool RGL_1DOcclusionTest(angle_t low, angle_t high);
#endif
#endif /* __RGL_OCCLUDE_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
