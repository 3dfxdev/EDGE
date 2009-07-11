//------------------------------------------------------------------------
//  BIT VECTORS
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor (C) 2001-2009 Andrew Apted
//                     (C) 1997-2003 André Majorel et al
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
//------------------------------------------------------------------------
//
//  Based on Yadex which incorporated code from DEU 5.21 that was put
//  in the public domain in 1994 by Raphaël Quinet and Brendon Wyber.
//
//------------------------------------------------------------------------

#ifndef YH_BITVEC  /* DO NOT INSERT ANYTHING BEFORE THIS LINE */
#define YH_BITVEC


#include <limits.h>

#include "ymemory.h"


extern const char _bitvec_msg1[];


class bitvec_c
{
public :
	bitvec_c (size_t n_elements)
	{ 
		a = (char *) GetMemory (n_elements / CHAR_BIT + 1);
		memset (a, 0, n_elements / CHAR_BIT + 1);
		_n_elements = n_elements;
	}

	~bitvec_c ()
	{
		FreeMemory (a);
	}

	size_t nelements () const  // Return the number of elements
	{
		return _n_elements;
	}

	int get (size_t n) const  // Get bit <n>
	{
		return a[n/CHAR_BIT] & (1 << (n % CHAR_BIT));
	}

	void set (size_t n)  // Set bit <n> to 1
	{
		a[n/CHAR_BIT] |= 1 << (n % CHAR_BIT);
	}

	void unset (size_t n)  // Set bit <n> to 0
	{
		a[n/CHAR_BIT] &= ~(1 << (n % CHAR_BIT));
	}

	void toggle (size_t n)  // Toggle bit <n>
	{
		a[n/CHAR_BIT] ^= (1 << (n % CHAR_BIT));
	}

	void frob (size_t n, sel_op_e op)  // Set, unset or toggle bit <n>
	{
		if (op == BOP_ADD)
			set (n);
		else if (op == BOP_REMOVE)
			unset (n);
		else
			toggle (n);
	}

private :
	size_t _n_elements;
	char *a;
};


#endif  /* DO NOT ADD ANYTHING AFTER THIS LINE */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
