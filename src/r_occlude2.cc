//----------------------------------------------------------------------------
//  EDGE2 OpenGL Rendering (Occlusion testing)
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2017  The EDGE2 Team.
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

#include "system/i_defs.h"

#include "../ddf/types.h"

#include "r_occlude.h"


// #define DEBUG_OCC  1

/*
*
** gl_clipper.cpp
**
** Handles visibility checks.
** Loosely based on the JDoom clipper.
**
**---------------------------------------------------------------------------
** Copyright 2003 Tim Stump
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
*/

#define ANGLE_MAX ANG_MAX
#define ANGLE_90 ANG90
#define ANGLE_180 ANG180
#define ANGLE_270 ANG270

/* Emacs style mode select   -*- C++ -*-
 *-----------------------------------------------------------------------------
 *
 *
 *  PrBoom: a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2000 by
 *  Jess Haas, Nicolas Kalkhof, Colin Phipps, Florian Schulze
 *  Copyright 2005, 2006 by
 *  Florian Schulze, Colin Phipps, Neil Stevens, Andrey Budko
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 *  02111-1307, USA.
 *
 * DESCRIPTION:
 *
 *---------------------------------------------------------------------
 */

/*
*
** gl_clipper.cpp
**
** Handles visibility checks.
** Loosely based on the JDoom clipper.
**
**---------------------------------------------------------------------------
** Copyright 2003 Tim Stump
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
*/

#include "r_occlude.h"
#include "m_bbox.h"

#include <SDL2/SDL_opengl.h>
#include <math.h>
#include "system/i_video.h"
#include "system/i_defs_gl.h"
#include "r_defs.h"
#include "z_zone.h"

#define dboolean bool
float frustum[6][4];

typedef struct clipnode_s
{
  struct clipnode_s *prev, *next;
  angle_t start, end;
} clipnode_t;

clipnode_t *freelist;
clipnode_t *clipnodes;
clipnode_t *cliphead;

static clipnode_t * gld_clipnode_GetNew(void);
static clipnode_t * gld_clipnode_NewRange(angle_t start, angle_t end);
static dboolean gld_clipper_IsRangeVisible(angle_t startAngle, angle_t endAngle);
static void gld_clipper_AddClipRange(angle_t start, angle_t end);
static void gld_clipper_RemoveRange(clipnode_t * range);
static void gld_clipnode_Free(clipnode_t *node);

static clipnode_t * gld_clipnode_GetNew(void)
{
  if (freelist)
  {
    clipnode_t * p = freelist;
    freelist = p->next;
    return p;
  }
  else
  {
    return (clipnode_t *)malloc (sizeof(clipnode_t));
  }
}

static clipnode_t * gld_clipnode_NewRange(angle_t start, angle_t end)
{
  clipnode_t * c = gld_clipnode_GetNew();
  c->start = start;
  c->end = end;
  c->next = c->prev=NULL;
  return c;
}

dboolean gld_clipper_SafeCheckRange(angle_t startAngle, angle_t endAngle)
{
  if(startAngle > endAngle)
  {
    return (gld_clipper_IsRangeVisible(startAngle, ANGLE_MAX) || gld_clipper_IsRangeVisible(0, endAngle));
  }

  return gld_clipper_IsRangeVisible(startAngle, endAngle);
}

static dboolean gld_clipper_IsRangeVisible(angle_t startAngle, angle_t endAngle)
{
  clipnode_t *ci;
  ci = cliphead;

  if (endAngle == 0 && ci && ci->start == 0)
    return false;

  while (ci != NULL && ci->start < endAngle)
  {
    if (startAngle >= ci->start && endAngle <= ci->end)
    {
      return false;
    }
    ci = ci->next;
  }

  return true;
}

static void gld_clipnode_Free(clipnode_t *node)
{
  node->next = freelist;
  freelist = node;
}

static void gld_clipper_RemoveRange(clipnode_t *range)
{
  if (range == cliphead)
  {
    cliphead = cliphead->next;
  }
  else
  {
    if (range->prev)
    {
      range->prev->next = range->next;
    }
    if (range->next)
    {
      range->next->prev = range->prev;
    }
  }

  gld_clipnode_Free(range);
}

