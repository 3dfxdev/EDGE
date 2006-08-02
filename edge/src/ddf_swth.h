//----------------------------------------------------------------------------
//  EDGE Data Definition File Codes (Switch textures)
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2005  The EDGE Team.
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

#ifndef __DDF_SWTH__
#define __DDF_SWTH__

#include "dm_defs.h"

#include "epi/utility.h"

//
// SWITCHES
//
#define BUTTONTIME  35

typedef struct switchcache_s
{
	const struct image_s *image[2];
}
switchcache_t;

class switchdef_c
{
public:
	switchdef_c();
	switchdef_c(switchdef_c &rhs);
	~switchdef_c() {};

private:
	void Copy(switchdef_c &src);

public:
	void CopyDetail(switchdef_c &src);
	void Default(void);
	switchdef_c& operator=(switchdef_c &rhs);

	ddf_base_c ddf;

	lumpname_c name1;
	lumpname_c name2;

	sfx_t *on_sfx;
	sfx_t *off_sfx;

	int time;

	switchcache_t cache;
};

// Our switchdefs container
class switchdef_container_c : public epi::array_c 
{
public:
	switchdef_container_c() : epi::array_c(sizeof(switchdef_c*)) {}
	~switchdef_container_c() { Clear(); } 

private:
	void CleanupObject(void *obj);

public:
	switchdef_c* Find(const char *name);
	int GetSize() {	return array_entries; } 
	int Insert(switchdef_c *sw) { return InsertObject((void*)&sw); }
	switchdef_c* operator[](int idx) { return *(switchdef_c**)FetchObject(idx); } 
};

extern switchdef_container_c switchdefs; 	// -ACB- 2004/06/04 Implemented

bool DDF_ReadSwitch(void *data, int size);

#endif  /* __DDF_SWTH__ */
