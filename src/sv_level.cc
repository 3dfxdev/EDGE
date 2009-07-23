//----------------------------------------------------------------------------
//  EDGE New SaveGame Handling (Level Data)
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
// See the file "docs/save_sys.txt" for a complete description of the
// new savegame system.
//
// This file handles:
//    surface_t      [SURF]
//    side_t         [SIDE]
//    line_t         [LINE]
//
//    region_properties_t  [RPRP]
//    extrafloor_t         [EXFL]
//    sector_t             [SECT]
// 

#include "i_defs.h"

#include <stdio.h>
#include <stdlib.h>

#include "epi/str_format.h"

#include "ddf/colormap.h"

#include "p_hubs.h"
#include "r_image.h"
#include "sv_chunk.h"
#include "sv_main.h"
#include "z_zone.h"

#undef SF
#define SF  SVFIELD


// forward decls.
int SV_SideCountElems(void);
int SV_SideFindElem(side_t *elem);
void * SV_SideGetElem(int index);
void SV_SideCreateElems(int num_elems);
void SV_SideFinaliseElems(void);

int SV_LineCountElems(void);
int SV_LineFindElem(line_t *elem);
void * SV_LineGetElem(int index);
void SV_LineCreateElems(int num_elems);
void SV_LineFinaliseElems(void);

int SV_ExfloorCountElems(void);
int SV_ExfloorFindElem(extrafloor_t *elem);
void * SV_ExfloorGetElem(int index);
void SV_ExfloorCreateElems(int num_elems);
void SV_ExfloorFinaliseElems(void);

int SV_SectorCountElems(void);
int SV_SectorFindElem(sector_t *elem);
void * SV_SectorGetElem(int index);
void SV_SectorCreateElems(int num_elems);
void SV_SectorFinaliseElems(void);

int SV_HubCountElems(void);
int SV_HubFindElem(sector_t *elem);
void * SV_HubGetElem(int index);
void SV_HubCreateElems(int num_elems);
void SV_HubFinaliseElems(void);

bool SR_LevelGetImage(void *storage, int index, void *extra);
bool SR_LevelGetColmap(void *storage, int index, void *extra);
bool SR_LevelGetSurface(void *storage, int index, void *extra);
bool SR_LevelGetSurfPtr(void *storage, int index, void *extra);
bool SR_LineGetSpecial(void *storage, int index, void *extra);
bool SR_SectorGetSpecial(void *storage, int index, void *extra);
bool SR_SectorGetProps(void *storage, int index, void *extra);
bool SR_SectorGetPropRef(void *storage, int index, void *extra);
bool SR_SectorGetGenMove(void *storage, int index, void *extra);

void SR_LevelPutImage(void *storage, int index, void *extra);
void SR_LevelPutColmap(void *storage, int index, void *extra);
void SR_LevelPutSurface(void *storage, int index, void *extra);
void SR_LevelPutSurfPtr(void *storage, int index, void *extra);
void SR_LinePutSpecial(void *storage, int index, void *extra);
void SR_SectorPutSpecial(void *storage, int index, void *extra);
void SR_SectorPutProps(void *storage, int index, void *extra);
void SR_SectorPutPropRef(void *storage, int index, void *extra);
void SR_SectorPutGenMove(void *storage, int index, void *extra);

bool SR_HubGetMap(void *storage, int index, void *extra);
void SR_HubPutMap(void *storage, int index, void *extra);


//----------------------------------------------------------------------------
//
//  SURFACE STRUCTURE
//
static surface_t sv_dummy_surface;

#define SV_F_BASE  sv_dummy_surface

static savefield_t sv_fields_surface[] =
{
	SF(image, "image", 1, SVT_STRING, SR_LevelGetImage, SR_LevelPutImage),
	SF(translucency, "translucency", 1, SVT_FLOAT, SR_GetFloat, SR_PutFloat),

	SF(offset, "offset", 1, SVT_VEC2, SR_GetVec2, SR_PutVec2),
	SF(scroll, "scroll", 1, SVT_VEC2, SR_GetVec2, SR_PutVec2),
	SF(x_mat, "x_mat", 1, SVT_VEC2, SR_GetVec2, SR_PutVec2),
	SF(y_mat, "y_mat", 1, SVT_VEC2, SR_GetVec2, SR_PutVec2),

	SF(override_p, "override_p", 1, SVT_STRING, SR_SectorGetPropRef, SR_SectorPutPropRef),

	SVFIELD_END
};

