//----------------------------------------------------------------------------
//  EDGE DDF base structure
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2008  The EDGE Team.
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

#ifndef __DDF_BASE_H__
#define __DDF_BASE_H__

#include "epi/math_crc.h"
#include "epi/utility.h"

class ddf_base_c
{
public:
	ddf_base_c();	
	ddf_base_c(const ddf_base_c &rhs);
	~ddf_base_c();	

private:
	void Copy(const ddf_base_c &src);

public:
	void Default(void);
	void SetUniqueName(const char *prefix, int num);
	
	ddf_base_c& operator= (const ddf_base_c &rhs);

	epi::strent_c name;

	int number;	

	epi::crc32_c crc;
};

#endif /*__DDF_BASE_H__*/

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
