//----------------------------------------------------------------------------
//  EDGE Data Definition Files Code (Main)
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

#include "local.h"

#include <limits.h>
#include <vector>

#include "epi/path.h"
#include "epi/str_format.h"

#include "colormap.h"
#include "attack.h"
#include "thing.h"


extern int engine_version;


void DDF_Init(int _engine_ver)
{
	engine_version = _engine_ver;

	DDF_StateInit();
	DDF_LanguageInit();
	DDF_SFXInit();
	DDF_ColmapInit();
	DDF_ImageInit();
	DDF_FontInit();
	DDF_AttackInit(); 
	DDF_WeaponInit();
	DDF_MobjInit();
	DDF_LinedefInit();
	DDF_SectorInit();
	DDF_SwitchInit();
	DDF_AnimInit();
	DDF_GameInit();
	DDF_LevelInit();
	DDF_MusicPlaylistInit();
}


//
// This goes through the information loaded via DDF and matchs any
// info stored as references.
//
void DDF_CleanUp(void)
{
	DDF_LanguageCleanUp();
	DDF_ImageCleanUp();
	DDF_FontCleanUp();
	DDF_MobjCleanUp();
	DDF_AttackCleanUp();
	DDF_StateCleanUp();
	DDF_LinedefCleanUp();
	DDF_SFXCleanUp();
	DDF_ColmapCleanUp();
	DDF_WeaponCleanUp();
	DDF_SectorCleanUp();
	DDF_SwitchCleanUp();
	DDF_AnimCleanUp();
	DDF_GameCleanUp();
	DDF_LevelCleanUp();
	DDF_MusicPlaylistCleanUp();
}



// DDF OBJECTS

// ---> mobj_strref class

const mobjtype_c *mobj_strref_c::GetRef()
{
	if (def)
		return def;

	def = mobjtypes.Lookup(name.c_str());

	return def;
}

// ---> damage class

//
// damage_c Constructor
//
damage_c::damage_c()
{
}

//
// damage_c Copy constructor
//
damage_c::damage_c(damage_c &rhs)
{
	Copy(rhs);
}

//
// damage_c Destructor
//
damage_c::~damage_c()
{
}

//
// damage_c::Copy
//
void damage_c::Copy(damage_c &src)
{
	nominal = src.nominal;
	linear_max = src.linear_max;
	error = src.error;
	delay = src.delay;

	obituary = src.obituary;
	pain = src.pain;
	death = src.death;
	overkill = src.overkill;
	
	no_armour = src.no_armour;
}

//
// damage_c::Default
//
void damage_c::Default(damage_c::default_e def)
{
	obituary.clear();

	switch (def)
	{
		case DEFAULT_MobjChoke:
		{
			nominal	= 6.0f;	
			linear_max = 14.0f;	
			error = -1.0f;
			delay = 2 * TICRATE;
			obituary = "OB_DROWN";
			no_armour = true;
			break;
		}

		case DEFAULT_Sector:
		{
			nominal = 0.0f;
			linear_max = -1.0f;
			error = -1.0f;
			delay = 31;
			no_armour = false;
			break;
		}
		
		case DEFAULT_Attack:
		case DEFAULT_Mobj:
		default:
		{
			nominal = 0.0f;
			linear_max = -1.0f;     
			error = -1.0f;     
			delay = 0;      
			no_armour = false;
			break;
		}
	}

	pain.Default();
	death.Default();
	overkill.Default();
}

//
// damage_c assignment operator
//
damage_c& damage_c::operator=(damage_c &rhs)
{
	if (&rhs != this)
		Copy(rhs);
		
	return *this;
}

// ---> label offset class

//
// label_offset_c Constructor
//
label_offset_c::label_offset_c()
{
	offset = 0;
}

//
// label_offset_c Copy constructor
//
label_offset_c::label_offset_c(label_offset_c &rhs)
{
	Copy(rhs);
}

//
// label_offset_c Destructor
//
label_offset_c::~label_offset_c()
{
}

//
// label_offset_c::Copy
//
void label_offset_c::Copy(label_offset_c &src)
{
	label = src.label;
	offset = src.offset;
}

//
// label_offset_c::Default
//
void label_offset_c::Default()
{
	label.clear();
	offset = 0;
}