void gld_clipper_SafeAddClipRange(angle_t startangle, angle_t endangle)
{
  if(startangle > endangle)
  {
    // The range has to added in two parts.
    gld_clipper_AddClipRange(startangle, ANGLE_MAX);
    gld_clipper_AddClipRange(0, endangle);
  }
  else
  {
    // Add the range as usual.
    gld_clipper_AddClipRange(startangle, endangle);
  }
}

angle_t gld_clipper_AngleToPseudo(angle_t ang)
{
  double vecx = cos(ang * M_PI / ANG180);
  double vecy = sin(ang * M_PI / ANG180);

  double result = vecy / (fabs(vecx) + fabs(vecy));
  if (vecx < 0)
  {
    result = 2.f - result;
  }
  return (angle_t)(result * (1<<30));
}

void gld_clipper_SafeAddClipRangeRealAngles(angle_t startangle, angle_t endangle)
{
  gld_clipper_SafeAddClipRange(
    gld_clipper_AngleToPseudo(startangle),
    gld_clipper_AngleToPseudo(endangle));
}


static void gld_clipper_AddClipRange(angle_t start, angle_t end)
{
  clipnode_t *node, *temp, *prevNode, *node2, *delnode;

  if (cliphead)
  {
    //check to see if range contains any old ranges
    node = cliphead;
    while (node != NULL && node->start < end)
    {
      if (node->start >= start && node->end <= end)
      {
        temp = node;
        node = node->next;
        gld_clipper_RemoveRange(temp);
      }
      else
      {
        if (node->start <= start && node->end >= end)
        {
          return;
        }
        else
        {
          node = node->next;
        }
      }
    }

    //check to see if range overlaps a range (or possibly 2)
    node = cliphead;
    while (node != NULL && node->start <= end)
    {
      if (node->end >= start)
      {
        // we found the first overlapping node
        if (node->start > start)
        {
          // the new range overlaps with this node's start point
          node->start = start;
        }
        if (node->end < end)
        {
          node->end = end;
        }

        node2 = node->next;
        while (node2 && node2->start <= node->end)
        {
          if (node2->end > node->end)
          {
            node->end = node2->end;
          }

          delnode = node2;
          node2 = node2->next;
          gld_clipper_RemoveRange(delnode);
        }
        return;
      }
      node = node->next;
    }

    //just add range
    node = cliphead;
    prevNode = NULL;
    temp = gld_clipnode_NewRange(start, end);
    while (node != NULL && node->start < end)
    {
      prevNode = node;
      node = node->next;
    }
    temp->next = node;
    if (node == NULL)
    {
      temp->prev = prevNode;
      if (prevNode)
      {
        prevNode->next = temp;
      }
      if (!cliphead)
      {
        cliphead = temp;
      }
    }
    else
    {
      if (node == cliphead)
      {
        cliphead->prev = temp;
        cliphead = temp;
      }
      else
      {
        temp->prev = prevNode;
        prevNode->next = temp;
        node->prev = temp;
      }
    }
  }
  else
  {
    temp = gld_clipnode_NewRange(start, end);
    cliphead = temp;
    return;
  }
}

void gld_clipper_Clear(void)
{
  clipnode_t *node = cliphead;
  clipnode_t *temp;

  while (node != NULL)
  {
    temp = node;
    node = node->next;
    gld_clipnode_Free(temp);
  }

  cliphead = NULL;
}

//The other PrBoom code for Frustrum is set up in 3DGE differently, so this is commented out for now. Will be added back.

#if 0
angle_t gld_FrustumAngle(void)
{
	double floatangle;
	angle_t a1;

	float tilt = (float)fabs(((double)(int)viewpitch) / ANG1);
	if (tilt > 90.0f)
	{
		tilt = 90.0f;
	}

	// If the pitch is larger than this you can look all around at a FOV of 90
	if (D_abs(viewpitch) > 46 * ANG1)
		return 0xffffffff;

	// ok, this is a gross hack that barely works...
	// but at least it doesn't overestimate too much...
	floatangle = 2.0f + (45.0f + (tilt / 1.9f)) * (float)render_fov * ratio_scale / render_multiplier / 90.0f;
	a1 = ANG1 * (int)floatangle;
	if (a1 >= ANG180)
		return 0xffffffff;
	return a1;
}


