//----------------------------------------------------------------------------
//  EDGE Data Definition File Code (Animated textures)
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

#ifndef __DDF_ANIM__
#define __DDF_ANIM__

#include "dm_defs.h"

#include "./epi/utility.h"

//
// source animation definition
//
// -KM- 98/07/31 Anims.ddf
//
class animdef_c
{
public:
	animdef_c();
	animdef_c(animdef_c &rhs);
	~animdef_c() {};

private:
	void Copy(animdef_c &src);

public:
	void CopyDetail(animdef_c &src);
	void Default(void);
	animdef_c& operator=(animdef_c &rhs);

	// Member vars....
	ddf_base_c ddf;

	enum  // types
	{
		A_Flat = 0,
		A_Texture,
		A_Graphic
	};

	int type;

	// -AJA- 2004/10/27: new SEQUENCE command for anims
	epi::strlist_c pics;

	// first and last names in TEXTURE1/2 lump
	lumpname_c endname;
	lumpname_c startname;

	// how many 1/35s ticks each frame lasts
	int speed;
};

// Our animdefs container
class animdef_container_c : public epi::array_c 
{
public:
	animdef_container_c() : epi::array_c(sizeof(animdef_c*)) {}
	~animdef_container_c() { Clear(); } 

private:
	void CleanupObject(void *obj);

public:
	int GetSize() {	return array_entries; } 
	int Insert(animdef_c *a) { return InsertObject((void*)&a); }
	animdef_c* operator[](int idx) { return *(animdef_c**)FetchObject(idx); } 
};

extern animdef_container_c animdefs;		// -ACB- 2004/06/03 Implemented 

bool DDF_ReadAnims(void *data, int size);

#endif  /* __DDF_ANIM__ */
