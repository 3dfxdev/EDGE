//----------------------------------------------------------------------------
//  EPI Stack Base Class
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
#ifndef __EPI_STACK_CLASS__
#define __EPI_STACK_CLASS__

namespace epi
{

// Typedefs
typedef int stack_block_t;

// Classes
class stack_c
{
public:
    stack_c(const unsigned int objsize);
    virtual ~stack_c();

protected:
	unsigned int stack_block_objsize;	
	unsigned int stack_objsize;		

	stack_block_t *stack;
	int stack_entries;
	int stack_max_entries;

	void* FetchObjectDirect(int pos) 
		{ return (void*)&stack[pos*stack_block_objsize]; }

public:
	// Object Management
    virtual void CleanupObject(void *obj) = 0;
    bool PushObject(void *obj);
    bool PopObject(void *obj);
	
	// List Management
    void Clear(void);
    bool Size(int entries);
    bool Trim(void);
};

};

#endif /* __EPI_STACK_CLASS__ */
