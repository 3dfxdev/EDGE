//----------------------------------------------------------------------------
//  EDGE Rendering things (objects as sprites) Code
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
//  Based on the DOOM source code, released by Id Software under the
//  following copyright:
//
//    Copyright (C) 1993-1996 by id Software, Inc.
//
//----------------------------------------------------------------------------

#ifndef __R_THINGS__
#define __R_THINGS__

#include "e_player.h"
#include "r_defs.h"

#include "epi/epiarray.h"

// utility macro.
#define ANG_2_ROT(angle)  ((angle_t)(angle) >> (ANGLEBITS-4))

//      
// Sprites are patches with a special naming convention so they can be
// recognized by R_InitSprites.  The base name is NNNNFx or NNNNFxFx,
// with x indicating the rotation, x = 0, 1-15.
//
// Horizontal flipping is used to save space, thus NNNNF2F8 defines a
// mirrored patch (F8 is the mirrored one).
//
// Some sprites will only have one picture used for all views: NNNNF0.
// In that case, the `rotated' field is false.
//
class spriteframe_c
{
public:
	spriteframe_c() : finished(false), rotated(false), extended(false)
	{
		for (int j = 0; j < 16; j++)
		{
			flip[j] = 0;
			images[j] = NULL;
		}
	}

	// whether this frame has been completed.  Completed frames cannot
	// be replaced by sprite lumps in older wad files.
	// 
	byte finished;
  
    // if not rotated, we don't have to determine the angle for the
    // sprite.  This is an optimisation.
    // 
	byte rotated;
  
	// normally zero, will be 1 if the [9ABCDEFG] rotations are used.
	byte extended;
    
	// Flip bits (1 = flip) to use for view angles 0-15.
	byte flip[16];
  
	// Images for each view angle 0-15.  Never NULL.
	const struct image_s *images[16];
};

//
// A sprite definition: a number of animation frames.
//
class spritedef_c
{
public:
	spritedef_c(const char *_name, bool _weap) : is_weapon(_weap), numframes(0), frames(NULL)
	{
		strcpy(name, _name);
	}

	// four letter sprite name (e.g. "TROO").
	char name[6];
  
  	bool is_weapon;

    // total number of frames.  Zero for missing sprites.
	int numframes;

	// sprite frames.
	spriteframe_c *frames;
};

class spritedef_array_c : public epi::array_c
{
public:
	spritedef_array_c() : epi::array_c(sizeof(spritedef_c*)) { }
	~spritedef_array_c() { Clear(); }

private:
	void CleanupObject(void *obj) { delete *(spritedef_c**)obj; }

public:
    // List Management
	int GetSize() { return array_entries; }
	int Insert(spritedef_c *c) { return InsertObject((void*)&c); }
	spritedef_c* operator[](int idx) { return *(spritedef_c**)FetchObject(idx); }
};

extern spritedef_array_c sprites;
extern int numsprites;  // same as sprites.GetSize()

int R_AddSpriteName(const char *name, int frame, bool is_weapon);
void R_InitSprites(void);

#endif