savestruct_t sv_struct_surface =
{
	NULL,                  // link in list
	"surface_t",           // structure name
	"surf",                // start marker
	sv_fields_surface,     // field descriptions
	SVDUMMY,               // dummy base
	true,                  // define_me
	NULL                   // pointer to known struct
};

#undef SV_F_BASE


//----------------------------------------------------------------------------
//
//  SIDE STRUCTURE
//
static side_t sv_dummy_side;

#define SV_F_BASE  sv_dummy_side

static savefield_t sv_fields_side[] =
{
	SF(top, "top", 1, SVT_STRUCT("surface_t"), SR_LevelGetSurface, SR_LevelPutSurface),
	SF(middle, "middle", 1, SVT_STRUCT("surface_t"), SR_LevelGetSurface, SR_LevelPutSurface),
	SF(bottom, "bottom", 1, SVT_STRUCT("surface_t"), SR_LevelGetSurface, SR_LevelPutSurface),

	// NOT HERE:
	//   sector: value is kept from level load.

	SVFIELD_END
};

savestruct_t sv_struct_side =
{
	NULL,          // link in list
	"side_t",      // structure name
	"side",        // start marker
	sv_fields_side,  // field descriptions
	SVDUMMY,       // dummy base
	true,          // define_me
	NULL           // pointer to known struct
};

#undef SV_F_BASE

savearray_t sv_array_side =
{
	NULL,               // link in list
	"sides",            // array name
	&sv_struct_side,    // array type
	true,               // define_me
	true,               // allow_hub

	SV_SideCountElems,     // count routine
	SV_SideGetElem,        // index routine
	SV_SideCreateElems,    // creation routine
	SV_SideFinaliseElems,  // finalisation routine

	NULL,     // pointer to known array
	0         // loaded size
};


//----------------------------------------------------------------------------
//
//  LINE STRUCTURE
//
static line_t sv_dummy_line;

#define SV_F_BASE  sv_dummy_line

static savefield_t sv_fields_line[] =
{
	SF(flags, "flags", 1, SVT_INT, SR_GetInt, SR_PutInt),
	SF(tag,   "tag",   1, SVT_INT, SR_GetInt, SR_PutInt),
	SF(count, "count", 1, SVT_INT, SR_GetInt, SR_PutInt),
	SF(special, "special", 1, SVT_STRING, SR_LineGetSpecial, SR_LinePutSpecial),
	SF(slide_door, "slide_door", 1, SVT_STRING, SR_LineGetSpecial, SR_LinePutSpecial),

	// NOT HERE:
	//   (many): values are kept from level load.
	//   gap stuff: regenerated from sector heights.
	//   validcount: only a temporary value for some routines.
	//   slider_move: regenerated by a pass of the active part list.

	SVFIELD_END
};

savestruct_t sv_struct_line =
{
	NULL,          // link in list
	"line_t",      // structure name
	"line",        // start marker
	sv_fields_line,  // field descriptions
	SVDUMMY,       // dummy base
	true,          // define_me
	NULL           // pointer to known struct
};

#undef SV_F_BASE

savearray_t sv_array_line =
{
	NULL,               // link in list
	"lines",            // array name
	&sv_struct_line,    // array type
	true,               // define_me
	true,               // allow_hub

	SV_LineCountElems,     // count routine
	SV_LineGetElem,        // index routine
	SV_LineCreateElems,    // creation routine
	SV_LineFinaliseElems,  // finalisation routine

	NULL,     // pointer to known array
	0         // loaded size
};


//----------------------------------------------------------------------------
//
//  REGION_PROPERTIES STRUCTURE
//
static region_properties_t sv_dummy_regprops;

#define SV_F_BASE  sv_dummy_regprops

