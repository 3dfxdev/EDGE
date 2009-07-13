//------------------------------------------------------------------------
//  SELECTION SET
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

#ifndef __M_SELECT_H__
#define __M_SELECT_H__

#include "m_bitvec.h"


#define MAX_STORE_SEL  2   // !!!! 16

class selection_c
{
private:
	obj_type_t type;

	int objs[MAX_STORE_SEL];
	int count;

	bitvec_c * bv;  // NULL unless needed

public:
	 selection_c(obj_type_t _type);
	~selection_c();

	bool get(int n) const;

	void set(int n);
	void clear(int n);
	void toggle(int n);

	void set_all();
	void clear_all();
	void toggle_all();

	void frob(int n, sel_op_e op);

	void merge(const selection_c& other);

private:
	void ConvertToBitvec();
};


#endif  /* __M_SELECT_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