//
// gld_FrustrumSetup
//

#define CALCMATRIX(a, b, c, d, e, f, g, h)\
  (float)(modelMatrix[a] * projMatrix[b] + \
  modelMatrix[c] * projMatrix[d] + \
  modelMatrix[e] * projMatrix[f] + \
  modelMatrix[g] * projMatrix[h])

#define NORMALIZE_PLANE(i)\
  t = (float)sqrt(\
    frustum[i][0] * frustum[i][0] + \
    frustum[i][1] * frustum[i][1] + \
    frustum[i][2] * frustum[i][2]); \
  frustum[i][0] /= t; \
  frustum[i][1] /= t; \
  frustum[i][2] /= t; \
  frustum[i][3] /= t

void gld_FrustrumSetup(void)
{
	float t;
	float clip[16];

	clip[0] = CALCMATRIX(0, 0, 1, 4, 2, 8, 3, 12);
	clip[1] = CALCMATRIX(0, 1, 1, 5, 2, 9, 3, 13);
	clip[2] = CALCMATRIX(0, 2, 1, 6, 2, 10, 3, 14);
	clip[3] = CALCMATRIX(0, 3, 1, 7, 2, 11, 3, 15);

	clip[4] = CALCMATRIX(4, 0, 5, 4, 6, 8, 7, 12);
	clip[5] = CALCMATRIX(4, 1, 5, 5, 6, 9, 7, 13);
	clip[6] = CALCMATRIX(4, 2, 5, 6, 6, 10, 7, 14);
	clip[7] = CALCMATRIX(4, 3, 5, 7, 6, 11, 7, 15);

	clip[8] = CALCMATRIX(8, 0, 9, 4, 10, 8, 11, 12);
	clip[9] = CALCMATRIX(8, 1, 9, 5, 10, 9, 11, 13);
	clip[10] = CALCMATRIX(8, 2, 9, 6, 10, 10, 11, 14);
	clip[11] = CALCMATRIX(8, 3, 9, 7, 10, 11, 11, 15);

	clip[12] = CALCMATRIX(12, 0, 13, 4, 14, 8, 15, 12);
	clip[13] = CALCMATRIX(12, 1, 13, 5, 14, 9, 15, 13);
	clip[14] = CALCMATRIX(12, 2, 13, 6, 14, 10, 15, 14);
	clip[15] = CALCMATRIX(12, 3, 13, 7, 14, 11, 15, 15);

	// Right plane
	frustum[0][0] = clip[3] - clip[0];
	frustum[0][1] = clip[7] - clip[4];
	frustum[0][2] = clip[11] - clip[8];
	frustum[0][3] = clip[15] - clip[12];
	NORMALIZE_PLANE(0);

	// Left plane
	frustum[1][0] = clip[3] + clip[0];
	frustum[1][1] = clip[7] + clip[4];
	frustum[1][2] = clip[11] + clip[8];
	frustum[1][3] = clip[15] + clip[12];
	NORMALIZE_PLANE(1);

	// Bottom plane
	frustum[2][0] = clip[3] + clip[1];
	frustum[2][1] = clip[7] + clip[5];
	frustum[2][2] = clip[11] + clip[9];
	frustum[2][3] = clip[15] + clip[13];
	NORMALIZE_PLANE(2);

	// Top plane
	frustum[3][0] = clip[3] - clip[1];
	frustum[3][1] = clip[7] - clip[5];
	frustum[3][2] = clip[11] - clip[9];
	frustum[3][3] = clip[15] - clip[13];
	NORMALIZE_PLANE(3);

	// Far plane
	frustum[4][0] = clip[3] - clip[2];
	frustum[4][1] = clip[7] - clip[6];
	frustum[4][2] = clip[11] - clip[10];
	frustum[4][3] = clip[15] - clip[14];
	NORMALIZE_PLANE(4);

	// Near plane
	frustum[5][0] = clip[3] + clip[2];
	frustum[5][1] = clip[7] + clip[6];
	frustum[5][2] = clip[11] + clip[10];
	frustum[5][3] = clip[15] + clip[14];
	NORMALIZE_PLANE(5);
}

