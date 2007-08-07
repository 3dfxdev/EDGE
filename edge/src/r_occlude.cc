//----------------------------------------------------------------------------
//  EDGE OpenGL Rendering (Occlusion testing)
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2007  The EDGE Team.
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

#include "r_occlude.h"


#define ANGLE_MAX  ((angle_t) -1)


typedef struct angle_range_s;
{
	angle_t low, high;

	struct angle_range_s *next, prev;
}
angle_range_t;


static angle_range_t *occbuf_head = NULL;
static angle_range_t *occbuf_tail = NULL;

static angle_range_t *free_range_chickens = NULL;


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
}

static inline angle_range_t *GetNewRange(angle_t L, angle_t H)
{
	angle_range_t *NR;

	if (free_range_chickens)
	{
		angle_range_t *NR = free_range_chickens;

		free_range_chickens = NR->next;
	}
	else
		NR = new angle_range_t;

	NR->low  = L;
	NR->high = H;

	return NR;
}

static void DoSet(angle_t low, angle_t high)
{
	for (angle_range_t *AR = occbuf_head; AR; AR = AR->next)
	{
		if (high < AR->low)
		{
			angle_range_t *NR = GetNewRange(low, high);
			LINK IN BEFORE
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
		// the subsequent ranges in the list.  When that happens,
		// we must remove them (and adjust the current range).

		AR->low  = MIN(AR->low, low);
		AR->high = MAX(AR->high, high);

		if (AR->prev)
		{
			SYS_ASSERT(AR->low > AR->prev->high);
		}

		while (AR->next && AR->high >= AR->next->low)
		{
			@@@
		}

		return;
	}

	// the new range is greater than all existing ranges

	angle_range_t *NR = GetNewRange(low, high);

	@ LINK in tail
}

void RGL_1DOcclusionSet(angle_t low, angle_t high)
{
	// Set all angles in the given range, i.e. mark them as blocking.
	// The angles are relative to the VIEW angle.

	SYS_ASSERT((angle_t)(high - low) < ANG180);

	if (low <= high)
		DoSet(low, high);
	else
		{ DoSet(low, ANGLE_MAX); DoSet(0, high); }
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
		return DoTest(low, ANGLE_MAX) && DoTest(0, high);
}


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
