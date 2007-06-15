//----------------------------------------------------------------------------
//  EDGE New SaveGame Handling (Level Data)
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2005  The EDGE Team.
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

#include "ddf_colm.h"
#include "sv_chunk.h"
#include "sv_main.h"
#include "z_zone.h"

#include "epi/strings.h"

#include <stdio.h>
#include <stdlib.h>

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

  SF(override_p, "override_p", 1, SVT_STRING, 
      SR_SectorGetPropRef, SR_SectorPutPropRef),

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
  SF(top, "top", 1, SVT_STRUCT("surface_t"), 
      SR_LevelGetSurface, SR_LevelPutSurface),
  SF(middle, "middle", 1, SVT_STRUCT("surface_t"), 
      SR_LevelGetSurface, SR_LevelPutSurface),
  SF(bottom, "bottom", 1, SVT_STRUCT("surface_t"), 
      SR_LevelGetSurface, SR_LevelPutSurface),

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

	// NOT HERE:
	//   (many): values are kept from level load.
	//   gap stuff: regenerated from sector heights.
	//   validcount: only a temporary value for some routines.
	//   slider_move: regenerated by a pass of the active part list.
	//   animate_next: regenerated by testing stuff.

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
	SF(colourmap,  "colourmap", 1, SVT_STRING, SR_LevelGetColmap, 
		SR_LevelPutColmap),

	SF(type, "type", 1, SVT_INT, SR_GetInt, SR_PutInt),
	SF(special, "special", 1, SVT_STRING, SR_SectorGetSpecial, 
		SR_SectorPutSpecial),

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
  SF(higher, "higher", 1, SVT_INDEX("extrafloors"),
      SR_SectorGetEF, SR_SectorPutEF),
  SF(lower, "lower", 1, SVT_INDEX("extrafloors"),
      SR_SectorGetEF, SR_SectorPutEF),
	SF(sector, "sector", 1, SVT_INDEX("sectors"), 
	SR_SectorGetSector, SR_SectorPutSector),

	SF(top_h, "top_h", 1, SVT_FLOAT, SR_GetFloat, SR_PutFloat),
	SF(bottom_h, "bottom_h", 1, SVT_FLOAT, SR_GetFloat, SR_PutFloat),
  SF(top, "top", 1, SVT_STRING, 
      SR_LevelGetSurfPtr, SR_LevelPutSurfPtr),
  SF(bottom, "bottom", 1, SVT_STRING, 
      SR_LevelGetSurfPtr, SR_LevelPutSurfPtr),
	SF(p, "p", 1, SVT_STRING, SR_SectorGetPropRef, SR_SectorPutPropRef),

  SF(ef_line, "ef_line", 1, SVT_INDEX("lines"), 
      SR_LineGetLine, SR_LinePutLine),
  SF(ctrl_next, "ctrl_next", 1, SVT_INDEX("extrafloors"),
      SR_SectorGetEF, SR_SectorPutEF),

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
  SF(floor, "floor", 1, SVT_STRUCT("surface_t"), 
      SR_LevelGetSurface, SR_LevelPutSurface),
  SF(ceil, "ceil", 1, SVT_STRUCT("surface_t"), 
      SR_LevelGetSurface, SR_LevelPutSurface),
	SF(f_h, "f_h", 1, SVT_FLOAT, SR_GetFloat, SR_PutFloat),
	SF(c_h, "c_h", 1, SVT_FLOAT, SR_GetFloat, SR_PutFloat),

  SF(props, "props", 1, SVT_STRUCT("region_properties_t"), 
      SR_SectorGetProps, SR_SectorPutProps),
	SF(p, "p", 1, SVT_STRING, SR_SectorGetPropRef, SR_SectorPutPropRef),

	SF(exfloor_used, "exfloor_used", 1, SVT_INT, SR_GetInt, SR_PutInt),
  SF(control_floors, "control_floors", 1, SVT_INDEX("extrafloors"),
      SR_SectorGetEF, SR_SectorPutEF),
	SF(sound_player, "sound_player", 1, SVT_INT, SR_GetInt, SR_PutInt),

  SF(bottom_ef, "bottom_ef", 1, SVT_INDEX("extrafloors"),
      SR_SectorGetEF, SR_SectorPutEF),
  SF(top_ef, "top_ef", 1, SVT_INDEX("extrafloors"),
      SR_SectorGetEF, SR_SectorPutEF),
  SF(bottom_liq, "bottom_liq", 1, SVT_INDEX("extrafloors"),
      SR_SectorGetEF, SR_SectorPutEF),
  SF(top_liq, "top_liq", 1, SVT_INDEX("extrafloors"),
      SR_SectorGetEF, SR_SectorPutEF),

	// NOT HERE:
	//   - floor_move, ceil_move: can be regenerated
	//   - (many): values remaining from level load are OK
	//   - soundtraversed & validcount: temp values, don't need saving
	//   - animate_next: regenerated by testing stuff.

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

	SV_SectorCountElems,     // count routine
	SV_SectorGetElem,        // index routine
	SV_SectorCreateElems,    // creation routine
	SV_SectorFinaliseElems,  // finalisation routine

	NULL,     // pointer to known array
	0         // loaded size
};


