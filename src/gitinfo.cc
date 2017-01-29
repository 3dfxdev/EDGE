//----------------------------------------------------------------------------
//  EDGE2 GitInfo
//----------------------------------------------------------------------------
// 
//  Copyright 2013 Marisa Heit. All rights reserved.
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
// This file is just here so that when gitinfo.h changes, only one source
// file needs to be recompiled.
//

#include "gitinfo.h"
#include "version.h"

const char *GetGitDescription()
{
	return GIT_DESCRIPTION;
}

const char *GetGitHash()
{
	return GIT_HASH;
}

const char *GetGitTime()
{
	return GIT_TIME;
}

const char *GetVersionString()
{
	if (GetGitDescription()[0] == '\0')
	{
		return VERSIONSTR;
	}
	else
	{
		return GIT_DESCRIPTION;
	}
}
