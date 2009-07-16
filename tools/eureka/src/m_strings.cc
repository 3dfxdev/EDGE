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


#define CHARS_PER_BLOCK  4096

class string_block_c
{
public:
	char data[CHARS_PER_BLOCK];

	int used;

public:
	 string_block_c() : used(0) { }
	~string_block_c() { }

	bool fits(int len) const
	{
		return (used + len + 1) < CHARS_PER_BLOCK;
	}

	int add(const char *str, int len)
	{
		int offset = used;

		memcpy(data + offset, str, len+1);
		used += len+1;

		return offset;
	}

	int find(const char *str, int len) const
	{
		int pos = 0;

		while (pos < used)
		{
			int p_len = (int)strlen(data + pos);

			if (len <= p_len)
			{
				if (memcmp(data + pos + (p_len-len), str, len) == 0)
					return pos + (p_len-len);
			}

			pos += p_len+1;
		}

		return -1;
	}
};


//------------------------------------------------------------------------


string_table_c::string_table_c() : blocks()
{
	// nothing needed
}

string_table_c::~string_table_c()
{
	clear();
}

int string_table_c::add(const char *str)
{
	if (! str || ! str[0])
		return 0;
	
	int len = (int)strlen(str);
	int offset = find(str, len);

	if (offset != -1)
		return offset;

	// FIXME: use negative offsets for huge strings
	if (len > CHARS_PER_BLOCK-8)
		FatalError("INTERNAL ERROR: string too long for string table (length=%d)\n", len);

	if (blocks.empty())
		blocks.push_back(new string_block_c);

	string_block_c *last = blocks.back();


	if (! last->fits(len))
	{
		// TODO: try some earlier blocks

		last = new string_block_c;

		blocks.push_back(last);
	}

	return last->add(str, len);
}

const char * string_table_c::get(int offset)
{
	SYS_ASSERT(offset >= 0);

	if (offset == 0)
		return "";
	
	offset--;

	int blk_num = offset / CHARS_PER_BLOCK;
	offset      = offset % CHARS_PER_BLOCK;

	SYS_ASSERT(blk_num < (int)blocks.size());

	return & blocks[blk_num]->data[offset];
}

void string_table_c::clear()
{
	for (int i = 0; i < (int)blocks.size(); i++)
		delete blocks[i];

	blocks.clear();
}

int string_table_c::find(const char *str, int len)
{
	for (int i = 0; i < (int)blocks.size(); i++)
	{
		int offset = blocks[i]->find(str, len);

		if (offset != -1)
			return (i * CHARS_PER_BLOCK) + offset;
	}

	return -1; // not found
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