//
// label_offset_c assignment operator
//
label_offset_c& label_offset_c::operator=(label_offset_c& rhs)
{
	if (&rhs != this)
		Copy(rhs);
		
	return *this;
}

// ---> dlight_info class

dlight_info_c::dlight_info_c()
{
	Default();
}

dlight_info_c::dlight_info_c(dlight_info_c &rhs)
{
	Copy(rhs);
}

void dlight_info_c::Copy(dlight_info_c &src)
{
	type   = src.type;
	shape  = src.shape;
	radius = src.radius;
	colour = src.colour;
	height = src.height;
	leaky  = src.leaky;

	cache_data = NULL;
}

void dlight_info_c::Default()
{
	type   = DLITE_None;
	radius = 32;
	colour = RGB_MAKE(255, 255, 255);
	height = PERCENT_MAKE(50);
	leaky  = false;

	shape.Set("DLIGHT_EXP");

	cache_data = NULL;
}

dlight_info_c& dlight_info_c::operator= (dlight_info_c &rhs)
{
	CHECK_SELF_ASSIGN(rhs);

	Copy(rhs);

	return *this;
}

// ---> weakness_info class

weakness_info_c::weakness_info_c()
{
	Default();
}

weakness_info_c::weakness_info_c(weakness_info_c &rhs)
{
	Copy(rhs);
}

void weakness_info_c::Copy(weakness_info_c &src)
{
	height[0] = src.height[0];
	height[1] = src.height[1];
	angle[0]  = src.angle[0];
	angle[1]  = src.angle[1];

	classes    = src.classes;
	multiply   = src.multiply;
	painchance = src.painchance;
}

void weakness_info_c::Default()
{
	height[0] = PERCENT_MAKE(  0);
	height[1] = PERCENT_MAKE(100);

	angle[0] = ANG0;
	angle[1] = ANG_MAX;

	classes   = BITSET_EMPTY;
	multiply  = 2.5;
	painchance = -1; // disabled
}

weakness_info_c& weakness_info_c::operator=(weakness_info_c &rhs)
{
	CHECK_SELF_ASSIGN(rhs);

	Copy(rhs);

	return *this;
}

float DDF_Tan(float degrees)
{
	degrees = CLAMP(-89.5f, degrees, 89.5f);

	return (float) tan(degrees * M_PI / 180.0f);
}



//----------------------------------------------------------------------------


void DDF_DummyFunction(const char *info, void *storage)
{
	/* does nothing */
}


//
// Get numeric value directly from the file
//
void DDF_MainGetNumeric(const char *info, void *storage)
{
	int *dest = (int *)storage;

	SYS_ASSERT(info && storage);

	if (isalpha(info[0]))
	{
		DDF_WarnError2(128, "Bad numeric value: %s\n", info);
		return;
	}

	// -KM- 1999/01/29 strtol accepts hex and decimal.
	*dest = strtol(info, NULL, 0);  // straight conversion - no messin'
}

//
// Get true/false from the file
//
// -KM- 1998/09/01 Gets a true/false value
//
void DDF_MainGetBoolean(const char *info, void *storage)
{
	bool *dest = (bool *)storage;

	SYS_ASSERT(info && storage);

	if ((stricmp(info, "TRUE") == 0) || (strcmp(info, "1") == 0))
	{
		*dest = true;
		return;
	}

	if ((stricmp(info, "FALSE") == 0) || (strcmp(info, "0") == 0))
	{
		*dest = false;
		return;
	}

	DDF_Error("Bad boolean value: %s\n", info);
}

//
// Get String value directly from the file
//
// -KM- 1998/07/31 Needed a string argument.  Based on DDF_MainGetNumeric.
// -AJA- 2000/02/09: Free any existing string (potential memory leak).
// -ACB- 2004/07/26: Use epi::strent_c
//
void DDF_MainGetString(const char *info, void *storage)
{
	epi::strent_c *dest = (epi::strent_c *)storage;

	SYS_ASSERT(info && storage);

	dest->Set(info);
}


void DDF_MainGetLumpName(const char *info, void *storage)
{
	// Gets the string and checks the length is valid for a lump.

	SYS_ASSERT(info && storage);

	lumpname_c *LN = (lumpname_c *)storage;

	if (strlen(info) == 9)
		DDF_WarnError2(131, "Name %s too long (should be 8 characters or less)\n", info);
	else if (strlen(info) > 9)
		DDF_Error("Name %s too long (must be 8 characters or less)\n", info);

	LN->Set(info);
}