dboolean gld_SphereInFrustum(float x, float y, float z, float radius)
{
	int p;

	for (p = 0; p < 4; p++)
	{
		if (frustum[p][0] * x +
			frustum[p][1] * y +
			frustum[p][2] * z +
			frustum[p][3] <= -radius)
		{
			return false;
		}
	}
	return true;
}
#endif // 0


#if 0


ClipNode * ClipNode::freelist;
int Clipper::anglecache;


//-----------------------------------------------------------------------------
//
// Destructor
//
//-----------------------------------------------------------------------------

Clipper::~Clipper()
{
	Clear();
	while (ClipNode::freelist != NULL)
	{
		ClipNode * node = ClipNode::freelist;
		ClipNode::freelist = node->next;
		delete node;
	}
}

//-----------------------------------------------------------------------------
//
// RemoveRange
//
//-----------------------------------------------------------------------------

void Clipper::RemoveRange(ClipNode * range)
{
	if (range == cliphead)
	{
		cliphead = cliphead->next;
	}
	else
	{
		if (range->prev) range->prev->next = range->next;
		if (range->next) range->next->prev = range->prev;
	}

	range->Free();
}

//-----------------------------------------------------------------------------
//
// Clear
//
//-----------------------------------------------------------------------------

void Clipper::Clear()
{
	ClipNode *node = cliphead;
	ClipNode *temp;

	while (node != NULL)
	{
		temp = node;
		node = node->next;
		temp->Free();
	}
	node = silhouette;

	while (node != NULL)
	{
		temp = node;
		node = node->next;
		temp->Free();
	}

	cliphead = NULL;
	silhouette = NULL;
	anglecache++;
}

//-----------------------------------------------------------------------------
//
// SetSilhouette
//
//-----------------------------------------------------------------------------

void Clipper::SetSilhouette()
{
	ClipNode *node = cliphead;
	ClipNode *last = NULL;

	while (node != NULL)
	{
		ClipNode *snode = ClipNode::NewRange(node->start, node->end);
		if (silhouette == NULL) silhouette = snode;
		snode->prev = last;
		if (last != NULL) last->next = snode;
		last = snode;
		node = node->next;
	}
}


//-----------------------------------------------------------------------------
//
// IsRangeVisible
//
//-----------------------------------------------------------------------------

bool Clipper::IsRangeVisible(angle_t startAngle, angle_t endAngle)
{
	ClipNode *ci;
	ci = cliphead;

	if (endAngle == 0 && ci && ci->start == 0) return false;

	while (ci != NULL && ci->start < endAngle)
	{
		if (startAngle >= ci->start && endAngle <= ci->end)
		{
			return false;
		}
		ci = ci->next;
	}

	return true;
}

//-----------------------------------------------------------------------------
//
// AddClipRange
//
//-----------------------------------------------------------------------------

