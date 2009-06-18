//----------------------------------------------------------------------------
//  EDGE Key handling
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2009  The EDGE Team.
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
//  Based on the DOOM source code, released by Id Software under the
//  following copyright:
//
//    Copyright (C) 1993-1996 by id Software, Inc.
//
//----------------------------------------------------------------------------

#include "i_defs.h"

#include "e_event.h"
#include "e_keys.h"
#include "e_input.h"
#include "e_main.h"


void key_binding_c::Clear()
{
	keys[0] = keys[1] = keys[2] = keys[3];
}

bool key_binding_c::AddBind(short keyd)
{
	if (HasKey(keyd))
		return true;

	int i;

	for (i = 0; i < 4; i++)
		if (keys[i] == 0)
		{
			keys[i] = keyd;
			return true;
		}

	return false; // failed
}

bool key_binding_c::HasKey(short keyd)
{
	return (keys[0] == keyd) || (keys[1] == keyd) ||
	       (keys[2] == keyd) || (keys[3] == keyd);
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
