//----------------------------------------------------------------------
//  COAL MEMORY BLOCKS
//----------------------------------------------------------------------
// 
//  Copyright (C) 2009  Andrew Apted
//
//  Coal is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as
//  published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  Coal is distributed in the hope that it will be useful, but
//  WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
//  the GNU General Public License for more details.
//
//----------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>

#include "coal.h"


namespace coal
{

#include "c_local.h"
#include "c_memory.h"


bgroup_c::~bgroup_c()
{
	while (used > 0)
	{
		used--;
		delete blocks[used];
	}
}


bmaster_c::~bmaster_c()
{
	while (used > 0)
	{
		used--;
		delete groups[used];
	}
}


int bmaster_c::alloc(int len)
{
}





}  // namespace coal

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