void Clipper::AddClipRange(angle_t start, angle_t end)
{
	ClipNode *node, *temp, *prevNode;

	if (cliphead)
	{
		//check to see if range contains any old ranges
		node = cliphead;
		while (node != NULL && node->start < end)
		{
			if (node->start >= start && node->end <= end)
			{
				temp = node;
				node = node->next;
				RemoveRange(temp);
			}
			else if (node->start <= start && node->end >= end)
			{
				return;
			}
			else
			{
				node = node->next;
			}
		}

		//check to see if range overlaps a range (or possibly 2)
		node = cliphead;
		while (node != NULL && node->start <= end)
		{
			if (node->end >= start)
			{
				// we found the first overlapping node
				if (node->start > start)
				{
					// the new range overlaps with this node's start point
					node->start = start;
				}

				if (node->end < end)
				{
					node->end = end;
				}

				ClipNode *node2 = node->next;
				while (node2 && node2->start <= node->end)
				{
					if (node2->end > node->end) node->end = node2->end;
					ClipNode *delnode = node2;
					node2 = node2->next;
					RemoveRange(delnode);
				}
				return;
			}
			node = node->next;
		}

		//just add range
		node = cliphead;
		prevNode = NULL;
		temp = ClipNode::NewRange(start, end);

		while (node != NULL && node->start < end)
		{
			prevNode = node;
			node = node->next;
		}

		temp->next = node;
		if (node == NULL)
		{
			temp->prev = prevNode;
			if (prevNode) prevNode->next = temp;
			if (!cliphead) cliphead = temp;
		}
		else
		{
			if (node == cliphead)
			{
				cliphead->prev = temp;
				cliphead = temp;
			}
			else
			{
				temp->prev = prevNode;
				prevNode->next = temp;
				node->prev = temp;
			}
		}
	}
	else
	{
		temp = ClipNode::NewRange(start, end);
		cliphead = temp;
		return;
	}
}


//-----------------------------------------------------------------------------
//
// RemoveClipRange
//
//-----------------------------------------------------------------------------

void Clipper::RemoveClipRange(angle_t start, angle_t end)
{
	ClipNode *node;

	if (silhouette)
	{
		node = silhouette;
		while (node != NULL && node->end <= start)
		{
			node = node->next;
		}
		if (node != NULL && node->start <= start)
		{
			if (node->end >= end) return;
			start = node->end;
			node = node->next;
		}
		while (node != NULL && node->start < end)
		{
			DoRemoveClipRange(start, node->start);
			start = node->end;
			node = node->next;
		}
		if (start >= end) return;
	}
	DoRemoveClipRange(start, end);
}

//-----------------------------------------------------------------------------
//
// RemoveClipRange worker function
//
//-----------------------------------------------------------------------------

void Clipper::DoRemoveClipRange(angle_t start, angle_t end)
{
	ClipNode *node, *temp;

	if (cliphead)
	{
		//check to see if range contains any old ranges
		node = cliphead;
		while (node != NULL && node->start < end)
		{
			if (node->start >= start && node->end <= end)
			{
				temp = node;
				node = node->next;
				RemoveRange(temp);
			}
			else
			{
				node = node->next;
			}
		}

		//check to see if range overlaps a range (or possibly 2)
		node = cliphead;
		while (node != NULL)
		{
			if (node->start >= start && node->start <= end)
			{
				node->start = end;
				break;
			}
			else if (node->end >= start && node->end <= end)
			{
				node->end = start;
			}
			else if (node->start < start && node->end > end)
			{
				temp = ClipNode::NewRange(end, node->end);
				node->end = start;
				temp->next = node->next;
				temp->prev = node;
				node->next = temp;
				if (temp->next) temp->next->prev = temp;
				break;
			}
			node = node->next;
		}
	}
}


//-----------------------------------------------------------------------------
//
// 
//
//-----------------------------------------------------------------------------

angle_t Clipper::AngleToPseudo(angle_t ang)
{
	float vecx = M_Cos(ang * M_PI / ANGLE_180);
	float vecy = M_Sin(ang * M_PI / ANGLE_180);

	float result = vecy / (fabs(vecx) + fabs(vecy));
	if (vecx < 0)
	{
		result = 2.f - result;
	}
	//return xs_Fix<30>::ToFix(result);
	return (angle_t)(result * (1 << 30));
}

//-----------------------------------------------------------------------------
//
// ! Returns the pseudoangle between the line p1 to (infinity, p1.y) and the 
// line from p1 to p2. The pseudoangle has the property that the ordering of 
// points by true angle around p1 and ordering of points by pseudoangle are the 
// same.
//
// For clipping exact angles are not needed. Only the ordering matters.
// This is about as fast as the fixed point R_PointToAngle2 but without
// the precision issues associated with that function.
//
//-----------------------------------------------------------------------------

float viewx, viewy;


void R_SetView()
{
	viewx = float(viewx);
	viewy = float(viewy);
}



