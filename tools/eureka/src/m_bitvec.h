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

#ifndef YH_BITVEC
#define YH_BITVEC


class bitvec_c
{
private:
	int num_elem;

	byte *d;

public:
	 bitvec_c(int n_elements);
	~bitvec_c();

	inline int size() const
	{
		return num_elem;
	}

	bool get(int n) const;  // Get bit <n>

	void set(int n);    // Set bit <n> to 1
	void clear(int n);  // Set bit <n> to 0
	void toggle(int n); // Toggle bit <n>

	void set_all();
	void clear_all();
	void toggle_all();

	void frob(int n, sel_op_e op);

	void merge(const bitvec_c& other);
};


#endif  /* YH_BITVEC */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
