//----------------------------------------------------------------------------
//  EDGE Data Definition File Code (Sounds)
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
// -KM- 1998/09/27 Finished :-)
//

#include "i_defs.h"

#include "dm_defs.h"
#include "dm_type.h"
#include "ddf_locl.h"
#include "m_random.h"
#include "s_sound.h"
#include "z_zone.h"

#undef  DF
#define DF  DDF_CMD

static sfxinfo_t buffer_sfx;
static sfxinfo_t *dynamic_sfx;

static const sfxinfo_t template_sfx =
{
	DDF_BASE_NIL,  // ddf

	"",     // lump_name
	{ 1, { 0 }}, // normal
	0,      // singularity
	999,    // priority (lower is more important)
	PERCENT_MAKE(100), // volume
	false,  // looping
	false,  // precious
	S_CLIPPING_DIST, // max_distance
	NULL,   // cache next
	NULL    // cache prev
};

sfxinfo_t ** S_sfx;
int numsfx = 0;
int num_disabled_sfx = 0;

static stack_array_t S_sfx_a;


#undef  DDF_CMD_BASE
#define DDF_CMD_BASE  buffer_sfx

static const commandlist_t sfx_commands[] =
{
	DF("LUMP NAME", lump_name, DDF_MainGetInlineStr10),
	DF("SINGULAR", singularity, DDF_MainGetNumeric),
	DF("PRIORITY", priority, DDF_MainGetNumeric),
	DF("VOLUME", volume, DDF_MainGetPercent),
	DF("LOOP", looping, DDF_MainGetBoolean),
	DF("PRECIOUS", precious, DDF_MainGetBoolean),
	DF("MAX DISTANCE", max_distance, DDF_MainGetFloat),

	// -AJA- backwards compatibility cruft...
	DF("!BITS",   ddf, DDF_DummyFunction),
	DF("!STEREO", ddf, DDF_DummyFunction),

	DDF_CMD_END
};

// -KM- 1998/10/29 Done sfx_t first so structure is aligned.
bastard_sfx_t bastard_sfx[] =
{
    {0, "swtchn"},
    {0, "tink"},
    {0, "radio"},
    {0, "oof"},
    {0, "pstop"},
    {0, "stnmov"},
    {0, "pistol"},
    {0, "swtchx"},
    {0, "jpmove"},
    {0, "jpidle"},
    {0, "jprise"},
    {0, "jpdown"},
    {0, "jpflow"}
};


//
//  DDF PARSE ROUTINES
//

static bool SoundStartEntry(const char *name)
{
	int i = -1;
	bool replaces = false;

	if (name && name[0])
	{
		for (i=num_disabled_sfx; i < numsfx; i++)
		{
			if (DDF_CompareName(S_sfx[i]->ddf.name, name) == 0)
			{
				dynamic_sfx = S_sfx[i];
				replaces = true;
				break;
			}
		}

		// NOTE: we normally adjust the array so that the newest entries
		// are at the end.  We *don't* do that here because we'd have to
		// update all normal.sound[0] references.
	}

	// not found, create a new one
	if (! replaces)
	{
		Z_SetArraySize(&S_sfx_a, ++numsfx);

		i = numsfx - 1;

		dynamic_sfx = S_sfx[i];
		dynamic_sfx->ddf.name = (name && name[0]) ? Z_StrDup(name) :
			DDF_MainCreateUniqueName("UNNAMED_SOUND", numsfx);
	}

	DEV_ASSERT2(i >= 0);

	dynamic_sfx->ddf.number = 0;

	// instantiate the static entry
	buffer_sfx = template_sfx;

	buffer_sfx.normal.num = 1;        // self reference
	buffer_sfx.normal.sounds[0] = i;  //

	return replaces;
}

static void SoundParseField(const char *field, const char *contents,
    int index, bool is_last)
{
#if (DEBUG_DDF)  
	L_WriteDebug("SOUND_PARSE: %s = %s;\n", field, contents);
#endif

	if (! DDF_MainParseField(sfx_commands, field, contents))
		DDF_WarnError2(0x128, "Unknown sounds.ddf command: %s\n", field);
}

static void SoundFinishEntry(void)
{
	ddf_base_t base;

	if (!buffer_sfx.lump_name[0])
		DDF_Error("Missing LUMP_NAME for sound.\n");

	// transfer static entry to dynamic entry
	// Keeps the ID info intact as well.

	base = dynamic_sfx->ddf;
	dynamic_sfx[0] = buffer_sfx;
	dynamic_sfx->ddf = base;

	// Compute CRC.  In this case, there is no need, since sounds have
	// zero impact on the game simulation itself.
	dynamic_sfx->ddf.crc = 0;
}

