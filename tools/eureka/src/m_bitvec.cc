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

#include "main.h"

#include "m_bitvec.h"


bitvec_c::bitvec_c(int n_elements) : num_elem(n_elements)
{
	int total = (num_elem / 8) + 1;

	d = new byte[total];

	memset(d, 0, total);
}

bitvec_c::~bitvec_c()
{
	delete[] d;
}

bool bitvec_c::get(int n) const
{
	return (d[n >> 3] & (1 << (n & 7))) ? true : false;
}

void bitvec_c::set(int n)
{
	d[n >> 3] |= (1 << (n & 7));
}

void bitvec_c::unset(int n)
{
	d[n >> 3] &= ~(1 << (n & 7));
}

void bitvec_c::toggle(int n)
{
	d[n >> 3] ^= (1 << (n & 7));
}

void bitvec_c::frob(int n, sel_op_e op)
{
	switch (op)
	{
		case BOP_ADD: set(n); break;
		case BOP_REMOVE: unset(n); break;
		default: toggle(n); break;
	}
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