angle_t R_PointToPseudoAngle(float x, float y)
{
	float vecx = x - viewx;
	float vecy = y - viewy;

	if (vecx == 0 && vecy == 0)
	{
		return 0;
	}
	else
	{
		float result = vecy / (fabs(vecx) + fabs(vecy));
		if (vecx < 0)
		{
			result = 2.f - result;
		}
		return (angle_t)(result * (1 << 30));
		//return xs_Fix<30>::ToFix(result);
	}
}



//-----------------------------------------------------------------------------
//
// R_CheckBBox
// Checks BSP node/subtree bounding box.
// Returns true
//  if some part of the bbox might be visible.
//
//-----------------------------------------------------------------------------
static const BYTE checkcoord[12][4] = // killough -- static const
{
  {3,0,2,1},
  {3,0,2,0},
  {3,1,2,0},
  {0},
  {2,0,2,1},
  {0,0,0,0},
  {3,1,3,0},
  {0},
  {2,0,3,1},
  {2,1,3,1},
  {2,1,3,0}
};

bool Clipper::CheckBox(const float *bspcoord)
{
	angle_t angle1, angle2;

	int        boxpos;
	const BYTE* check;

	// Find the corners of the box
	// that define the edges from current viewpoint.
	boxpos = (viewx <= bspcoord[BOXLEFT] ? 0 : viewx < bspcoord[BOXRIGHT] ? 1 : 2) +
		(viewy >= bspcoord[BOXTOP] ? 0 : viewy > bspcoord[BOXBOTTOM] ? 4 : 8);

	if (boxpos == 5) return true;

	check = checkcoord[boxpos];
	angle1 = R_PointToPseudoAngle(bspcoord[check[0]], bspcoord[check[1]]);
	angle2 = R_PointToPseudoAngle(bspcoord[check[2]], bspcoord[check[3]]);

	return SafeCheckRange(angle2, angle1);
}
#endif // 0





//older code
#if 0
typedef struct angle_range_s
{
	angle_t low, high;

	struct angle_range_s *next;
	struct angle_range_s *prev;
}
angle_range_t;


static angle_range_t *occbuf_head = NULL;
static angle_range_t *occbuf_tail = NULL;

static angle_range_t *free_range_chickens = NULL;


#ifdef DEBUG_OCC
static void ValidateBuffer(void)
{
	if (!occbuf_head)
	{
		SYS_ASSERT(!occbuf_tail);
		return;
	}

	for (angle_range_t *AR = occbuf_head; AR; AR = AR->next)
	{
		SYS_ASSERT(AR->low <= AR->high);

		if (AR->next)
		{
			SYS_ASSERT(AR->next->prev == AR);
			SYS_ASSERT(AR->next->low > AR->high);
		}
		else
		{
			SYS_ASSERT(AR == occbuf_tail);
		}

		if (AR->prev)
		{
			SYS_ASSERT(AR->prev->next == AR);
		}
		else
		{
			SYS_ASSERT(AR == occbuf_head);
		}
	}
}
#endif // DEBUG_OCC


void RGL_1DOcclusionClear(void)
{
	// Clear all angles in the whole buffer
	// )i.e. mark them as open / non-blocking).

	if (occbuf_head)
	{
		occbuf_tail->next = free_range_chickens;

		free_range_chickens = occbuf_head;

		occbuf_head = NULL;
		occbuf_tail = NULL;
	}

#ifdef DEBUG_OCC
	ValidateBuffer();
#endif
}

static inline angle_range_t *GetNewRange(angle_t low, angle_t high)
{
	angle_range_t *R;

	if (free_range_chickens)
	{
		R = free_range_chickens;

		free_range_chickens = R->next;
	}
	else
		R = new angle_range_t;

	R->low = low;
	R->high = high;

	return R;
}

static inline void LinkBefore(angle_range_t *X, angle_range_t *N)
{
	// X = eXisting range
	// N = New range

	N->next = X;
	N->prev = X->prev;

	X->prev = N;

	if (N->prev)
		N->prev->next = N;
	else
		occbuf_head = N;
}