static savefield_t sv_fields_regprops[] =
{
	SF(lightlevel, "lightlevel_i", 1, SVT_INT, SR_GetInt, SR_PutInt),
	SF(colourmap,  "colourmap", 1, SVT_STRING, SR_LevelGetColmap, SR_LevelPutColmap),

	SF(type, "type", 1, SVT_INT, SR_GetInt, SR_PutInt),
	SF(special, "special", 1, SVT_STRING, SR_SectorGetSpecial, SR_SectorPutSpecial),

	SF(gravity, "gravity", 1, SVT_FLOAT, SR_GetFloat, SR_PutFloat),
	SF(friction, "friction", 1, SVT_FLOAT, SR_GetFloat, SR_PutFloat),
	SF(viscosity, "viscosity", 1, SVT_FLOAT, SR_GetFloat, SR_PutFloat),
	SF(drag, "drag", 1, SVT_FLOAT, SR_GetFloat, SR_PutFloat),
	SF(push, "push", 1, SVT_VEC3, SR_GetVec3, SR_PutVec3),

	SVFIELD_END
};

savestruct_t sv_struct_regprops =
{
	NULL,                   // link in list
	"region_properties_t",  // structure name
	"rprp",                 // start marker
	sv_fields_regprops,     // field descriptions
	SVDUMMY,                // dummy base
	true,                   // define_me
	NULL                    // pointer to known struct
};

#undef SV_F_BASE


//----------------------------------------------------------------------------
//
//  EXTRAFLOOR STRUCTURE
//
static extrafloor_t sv_dummy_exfloor;

#define SV_F_BASE  sv_dummy_exfloor

static savefield_t sv_fields_exfloor[] =
{
	SF(higher, "higher", 1, SVT_INDEX("extrafloors"), SR_SectorGetEF, SR_SectorPutEF),
	SF(lower, "lower", 1, SVT_INDEX("extrafloors"), SR_SectorGetEF, SR_SectorPutEF),
	SF(sector, "sector", 1, SVT_INDEX("sectors"), SR_SectorGetSector, SR_SectorPutSector),

	SF(top_h, "top_h", 1, SVT_FLOAT, SR_GetFloat, SR_PutFloat),
	SF(bottom_h, "bottom_h", 1, SVT_FLOAT, SR_GetFloat, SR_PutFloat),
	SF(top, "top", 1, SVT_STRING, SR_LevelGetSurfPtr, SR_LevelPutSurfPtr),
	SF(bottom, "bottom", 1, SVT_STRING, SR_LevelGetSurfPtr, SR_LevelPutSurfPtr),

	SF(p, "p", 1, SVT_STRING, SR_SectorGetPropRef, SR_SectorPutPropRef),
	SF(ef_line, "ef_line", 1, SVT_INDEX("lines"), SR_LineGetLine, SR_LinePutLine),
	SF(ctrl_next, "ctrl_next", 1, SVT_INDEX("extrafloors"), SR_SectorGetEF, SR_SectorPutEF),

	// NOT HERE:
	//   - sector: can be regenerated.
	//   - ef_info: cached value, regenerated from ef_line.

	SVFIELD_END
};

savestruct_t sv_struct_exfloor =
{
	NULL,              // link in list
	"extrafloor_t",    // structure name
	"exfl",            // start marker
	sv_fields_exfloor, // field descriptions
  SVDUMMY,           // dummy base
	true,              // define_me
	NULL               // pointer to known struct
};

#undef SV_F_BASE

savearray_t sv_array_exfloor =
{
	NULL,               // link in list
	"extrafloors",      // array name
	&sv_struct_exfloor, // array type
	true,               // define_me
	true,               // allow_hub

	SV_ExfloorCountElems,     // count routine
	SV_ExfloorGetElem,        // index routine
	SV_ExfloorCreateElems,    // creation routine
	SV_ExfloorFinaliseElems,  // finalisation routine

	NULL,     // pointer to known array
	0         // loaded size
};


//----------------------------------------------------------------------------
//
//  SECTOR STRUCTURE
//
static sector_t sv_dummy_sector;

#define SV_F_BASE  sv_dummy_sector