static void SoundClearAll(void)
{
	// not safe to delete entries -- mark them disabled

	num_disabled_sfx = numsfx;
}

void DDF_ReadSFX(void *data, int size)
{
	readinfo_t sfx_r;

	sfx_r.memfile = (char*)data;
	sfx_r.memsize = size;
	sfx_r.tag = "SOUNDS";
	sfx_r.entries_per_dot = 8;

	if (sfx_r.memfile)
	{
		sfx_r.message = NULL;
		sfx_r.filename = NULL;
		sfx_r.lumpname = "DDFSFX";
	}
	else
	{
		sfx_r.message = "DDF_InitSounds";
		sfx_r.filename = "sounds.ddf";
		sfx_r.lumpname = NULL;
	}

	sfx_r.start_entry  = SoundStartEntry;
	sfx_r.parse_field  = SoundParseField;
	sfx_r.finish_entry = SoundFinishEntry;
	sfx_r.clear_all    = SoundClearAll;

	DDF_MainReadFile(&sfx_r);
}

void DDF_SFXInit(void)
{
	Z_InitStackArray(&S_sfx_a, (void ***)&S_sfx, sizeof(sfxinfo_t), 0);

	// create the null sfx
	Z_SetArraySize(&S_sfx_a, numsfx = 1);
	S_sfx[numsfx-1][0] = template_sfx;
}

void DDF_SFXCleanUp(void)
{
	int i;

	for (i = sizeof(bastard_sfx) / sizeof(bastard_sfx_t); i--; )
		bastard_sfx[i].s = DDF_SfxLookupSound(bastard_sfx[i].name);
}

//
// DDF_SfxLookupSound
//
// -ACB- 1999/11/06:
//   Fixed from using zero-length array style malloc - Not ANSI Compliant.
//   Renamed DDF_SfxLookupSound to destingush from the one in DDF_MAIN.C.
//
// -AJA- 2001/11/03: 
//   Modified to count matches first, then allocate the memory.
//   Prevents memory fragmentation.
//
sfx_t *DDF_SfxLookupSound(const char *name, bool error)
{
	int i, count;
	int last_id = -1;

	sfx_t *r;

	// NULL Sound
	if (!name || !name[0])
		return NULL;

	// count them
	for (count=0, i=numsfx-1; i >= num_disabled_sfx; i--)
	{
		if (strncasecmpwild(name, S_sfx[i]->ddf.name, 8) == 0)
		{
			count++;
			last_id = i;
		}
	}

	if (count == 0)
	{
		if (!lax_errors && !error)
			DDF_Error("Unknown SFX: '%.8s'\n", name);

		return NULL;
	}

	// -AJA- optimisation to save some memory
	if (count == 1)
	{
		sfx_t *r = & (S_sfx[last_id]->normal);

		DEV_ASSERT2(r->num == 1);

		return r;
	}

	// allocate elements.  Uses (count-1) since sfx_t already includes
	// the first integer.
	// 
	r = (sfx_t *) Z_New(byte, sizeof(sfx_t) + (count-1) * sizeof(int));

	// now store them
	for (r->num=0, i=numsfx-1; i >= num_disabled_sfx; i--)
	{
		if (strncasecmpwild(name, S_sfx[i]->ddf.name, 8) == 0)
			r->sounds[r->num++] = i;
	}

	DEV_ASSERT2(r->num == count);

	return r;
}

//
// DDF_MainLookupSound
//
// Lookup the sound specified.
//
// -ACB- 1998/07/08 Checked the S_sfx table for sfx names.
// -ACB- 1998/07/18 Removed to the need set *currentcmdlist[commandref].data to -1
// -KM- 1998/09/27 Fixed this func because of sounds.ddf
// -KM- 1998/10/29 sfx_t finished
//
void DDF_MainLookupSound(const char *info, void *storage)
{
	sfx_t **dest = (sfx_t **)storage;

	DEV_ASSERT2(info && storage);

	*dest = DDF_SfxLookupSound(info);
}

//
// DDF_SfxSelect
// 
// Choose one of the sounds in sfx_t.  Mainly here for data hiding,
// keeping numsfx and S_sfx out of other code.
// 
sfxinfo_t *DDF_SfxSelect(const sfx_t *sound_id)
{
	int snd_num;

	DEV_ASSERT2(sound_id->num >= 1);

	// -KM- 1999/01/31 Using P_Random here means demos and net games 
	//  get out of sync.
	// -AJA- 1999/07/19: That's why we use M_Random instead :).

	snd_num = sound_id->sounds[M_Random() % sound_id->num];

	DEV_ASSERT2(0 <= snd_num && snd_num < numsfx);

	return S_sfx[snd_num];
}