//----------------------------------------------------------------------------

//
// SV_SideCountElems
//
int SV_SideCountElems(void)
{
	return numsides;
}

//
// SV_SideGetElem
//
void *SV_SideGetElem(int index)
{
	if (index < 0 || index >= numsides)
	{
		I_Warning("LOADGAME: Invalid Side: %d\n", index);
		index = 0;
	}

	return sides + index;
}

//
// SV_SideFindElem
//
int SV_SideFindElem(side_t *elem)
{
	SYS_ASSERT(sides <= elem && elem < (sides + numsides));

	return elem - sides;
}

//
// SV_SideCreateElems
//
void SV_SideCreateElems(int num_elems)
{
	/* nothing much to do -- sides created from level load, and defaults
	* are initialised there.
	*/

	if (num_elems != numsides)
		I_Error("LOADGAME: SIDE MISMATCH !  (%d != %d)\n",
		num_elems, numsides);
}

//
// SV_SideFinaliseElems
//
void SV_SideFinaliseElems(void)
{
	/* nothing to do */
}


//----------------------------------------------------------------------------

//
// SV_LineCountElems
//
int SV_LineCountElems(void)
{
	return numlines;
}

//
// SV_LineGetElem
//
void *SV_LineGetElem(int index)
{
	if (index < 0 || index >= numlines)
	{
		I_Warning("LOADGAME: Invalid Line: %d\n", index);
		index = 0;
	}

	return lines + index;
}

//
// SV_LineFindElem
//
int SV_LineFindElem(line_t *elem)
{
	SYS_ASSERT(lines <= elem && elem < (lines + numlines));

	return elem - lines;
}

//
// SV_LineCreateElems
//
void SV_LineCreateElems(int num_elems)
{
	/* nothing much to do -- lines are created from level load, and
	* defaults are initialised there.
	*/

	if (num_elems != numlines)
		I_Error("LOADGAME: LINE MISMATCH !  (%d != %d)\n",
		num_elems, numlines);

	// clear animate list
	line_speciallist = NULL;
}

//
// SV_LineFinaliseElems
//
// NOTE: line gaps done in Sector finaliser.
//
void SV_LineFinaliseElems(void)
{
	int i;

	gen_move_t *gen;
	slider_move_t *smov;

	// clear animate list
	line_speciallist = NULL;

	for (i=0; i < numlines; i++)
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
	for (gen = active_movparts; gen; gen = gen->next)
	{
		if (gen->whatiam != MDT_SLIDER)
			continue;

		smov = (slider_move_t *)gen;
		SYS_ASSERT(smov->line);

		smov->line->slider_move = smov;
	}
}


//----------------------------------------------------------------------------

//
// SV_ExfloorCountElems
//
int SV_ExfloorCountElems(void)
{
	return numextrafloors;
}

