/* 
    TiMidity -- Experimental MIDI to WAVE converter
    Copyright (C) 1995 Tuukka Toivonen <toivonen@clinet.fi>

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

    output.c
    
    Audio output (to file / device) functions.
*/

#include "config.h"

#include "output.h"
#include "tables.h"


/*****************************************************************/
/* Some functions to convert signed 32-bit data to other formats */

#define CLIP_THRESHHOLD  ((1L << (31-GUARD_BITS)) - 1)


void s32tos8(void *dp, const s32_t *lp, int count)
{
	s8_t *dest = (s8_t *)dp;

	while (count--)
	{
		s32_t val = *lp++;

		     if (val >  CLIP_THRESHHOLD) val =  CLIP_THRESHHOLD;
		else if (val < -CLIP_THRESHHOLD) val = -CLIP_THRESHHOLD;

		*dest++ = (s8_t) (val >> (32-8-GUARD_BITS));
	}
}

void s32tou8(void *dp, const s32_t *lp, int count)
{
	u8_t *dest = (u8_t *)dp;

	while (count--)
	{
		s32_t val = *lp++;

		     if (val >  CLIP_THRESHHOLD) val =  CLIP_THRESHHOLD;
		else if (val < -CLIP_THRESHHOLD) val = -CLIP_THRESHHOLD;

		*dest++ = (u8_t) (0x80 ^ (val >> (32-8-GUARD_BITS)));
	}
}

void s32tos16(void *dp, const s32_t *lp, int count)
{
	s16_t *dest = (s16_t *)dp;

	while (count--)
	{
		s32_t val = *lp++;

		     if (val >  CLIP_THRESHHOLD) val =  CLIP_THRESHHOLD;
		else if (val < -CLIP_THRESHHOLD) val = -CLIP_THRESHHOLD;

		*dest++ = (s16_t)(val >> (32-16-GUARD_BITS));
	}
}

void s32tou16(void *dp, const s32_t *lp, int count)
{
	u16_t *dest = (u16_t *)dp;

	while (count--)
	{
		s32_t val = *lp++;

		     if (val >  CLIP_THRESHHOLD) val =  CLIP_THRESHHOLD;
		else if (val < -CLIP_THRESHHOLD) val = -CLIP_THRESHHOLD;

		*dest++ = (u16_t) (0x8000 ^ (val >> (32-16-GUARD_BITS)));
	}
}

void s32tos16x(void *dp, const s32_t *lp, int count)
{
	s16_t *dest = (s16_t *)dp;

	while (count--)
	{
		s32_t val = *lp++;

		     if (val >  CLIP_THRESHHOLD) val =  CLIP_THRESHHOLD;
		else if (val < -CLIP_THRESHHOLD) val = -CLIP_THRESHHOLD;

		val >>= (32-16-GUARD_BITS);

		*dest++ = (s16_t)EPI_Swap16((u16_t)val);
	}
}

void s32tou16x(void *dp, const s32_t *lp, int count)
{
	u16_t *dest = (u16_t *)dp;

	while (count--)
	{
		s32_t val = *lp++;

		     if (val >  CLIP_THRESHHOLD) val =  CLIP_THRESHHOLD;
		else if (val < -CLIP_THRESHHOLD) val = -CLIP_THRESHHOLD;

		val >>= (32-16-GUARD_BITS);

		*dest++ = (u16_t) (0x8000 ^ EPI_Swap16((u16_t)val));
	}
}

void s32toulaw(void *dp, const s32_t *lp, int count)
{
	u8_t *dest = (u8_t *)dp;

	while (count--)
	{
		s32_t val = *lp++;

		     if (val >  CLIP_THRESHHOLD) val =  CLIP_THRESHHOLD;
		else if (val < -CLIP_THRESHHOLD) val = -CLIP_THRESHHOLD;

		val >>= (32-13-GUARD_BITS);

		// Note: _l2u has been offsetted by 4096 to allow for
		//       the negative values here.  (-AJA- ick!)
		*dest++ = _l2u[val];
	}
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
