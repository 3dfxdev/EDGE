//------------------------------------------------------------------------
//  SELECTION SET
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2009 Andrew Apted
//  Copyright (C) 1997-2003 André Majorel et al
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

class selection_iterator_c;


#define MAX_STORE_SEL  32

class selection_c
{
friend class selection_iterator_c;

private:
	obj_type_t type;

	int objs[MAX_STORE_SEL];
	int count;

	bitvec_c * bv;  // NULL unless needed
	int b_count;

public:
	 selection_c(obj_type_t _type = OBJ_NONE, int _initial = -1);
	~selection_c();

	obj_type_t what_type() const { return type; }

	void change_type(obj_type_t new_type);

	bool empty() const;
	bool notempty() const { return ! empty(); }

	bool get(int n) const;

	void set(int n);
	void clear(int n);
	void toggle(int n);

	void set_all();
	void clear_all();
	void toggle_all();

	void frob(int n, sel_op_e op);

	void merge(const selection_c& other);

	void begin(selection_iterator_c * it);
	// sets up the passed iterator for iterating over all the
	// object numbers contained in this selection.
	// Modifying the set is NOT allowed during a traversal.

private:
	void ConvertToBitvec();
};


class selection_iterator_c
{
friend class selection_c;

private:
	selection_c *sel;

	// this is the position in the objs[] array when there is no
	// bit vector, otherwise it is the object number itself
	// (and the corresponding bit will be one).
	int pos;

public:
	bool at_end() const;

	int operator* () const;

	selection_iterator_c& operator++ ();
};


#endif  /* __M_SELECT_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