void DDF_MainRefAttack(const char *info, void *storage)
{
	atkdef_c **dest = (atkdef_c **)storage;

	SYS_ASSERT(info && storage);

	*dest = (atkdef_c*)atkdefs.Lookup(info);
	if (*dest == NULL)
		DDF_WarnError2(128, "Unknown Attack: %s\n", info);
}


int DDF_MainLookupDirector(const mobjtype_c *info, const char *ref)
{
	const char *p = strchr(ref, ':');

	int len = p ? (p - ref) : strlen(ref);

	if (len <= 0)
		DDF_Error("Bad Director '%s' : Nothing after divide\n", ref);

	std::string director(ref, len);

	int index = DDF_StateFindLabel(info->states, director.c_str());

	if (p)
		index += MAX(0, atoi(p + 1) - 1);

	if (index >= (int)info->states.size())
	{
		DDF_Warning("Bad Director '%s' : offset is too large\n", ref);
		index = MAX(0, (int)info->states.size() - 1);
	}

	return index;
}




void DDF_MainGetFloat(const char *info, void *storage)
{
	float *dest = (float *)storage;

	SYS_ASSERT(info && storage);

	if (sscanf(info, "%f", dest) != 1)
		DDF_Error("Bad floating point value: %s\n", info);
}

// -AJA- 1999/09/11: Added DDF_MainGetAngle and DDF_MainGetSlope.

void DDF_MainGetAngle(const char *info, void *storage)
{
	SYS_ASSERT(info && storage);

	angle_t *dest = (angle_t *)storage;

	float val;

	if (sscanf(info, "%f", &val) != 1)
		DDF_Error("Bad angle value: %s\n", info);

	if ((int) val == 360)
		val = 359.5;
	else if (val > 360.0f)
		DDF_WarnError2(129, "Angle '%s' too large (must be less than 360)\n", info);

	*dest = FLOAT_2_ANG(val);
}

void DDF_MainGetSlope(const char *info, void *storage)
{
	float val;
	float *dest = (float *)storage;

	SYS_ASSERT(info && storage);

	if (sscanf(info, "%f", &val) != 1)
		DDF_Error("Bad slope value: %s\n", info);

	*dest = DDF_Tan(val);
}

//
// DDF_MainGetPercent
//
// Reads percentages (0%..100%).
//
void DDF_MainGetPercent(const char *info, void *storage)
{
	percent_t *dest = (percent_t *)storage;
	char s[101];
	char *p;
	float f;

	// check that the string is valid
	Z_StrNCpy(s, info, 100);
	for (p = s; isdigit(*p) || *p == '.'; p++)
	{ /* do nothing */ }

	// the number must be followed by %
	if (*p != '%')
	{
		DDF_WarnError2(128, "Bad percent value '%s': Should be a number followed by %%\n", info);
		// -AJA- 2001/01/27: backwards compatibility
		DDF_MainGetFloat(s, &f);
		*dest = MAX(0, MIN(1, f));
		return;
	}

	*p = 0;
  
	DDF_MainGetFloat(s, &f);
	if (f < 0.0f || f > 100.0f)
		DDF_Error("Bad percent value '%s': Must be between 0%% and 100%%\n", s);

	*dest = f / 100.0f;
}

//
// DDF_MainGetPercentAny
//
// Like the above routine, but allows percentages outside of the
// 0-100% range (which is useful in same instances).
//
void DDF_MainGetPercentAny(const char *info, void *storage)
{
	percent_t *dest = (percent_t *)storage;
	char s[101];
	char *p;
	float f;

	// check that the string is valid
	Z_StrNCpy(s, info, 100);
	for (p = s; isdigit(*p) || *p == '.'; p++)
	{ /* do nothing */ }

	// the number must be followed by %
	if (*p != '%')
	{
		DDF_WarnError2(128, "Bad percent value '%s': Should be a number followed by %%\n", info);
		// -AJA- 2001/01/27: backwards compatibility
		DDF_MainGetFloat(s, dest);
		return;
	}

	*p = 0;
  
	DDF_MainGetFloat(s, &f);

	*dest = f / 100.0f;
}

