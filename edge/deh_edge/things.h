//----------------------------------------------------------------------------
//  THING conversion
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2004  The EDGE Team.
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
#ifndef __THINGS__
#define __THINGS__

#include "info.h"

namespace Things
{
	void BeginLump(void);
	void FinishLump(void);

	void ConvertAll(void);

	void HandleFlags(mobjinfo_t *info, int mt_num, int player);
	const char *GetSound(int sound_id);
}


#endif /* __THINGS__ */
