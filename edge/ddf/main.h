//----------------------------------------------------------------------------
//  EDGE Data Definition File Code (Main)
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

#ifndef __DDF_MAIN_H__
#define __DDF_MAIN_H__

#include "epi/file.h"
#include "epi/utility.h"

#include "types.h"

#define DEBUG_DDF  0

// Forward declarations
struct condition_check_s;
struct sfx_s;
struct state_s;

struct mobj_s;  // engine class

class animdef_c;
class atkdef_c;
class colourmap_c;
class dlight_info_c;
class donutdef_c;
class extrafloordef_c;
class fontpatch_c;
class fontdef_c;
class gamedef_c;
class imagedef_c;
class lightdef_c;
class linetype_c;
class mapdef_c;
class map_finaledef_c;

class mobjtype_c;
class movplanedef_c;
class pickup_effect_c;
class pl_entry_c;
class sectortype_c;
class sfxdef_c;
class sliding_door_c;
class switchdef_c;
class teleportdef_c;
class weakness_info_c;
class weapondef_c;
class wi_animdef_c;


// State updates, number of tics / second.
#define TICRATE   35

// Misc playsim constants
#define CEILSPEED   		1.0f
#define FLOORSPEED  		1.0f

#define GRAVITY     		8.0f
#define FRICTION    		0.9063f
#define VISCOSITY   		0.0f
#define DRAG        		0.99f
#define RIDE_FRICTION    	0.7f


// ------------------------------------------------------------------
// -------------------------EXTERNALISATIONS-------------------------
// ------------------------------------------------------------------

// if true, prefer to crash out on various errors
extern bool strict_errors;

// if true, prefer to ignore or fudge various (serious) errors
extern bool lax_errors;

// if true, disable warning messages
extern bool no_warnings;

// if true, disable obsolete warning messages
extern bool no_obsoletes;

void DDF_Init(int _engine_ver);
void DDF_CleanUp(void);
void DDF_SetWhere(const std::string& dir);

bool DDF_MainParseCondition(const char *str, struct condition_check_s *cond);
void DDF_MainGetWhenAppear(const char *info, void *storage);
void DDF_MainGetRGB(const char *info, void *storage);
bool DDF_MainDecodeBrackets(const char *info, char *outer, char *inner, int buf_len);
const char *DDF_MainDecodeList(const char *info, char divider, bool simple);

int DDF_CompareName(const char *A, const char *B);

bool DDF_WeaponIsUpgrade(weapondef_c *weap, weapondef_c *old);

bool DDF_IsBoomLineType(int num);
bool DDF_IsBoomSectorType(int num);
void DDF_BoomClearGenTypes(void);
linetype_c *DDF_BoomGetGenLine(int number);
sectortype_c *DDF_BoomGetGenSector(int number);

void DDF_Parse(void *data, int length, const char *name);

#endif /* __DDF_MAIN_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