static inline void LinkInTail(angle_range_t *N)
{
	N->next = NULL;
	N->prev = occbuf_tail;

	if (occbuf_tail)
		occbuf_tail->next = N;
	else
		occbuf_head = N;

	occbuf_tail = N;
}

static inline void RemoveRange(angle_range_t *R)
{
	if (R->next)
		R->next->prev = R->prev;
	else
		occbuf_tail = R->prev;

	if (R->prev)
		R->prev->next = R->next;
	else
		occbuf_head = R->next;

	// add it to the quick-alloc list
	R->next = free_range_chickens;
	R->prev = NULL;

	free_range_chickens = R;
}

static void DoSet(angle_t low, angle_t high)
{
	for (angle_range_t *AR = occbuf_head; AR; AR = AR->next)
	{
		if (high < AR->low)
		{
			LinkBefore(AR, GetNewRange(low, high));
			return;
		}

		if (low > AR->high)
			continue;

		// the new range overlaps the old range.
		//
		// The above test (i.e. low > AR->high) guarantees that if
		// we reduce AR->low, it cannot touch the previous range.
		//
		// However by increasing AR->high we may touch or overlap
		// some subsequent ranges in the list.  When that happens,
		// we must remove them (and adjust the current range).

		AR->low = MIN(AR->low, low);
		AR->high = MAX(AR->high, high);

#ifdef DEBUG_OCC
		if (AR->prev)
		{
			SYS_ASSERT(AR->low > AR->prev->high);
	}
#endif
		while (AR->next && AR->high >= AR->next->low)
		{
			AR->high = MAX(AR->high, AR->next->high);

			RemoveRange(AR->next);
		}

		return;
}

	// the new range is greater than all existing ranges

	LinkInTail(GetNewRange(low, high));
}

void RGL_1DOcclusionSet(angle_t low, angle_t high)
{
	// Set all angles in the given range, i.e. mark them as blocking.
	// The angles are relative to the VIEW angle.

	SYS_ASSERT((angle_t)(high - low) < ANG180);

	if (low <= high)
		DoSet(low, high);
	else
	{
		DoSet(low, ANG_MAX); DoSet(0, high);
	}

#ifdef DEBUG_OCC
	ValidateBuffer();
#endif
}

static inline bool DoTest(angle_t low, angle_t high)
{
	for (angle_range_t *AR = occbuf_head; AR; AR = AR->next)
	{
		if (AR->low <= low && high <= AR->high)
			return true;

		if (AR->high > low)
			break;
	}

	return false;
}

bool RGL_1DOcclusionTest(angle_t low, angle_t high)
{
	// Check whether all angles in the given range are set (i.e. blocked).
	// Returns true if the entire range is blocked, false otherwise.
	// Angles are relative to the VIEW angle.

	SYS_ASSERT((angle_t)(high - low) < ANG180);

	if (low <= high)
		return DoTest(low, high);
	else
		return DoTest(low, ANG_MAX) && DoTest(0, high);
}
#endif // 0



#if 0 // OLD CODE

//----------------------------------------------------------------------------
//
//  SPECIAL 1D OCCLUSION BUFFER
//

#define ONED_POWER  12  // 4096 angles
#define ONED_SIZE   (1 << ONED_POWER)
#define ONED_TOTAL  (ONED_SIZE / 32)

// 1 bit per angle, packed into 32 bit values.
// (NOTE: for speed reasons, 1 is "clear", and 0 is "blocked")
//
static u32_t oned_oculus_buffer[ONED_TOTAL];

// -AJA- these values could be computed (rather than looked-up)
// without too much trouble.  For now I want to get the logic correct.
//
static u32_t oned_low_masks[32] =
{
	0xFFFFFFFF, 0x7FFFFFFF, 0x3FFFFFFF, 0x1FFFFFFF,
	0x0FFFFFFF, 0x07FFFFFF, 0x03FFFFFF, 0x01FFFFFF,
	0x00FFFFFF, 0x007FFFFF, 0x003FFFFF, 0x001FFFFF,
	0x000FFFFF, 0x0007FFFF, 0x0003FFFF, 0x0001FFFF,
	0x0000FFFF, 0x00007FFF, 0x00003FFF, 0x00001FFF,
	0x00000FFF, 0x000007FF, 0x000003FF, 0x000001FF,
	0x000000FF, 0x0000007F, 0x0000003F, 0x0000001F,
	0x0000000F, 0x00000007, 0x00000003, 0x00000001
};

