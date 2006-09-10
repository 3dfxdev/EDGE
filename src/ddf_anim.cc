//----------------------------------------------------------------------------
//  EDGE Data Definition File Code (Animated textures)
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
// Animated Texture/Flat Setup and Parser Code
//

#include "i_defs.h"

#include <string.h>

#include "ddf_locl.h"
#include "ddf_main.h"
#include "ddf_anim.h"

#include "dm_state.h"
#include "p_spec.h"
#include "r_local.h"

#undef  DF
#define DF  DDF_CMD

static animdef_c buffer_anim;
static animdef_c *dynamic_anim;

static void DDF_AnimGetType(const char *info, void *storage);
static void DDF_AnimGetPic (const char *info, void *storage);

// -ACB- 1998/08/10 Use DDF_MainGetLumpName for getting the..lump name.
// -KM- 1998/09/27 Use DDF_MainGetTime for getting tics

#define DDF_CMD_BASE  buffer_anim

static const commandlist_t anim_commands[] =
{
	DF("TYPE",     type,      DDF_AnimGetType),
	DF("SEQUENCE", pics,      DDF_AnimGetPic),
	DF("SPEED",    speed,     DDF_MainGetTime),
	DF("FIRST",    startname, DDF_MainGetInlineStr10),
	DF("LAST",     endname,   DDF_MainGetInlineStr10),

	DDF_CMD_END
};

// Floor/ceiling animation sequences, defined by first and last frame,
// i.e. the flat (64x64 tile) name or texture name to be used.
//
// The full animation sequence is given using all the flats between
// the start and end entry, in the order found in the WAD file.
//

// -ACB- 2004/06/03 Replaced array and size with purpose-built class
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
		animdef_c *a;

		for (it = animdefs.GetBaseIterator(); it.IsValid(); it++)
		{
			a = ITERATOR_TO_TYPE(it, animdef_c*);
			if (DDF_CompareName(a->ddf.name.GetString(), name) == 0)
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
		dynamic_anim = new animdef_c;

		if (name && name[0])
			dynamic_anim->ddf.name.Set(name);
		else
			dynamic_anim->ddf.SetUniqueName("UNNAMED_ANIM", animdefs.GetSize());

		animdefs.Insert(dynamic_anim);
	}

	dynamic_anim->ddf.number = 0;

	// instantiate the static entry
	buffer_anim.Default();
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
	if (buffer_anim.speed <= 0)
	{
		DDF_WarnError2(0x128, "Bad TICS value for anim: %d\n", buffer_anim.speed);
		buffer_anim.speed = 8;
	}

	if (buffer_anim.pics.GetSize() == 0)
	{
		if (!buffer_anim.startname || !buffer_anim.startname[0] ||
		    !buffer_anim.endname   || !buffer_anim.endname[0])
		{
			DDF_Error("Missing animation sequence.\n");
		}

		if (buffer_anim.type == animdef_c::A_Graphic)
			DDF_Error("TYPE=GRAPHIC animations must use the SEQUENCE command.\n");
	}

	// transfer static entry to dynamic entry
	dynamic_anim->CopyDetail(buffer_anim);

	// Compute CRC.  In this case, there is no need, since animations
	// have zero impact on the game simulation.
	dynamic_anim->ddf.crc.Reset();
}

static void AnimClearAll(void)
{
	// safe to just delete all animations
	animdefs.Clear();
}


bool DDF_ReadAnims(void *data, int size)
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

	return DDF_MainReadFile(&anims);
}

//
// DDF_AnimInit
//
void DDF_AnimInit(void)
{
	animdefs.Clear();			// <-- Consistent with existing behaviour (-ACB- 2004/05/04)
}

//
// DDF_AnimCleanUp
//
void DDF_AnimCleanUp(void)
{
	animdefs.Trim();			// <-- Reduce to allocated size
}

//
// DDF_AnimGetType
//
static void DDF_AnimGetType(const char *info, void *storage)
{
	DEV_ASSERT2(storage);

	int *type = (int *) storage;

	if (DDF_CompareName(info, "FLAT") == 0)
		(*type) = animdef_c::A_Flat;
	else if (DDF_CompareName(info, "TEXTURE") == 0)
		(*type) = animdef_c::A_Texture;
	else if (DDF_CompareName(info, "GRAPHIC") == 0)
		(*type) = animdef_c::A_Graphic;
	else
	{
		DDF_WarnError2(0x128, "Unknown animation type: %s\n", info);
		(*type) = animdef_c::A_Flat;
	}
}

//
// DDF_AnimGetPic
//
static void DDF_AnimGetPic (const char *info, void *storage)
{
	buffer_anim.pics.Insert(info);
}

void DDF_ParseANIMATED(const byte *data, int size)
{
	for (; size >= 23; data += 23, size -= 23)
	{
		if (data[0] & 0x80)  // end marker
			break;

		int speed = data[19] + (data[20] << 8);

		char first[10];
		char  last[10];

		// make sure names are NUL-terminated
		memcpy(first, data+10, 9);  last[8] = 0;
		memcpy( last, data+ 1, 9); first[8] = 0;

		L_WriteDebug("- ANIMATED LUMP: start '%s' : end '%s'\n", first, last);

		// ignore zero-length names
		if (!first[0] || !last[0])
			continue;

		animdef_c *def = new animdef_c;

		def->ddf.SetUniqueName("BOOM_ANIM", animdefs.GetSize());
		def->ddf.number = 0;
		def->ddf.crc.Reset();

		def->Default();
		
		def->type = (data[0] & 1) ? animdef_c::A_Texture : animdef_c::A_Flat;
		def->speed = MAX(1, speed);

		def->startname.Set(first);
		def->endname  .Set(last);

		animdefs.Insert(def);
	}
}


// ---> animdef_c class

//
// animdef_c constructor
//
animdef_c::animdef_c()
{
	Default();
}

//
// animdef_c Copy constructor
//
animdef_c::animdef_c(animdef_c &rhs)
{
	Copy(rhs);
}

//
// animdef_c::Copy()
//
void animdef_c::Copy(animdef_c &src)
{
	ddf = src.ddf;
	CopyDetail(src);
}

//
// animdef_c::CopyDetail()
//
// Copies all the detail with the exception of ddf info
//
void animdef_c::CopyDetail(animdef_c &src)
{
	type = src.type;
	pics = src.pics;
	startname = src.startname;
	endname = src.endname;
	speed = src.speed;
}

//
// animdef_c::Default()
//
void animdef_c::Default()
{
	ddf.Default();

	type = A_Texture;

	pics.Clear();
	startname.Clear();
	endname.Clear();

	speed = 8;
}

//
// animdef_c assignment operator
//
animdef_c& animdef_c::operator=(animdef_c &rhs)
{
	if (&rhs != this)
		Copy(rhs);
	
	return *this;
}

// ---> animdef_container_c class

//
// animdef_container_c::CleanupObject()
//
void animdef_container_c::CleanupObject(void *obj)
{
	animdef_c *a = *(animdef_c**)obj;

	if (a)
		delete a;

	return;
}