// -KM- 1998/09/27 You can end a number with T to specify tics; ie 35T
// means 35 tics while 3.5 means 3.5 seconds.

void DDF_MainGetTime(const char *info, void *storage)
{
	float val;
	int *dest = (int *)storage;

	SYS_ASSERT(info && storage);

	// -ES- 1999/09/14 MAXT means that time should be maximal.
	if (!stricmp(info, "maxt"))
	{
		*dest = INT_MAX; // -ACB- 1999/09/22 Standards, Please.
		return;
	}

	if (strchr(info, 'T'))
	{
		DDF_MainGetNumeric(info, storage);
		return;
	}

	if (sscanf(info, "%f", &val) != 1)
		DDF_Error("Bad time value: %s\n", info);

	*dest = (int)(val * (float)TICRATE);
}


void DDF_MainGetColourmap(const char *info, void *storage)
{
	const colourmap_c **result = (const colourmap_c **)storage;

	*result = colourmaps.Lookup(info);
	if (*result == NULL)
		DDF_Error("DDF_MainGetColourmap: No such colourmap '%s'\n", info);
	
}


void DDF_MainGetRGB(const char *info, void *storage)
{
	rgbcol_t *result = (rgbcol_t *)storage;
	int r, g, b;

	SYS_ASSERT(info && storage);

	if (DDF_CompareName(info, "NONE") == 0)
	{
		*result = RGB_NO_VALUE;
		return;
	}

	if (sscanf(info, " #%2x%2x%2x ", &r, &g, &b) != 3)
		DDF_Error("Bad RGB colour value: %s\n", info);

	*result = (r << 16) | (g << 8) | b;

	// silently change if matches the "none specified" value
	if (*result == RGB_NO_VALUE)
		*result ^= RGB_MAKE(1,1,1);
}

//
// WhenAppear
//
// Syntax:  [ '!' ]  [ SKILL ]  ':'  [ NETMODE ]
//
// SKILL = digit { ':' digit }  |  digit '-' digit.
// NETMODE = 'sp'  |  'coop'  |  'dm'.
//
// When no skill was specified, it's as though all were specified.
// Same for the netmode.
//
// -AJA- 2004/10/28: Dodgy-est crap ever, now with ranges and negation.
//
void DDF_MainGetWhenAppear(const char *info, void *storage)
{
	when_appear_e *result = (when_appear_e *)storage;

	*result = WNAP_None;

	bool negate = (info[0] == '!');

	const char *range = strstr(info, "-");

	if (range)
	{
		if (range <= info   || range[+1] == 0  ||
			range[-1] < '1' || range[-1] > '5' ||
			range[+1] < '1' || range[+1] > '5' ||
			range[-1] > range[+1])
		{
			DDF_Error("Bad range in WHEN_APPEAR value: %s\n", info);
			return;
		}

		for (char sk = '1'; sk <= '5'; sk++)
			if (range[-1] <= sk && sk <= range[+1])
				*result = (when_appear_e)(*result | (WNAP_SkillLevel1 << (sk - '1')));
	}
	else
	{
		if (strstr(info, "1"))
			*result = (when_appear_e)(*result | WNAP_SkillLevel1);

		if (strstr(info, "2"))
			*result = (when_appear_e)(*result | WNAP_SkillLevel2);

		if (strstr(info, "3"))
			*result = (when_appear_e)(*result | WNAP_SkillLevel3);

		if (strstr(info, "4"))
			*result = (when_appear_e)(*result | WNAP_SkillLevel4);

		if (strstr(info, "5"))
			*result = (when_appear_e)(*result | WNAP_SkillLevel5);
	}

	if (strstr(info, "SP") || strstr(info, "sp"))
		*result = (when_appear_e)(*result| WNAP_Single);

	if (strstr(info, "COOP") || strstr(info, "coop"))
		*result = (when_appear_e)(*result | WNAP_Coop);

	if (strstr(info, "DM") || strstr(info, "dm"))
		*result = (when_appear_e)(*result | WNAP_DeathMatch);

	// allow more human readable strings...

	if (negate)
		*result = (when_appear_e)(*result ^ (WNAP_SkillBits | WNAP_NetBits));

	if ((*result & WNAP_SkillBits) == 0)
		*result = (when_appear_e)(*result | WNAP_SkillBits);

	if ((*result & WNAP_NetBits) == 0)
		*result = (when_appear_e)(*result | WNAP_NetBits);
}