static savefield_t sv_fields_sector[] =
{
	SF(floor, "floor", 1, SVT_STRUCT("surface_t"), SR_LevelGetSurface, SR_LevelPutSurface),
	SF(ceil, "ceil", 1, SVT_STRUCT("surface_t"), SR_LevelGetSurface, SR_LevelPutSurface),
	SF(f_h, "f_h", 1, SVT_FLOAT, SR_GetFloat, SR_PutFloat),
	SF(c_h, "c_h", 1, SVT_FLOAT, SR_GetFloat, SR_PutFloat),

	SF(props, "props", 1, SVT_STRUCT("region_properties_t"), SR_SectorGetProps, SR_SectorPutProps),
	SF(p, "p", 1, SVT_STRING, SR_SectorGetPropRef, SR_SectorPutPropRef),

	SF(exfloor_used, "exfloor_used", 1, SVT_INT, SR_GetInt, SR_PutInt),
	SF(control_floors, "control_floors", 1, SVT_INDEX("extrafloors"),
			SR_SectorGetEF, SR_SectorPutEF),
	SF(sound_player, "sound_player", 1, SVT_INT, SR_GetInt, SR_PutInt),

	SF(bottom_ef, "bottom_ef", 1, SVT_INDEX("extrafloors"), SR_SectorGetEF, SR_SectorPutEF),
	SF(top_ef, "top_ef", 1, SVT_INDEX("extrafloors"), SR_SectorGetEF, SR_SectorPutEF),
	SF(bottom_liq, "bottom_liq", 1, SVT_INDEX("extrafloors"), SR_SectorGetEF, SR_SectorPutEF),
	SF(top_liq, "top_liq", 1, SVT_INDEX("extrafloors"), SR_SectorGetEF, SR_SectorPutEF),

	// NOT HERE:
	//   - floor_move, ceil_move: can be regenerated
	//   - (many): values remaining from level load are OK
	//   - soundtraversed & validcount: temp values, don't need saving

	SVFIELD_END
};

savestruct_t sv_struct_sector =
{
	NULL,              // link in list
	"sector_t",        // structure name
	"sect",            // start marker
	sv_fields_sector,  // field descriptions
	SVDUMMY,           // dummy base
	true,              // define_me
	NULL               // pointer to known struct
};

#undef SV_F_BASE

savearray_t sv_array_sector =
{
	NULL,               // link in list
	"sectors",          // array name
	&sv_struct_sector,  // array type
	true,               // define_me
	true,               // allow_hub

	SV_SectorCountElems,     // count routine
	SV_SectorGetElem,        // index routine
	SV_SectorCreateElems,    // creation routine
	SV_SectorFinaliseElems,  // finalisation routine

	NULL,     // pointer to known array
	0         // loaded size
};


//----------------------------------------------------------------------------
//
//  HUB STRUCTURE
//
static hub_info_c sv_dummy_hub;

#define SV_F_BASE  sv_dummy_hub

static savefield_t sv_fields_hub[] =
{
	SF(index, "index", 1, SVT_INT, SR_GetInt, SR_PutInt),

	SF(map_name, "map_name", 1, SVT_STRING, SR_HubGetMap, SR_HubPutMap),

	SVFIELD_END
};

savestruct_t sv_struct_hub =
{
	NULL,              // link in list
	"hub_info_c",      // structure name
	"dhub",            // start marker
	sv_fields_hub,     // field descriptions
	SVDUMMY,           // dummy base
	true,              // define_me
	NULL               // pointer to known struct
};

#undef SV_F_BASE

savearray_t sv_array_hub =
{
	NULL,               // link in list
	"active_hubs",      // array name
	&sv_struct_hub,     // array type
	true,               // define_me
	false,              // allow_hub

	SV_HubCountElems,     // count routine
	SV_HubGetElem,        // index routine
	SV_HubCreateElems,    // creation routine
	SV_HubFinaliseElems,  // finalisation routine

	NULL,     // pointer to known array
	0         // loaded size
};


//----------------------------------------------------------------------------

int SV_SideCountElems(void)
{
	return numsides;
}

void *SV_SideGetElem(int index)
{
	if (index < 0 || index >= numsides)
	{
		I_Warning("LOADGAME: Invalid Side: %d\n", index);
		index = 0;
	}

	return sides + index;
}

int SV_SideFindElem(side_t *elem)
{
	SYS_ASSERT(sides <= elem && elem < (sides + numsides));

	return elem - sides;
}

void SV_SideCreateElems(int num_elems)
{
	/* nothing much to do -- sides created from level load, and defaults
	* are initialised there.
	*/

	if (num_elems != numsides)
		I_Error("LOADGAME: SIDE MISMATCH !  (%d != %d)\n",
		num_elems, numsides);
}

void SV_SideFinaliseElems(void)
{
	/* nothing to do */
}


//----------------------------------------------------------------------------

extern std::list<line_t *> active_line_anims;
extern std::vector<slider_move_t *> active_sliders;

int SV_LineCountElems(void)
{
	return numlines;
}

