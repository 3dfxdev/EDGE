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

#ifndef __M_STRINGS_H__
#define __M_STRINGS_H__

class string_block_c;


class string_table_c
{
private:
	std::vector<string_block_c *> blocks;

public:
	 string_table_c();
	~string_table_c();

	int add(const char *str);
	// find the given string in the string table, returning the
	// offset if it exists, otherwise adds the string and returns
	// the new offset.  The empty string and NULL are always 0.

	const char * get(int offset);
	// get the string from an offset.  Zero always returns an
	// empty string.  NULL is never returned.

	void clear();
	// remove all strings.

private:
	int find(const char *str, int len);
	// returns -1 when not found.
};


#endif  /* __M_STRINGS_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