//
// SV_ExfloorGetElem
//
void *SV_ExfloorGetElem(int index)
{
	if (index < 0 || index >= numextrafloors)
	{
		I_Warning("LOADGAME: Invalid Extrafloor: %d\n", index);
		index = 0;
	}

	return extrafloors + index;
}

//
// SV_ExfloorFindElem
//
int SV_ExfloorFindElem(extrafloor_t *elem)
{
	SYS_ASSERT(extrafloors <= elem && elem < (extrafloors + numextrafloors));

	return elem - extrafloors;
}

//
// SV_ExfloorCreateElems
//
void SV_ExfloorCreateElems(int num_elems)
{
	/* nothing much to do -- extrafloors are created from level load, and
	* defaults are initialised there.
	*/

	if (num_elems != numextrafloors)
		I_Error("LOADGAME: Extrafloor MISMATCH !  (%d != %d)\n",
		num_elems, numextrafloors);
}

//
// SV_ExfloorFinaliseElems
//
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
			ef->ef_info = &linetypes[0]->ef;
			continue;
		}

		ef->ef_info = &ef->ef_line->special->ef;
	}
}


//----------------------------------------------------------------------------

//
// SV_SectorCountElems
//
int SV_SectorCountElems(void)
{
	return numsectors;
}

//
// SV_SectorGetElem
//
void *SV_SectorGetElem(int index)
{
	if (index < 0 || index >= numsectors)
	{
		I_Warning("LOADGAME: Invalid Sector: %d\n", index);
		index = 0;
	}

	return sectors + index;
}

//
// SV_SectorFindElem
//
int SV_SectorFindElem(sector_t *elem)
{
	SYS_ASSERT(sectors <= elem && elem < (sectors + numsectors));

	return elem - sectors;
}

//
// SV_SectorCreateElems
//
void SV_SectorCreateElems(int num_elems)
{
	// nothing much to do -- sectors are created from level load,
	// and defaults are initialised there.

	if (num_elems != numsectors)
		I_Error("LOADGAME: SECTOR MISMATCH !  (%d != %d)\n",
		num_elems, numsectors);
}

//
// SV_SectorFinaliseElems
//
void SV_SectorFinaliseElems(void)
{
	int i;

	gen_move_t *gen;
	plane_move_t *pmov;

	// clear animate list
	sect_speciallist = NULL;

	for (i=0; i < numsectors; i++)
	{
		sector_t *sec = sectors + i;

		P_RecomputeGapsAroundSector(sec);
		P_RecomputeTilesInSector(sec);
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
				sec->props.special->ddf.number : 0;
		}
	}

	// scan active parts, regenerate floor_move and ceil_move
	for (gen = active_movparts; gen; gen = gen->next)
	{
		switch (gen->whatiam)
		{
		case MDT_PLANE:
			pmov = (plane_move_t *)gen;
			SYS_ASSERT(pmov->sector);

			if (pmov->is_ceiling)
				pmov->sector->ceil_move = gen;
			else
				pmov->sector->floor_move = gen;
			break;

		default:
			break;
		}
	}
}


//----------------------------------------------------------------------------

//
// SR_LevelGetSurface
//
bool SR_LevelGetSurface(void *storage, int index, void *extra)
{
	surface_t *dest = (surface_t *)storage + index;

	if (! sv_struct_surface.counterpart)
		return true;

	return SV_LoadStruct(dest, sv_struct_surface.counterpart);
}

//
// SR_LevelPutSurface
//
void SR_LevelPutSurface(void *storage, int index, void *extra)
{
	surface_t *src = (surface_t *)storage + index;

	SV_SaveStruct(src, &sv_struct_surface);
}


//
// SR_LevelGetSurfPtr
//
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

	Z_Free((char *)str);
	return true;
}

//
// SR_LevelPutSurfPtr
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


//
// SR_LevelGetImage
//
bool SR_LevelGetImage(void *storage, int index, void *extra)
{
	const image_t ** dest = (const image_t **)storage + index;
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

	Z_Free((char *)str);
	return true;
}

//
// SR_LevelPutImage
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
	const image_t *src = ((const image_t **)storage)[index];

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


