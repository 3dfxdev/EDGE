//----------------------------------------------------------------------------
//  EDGE Basic Types
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

#define LOOKUP_CACHESIZE 211


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


// a bitset is a set of named bits, from `A' to `Z'.
typedef int bitset_t;

#define BITSET_EMPTY  0
#define BITSET_FULL   0x7FFFFFFF
#define BITSET_MAKE(ch)  (1 << ((ch) - 'A'))


// -AJA- 1999/10/24: Reimplemented when_appear_e type.
typedef enum
{
	WNAP_None        = 0x0000,

	WNAP_SkillLevel1 = 0x0001,
	WNAP_SkillLevel2 = 0x0002,
	WNAP_SkillLevel3 = 0x0004,
	WNAP_SkillLevel4 = 0x0008,
	WNAP_SkillLevel5 = 0x0010,

	WNAP_Single      = 0x0100,
	WNAP_Coop        = 0x0200,
	WNAP_DeathMatch  = 0x0400,

	WNAP_SkillBits   = 0x001F,
	WNAP_NetBits     = 0x0700
}
when_appear_e;

#define DEFAULT_APPEAR  ((when_appear_e)(0xFFFF))


// Our lumpname class
#define LUMPNAME_SIZE 10

class lumpname_c
{
public:
	lumpname_c() { clear(); }
	lumpname_c(lumpname_c &rhs) { Set(rhs.data); }
	~lumpname_c() {};

private:
	char data[LUMPNAME_SIZE];

public:
	void clear() { data[0] = '\0'; }

	const char *c_str() const { return data; }

	inline bool empty() const { return data[0] == '\0'; }

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
	const char *GetName() const { return name.c_str(); }

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


// override labels for various states, if the object being damaged
// has such a state then it is used instead of the normal ones
// (PAIN, DEATH, OVERKILL).  Defaults to NULL.
class label_offset_c
{
public:
	label_offset_c();
	label_offset_c(label_offset_c &rhs);
	~label_offset_c(); 

private:
	void Copy(label_offset_c &src);

public:
	void Default();
	label_offset_c& operator=(label_offset_c &rhs);

	epi::strent_c label;
	int offset;
};


class damage_c
{
public:
	damage_c();
	damage_c(damage_c &rhs);
	~damage_c();
	
	enum default_e
	{
		DEFAULT_Attack,
		DEFAULT_Mobj,
		DEFAULT_MobjChoke,
		DEFAULT_Sector,
		DEFAULT_Numtypes
	};
	
private:
	void Copy(damage_c &src);

public:
	void Default(default_e def);
	damage_c& operator= (damage_c &rhs);

	// nominal damage amount (required)
	float nominal;

	// used for DAMAGE.MAX: when this is > 0, the damage is random
	// between nominal and linear_max, where each value has equal
	// probability.
	float linear_max;
  
	// used for DAMAGE.ERROR: when this is > 0, the damage is the
	// nominal value +/- this error amount, with a bell-shaped
	// distribution (values near the nominal are much more likely than
	// values at the outer extreme).
	float error;

	// delay (in terms of tics) between damage application, e.g. 34
	// would be once every second.  Only used for slime/crush damage.
	int delay;

	// death message, names an entry in LANGUAGES.LDF
	epi::strent_c obituary;

	// override labels for various states, if the object being damaged
	// has such a state then it is used instead of the normal ones
	// (PAIN, DEATH, OVERKILL).  Defaults to NULL.
	label_offset_c pain, death, overkill;

	// this flag says that the damage is unaffected by the player's
	// armour -- and vice versa.
	bool no_armour;
};


#endif /*__DDF_TYPE_H__*/

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
