//----------------------------------------------------------------------------
//  EDGE Basic Types
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
//
//  Based on the DOOM source code, released by Id Software under the
//  following copyright:
//
//    Copyright (C) 1993-1996 by id Software, Inc.
//
//----------------------------------------------------------------------------

#ifndef __DDF_TYPE_H__
#define __DDF_TYPE_H__

#include "epi/utility.h"

class mobjtype_c;


// RGB 8:8:8
// (FIXME: use epi::colour_c)
typedef unsigned int rgbcol_t;

#define RGB_NO_VALUE  0x00FFFF  /* bright CYAN */

#define RGB_MAKE(r,g,b)  (((r) << 16) | ((g) << 8) | (b))

#define RGB_RED(rgbcol)  (((rgbcol) >> 16) & 0xFF)
#define RGB_GRN(rgbcol)  (((rgbcol) >>  8) & 0xFF)
#define RGB_BLU(rgbcol)  (((rgbcol)      ) & 0xFF)


// percentage type.  Ranges from 0.0f - 1.0f
typedef float percent_t;

#define PERCENT_MAKE(val)  ((val) / 100.0f)
#define PERCENT_2_FLOAT(perc)  (perc)


typedef u32_t angle_t;

#define ANGLEBITS  32

// Binary Angle Measument, BAM.
#define ANG0   0x00000000
#define ANG1   0x00B60B61
#define ANG45  0x20000000
#define ANG90  0x40000000
#define ANG135 0x60000000
#define ANG180 0x80000000
#define ANG225 0xa0000000
#define ANG270 0xc0000000
#define ANG315 0xe0000000

#define ANG_MAX 0xffffffff

// Only use this one with float.
#define ANG360  (4294967296.0)

#define ANG5   (ANG45/9)

// Conversion macros:

#define F2AX(n)  (((n) < 0) ? (360.0f + (n)) : (n))
#define ANG_2_FLOAT(a)  ((float) (a) * 360.0f / 4294967296.0f)
#define FLOAT_2_ANG(n)  ((angle_t) (F2AX(n) / 360.0f * 4294967296.0f))


// Our lumpname class
#define LUMPNAME_SIZE 10

class lumpname_c
{
public:
	lumpname_c() { Clear(); }
	lumpname_c(lumpname_c &rhs) { Set(rhs.data); }
	~lumpname_c() {};

private:
	char data[LUMPNAME_SIZE];

public:
	void Clear() { data[0] = '\0'; }

	const char* GetString() const { return data; }

	inline bool IsEmpty() const { return data[0] == '\0'; }

	void Set(const char *s) 
	{
		int i;

		for (i=0; i<(LUMPNAME_SIZE-1) && *s; i++, s++)
			data[i] = *s;

		data[i] = '\0';
	}

	lumpname_c& operator=(lumpname_c &rhs) 
	{
		if (&rhs != this) 
			Set(rhs.data);
			 
		return *this; 
	}
	
	char operator[](int idx) const { return data[idx]; }
	operator const char* () const { return data; }
};


class mobj_strref_c
{
public:
	mobj_strref_c() : name(), def(NULL) { }
	mobj_strref_c(const char *s) : name(s), def(NULL) { }
	mobj_strref_c(const mobj_strref_c &rhs) : name(rhs.name), def(NULL) { }
	~mobj_strref_c() {};

private:
	epi::strent_c name;

	const mobjtype_c *def;

public:
	const char *GetName() const { return name.GetString(); }

	const mobjtype_c *GetRef();
	// Note: this returns NULL if not found, in which case you should
	// produce an error, since future calls will do the search again.

	mobj_strref_c& operator= (mobj_strref_c &rhs) 
	{
		if (&rhs != this) 
		{
			name = rhs.name;
			def = NULL;
		}
			 
		return *this; 
	}
};


#endif /*__DDF_TYPE_H__*/

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
