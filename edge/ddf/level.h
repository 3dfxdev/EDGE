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

#ifndef __DDF_LEVEL_H__
#define __DDF_LEVEL_H__

#include "base.h"
#include "types.h"


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
	const colourmap_c *text_colmap;

	// Pic
	epi::strlist_c pics;
	unsigned int picwait;

	// FIXME!!! Booleans should be bit-sets/flags here...
	// Cast
	bool docast;
	
	// Bunny
	bool dobunny;

	// Music
	int music;
};

// FIXME!!! Move into mapdef_c?
typedef enum
{
	MPF_None          = 0x0,
	MPF_Jumping       = 0x1,
	MPF_Mlook         = 0x2,
	MPF_Cheats        = 0x8,
	MPF_ItemRespawn   = 0x10,
	MPF_FastParm      = 0x20,     // Fast Monsters
	MPF_ResRespawn    = 0x40,     // Resurrect Monsters (else Teleport)
	MPF_StretchSky    = 0x80,
	MPF_True3D        = 0x100,    // True 3D Gameplay
	MPF_Stomp         = 0x200,    // Monsters can stomp players
	MPF_MoreBlood     = 0x400,    // Make a bloody mess
	MPF_Respawn       = 0x800,
	MPF_AutoAim       = 0x1000,
	MPF_AutoAimMlook  = 0x2000,
	MPF_ResetPlayer   = 0x4000,   // Force player back to square #1
	MPF_Extras        = 0x8000,
	MPF_LimitZoom     = 0x10000,  // Limit zoom to certain weapons
	MPF_Shadows       = 0x20000,
	MPF_Halos         = 0x40000,
	MPF_Crouching     = 0x80000,
	MPF_Kicking       = 0x100000, // Weapon recoil
	MPF_BoomCompat    = 0x200000,
	MPF_WeaponSwitch  = 0x400000,
	MPF_NoMonsters    = 0x800000, // (Only for demos!)
	MPF_PassMissile   = 0x1000000,
}
mapsettings_e;

// FIXME!!! Move into mapdef_c?
typedef enum
{
	// standard Doom shading
	LMODEL_Doom = 0,

	// Doom shading without the brighter N/S, darker E/W walls
	LMODEL_Doomish = 1,

	// flat lighting (no shading at all)
	LMODEL_Flat = 2,

	// vertex lighting
	LMODEL_Vertex = 3,

 	// Invalid (-ACB- 2003/10/06: MSVC wants the invalid value as part of the enum)
 	LMODEL_Invalid = 999
}
lighting_model_e;

// FIXME!!! Move into mapdef_c?
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

	// next in the list
	mapdef_c *next;				// FIXME!! Gamestate information

	// level description, a reference to languages.ldf
	epi::strent_c description;
  
  	lumpname_c namegraphic;
  	lumpname_c lump;
   	lumpname_c sky;
   	lumpname_c surround;
   	
   	int music;
 
	int partime;
	epi::strent_c episode_name;			

	// flags come in two flavours: "force on" and "force off".  When not
	// forced, then the user is allowed to control it (not applicable to
	// all the flags, e.g. RESET_PLAYER).
	int force_on;
	int force_off;

	// name of the next normal level
	lumpname_c nextmapname;

	// name of the secret level
	lumpname_c secretmapname;

	// -KM- 1998/11/25 All lines with this trigger will be activated at
	// the level start. (MAP07)
	int autotag;

	// -AJA- 2000/08/23: lighting model & intermission style
	lighting_model_e lighting;
	intermission_style_e wistyle;

	// -KM- 1998/11/25 Generalised finales.
	// FIXME!!! Suboptimal - should be pointers/references
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