void *SV_LineGetElem(int index)
{
	if (index < 0 || index >= numlines)
	{
		I_Warning("LOADGAME: Invalid Line: %d\n", index);
		index = 0;
	}

	return lines + index;
}

int SV_LineFindElem(line_t *elem)
{
	SYS_ASSERT(lines <= elem && elem < (lines + numlines));

	return elem - lines;
}

void SV_LineCreateElems(int num_elems)
{
	// nothing much to do -- lines are created from level load,
	// and defaults are initialised there.

	if (num_elems != numlines)
		I_Error("LOADGAME: LINE MISMATCH !  (%d != %d)\n",
			num_elems, numlines);

	// clear animate list
	active_line_anims.clear();
}

//
// NOTE: line gaps done in Sector finaliser.
//
void SV_LineFinaliseElems(void)
{
	for (int i = 0; i < numlines; i++)
	{
		line_t *ld = lines + i;
		side_t *s1, *s2;

		s1 = ld->side[0];
		s2 = ld->side[1];

		// check for animation
		if (s1 && (s1->top.scroll.x || s1->top.scroll.y ||
			s1->middle.scroll.x || s1->middle.scroll.y ||
			s1->bottom.scroll.x || s1->bottom.scroll.y))
		{
			P_AddSpecialLine(ld);
		}

		if (s2 && (s2->top.scroll.x || s2->top.scroll.y ||
			s2->middle.scroll.x || s2->middle.scroll.y ||
			s2->bottom.scroll.x || s2->bottom.scroll.y))
		{
			P_AddSpecialLine(ld);
		}
	}

	// scan active parts, regenerate slider_move field
	std::vector<slider_move_t *>::iterator SMI;

	for (SMI  = active_sliders.begin();
		 SMI != active_sliders.end();
		 SMI++)
	{
		SYS_ASSERT((*SMI)->line);

		(*SMI)->line->slider_move = (*SMI);
	}
}


//----------------------------------------------------------------------------

int SV_ExfloorCountElems(void)
{
	return numextrafloors;
}

void *SV_ExfloorGetElem(int index)
{
	if (index < 0 || index >= numextrafloors)
	{
		I_Warning("LOADGAME: Invalid Extrafloor: %d\n", index);
		index = 0;
	}

	return extrafloors + index;
}

int SV_ExfloorFindElem(extrafloor_t *elem)
{
	SYS_ASSERT(extrafloors <= elem && elem < (extrafloors + numextrafloors));

	return elem - extrafloors;
}

void SV_ExfloorCreateElems(int num_elems)
{
	/* nothing much to do -- extrafloors are created from level load, and
	* defaults are initialised there.
	*/

	if (num_elems != numextrafloors)
		I_Error("LOADGAME: Extrafloor MISMATCH !  (%d != %d)\n",
		num_elems, numextrafloors);
}

void SV_ExfloorFinaliseElems(void)
{
	int i;

	// need to regenerate the ef_info fields
	for (i=0; i < numextrafloors; i++)
	{
		extrafloor_t *ef = extrafloors + i;

		// skip unused extrafloors
		if (ef->ef_line == NULL)
			continue;

		if (!ef->ef_line->special ||
			!(ef->ef_line->special->ef.type & EXFL_Present))
		{
			I_Warning("LOADGAME: Missing Extrafloor Special !\n");
			ef->ef_info = &linetypes.Lookup(0)->ef;
			continue;
		}

		ef->ef_info = &ef->ef_line->special->ef;
	}
}


//----------------------------------------------------------------------------

extern std::list<sector_t *> active_sector_anims;
extern std::vector<plane_move_t *>  active_planes;

int SV_SectorCountElems(void)
{
	return numsectors;
}

void *SV_SectorGetElem(int index)
{
	if (index < 0 || index >= numsectors)
	{
		I_Warning("LOADGAME: Invalid Sector: %d\n", index);
		index = 0;
	}

	return sectors + index;
}

int SV_SectorFindElem(sector_t *elem)
{
	SYS_ASSERT(sectors <= elem && elem < (sectors + numsectors));

	return elem - sectors;
}

void SV_SectorCreateElems(int num_elems)
{
	// nothing much to do -- sectors are created from level load,
	// and defaults are initialised there.

	if (num_elems != numsectors)
		I_Error("LOADGAME: SECTOR MISMATCH !  (%d != %d)\n",
			num_elems, numsectors);

	// clear animate list
	active_sector_anims.clear();
}

