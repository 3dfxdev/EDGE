//----------------------------------------------------------------------------
//  EDGE OpenGL Rendering (Occlusion testing)
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2005  The EDGE Team.
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

// this conditional applies to the whole file
#include "i_defs.h"

#include "con_cvar.h"
#include "dm_defs.h"
#include "dm_state.h"
#include "e_search.h"
#include "m_argv.h"
#include "m_bbox.h"
#include "m_random.h"
#include "p_local.h"
#include "p_mobj.h"
#include "r_defs.h"
#include "r_main.h"
#include "r_sky.h"
#include "r_state.h"
#include "r_things.h"
#include "r2_defs.h"
#include "rgl_defs.h"
#include "v_colour.h"
#include "v_res.h"
#include "w_textur.h"
#include "w_wad.h"
#include "v_ctx.h"
#include "z_zone.h"


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
static unsigned long oned_oculus_buffer[ONED_TOTAL];

// -AJA- these values could be computed (rather than looked-up)
// without too much trouble.  For now I want to get the logic correct.
//
static unsigned long oned_low_masks[32] =
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

static unsigned long oned_high_masks[32] =
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
	DEV_ASSERT2((angle_t)(high - low) < ANG180);

	unsigned int low_b, high_b;

	low  >>= (ANGLEBITS - ONED_POWER);
	high >>= (ANGLEBITS - ONED_POWER);

	low_b  = low  & 0x1F;  low  >>= 5; 
	high_b = high & 0x1F;  high >>= 5; 

#if 0  // TEMP DEBUGGING CODE
unsigned int low_orig = low;
if (rgl_1d_debug)
{
	L_WriteDebug("> low = %d.%d  high = %d.%d\n", low, low_b, high, high_b);
	L_WriteDebug("> Low mask = 0x%08x  High mask = 0x%08x\n",
		LOW_MASK(low_b), HIGH_MASK(high_b));
}
#endif

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

#if 0  // TEMP DEBUGGING CODE
if (rgl_1d_debug)
{
	low = low_orig;

	L_WriteDebug("> ");
	L_WriteDebug("0x%08x", oned_oculus_buffer[low]);

	if (low != high)
	{
		low = (low+1) % ONED_TOTAL;

		for (; low != high; low = (low+1) % ONED_TOTAL)
			L_WriteDebug(" 0x%08x", oned_oculus_buffer[low]);

		L_WriteDebug(" 0x%08x", oned_oculus_buffer[high]);
	}

	L_WriteDebug(" <\n");
}
#endif
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
	DEV_ASSERT2((angle_t)(high - low) < ANG180);

	unsigned int low_b, high_b;

	low  >>= (ANGLEBITS - ONED_POWER);
	high >>= (ANGLEBITS - ONED_POWER);

	low_b  = low  & 0x1F;  low  >>= 5; 
	high_b = high & 0x1F;  high >>= 5; 

#if 0  // TEMP DEBUGGING CODE
unsigned int low_orig = low;
if (rgl_1d_debug)
{
	L_WriteDebug("? low = %d.%d  high = %d.%d\n", low, low_b, high, high_b);
	L_WriteDebug("? Low mask = 0x%08x  High mask = 0x%08x\n",
		LOW_MASK(low_b), HIGH_MASK(high_b));

	L_WriteDebug("? ");
	L_WriteDebug("0x%08x", oned_oculus_buffer[low]);

	if (low != high)
	{
		low = (low+1) % ONED_TOTAL;

		for (; low != high; low = (low+1) % ONED_TOTAL)
			L_WriteDebug(" 0x%08x", oned_oculus_buffer[low]);

		L_WriteDebug(" 0x%08x", oned_oculus_buffer[high]);
	}

	L_WriteDebug(".\n");

	low = low_orig;
}
#endif

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

typedef struct saved_occbuf_s
{
	struct saved_occbuf_s *link;
	unsigned long buffer[ONED_TOTAL];
}
saved_occbuf_t;

static saved_occbuf_t *occ_stack = NULL;

void RGL_1DOcclusionPush(void)
{
	saved_occbuf_t *cur = new saved_occbuf_t;

	memcpy(cur->buffer, oned_oculus_buffer, sizeof(oned_oculus_buffer));

	cur->link = occ_stack;
	occ_stack = cur;
}

void RGL_1DOcclusionPop(void)
{
	DEV_ASSERT2(occ_stack != NULL);

	saved_occbuf_t *cur = occ_stack;
	occ_stack = occ_stack->link;

	memcpy(oned_oculus_buffer, cur->buffer, sizeof(oned_oculus_buffer));
	delete cur;
}

