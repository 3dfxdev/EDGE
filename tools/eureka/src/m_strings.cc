//------------------------------------------------------------------------
//  STRING TABLE
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor (C) 2001-2009 Andrew Apted
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

#include "main.h"

#include "m_strings.h"


class string_block_c { int foo; };  //!!!!!


string_table_c::string_table_c() : blocks()
{
}

string_table_c::~string_table_c()
{
	for (int i = 0; i < (int)blocks.size(); i++)
		delete blocks[i];
}

int string_table_c::add(const char *str)
{
	if (! str || ! str[0])
		return 0;
	
	// FIXME
	
	return 1;
}

const char * string_table_c::get(int offset)
{
	// FIXME

	return "";
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