void SV_SectorFinaliseElems(void)
{
	for (int i = 0; i < numsectors; i++)
	{
		sector_t *sec = sectors + i;

		P_RecomputeGapsAroundSector(sec);
///---	P_RecomputeTilesInSector(sec);
		P_FloodExtraFloors(sec);

		// check for animation
		if (sec->floor.scroll.x || sec->floor.scroll.y ||
			sec->ceil.scroll.x  || sec->ceil.scroll.y)
		{
			P_AddSpecialSector(sec);
		}

		// fix 'type' field for older save-games
		if (savegame_version < 0x12903)
		{
			sec->props.type = sec->props.special ?
				atoi(sec->props.special->name.c_str()) : 0;
		}
	}

	// scan active parts, regenerate floor_move and ceil_move
	std::vector<plane_move_t *>::iterator PMI;

	for (PMI  = active_planes.begin();
		 PMI != active_planes.end();
		 PMI++)
	{
		plane_move_t * pmov = *PMI;

		SYS_ASSERT(pmov->sector);

		if (pmov->is_ceiling)
			pmov->sector->ceil_move = pmov;
		else
			pmov->sector->floor_move = pmov;
	}
}


//----------------------------------------------------------------------------

extern std::vector<hub_info_c *> active_hubs;

int SV_HubCountElems(void)
{
	return (int)active_hubs.size();
}

void *SV_HubGetElem(int index)
{
	if (index < 0 || index >= SV_HubCountElems())
	{
		I_Warning("LOADGAME: Invalid Hub: %d\n", index);
		index = 0;
	}

	return active_hubs[index];
}

int SV_HubFindElem(hub_info_c *elem)
{
	int index = 0;

	std::vector<hub_info_c *>::iterator HI;

	for (HI  = active_hubs.begin();
         HI != active_hubs.end() && (*HI) != elem;
         HI++)
	{
		index++;
	}

	if (HI == active_hubs.end())
		I_Error("LOADGAME: No such HubPtr: %p\n", elem);

	return index;
}

void SV_HubCreateElems(int num_elems)
{
	HUB_DestroyAll();

	for (; num_elems > 0; num_elems--)
	{
		active_hubs.push_back(new hub_info_c(-1, "DUMMY"));
	}
}

void SV_HubFinaliseElems(void)
{
	// just a verification pass
	for (unsigned int j = 0; j < active_hubs.size(); j++)
	{
		hub_info_c *H = active_hubs[j];

		if (!H || H->index < 0)
			I_Error("Failure loading HUB information!");
	}
}


//----------------------------------------------------------------------------

bool SR_LevelGetSurface(void *storage, int index, void *extra)
{
	surface_t *dest = (surface_t *)storage + index;

	if (! sv_struct_surface.counterpart)
		return true;

	return SV_LoadStruct(dest, sv_struct_surface.counterpart);
}

void SR_LevelPutSurface(void *storage, int index, void *extra)
{
	surface_t *src = (surface_t *)storage + index;

	SV_SaveStruct(src, &sv_struct_surface);
}


bool SR_LevelGetSurfPtr(void *storage, int index, void *extra)
{
	surface_t ** dest = (surface_t **)storage + index;

	const char *str;
	int num;

	str = SV_GetString();

	if (! str)
	{
		(*dest) = NULL;
		return true;
	}

	if (str[1] != ':')
		I_Error("SR_LevelGetSurfPtr: invalid surface string `%s'\n", str);

	num = strtol(str+2, NULL, 0);

	if (num < 0 || num >= numsectors)
	{
		I_Warning("SR_LevelGetSurfPtr: bad sector ref %d\n", num);
		num = 0;
	}

	if (str[0] == 'F')
		(*dest) = &sectors[num].floor;
	else if (str[0] == 'C')
		(*dest) = &sectors[num].ceil;
	else
		I_Error("SR_LevelGetSurfPtr: invalid surface plane `%s'\n", str);

	SV_FreeString(str);
	return true;
}