static u32_t oned_high_masks[32] =
{
	0x80000000, 0xC0000000, 0xE0000000, 0xF0000000,
	0xF8000000, 0xFC000000, 0xFE000000, 0xFF000000,
	0xFF800000, 0xFFC00000, 0xFFE00000, 0xFFF00000,
	0xFFF80000, 0xFFFC0000, 0xFFFE0000, 0xFFFF0000,
	0xFFFF8000, 0xFFFFC000, 0xFFFFE000, 0xFFFFF000,
	0xFFFFF800, 0xFFFFFC00, 0xFFFFFE00, 0xFFFFFF00,
	0xFFFFFF80, 0xFFFFFFC0, 0xFFFFFFE0, 0xFFFFFFF0,
	0xFFFFFFF8, 0xFFFFFFFC, 0xFFFFFFFE, 0xFFFFFFFF
};

#define LOW_MASK(L)   oned_low_masks[L]
#define HIGH_MASK(H)  oned_high_masks[H]


//
// RGL_1DOcclusionClear
//
// Clear all angles in the given range.  (Clear means open, i.e. not
// blocked).  The angles are relative to the VIEW angle.
//
void RGL_1DOcclusionClear(void)
{
	int i;

	for (i = 0; i < ONED_TOTAL; i++)
		oned_oculus_buffer[i] = 0xFFFFFFFF;
}

//
// RGL_1DOcclusionSet
//
// Set all angles in the given range, i.e. mark them as blocking.  The
// angles are relative to the VIEW angle.
//
void RGL_1DOcclusionSet(angle_t low, angle_t high)
{
	SYS_ASSERT((angle_t)(high - low) < ANG180);

	unsigned int low_b, high_b;

	low  >>= (ANGLEBITS - ONED_POWER);
	high >>= (ANGLEBITS - ONED_POWER);

	low_b  = low  & 0x1F;  low  >>= 5; 
	high_b = high & 0x1F;  high >>= 5; 

	if (low == high)
	{
		oned_oculus_buffer[low] &= ~(LOW_MASK(low_b) & HIGH_MASK(high_b));
	}
	else
	{
		oned_oculus_buffer[low]  &= ~LOW_MASK(low_b);
		oned_oculus_buffer[high] &= ~HIGH_MASK(high_b);

		low = (low+1) % ONED_TOTAL;

		for (; low != high; low = (low+1) % ONED_TOTAL)
			oned_oculus_buffer[low] = 0x00000000;
	}
}

//
// RGL_1DOcclusionTest
//
// Check whether all angles in the given range are set (i.e. blocked).
// Returns true if the entire range is blocked, false otherwise.
// Angles are relative to the VIEW angle.
//
bool RGL_1DOcclusionTest(angle_t low, angle_t high)
{
	SYS_ASSERT((angle_t)(high - low) < ANG180);

	unsigned int low_b, high_b;

	low  >>= (ANGLEBITS - ONED_POWER);
	high >>= (ANGLEBITS - ONED_POWER);

	low_b  = low  & 0x1F;  low  >>= 5; 
	high_b = high & 0x1F;  high >>= 5; 

	if (low == high)
		return ! (oned_oculus_buffer[low] & (LOW_MASK(low_b) & HIGH_MASK(high_b)));

	if (oned_oculus_buffer[low] & LOW_MASK(low_b))
		return false;

	if (oned_oculus_buffer[high] & HIGH_MASK(high_b))
		return false;

	low = (low+1) % ONED_TOTAL;

	for (; low != high; low = (low+1) % ONED_TOTAL)
		if (oned_oculus_buffer[low])
			return false;

	return true;
}

#endif  // OLD CODE

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
