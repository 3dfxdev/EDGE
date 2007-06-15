//----------------------------------------------------------------------------
//  EDGE Stack Base Class
//----------------------------------------------------------------------------
//
//  Copyright (c) 2003-2005  The EDGE Team.
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
//----------------------------------------------------------------------------
//
#include "epi.h"
#include "stacks.h"

#include <string.h>

namespace epi
{

//
// Constructor
//
stack_c::stack_c(const unsigned int objsize) :
    stack(NULL), stack_entries(0), stack_max_entries(0)
{
    // Precalc size to the nearest block
    stack_block_objsize = 
		(objsize+(sizeof(stack_block_t)-1))/sizeof(stack_block_t);

	// Need this for memcpy()'s later
    stack_objsize = objsize;
}

//
// Deconstructor
//
stack_c::~stack_c()
{
	Clear();
}

//
// stack_c::Clear
//
void stack_c::Clear(void)
{
    if (stack)
    {
        while(stack_entries>=0)
        {
            CleanupObject(FetchObjectDirect(stack_entries-1));
            stack_entries--;
        }
        
        delete [] stack;
        
        stack = NULL;
        stack_entries = 0;
        stack_max_entries = 0;
    }
}

//
// bool stack_c::Size
//
bool stack_c::Size(int entries)
{
    stack_block_t *newstack;

    if (entries <= 0)
        return false;

    newstack = new stack_block_t[stack_block_objsize*entries];
    if (!newstack)
        return false;

    // if there is any existing data, then copy from old data
    if (stack_entries>0)
    {
        int datasize;
        
        datasize = (entries>stack_entries)?stack_entries:entries;
        datasize *= stack_block_objsize;
        datasize *= sizeof(stack_block_t);
        memcpy(newstack, stack, datasize);

        // Remove any entries that get nuked by the resize
        while (stack_entries>entries)
        {
            CleanupObject(FetchObjectDirect(stack_entries-1));
            stack_entries--;
        }
        
        delete [] stack;
    }
    
    // Set the new stack and the new max
    stack = newstack;
    stack_max_entries = entries;
    return true;
}

//
// bool stack_c::Trim
//
bool stack_c::Trim(void)
{
	return Size(stack_entries);
}

//
// bool stack_c::PushObject
//
bool stack_c::PushObject(void *obj)
{
	if (!obj)
		return false;

	if (stack_entries >= stack_max_entries)
	{
		int new_stack_size;

   		if (!stack_entries)
			new_stack_size = 2;
		else
			new_stack_size = stack_entries * 2;

	    if (!Size(new_stack_size))
            return false;
	}

    memcpy(FetchObjectDirect(stack_entries), obj, stack_objsize);
	stack_entries++;

	return true;
}

//
// bool stack_c::PopObject
//
bool stack_c::PopObject(void *obj)
{
	if (!stack_entries)
		return false;

	memcpy(obj, FetchObjectDirect(stack_entries-1), stack_objsize);
	stack_entries--;

	return true;
}

};