//
// Format of the string:
//
//    <floor/ceil>  `:'  <sector num>
// 
// The first character is `F' for the floor surface of the sector,
// otherwise `C' for its ceiling.
//
void SR_LevelPutSurfPtr(void *storage, int index, void *extra)
{
	surface_t *src = ((surface_t **)storage)[index];

	char buffer[64];
	int i;

	if (! src)
	{
		SV_PutString(NULL);
		return;
	}

	// not optimal, but safe
	for (i=0; i < numsectors; i++)
	{
		if (src == &sectors[i].floor)
		{
			sprintf(buffer, "F:%d", i);
			SV_PutString(buffer);
			return;
		}
		else if (src == &sectors[i].ceil)
		{
			sprintf(buffer, "C:%d", i);
			SV_PutString(buffer);
			return;
		}
	}

	I_Warning("SR_LevelPutSurfPtr: surface %p not found !\n", src);
	SV_PutString("F:0");
}


bool SR_LevelGetImage(void *storage, int index, void *extra)
{
	const image_c ** dest = (const image_c **)storage + index;
	const char *str;

	str = SV_GetString();

	if (! str)
	{
		(*dest) = NULL;
		return true;
	}

	if (str[1] != ':')
		I_Warning("SR_LevelGetImage: invalid image string `%s'\n", str);

	(*dest) = W_ImageParseSaveString(str[0], str + 2);

	SV_FreeString(str);
	return true;
}

//
// Format of the string is:
//
//   <type char>  `:'  <name>
//
// The type character is `F' for flat, `T' for texture, etc etc..
// Also `*' is valid and means that type is not important.  Some
// examples: "F:FLAT10" and "T:STARTAN3".
//
void SR_LevelPutImage(void *storage, int index, void *extra)
{
	const image_c *src = ((const image_c **)storage)[index];

	char buffer[64];

	if (! src)
	{
		SV_PutString(NULL);
		return;
	}

	W_ImageMakeSaveString(src, buffer, buffer + 2);
	buffer[1] = ':';

	SV_PutString(buffer);
}


bool SR_LevelGetColmap(void *storage, int index, void *extra)
{
	const colourmap_c ** dest = (const colourmap_c **)storage + index;
	const char *str;

	str = SV_GetString();

	if (str)
		(*dest) = colourmaps.Lookup(str);
	else
		(*dest) = NULL;

	// -AJA- 2008/03/15: backwards compatibility
	if (*dest && stricmp((*dest)->name.c_str(), "NORMAL") == 0)
		*dest = NULL;

	SV_FreeString(str);
	return true;
}

//
// The string is the name of the colourmap.
//
void SR_LevelPutColmap(void *storage, int index, void *extra)
{
	const colourmap_c *src = ((const colourmap_c **)storage)[index];

	if (src)
		SV_PutString(src->name.c_str());
	else
		SV_PutString(NULL);
}


bool SR_LineGetSpecial(void *storage, int index, void *extra)
{
	const linetype_c ** dest = (const linetype_c **)storage + index;
	const char *str;

	str = SV_GetString();

	if (! str)
	{
		(*dest) = NULL;
		return true;
	}

	if (str[0] != ':')
		I_Error("SR_LineGetSpecial: invalid special `%s'\n", str);

	(*dest) = P_LookupLineType(strtol(str+1, NULL, 0));

	SV_FreeString(str);
	return true;
}

//
// Format of the string will usually be a colon followed by the
// linedef number (e.g. ":123").  Alternatively it can be the ddf
// name, but this shouldn't be needed currently (reserved for future
// use).
//
void SR_LinePutSpecial(void *storage, int index, void *extra)
{
	const linetype_c *src = ((const linetype_c **)storage)[index];

	if (! src)
	{
		SV_PutString(NULL);
		return;
	}

	std::string s = epi::STR_Format(":%s", src->name.c_str());

	SV_PutString(s.c_str());
}


bool SR_SectorGetSpecial(void *storage, int index, void *extra)
{
	const sectortype_c ** dest = (const sectortype_c **)storage + index;
	const char *str;

	str = SV_GetString();

	if (! str)
	{
		(*dest) = NULL;
		return true;
	}

	if (str[0] != ':')
		I_Error("SR_SectorGetSpecial: invalid special `%s'\n", str);

	(*dest) = P_LookupSectorType(strtol(str+1, NULL, 0));

	SV_FreeString(str);
	return true;
}

//
// Format of the string will usually be a colon followed by the
// sector number (e.g. ":123").  Alternatively it can be the ddf
// name, but this shouldn't be needed currently (reserved for future
// use).
//
void SR_SectorPutSpecial(void *storage, int index, void *extra)
{
	const sectortype_c *src = ((const sectortype_c **)storage)[index];

	if (! src)
	{
		SV_PutString(NULL);
		return;
	}

	std::string s = epi::STR_Format(":%s", src->name.c_str());

	SV_PutString(s.c_str());
}