#if 0  // DEBUGGING ONLY
void Test_ParseWhenAppear(void)
{
	when_appear_e val;

	DDF_MainGetWhenAppear("1", &val);  printf("WNAP '1' --> 0x%04x\n", val);
	DDF_MainGetWhenAppear("3", &val);  printf("WNAP '3' --> 0x%04x\n", val);
	DDF_MainGetWhenAppear("5", &val);  printf("WNAP '5' --> 0x%04x\n", val);
	DDF_MainGetWhenAppear("7", &val);  printf("WNAP '7' --> 0x%04x\n", val);

	DDF_MainGetWhenAppear("1:2", &val);  printf("WNAP '1:2' --> 0x%04x\n", val);
	DDF_MainGetWhenAppear("5:3:1", &val);  printf("WNAP '5:3:1' --> 0x%04x\n", val);

	DDF_MainGetWhenAppear("1-3", &val);  printf("WNAP '1-3' --> 0x%04x\n", val);
	DDF_MainGetWhenAppear("4-5", &val);  printf("WNAP '4-5' --> 0x%04x\n", val);
	DDF_MainGetWhenAppear("0-2", &val);  printf("WNAP '0-2' --> 0x%04x\n", val);
	DDF_MainGetWhenAppear("3-6", &val);  printf("WNAP '3-6' --> 0x%04x\n", val);
	DDF_MainGetWhenAppear("5-1", &val);  printf("WNAP '5-1' --> 0x%04x\n", val);

	DDF_MainGetWhenAppear("sp", &val);  printf("WNAP 'sp' --> 0x%04x\n", val);
	DDF_MainGetWhenAppear("coop", &val);  printf("WNAP 'coop' --> 0x%04x\n", val);
	DDF_MainGetWhenAppear("dm", &val);  printf("WNAP 'dm' --> 0x%04x\n", val);
	DDF_MainGetWhenAppear("sp:coop", &val);  printf("WNAP 'sp:coop' --> 0x%04x\n", val);
	DDF_MainGetWhenAppear("sp:dm", &val);  printf("WNAP 'sp:dm' --> 0x%04x\n", val);
	DDF_MainGetWhenAppear("sp:coop:dm", &val);  printf("WNAP 'sp:coop:dm' --> 0x%04x\n", val);

	DDF_MainGetWhenAppear("sp:3", &val);  printf("WNAP 'sp:3' --> 0x%04x\n", val);
	DDF_MainGetWhenAppear("sp:3:4", &val);  printf("WNAP 'sp:3:4' --> 0x%04x\n", val);
	DDF_MainGetWhenAppear("sp:3-5", &val);  printf("WNAP 'sp:3-5' --> 0x%04x\n", val);

	DDF_MainGetWhenAppear("sp:dm:3", &val);  printf("WNAP 'sp:dm:3' --> 0x%04x\n", val);
	DDF_MainGetWhenAppear("sp:dm:3:4", &val);  printf("WNAP 'sp:dm:3:4' --> 0x%04x\n", val);
	DDF_MainGetWhenAppear("sp:dm:3-5", &val);  printf("WNAP 'sp:dm:3-5' --> 0x%04x\n", val);

	DDF_MainGetWhenAppear("DM:COOP:SP:3", &val);  printf("WNAP 'DM:COOP:SP:3' --> 0x%04x\n", val);
	DDF_MainGetWhenAppear("DM:COOP:SP:3:4", &val);  printf("WNAP 'DM:COOP:SP:3:4' --> 0x%04x\n", val);
	DDF_MainGetWhenAppear("DM:COOP:SP:3-5", &val);  printf("WNAP 'DM:COOP:SP:3-5' --> 0x%04x\n", val);

	printf("------------------------------------------------------------\n");

	DDF_MainGetWhenAppear("!1", &val);  printf("WNAP '!1' --> 0x%04x\n", val);
	DDF_MainGetWhenAppear("!3", &val);  printf("WNAP '!3' --> 0x%04x\n", val);
	DDF_MainGetWhenAppear("!5", &val);  printf("WNAP '!5' --> 0x%04x\n", val);
	DDF_MainGetWhenAppear("!7", &val);  printf("WNAP '!7' --> 0x%04x\n", val);

	DDF_MainGetWhenAppear("!1:2", &val);  printf("WNAP '!1:2' --> 0x%04x\n", val);
	DDF_MainGetWhenAppear("!5:3:1", &val);  printf("WNAP '!5:3:1' --> 0x%04x\n", val);

	DDF_MainGetWhenAppear("!1-3", &val);  printf("WNAP '!1-3' --> 0x%04x\n", val);
	DDF_MainGetWhenAppear("!4-5", &val);  printf("WNAP '!4-5' --> 0x%04x\n", val);
	DDF_MainGetWhenAppear("!0-2", &val);  printf("WNAP '!0-2' --> 0x%04x\n", val);
	DDF_MainGetWhenAppear("!3-6", &val);  printf("WNAP '!3-6' --> 0x%04x\n", val);
	DDF_MainGetWhenAppear("!5-1", &val);  printf("WNAP '!5-1' --> 0x%04x\n", val);

	DDF_MainGetWhenAppear("!sp", &val);  printf("WNAP '!sp' --> 0x%04x\n", val);
	DDF_MainGetWhenAppear("!coop", &val);  printf("WNAP '!coop' --> 0x%04x\n", val);
	DDF_MainGetWhenAppear("!dm", &val);  printf("WNAP '!dm' --> 0x%04x\n", val);
	DDF_MainGetWhenAppear("!sp:coop", &val);  printf("WNAP '!sp:coop' --> 0x%04x\n", val);
	DDF_MainGetWhenAppear("!sp:dm", &val);  printf("WNAP '!sp:dm' --> 0x%04x\n", val);
	DDF_MainGetWhenAppear("!sp:coop:dm", &val);  printf("WNAP '!sp:coop:dm' --> 0x%04x\n", val);

	DDF_MainGetWhenAppear("!sp:3", &val);  printf("WNAP '!sp:3' --> 0x%04x\n", val);
	DDF_MainGetWhenAppear("!sp:3:4", &val);  printf("WNAP '!sp:3:4' --> 0x%04x\n", val);
	DDF_MainGetWhenAppear("!sp:3-5", &val);  printf("WNAP '!sp:3-5' --> 0x%04x\n", val);

	DDF_MainGetWhenAppear("!sp:dm:3", &val);  printf("WNAP '!sp:dm:3' --> 0x%04x\n", val);
	DDF_MainGetWhenAppear("!sp:dm:3:4", &val);  printf("WNAP '!sp:dm:3:4' --> 0x%04x\n", val);
	DDF_MainGetWhenAppear("!sp:dm:3-5", &val);  printf("WNAP '!sp:dm:3-5' --> 0x%04x\n", val);

	DDF_MainGetWhenAppear("!DM:COOP:SP:3", &val);  printf("WNAP '!DM:COOP:SP:3' --> 0x%04x\n", val);
	DDF_MainGetWhenAppear("!DM:COOP:SP:3:4", &val);  printf("WNAP '!DM:COOP:SP:3:4' --> 0x%04x\n", val);
	DDF_MainGetWhenAppear("!DM:COOP:SP:3-5", &val);  printf("WNAP '!DM:COOP:SP:3-5' --> 0x%04x\n", val);
}
#endif