//
// SR_LevelGetColmap
//
bool SR_LevelGetColmap(void *storage, int index, void *extra)
{
	const colourmap_c ** dest = (const colourmap_c **)storage + index;
	const char *str;

	str = SV_GetString();

	if (! str)
		I_Error("SR_LevelGetColmap: NULL found !\n");

	(*dest) = colourmaps.Lookup(str);

	Z_Free((char *)str);
	return true;
}

//
// SR_LevelPutColmap
//
// The string is the name of the colourmap.  NULL strings are not
// allowed or used.
//
void SR_LevelPutColmap(void *storage, int index, void *extra)
{
	const colourmap_c *src = ((const colourmap_c **)storage)[index];

	SYS_ASSERT(src);

	SV_PutString(src->ddf.name);
}


//
// SR_LineGetSpecial
//
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

	Z_Free((char *)str);
	return true;
}

//
// SR_LinePutSpecial
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

	epi::string_c s;
	s.Format(":%d", src->ddf.number);
	SV_PutString(s.GetString());
}


//
// SR_SectorGetSpecial
//
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

	Z_Free((char *)str);
	return true;
}

//
// SR_SectorPutSpecial
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

	epi::string_c s;
	s.Format(":%d", src->ddf.number);
	SV_PutString(s.GetString());
}


//----------------------------------------------------------------------------

//
// SR_SectorGetProps
//
bool SR_SectorGetProps(void *storage, int index, void *extra)
{
	region_properties_t *dest = (region_properties_t *)storage + index;

	if (! sv_struct_regprops.counterpart)
		return true;

	return SV_LoadStruct(dest, sv_struct_regprops.counterpart);
}

//
// SR_SectorPutProps
//
void SR_SectorPutProps(void *storage, int index, void *extra)
{
	region_properties_t *src = (region_properties_t *)storage + index;

	SV_SaveStruct(src, &sv_struct_regprops);
}


//
// SR_SectorGetPropRef
//
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

	Z_Free((char *)str);
	return true;
}

//
// SR_SectorPutPropRef
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


//
// SR_LineGetLine
//
bool SR_LineGetLine(void *storage, int index, void *extra)
{
	line_t ** dest = (line_t **)storage + index;

	int swizzle = SV_GetInt();

	*dest = (line_t*)((swizzle == 0) ? NULL : SV_LineGetElem(swizzle - 1));
	return true;
}

//
// SR_LinePutLine
//
void SR_LinePutLine(void *storage, int index, void *extra)
{
	line_t *elem = ((line_t **)storage)[index];

	int swizzle = (elem == NULL) ? 0 : SV_LineFindElem(elem) + 1;

	SV_PutInt(swizzle);
}


//
// SR_SectorGetSector
//
bool SR_SectorGetSector(void *storage, int index, void *extra)
{
	sector_t ** dest = (sector_t **)storage + index;

	int swizzle = SV_GetInt();

	*dest = (sector_t*)((swizzle == 0) ? NULL : SV_SectorGetElem(swizzle - 1));
	return true;
}

//
// SR_SectorPutSector
//
void SR_SectorPutSector(void *storage, int index, void *extra)
{
	sector_t *elem = ((sector_t **)storage)[index];

	int swizzle = (elem == NULL) ? 0 : SV_SectorFindElem(elem) + 1;

	SV_PutInt(swizzle);
}

//
// SR_SectorGetEF
//
bool SR_SectorGetEF(void *storage, int index, void *extra)
{
	extrafloor_t ** dest = (extrafloor_t **)storage + index;

	int swizzle = SV_GetInt();

	*dest = (extrafloor_t*)((swizzle == 0) ? NULL : SV_ExfloorGetElem(swizzle - 1));
	return true;
}

//
// SR_SectorPutEF
//
void SR_SectorPutEF(void *storage, int index, void *extra)
{
	extrafloor_t *elem = ((extrafloor_t **)storage)[index];

	int swizzle = (elem == NULL) ? 0 : SV_ExfloorFindElem(elem) + 1;

	SV_PutInt(swizzle);
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