//----------------------------------------------------------------------------

bool SR_SectorGetProps(void *storage, int index, void *extra)
{
	region_properties_t *dest = (region_properties_t *)storage + index;

	if (! sv_struct_regprops.counterpart)
		return true;

	return SV_LoadStruct(dest, sv_struct_regprops.counterpart);
}

void SR_SectorPutProps(void *storage, int index, void *extra)
{
	region_properties_t *src = (region_properties_t *)storage + index;

	SV_SaveStruct(src, &sv_struct_regprops);
}


bool SR_SectorGetPropRef(void *storage, int index, void *extra)
{
	region_properties_t ** dest = (region_properties_t **)storage + index;

	const char *str;
	int num;

	str = SV_GetString();

	if (! str)
	{
		(*dest) = NULL;
		return true;
	}

	num = strtol(str, NULL, 0);

	if (num < 0 || num >= numsectors)
	{
		I_Warning("SR_SectorGetPropRef: bad sector ref %d\n", num);
		num = 0;
	}

	(*dest) = &sectors[num].props;

	SV_FreeString(str);
	return true;
}

//
// Format of the string is just the sector number containing the
// properties.
//
void SR_SectorPutPropRef(void *storage, int index, void *extra)
{
	region_properties_t *src = ((region_properties_t **)storage)[index];

	char buffer[64];
	int i;

	if (! src)
	{
		SV_PutString(NULL);
		return;
	}

	// not optimal, but safe
	for (i=0; i < numsectors; i++)
	{
		if (&sectors[i].props == src)
			break;
	}

	if (i >= numsectors)
	{
		I_Warning("SR_SectorPutPropRef: properties %p not found !\n", src);
		i = 0;
	}

	sprintf(buffer, "%d", i);
	SV_PutString(buffer);
}


bool SR_LineGetLine(void *storage, int index, void *extra)
{
	line_t ** dest = (line_t **)storage + index;

	int swizzle = SV_GetInt();

	*dest = (line_t*)((swizzle == 0) ? NULL : SV_LineGetElem(swizzle - 1));
	return true;
}

void SR_LinePutLine(void *storage, int index, void *extra)
{
	line_t *elem = ((line_t **)storage)[index];

	int swizzle = (elem == NULL) ? 0 : SV_LineFindElem(elem) + 1;

	SV_PutInt(swizzle);
}


bool SR_SectorGetSector(void *storage, int index, void *extra)
{
	sector_t ** dest = (sector_t **)storage + index;

	int swizzle = SV_GetInt();

	*dest = (sector_t*)((swizzle == 0) ? NULL : SV_SectorGetElem(swizzle - 1));
	return true;
}

void SR_SectorPutSector(void *storage, int index, void *extra)
{
	sector_t *elem = ((sector_t **)storage)[index];

	int swizzle = (elem == NULL) ? 0 : SV_SectorFindElem(elem) + 1;

	SV_PutInt(swizzle);
}

bool SR_SectorGetEF(void *storage, int index, void *extra)
{
	extrafloor_t ** dest = (extrafloor_t **)storage + index;

	int swizzle = SV_GetInt();

	*dest = (extrafloor_t*)((swizzle == 0) ? NULL : SV_ExfloorGetElem(swizzle - 1));
	return true;
}

void SR_SectorPutEF(void *storage, int index, void *extra)
{
	extrafloor_t *elem = ((extrafloor_t **)storage)[index];

	int swizzle = (elem == NULL) ? 0 : SV_ExfloorFindElem(elem) + 1;

	SV_PutInt(swizzle);
}


//----------------------------------------------------------------------------

bool SR_HubGetMap(void *storage, int index, void *extra)
{
	const char ** dest = (const char **)storage + index;

	// the HUB code uses SV_DupString/SV_FreeString for the
	// map_name field, hence is 100% compatible with savegame
	// usage of strings.

	if (*dest)
		SV_FreeString(*dest);

	(*dest) = SV_GetString();

	return true;
}

void SR_HubPutMap(void *storage, int index, void *extra)
{
	const char *src = ((const char **)storage)[index];

	SV_PutString(src);
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
