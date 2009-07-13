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

#include "main.h"

#include "m_select.h"


selection_c::selection_c(obj_type_t _type) : type(_type),
	count(0), bv(NULL)
{ }

selection_c::~selection_c()
{
	if (bv)
		delete bv;
}

bool selection_c::get(int n) const
{
	if (bv)
		return bv->get(n);

	// FIXME
}

void selection_c::set(int n)
{
	if (bv)
	{
		bv->set(n); return;
	}

	// FIXME
}

void selection_c::clear(int n)
{
	if (bv)
	{
		bv->clear(n); return;
	}

	// FIXME
}

void selection_c::toggle(int n)
{
	if (bv)
	{
		bv->toggle(n); return;
	}

	if (get(n))
		clear(n);
	else
		set(n);
}

void selection_c::clear_all()
{
	if (bv)
	{
		delete bv;
		bv = NULL;
	}

	count = 0;
}

void selection_c::set_all()
{
	if (! bv)
		ConvertToBitvec();
	
	bv->set_all();
}

void selection_c::toggle_all()
{
	if (! bv)
		ConvertToBitvec();
	
	bv->toggle_all();
}

void selection_c::frob(int n, sel_op_e op)
{
	switch (op)
	{
		case BOP_ADD: set(n); break;
		case BOP_REMOVE: clear(n); break;
		default: toggle(n); break;
	}
}

void selection_c::merge(const selection_c& other)
{
	if (! other.bv)
	{
		for (int i = 0; i < other.count; i++)
			set(other.objs[i]);
		return;
	}

	if (! bv)
		ConvertToBitvec();
	
	bv->merge(*other.bv);
}

void selection_c::ConvertToBitvec()
{
	SYS_ASSERT(! bv);

	int num_elem = 1234; // FIXME !!!!!

	bv = new bitvec_c(num_elem);

	for (int i = 0; i < count; i++)
	{
		bv->set(objs[i]);
	}

	count = 0;
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
