//----------------------------------------------------------------------------
//  EDGE Sky Handling Code
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2004  The EDGE Team.
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
//
//  Based on the DOOM source code, released by Id Software under the
//  following copyright:
//
//    Copyright (C) 1993-1996 by id Software, Inc.
//
//----------------------------------------------------------------------------

#include "i_defs.h"
#include "r_sky.h"

#include "dm_state.h"
#include "m_fixed.h"
#include "r_data.h"
#include "r_main.h"
#include "z_zone.h"

//
// sky mapping
//
float skytexturemid;

const struct image_s *sky_image;

float skytexturescale;

typedef struct sec_sky_ring_s
{
	// which group of connected skies (0 if none)
	int group;

	// link of sector in RING
	struct sec_sky_ring_s *next;
	struct sec_sky_ring_s *prev;

	// maximal sky height of group
	float max_h;
}
sec_sky_ring_t;

//
// R_InitSkyMap
//
// Called every frame to calculate the sky texture's scale.
//
// When stretchsky is off, the sky looks just like original DOOM's sky.
// It will tile if you look up or down, or increase the FOV to more than 90.
// When stretchsky is on, the sky is stretched to 9/5 of the original size.
// 9/5 is 360/200, where 200 is original screen height and 360 is the number
// of texels between the lowest and the highest texel you can see
// when looking maximally up or down in fullscreen and FOV 90.
// In FOVs bigger than 90, the sky will tile.
//
void R_InitSkyMap(void)
{
	if (level_flags.stretchsky)
	{
		skytexturemid = 100 - 160 * topslope * 5 / 9;
		skytexturescale = 160 * (topslope - bottomslope) * 5 / (viewheight * 9);
	}
	else
	{
		skytexturemid = 100 - 160 * topslope;
		skytexturescale = 160 * (topslope - bottomslope) / viewheight;
	}
}

//
// R_ComputeSkyHeights
// 
// This routine computes the sky height field in sector_t, which is
// the maximal sky height over all sky sectors (ceiling only) which
// are joined by 2S linedefs.
//
// Algorithm: Initially all sky sectors are in individual groups.  Now
// we scan the linedef list.  For each 2-sectored line with sky on
// both sides, merge the two groups into one.  Simple :).  We can
// compute the maximal height of the group as we go.
// 
void R_ComputeSkyHeights(void)
{
	int i;
	line_t *ld;
	sector_t *sec;
	sec_sky_ring_t *rings;

	// --- initialise ---

	rings = Z_ClearNew(sec_sky_ring_t, numsectors);

	for (i=0, sec=sectors; i < numsectors; i++, sec++)
	{
		if (! IS_SKY(sec->ceil))
			continue;

		rings[i].group = (i + 1);
		rings[i].next = rings[i].prev = rings + i;
		rings[i].max_h = sec->c_h;
	}

	// --- make the pass over linedefs ---

	for (i=0, ld=lines; i < numlines; i++, ld++)
	{
		sector_t *sec1, *sec2;
		sec_sky_ring_t *ring1, *ring2, *tmp_R;

		if (! ld->side[0] || ! ld->side[1])
			continue;

		sec1 = ld->frontsector;
		sec2 = ld->backsector;

		DEV_ASSERT2(sec1 && sec2);

		if (sec1 == sec2)
			continue;

		ring1 = rings + (sec1 - sectors);
		ring2 = rings + (sec2 - sectors);

		// we require sky on both sides
		if (ring1->group == 0 || ring2->group == 0)
			continue;

		// already in the same group ?
		if (ring1->group == ring2->group)
			continue;

		// swap sectors to ensure the lower group is added to the higher
		// group, since we don't need to update the `max_h' fields of the
		// highest group.

		if (ring1->max_h < ring2->max_h)
		{
			tmp_R = ring1;  ring1 = ring2;  ring2 = tmp_R;
		}

		// update the group numbers in the second group

		ring2->group = ring1->group;
		ring2->max_h = ring1->max_h;

		for (tmp_R=ring2->next; tmp_R != ring2; tmp_R=tmp_R->next)
		{
			tmp_R->group = ring1->group;
			tmp_R->max_h = ring1->max_h;
		}

		// merge 'em baby...

		ring1->next->prev = ring2;
		ring2->next->prev = ring1;

		tmp_R = ring1->next; 
		ring1->next = ring2->next;
		ring2->next = tmp_R;
	}

	// --- now store the results, and free up ---

	for (i=0, sec=sectors; i < numsectors; i++, sec++)
	{
		if (rings[i].group > 0)
			sec->sky_h = rings[i].max_h;

#if 0   // DEBUG CODE
		L_WriteDebug("SKY: sec %d  group %d  max_h %1.1f\n", i,
				rings[i].group, rings[i].max_h);
#endif
	}

	Z_Free(rings);
}
