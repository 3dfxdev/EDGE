//----------------------------------------------------------------------------
//  EDGE Data Definition File Code (Main)
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

#ifndef __DDF_SFX_H__
#define __DDF_SFX_H__

#include "epi/utility.h"

#include "base.h"
#include "types.h"


#define S_CLOSE_DIST     160.0f
#define S_CLIPPING_DIST  4000.0f


// ----------------------------------------------------------------
// ------------------------ SOUND EFFECTS -------------------------
// ----------------------------------------------------------------

// -KM- 1998/10/29
typedef struct sfx_s
{
	int num;
	int sounds[1]; // -ACB- 1999/11/06 Zero based array is not ANSI compliant
	// -AJA- I'm also relying on the [1] within sfxdef_c.
}
sfx_t;

// Bastard SFX are sounds that are hardcoded into the
// code.  They should be removed if at all possible

typedef struct
{
	sfx_t *s;
	const char *name;
}
bastard_sfx_t;

extern bastard_sfx_t bastard_sfx[];

#define sfx_swtchn bastard_sfx[0].s
#define sfx_tink bastard_sfx[1].s
#define sfx_radio bastard_sfx[2].s
#define sfx_oof bastard_sfx[3].s
#define sfx_pstop bastard_sfx[4].s
#define sfx_stnmov bastard_sfx[5].s
#define sfx_pistol bastard_sfx[6].s
#define sfx_swtchx bastard_sfx[7].s
#define sfx_jpmove bastard_sfx[8].s
#define sfx_jpidle bastard_sfx[9].s
#define sfx_jprise bastard_sfx[10].s
#define sfx_jpdown bastard_sfx[11].s
#define sfx_jpflow bastard_sfx[12].s

#define sfx_None (sfx_t*) NULL

// Sound Effect Definition Class
class sfxdef_c
{
public:
	sfxdef_c();
	sfxdef_c(sfxdef_c &rhs);
	~sfxdef_c();
	
private:
	void Copy(sfxdef_c &src);
	
public:
	void CopyDetail(sfxdef_c &src);
	void Default(void);
	sfxdef_c& operator=(sfxdef_c &rhs);
	
	// sound's name, etc..
	ddf_base_c ddf;

    // full sound lump name (or file name)
	lumpname_c lump_name;
	epi::strent_c file_name;

	// sfxinfo ID number
	// -AJA- Changed to a sfx_t.  It serves two purposes: (a) hold the
	//       sound ID, like before, (b) better memory usage, as we don't
	//       need to allocate a new sfx_t for non-wildcard sounds.
	sfx_t normal;

    // Sfx singularity (only one at a time), or 0 if not singular
	int singularity;

	// Sfx priority
	int priority;

	// volume adjustment (100% is normal, lower is quieter)
	percent_t volume;

	// -KM- 1998/09/01  Looping: for non NULL origins
	bool looping;

	// -AJA- 2000/04/19: Prefer to play the whole sound rather than
	//       chopping it off with a new sound.
	bool precious;
 
    // distance limit, if the hearer is further away than `max_distance'
    // then the this sound won't be played at all.
	float max_distance;
 };

// Our sound effect definition container
class sfxdef_container_c : public epi::array_c 
{
public:
	sfxdef_container_c() : epi::array_c(sizeof(sfxdef_c*)) { num_disabled = 0; }
	~sfxdef_container_c() { Clear(); } 

private:
	void CleanupObject(void *obj);
	
	int num_disabled;					// Number of disabled 

public:
	// List management
	int GetSize() { return array_entries; } 
	int Insert(sfxdef_c *s) { return InsertObject((void*)&s); }
	sfxdef_c* operator[](int idx) { return *(sfxdef_c**)FetchObject(idx); } 
	
	// Inliners
	int GetDisabledCount() { return num_disabled; }
	void SetDisabledCount(int _num_disabled) { num_disabled = _num_disabled; }
	
	// Lookup functions
	sfx_t* GetEffect(const char *name, bool error = true);
	sfxdef_c* Lookup(const char *name);
};

// ----------EXTERNALISATIONS----------

extern sfxdef_container_c sfxdefs;			// -ACB- 2004/07/25 Implemented

bool DDF_ReadSFX(void *data, int size);

#endif // __DDF_SFX_H__

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
