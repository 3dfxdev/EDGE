//----------------------------------------------------------------------------
//  EDGE OpenGL Rendering : Polygonizer
//----------------------------------------------------------------------------
//
//  Copyright (c) 1999-2009  Andrew Apted
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

#include "r_defs.h"
#include "r_state.h"
#include "p_local.h"


static int max_segs;
static int max_subsecs;

static void Poly_Setup(void)
{
	max_segs    = numsides * 4;
	max_subsecs = numlines * 2;
}

// Nasty allocation crud....  Fixme

static inline seg_t *NewSeg(void)
{
	if (numsegs >= max_segs)
		I_Error("R_PolygonizeMap: too many segs !\n");

	seg_t *seg = segs + numsegs;

	memset(seg, 0, sizeof(seg_t));

	numsegs++;

	return seg;
}

static inline subsector_t *NewSubsector(void)
{
	if (numsubsectors >= max_segs)
		I_Error("WF_BuildBSP: too many subsectors !\n");

	subsector_t *sub = subsectors + numsubsectors;

	memset(sub, 0, sizeof(subsector_t));

	numsubsectors++;

	return sub;
}


void R_PolygonizeMap(void)
{
	Poly_Setup();
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
