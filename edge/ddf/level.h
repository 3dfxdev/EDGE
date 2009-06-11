//----------------------------------------------------------------------------
//  EDGE Data Definition File Code (Main)
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2009  The EDGE Team.
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

#ifndef __DDF_LEVEL_H__
#define __DDF_LEVEL_H__

#include "main.h"


// ------------------------------------------------------------------
// ---------------MAP STRUCTURES AND CONFIGURATION-------------------
// ------------------------------------------------------------------

// -KM- 1998/11/25 Added generalised Finale type.
class map_finaledef_c
{
public:
	map_finaledef_c();
	map_finaledef_c(map_finaledef_c &rhs);
	~map_finaledef_c();

private:
	void Copy(map_finaledef_c &src);
	
public:
	void Default(void);
	map_finaledef_c& operator=(map_finaledef_c &rhs);

	// Text
	epi::strent_c text;
	lumpname_c text_back;
	lumpname_c text_flat;
	float text_speed;
	unsigned int text_wait;
	rgbcol_t text_rgb;
	const colourmap_c *text_colmap;

	// Pic
	epi::strlist_c pics;
	unsigned int picwait;

	// Cast
	bool docast;
	
	// Bunny
	bool dobunny;

	// Music
	int music;
};

typedef enum
{
	MPF_NONE          = 0x0,

	MPF_NoCheats      = (1 << 3),
	MPF_NoJumping     = (1 << 0),
	MPF_NoMLook       = (1 << 1),
	MPF_NoCrouching   = (1 << 19),
	MPF_NoTrue3D      = (1 << 8),    // DOOM 2D physics
	MPF_NoMoreBlood   = (1 << 10),   // Make a bloody mess

	MPF_ItemRespawn   = (1 << 4),
	MPF_MonRespawn    = (1 << 11),
	MPF_FastMon       = (1 << 5),    // Fast Monsters
	MPF_Stomp         = (1 << 9),    // Monsters can stomp players
	MPF_ResetPlayer   = (1 << 14),   // Force player back to square #1
	MPF_LimitZoom     = (1 << 16),   // Limit zoom to certain weapons
}
map_setting_e;

#define MPF_ALL_NEGATIVES  \
	(MPF_NoCheats | MPF_NoJumping | MPF_NoMLook |  \
	 MPF_NoCrouching | MPF_NoTrue3D | MPF_NoMoreBlood)

typedef enum
{
	// standard Doom intermission stats
	WISTYLE_Doom = 0,

	// no stats at all
	WISTYLE_None = 1
}
intermission_style_e;

class mapdef_c
{
public:
	mapdef_c();
	mapdef_c(mapdef_c &rhs);
	~mapdef_c();
	
private:
	void Copy(mapdef_c &src);
	
public:
	void CopyDetail(mapdef_c &src);
	void Default(void);
	mapdef_c& operator=(mapdef_c &rhs);

	ddf_base_c ddf;

///---	// next in the list
///---	mapdef_c *next;				// FIXME!! Gamestate information

	// level description, a reference to languages.ldf
	epi::strent_c description;
  
  	lumpname_c namegraphic;
  	lumpname_c lump;
   	lumpname_c sky;
   	lumpname_c surround;
   	
   	int music;
 
	int partime;

	gamedef_c *episode;  // set during DDF_CleanUp
	epi::strent_c episode_name;			

	// contains a bitmask of MPF_XXX flags
	int features;

	// name of the next normal level
	lumpname_c nextmapname;

	// name of the secret level
	lumpname_c secretmapname;

	// -KM- 1998/11/25 All lines with this trigger will be activated at
	// the level start. (MAP07)
	int autotag;

	intermission_style_e wistyle;

	// -KM- 1998/11/25 Generalised finales.
	map_finaledef_c f_pre;
	map_finaledef_c f_end;
};

// Our mapdefs container
class mapdef_container_c : public epi::array_c 
{
public:
	mapdef_container_c() : epi::array_c(sizeof(mapdef_c*)) {}
	~mapdef_container_c() { Clear(); } 

private:
	void CleanupObject(void *obj);

public:
	mapdef_c* Lookup(const char *name);
	int GetSize() {	return array_entries; } 
	int Insert(mapdef_c *m) { return InsertObject((void*)&m); }
	mapdef_c* operator[](int idx) { return *(mapdef_c**)FetchObject(idx); } 
};


// -------EXTERNALISATIONS-------

extern mapdef_container_c mapdefs;			// -ACB- 2004/06/29 Implemented

bool DDF_ReadLevels(void *data, int size);

#endif // __DDF_LEVEL_H__

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