void DDF_MainGetBitSet(const char *info, void *storage)
{
	bitset_t *result = (bitset_t *)storage;
	int start, end;

	SYS_ASSERT(info && storage);

	// allow a numeric value
	if (sscanf(info, " %i ", result) == 1)
		return;

	*result = BITSET_EMPTY;

	for (; *info; info++)
	{
		if (*info < 'A' || *info > 'Z')
			continue;
    
		start = end = (*info) - 'A';

		// handle ranges
		if (info[1] == '-' && 'A' <= info[2] && info[2] <= 'Z' &&
			info[2] >= info[0])
		{
			end = info[2] - 'A';
		}

		for (; start <= end; start++)
			(*result) |= (1 << start);
	}
}


static int FindSpecialFlag(const char *prefix, const char *name,
						   const specflags_t *flag_set)
{
	int i;
	char try_name[512];

	for (i=0; flag_set[i].name; i++)
	{
		const char *current = flag_set[i].name;
		bool obsolete = false;

		if (current[0] == '!')
		{
			current++;
			obsolete = true;
		}
    
		sprintf(try_name, "%s%s", prefix, current);
    
		if (DDF_CompareName(name, try_name) == 0)
		{
			if (obsolete)
				DDF_Obsolete("The ddf flag `%s' is obsolete !\n", try_name);

			return i;
		}
	}

	return -1;
}

