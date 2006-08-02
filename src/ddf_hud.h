//----------------------------------------------------------------------------
//  EDGE Data Definition File Code (HUD)
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

#ifndef __DDF_HUD__
#define __DDF_HUD__

#include "dm_defs.h"

#include "epi/utility.h"

//
// -AJA- 2004/11/25 HUD.ddf
//
class hud_element_c
{
public:
	hud_element_c();
	~hud_element_c();

	hud_element_c *next;  // link in list

	// FIXME !!!!
};

class huddef_c
{
public:
	huddef_c();
	huddef_c(const huddef_c &rhs);
	~huddef_c() {};

private:
	void Copy(const huddef_c &src);

public:
	void CopyDetail(const huddef_c &src);
	void Default(void);
	huddef_c& operator= (const huddef_c &rhs);

	// Member vars....
	ddf_base_c ddf;

	enum  // usage values
	{
		U_Player = 0,
		U_Automap,
		U_RTS
	};

	int usage;

	int left_size;
	int right_size;
	int top_size;
	int bottom_size;

	hud_element_c *elements;
};

// Our huddefs container
class huddef_container_c : public epi::array_c 
{
public:
	huddef_container_c() : epi::array_c(sizeof(huddef_c*)) {}
	~huddef_container_c() { Clear(); } 

private:
	void CleanupObject(void *obj);

public:
	int GetSize() {	return array_entries; } 
	int Insert(huddef_c *a) { return InsertObject((void*)&a); }
	huddef_c* operator[](int idx) { return *(huddef_c**)FetchObject(idx); } 

	// Search Functions
	huddef_c* Lookup(const char* refname);
};

extern huddef_container_c huddefs;

void DDF_MainLookupHUD(const char *info, void *storage);
bool DDF_ReadHuds(void *data, int size);

#endif  /* __DDF_HUD__ */
