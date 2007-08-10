//----------------------------------------------------------------------------
//  EDGE Data Definition File Code (Fonts)
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2007  The EDGE Team.
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

#ifndef __DDF_FONT__
#define __DDF_FONT__

#include "epi/utility.h"

#include "ddf_base.h"
#include "ddf_type.h"

//
// -AJA- 2004/11/13 Fonts.ddf
//
typedef enum
{
	FNTYP_Patch = 0   // font is made up of individual patches
}
fonttype_e;

class fontpatch_c
{
public:
	fontpatch_c(int _ch1, int _ch2, const char *_pat1);
	~fontpatch_c();

	fontpatch_c *next;  // link in list

	int char1, char2;  // range

	epi::strent_c patch1;
};

class fontdef_c
{
public:
	fontdef_c();
	fontdef_c(const fontdef_c &rhs);
	~fontdef_c() {};

private:
	void Copy(const fontdef_c &src);

public:
	void CopyDetail(const fontdef_c &src);
	void Default(void);
	fontdef_c& operator= (const fontdef_c &rhs);

	// Member vars....
	ddf_base_c ddf;

	fonttype_e type;

	fontpatch_c *patches;
	epi::strent_c missing_patch;
};

// Our fontdefs container
class fontdef_container_c : public epi::array_c 
{
public:
	fontdef_container_c() : epi::array_c(sizeof(fontdef_c*)) {}
	~fontdef_container_c() { Clear(); } 

private:
	void CleanupObject(void *obj);

public:
	int GetSize() {	return array_entries; } 
	int Insert(fontdef_c *a) { return InsertObject((void*)&a); }
	fontdef_c* operator[](int idx) { return *(fontdef_c**)FetchObject(idx); } 

	// Search Functions
	fontdef_c* Lookup(const char* refname);
};

extern fontdef_container_c fontdefs;

void DDF_MainLookupFont(const char *info, void *storage);
bool DDF_ReadFonts(void *data, int size);

#endif  /* __DDF_FONT__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