checkflag_result_e DDF_MainCheckSpecialFlag(const char *name,
							 const specflags_t *flag_set, int *flag_value, 
							 bool allow_prefixes, bool allow_user)
{
	int index;
	int negate = 0;
	int user = 0;

	// try plain name...
	index = FindSpecialFlag("", name, flag_set);
  
	if (allow_prefixes)
	{
		// try name with ENABLE_ prefix...
		if (index == -1)
		{
			index = FindSpecialFlag("ENABLE_", name, flag_set);
		}

		// try name with NO_ prefix...
		if (index == -1)
		{
			negate = 1;
			index = FindSpecialFlag("NO_", name, flag_set);
		}

		// try name with NOT_ prefix...
		if (index == -1)
		{
			negate = 1;
			index = FindSpecialFlag("NOT_", name, flag_set);
		}

		// try name with DISABLE_ prefix...
		if (index == -1)
		{
			negate = 1;
			index = FindSpecialFlag("DISABLE_", name, flag_set);
		}

		// try name with USER_ prefix...
		if (index == -1 && allow_user)
		{
			user = 1;
			negate = 0;
			index = FindSpecialFlag("USER_", name, flag_set);
		}
	}

	if (index < 0)
		return CHKF_Unknown;

	(*flag_value) = flag_set[index].flags;

	if (flag_set[index].negative)
		negate = !negate;
  
	if (user)
		return CHKF_User;
  
	if (negate)
		return CHKF_Negative;

	return CHKF_Positive;
}

//
// Decode a keyword followed by something in () brackets.  Buf_len gives
// the maximum size of the output buffers.  The outer keyword is required
// to be non-empty, though the inside can be empty.  Returns false if
// cannot be parsed (e.g. no brackets).  Handles strings.
//
bool DDF_MainDecodeBrackets(const char *info, char *outer, char *inner,
	int buf_len)
{
	const char *pos = info;
	
	while (*pos && *pos != '(')
		pos++;
	
	if (*pos == 0 || pos == info)
		return false;
	
	if (pos - info >= buf_len)  // overflow
		return false;

	strncpy(outer, info, pos - info);
	outer[pos - info] = 0;

	pos++;  // skip the '('

	info = pos;

	bool in_string = false;

	while (*pos && (in_string || *pos != ')'))
	{
		// handle escaped quotes
		if (pos[0] == '\\' && pos[1] == '"')
		{
			pos += 2;
			continue;
		}

		if (*pos == '"')
			in_string = ! in_string;

		pos++;
	}

	if (*pos == 0)
		return false;

	if (pos - info >= buf_len)  // overflow
		return false;

	strncpy(inner, info, pos - info);
	inner[pos - info] = 0;

	return true;
}

//
// DDF_MainDecodeList
//
// Find the dividing character.  Returns NULL if not found.
// Handles strings and brackets unless simple is true.
//
const char *DDF_MainDecodeList(const char *info, char divider, bool simple)
{
	int  brackets  = 0;
	bool in_string = false;

	const char *pos = info;

	for (;;)
	{
		if (*pos == 0)
			break;

		if (brackets == 0 && !in_string && *pos == divider)
			return pos;

		// handle escaped quotes
		if (! simple)
		{
			if (pos[0] == '\\' && pos[1] == '"')
			{
				pos += 2;
				continue;
			}

			if (*pos == '"')
				in_string = ! in_string;

			if (!in_string && *pos == '(')
				brackets++;

			if (!in_string && *pos == ')')
			{
				brackets--;
				if (brackets < 0)
					DDF_Error("Too many ')' found: %s\n", info);
			}
		}

		pos++;
	}

	if (in_string)
		DDF_Error("Unterminated string found: %s\n", info);

	if (brackets != 0)
		DDF_Error("Unclosed brackets found: %s\n", info);

	return NULL;
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
