//----------------------------------------------------------------------------
//  EDGE Data Definition File Code (Animated textures)
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
// Animated Texture/Flat Setup and Parser Code
//

#include "i_defs.h"

#include "ddf_locl.h"
#include "ddf_main.h"
#include "dm_state.h"
#include "p_spec.h"
#include "r_local.h"
#include "z_zone.h"

#undef  DF
#define DF  DDF_CMD

static animdef_t buffer_anim;
static animdef_t *dynamic_anim;

static const animdef_t template_anim =
{
	DDF_BASE_NIL,  // ddf

	true,  // istexture
	"",    // endname
	"",    // startname
	8,     // speed
};

static void DDF_AnimGetType(const char *info, void *storage);

// -ACB- 1998/08/10 Use DDF_MainGetLumpName for getting the..lump name.
// -KM- 1998/09/27 Use DDF_MainGetTime for getting tics

#define DDF_CMD_BASE  buffer_anim

static const commandlist_t anim_commands[] =
{
	DF("TYPE",   istexture, DDF_AnimGetType),
	DF("SPEED",  speed,     DDF_MainGetTime),
	DF("FIRST",  startname, DDF_MainGetInlineStr10),
	DF("LAST",   endname,   DDF_MainGetInlineStr10),

	DDF_CMD_END
};

// Floor/ceiling animation sequences, defined by first and last frame,
// i.e. the flat (64x64 tile) name or texture name to be used.
//
// The full animation sequence is given using all the flats between
// the start and end entry, in the order found in the WAD file.
//

// -ACB- 2004/06/03 Replaced array and size with purpose-build class
animdef_container_c animdefs;

//
//  DDF PARSE ROUTINES
//
static bool AnimStartEntry(const char *name)
{
	bool replaces = false;

	if (name && name[0])
	{
		epi::array_iterator_c it;
		animdef_t *a;

		for (it = animdefs.GetBaseIterator(); it.IsValid(); it++)
		{
			a = ITERATOR_TO_TYPE(it, animdef_t*);
			if (DDF_CompareName(a->ddf.name, name) == 0)
			{
				dynamic_anim = a;
				replaces = true;
				break;
			}
		}
	}

	// not found, create a new one
	if (! replaces)
	{
		dynamic_anim = new animdef_t;

		memset(dynamic_anim, 0, sizeof(animdef_t));

		dynamic_anim->ddf.name = (name && name[0]) ? Z_StrDup(name) :
			DDF_MainCreateUniqueName("UNNAMED_ANIM", animdefs.GetSize());

		animdefs.Insert(dynamic_anim);
	}

	dynamic_anim->ddf.number = 0;

	// instantiate the static entry
	buffer_anim = template_anim;
	return replaces;
}

static void AnimParseField(const char *field, const char *contents, int index, bool is_last)
{
#if (DEBUG_DDF)  
	L_WriteDebug("ANIM_PARSE: %s = %s;\n", field, contents);
#endif

	if (! DDF_MainParseField(anim_commands, field, contents))
		DDF_WarnError2(0x128, "Unknown anims.ddf command: %s\n", field);
}

static void AnimFinishEntry(void)
{
	ddf_base_t base;

	if (buffer_anim.speed <= 0)
	{
		DDF_WarnError2(0x128, "Bad TICS value for anim: %d\n", buffer_anim.speed);
		buffer_anim.speed = 8;
	}

	if (!buffer_anim.startname || !buffer_anim.startname[0])
		DDF_Error("Missing first name for anim.\n");

	if (!buffer_anim.startname || !buffer_anim.startname[0])
		DDF_Error("Missing last name for anim.\n");

	// transfer static entry to dynamic entry

	base = dynamic_anim->ddf;
	dynamic_anim[0] = buffer_anim;
	dynamic_anim->ddf = base;

	// Compute CRC.  In this case, there is no need, since animations
	// have zero impact on the game simulation.
	dynamic_anim->ddf.crc = 0;
}

static void AnimClearAll(void)
{
	// safe to just delete all animations
	animdefs.Clear();
}


void DDF_ReadAnims(void *data, int size)
{
	readinfo_t anims;

	anims.memfile = (char*)data;
	anims.memsize = size;
	anims.tag = "ANIMATIONS";
	anims.entries_per_dot = 2;

	if (anims.memfile)
	{
		anims.message = NULL;
		anims.filename = NULL;
		anims.lumpname = "DDFANIM";
	}
	else
	{
		anims.message = "DDF_InitAnimations";
		anims.filename = "anims.ddf";
		anims.lumpname = NULL;
	}

	anims.start_entry  = AnimStartEntry;
	anims.parse_field  = AnimParseField;
	anims.finish_entry = AnimFinishEntry;
	anims.clear_all    = AnimClearAll;

	DDF_MainReadFile(&anims);
}

void DDF_AnimInit(void)
{
	/* nothing to do */
}

void DDF_AnimCleanUp(void)
{
	animdefs.Trim();			// <-- Reduce to allocated size
}

//
// DDF_AnimGetType
//
static void DDF_AnimGetType(const char *info, void *storage)
{
	bool *is_tex = (bool *) storage;

	DEV_ASSERT2(storage);

	if (DDF_CompareName(info, "FLAT") == 0)
		(*is_tex) = false;
	else if (DDF_CompareName(info, "TEXTURE") == 0)
		(*is_tex) = true;
	else
	{
		DDF_WarnError2(0x128, "Unknown animation type: %s\n", info);
		(*is_tex) = false;
	}
}

// List Management
void animdef_container_c::CleanupObject(void *obj)
{
	animdef_t *a = *(animdef_t**)obj;

	if (a->ddf.name) { Z_Free(a->ddf.name); }

	return;
}

