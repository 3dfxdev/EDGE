//----------------------------------------------------------------------------
//  EDGE Data Definition File Code (Game settings)
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2003  The EDGE Team.
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
// Overall Game Setup and Parser Code
//

#include "i_defs.h"

#include "ddf_locl.h"
#include "ddf_main.h"
#include "dm_state.h"
#include "m_fixed.h"
#include "m_math.h"
#include "p_mobj.h"
#include "z_zone.h"

#undef  DF
#define DF  DDF_CMD

static wi_map_t buffer_wi;
static wi_map_t *dynamic_wi;

static const wi_map_t template_wi = 
{
	DDF_BASE_NIL,	// ddf

	NULL,			// anims
	0,				// numanims
	NULL,			// mappos
	NULL,			// mapdone
	0,				// nummaps

	"",				// background
	"",				// splatpic
	{"", ""},		// yah[2]
	"",				// bg_camera
	0,				// music

	sfx_None,		// percent
	sfx_None,		// done
	sfx_None,		// endmap
	sfx_None,		// nextmap
	sfx_None,		// accel_snd
	sfx_None,		// frag_snd

	"",				// firstmap
	"",				// namegraphic
	NULL,			// titlepics
	0,				// numtitlepics
	0,				// titlemusic;
	TICRATE * 4		// titletics;
};

static void DDF_GameGetPic (const char *info, void *storage);
static void DDF_GameGetFrames (const char *info, void *storage);
static void DDF_GameGetMap (const char *info, void *storage);

wi_map_t **wi_maps = NULL;
int num_wi_maps = 0;

static stack_array_t wi_maps_a;

static wi_anim_t buffer_anim;
static wi_frame_t buffer_frame;

#define DDF_CMD_BASE  buffer_wi

static const commandlist_t wi_commands[] = 
{
	DF ("INTERMISSION GRAPHIC", background, DDF_MainGetInlineStr10),
	DF ("INTERMISSION CAMERA", bg_camera, DDF_MainGetInlineStr32),
	DF ("INTERMISSION MUSIC", music, DDF_MainGetNumeric),
	DF ("SPLAT GRAPHIC", splatpic, DDF_MainGetInlineStr10),
	DF ("YAH1 GRAPHIC", yah[0], DDF_MainGetInlineStr10),
	DF ("YAH2 GRAPHIC", yah[1], DDF_MainGetInlineStr10),
	DF ("PERCENT SOUND", percent, DDF_MainLookupSound),
	DF ("DONE SOUND", done, DDF_MainLookupSound),
	DF ("ENDMAP SOUND", endmap, DDF_MainLookupSound),
	DF ("NEXTMAP SOUND", nextmap, DDF_MainLookupSound),
	DF ("ACCEL SOUND", accel_snd, DDF_MainLookupSound),
	DF ("FRAG SOUND", frag_snd, DDF_MainLookupSound),
	DF ("FIRSTMAP", firstmap, DDF_MainGetInlineStr10),
	DF ("NAME GRAPHIC", namegraphic, DDF_MainGetInlineStr10),
	DF ("TITLE MUSIC", titlemusic, DDF_MainGetNumeric),
	DF ("TITLE TIME", titletics, DDF_MainGetTime),

	// these don't quite fit in yet
	DF ("TITLE GRAPHIC", ddf, DDF_GameGetPic),
	DF ("MAP", ddf, DDF_GameGetMap),
	{"ANIM", DDF_GameGetFrames, &buffer_frame, NULL},

	DDF_CMD_END
};


//
//  DDF PARSE ROUTINES
//

static bool GameStartEntry (const char *name)
{
	int i;
	bool replaces = false;

	if (name && name[0])
	{
		for (i = 0; i < num_wi_maps; i++)
		{
			if (DDF_CompareName (wi_maps[i]->ddf.name, name) == 0)
			{
				dynamic_wi = wi_maps[i];
				replaces = true;
				break;
			}
		}

		// if found, adjust pointer array to keep newest entries at end
		if (replaces && i < (num_wi_maps - 1))
		{
			Z_MoveData (wi_maps + i, wi_maps + i + 1, wi_map_t *,
				    num_wi_maps - i);
			wi_maps[num_wi_maps - 1] = dynamic_wi;
		}
	}

	// not found, create a new one
	if (!replaces)
	{
		Z_SetArraySize (&wi_maps_a, ++num_wi_maps);

		dynamic_wi = wi_maps[num_wi_maps - 1];
		dynamic_wi->ddf.name = (name && name[0]) ? Z_StrDup (name) :
			DDF_MainCreateUniqueName ("UNNAMED_WI_MAP",
						  num_wi_maps);
	}

	dynamic_wi->ddf.number = 0;

	Z_Clear (&buffer_anim, wi_anim_t, 1);
	Z_Clear (&buffer_frame, wi_frame_t, 1);

	// instantiate the static entry
	buffer_wi = template_wi;

	return replaces;
}

static void GameParseField (const char *field, const char *contents,
		int index, bool is_last)
{
#if (DEBUG_DDF)
	L_WriteDebug ("GAME_PARSE: %s = %s;\n", field, contents);
#endif

	if (!DDF_MainParseField (wi_commands, field, contents))
		DDF_WarnError ("Unknown games.ddf command: %s\n", field);
}

static void GameFinishEntry (void)
{
	ddf_base_t base;

	// FIXME: check stuff...

	// transfer static entry to dynamic entry

	base = dynamic_wi->ddf;
	dynamic_wi[0] = buffer_wi;
	dynamic_wi->ddf = base;

	// compute CRC...
	CRC32_Init (&dynamic_wi->ddf.crc);

	// FIXME: more stuff...

	CRC32_Done (&dynamic_wi->ddf.crc);
}

static void GameClearAll (void)
{
	// safe to delete game entries

	num_wi_maps = 0;
	Z_SetArraySize (&wi_maps_a, num_wi_maps);
}


void DDF_ReadGames (void *data, int size)
{
	readinfo_t games;

	games.memfile = (char *) data;
	games.memsize = size;
	games.tag = "GAMES";
	games.entries_per_dot = 1;

	if (games.memfile)
	{
		games.message = NULL;
		games.filename = NULL;
		games.lumpname = "DDFGAME";
	}
	else
	{
		games.message = "DDF_InitGames";
		games.filename = "games.ddf";
		games.lumpname = NULL;
	}

	games.start_entry = GameStartEntry;
	games.parse_field = GameParseField;
	games.finish_entry = GameFinishEntry;
	games.clear_all = GameClearAll;

	DDF_MainReadFile (&games);
}

void DDF_GameInit (void)
{
	Z_InitStackArray (&wi_maps_a, (void ***) &wi_maps, sizeof (wi_map_t),
			  0);
}

void DDF_GameCleanUp (void)
{
	if (num_wi_maps == 0)
		I_Error ("There are no games defined in DDF !\n");
}


static void DDF_GameAddFrame (void)
{
	Z_Resize (buffer_anim.frames, wi_frame_t, buffer_anim.numframes + 1);
	buffer_anim.frames[buffer_anim.numframes++] = buffer_frame;
	memset (&buffer_frame, 0, sizeof (wi_frame_t));
}

static void DDF_GameAddAnim (void)
{
	Z_Resize (buffer_wi.anims, wi_anim_t, buffer_wi.numanims + 1);
	if (buffer_anim.level[0])
		buffer_anim.type = WI_LEVEL;
	else
		buffer_anim.type = WI_NORMAL;
	buffer_wi.anims[buffer_wi.numanims++] = buffer_anim;
	memset (&buffer_anim, 0, sizeof (wi_anim_t));
}

static void DDF_GameGetFrames (const char *info, void *storage)
{
	char *s = Z_StrDup (info);
	char *tok;

	wi_frame_t *f = (wi_frame_t *) storage;

	int i;

	struct
	{
		int type;
		void *dest;
	}
	f_dest[4];

	f_dest[0].type = 1;
	f_dest[0].dest = f->pic;
	f_dest[1].type = 0;
	f_dest[1].dest = &f->tics;
	f_dest[2].type = 0;
	f_dest[2].dest = &f->pos.x;
	f_dest[3].type = 2;
	f_dest[3].dest = &f->pos.y;

	tok = strtok (s, ":");

	if (tok[0] == '#')
	{
		if (!buffer_anim.numframes)
		{
			Z_StrNCpy (buffer_anim.level, tok + 1, 8);
			tok = strtok (NULL, ":");
		}
		else if (!strncmp (tok, "#END", 4))
		{
			DDF_GameAddAnim ();
			Z_Free (s);
			return;
		}
		else
			DDF_Error ("Invalid # command '%s'\n", tok);
	}

	for (i = 0; tok && i < 4; tok = strtok (NULL, ":"), i++)
	{
		if (f_dest[i].type & 1)
			Z_StrNCpy ((char *) f_dest[i].dest, tok, 8);
		else
			*(int *) f_dest[i].dest = atoi (tok);

		if (f_dest[i].type & 2)
		{
			DDF_GameAddFrame ();
			Z_Free (s);
			return;
		}
	}
	DDF_Error ("Bad Frame command '%s'\n", info);
}

static void DDF_GameGetMap(const char *info, void *storage)
{
	char *s = Z_StrDup (info);
	char *tok;
	struct
	{
		int type;
		void *dest;
	}
	dest[3];
	int i;

	Z_Resize (buffer_wi.mappos, mappos_t, buffer_wi.nummaps + 1);
	Z_Resize (buffer_wi.mapdone, bool, buffer_wi.nummaps + 1);

	dest[0].type = 1;
	dest[0].dest = buffer_wi.mappos[buffer_wi.nummaps].name;
	dest[1].type = 0;
	dest[1].dest = &buffer_wi.mappos[buffer_wi.nummaps].pos.x;
	dest[2].type = 2;
	dest[2].dest = &buffer_wi.mappos[buffer_wi.nummaps++].pos.y;
	for (tok = strtok (s, ":"), i = 0;
	     tok && i < 3; tok = strtok (NULL, ":"), i++)
	{
		if (dest[i].type & 1)
			Z_StrNCpy ((char *) dest[i].dest, tok, 8);
		else
			*(int *) dest[i].dest = atoi (tok);
		if (dest[i].type & 2)
		{
			Z_Free (s);
			return;
		}
	}
	DDF_Error ("Bad Map command '%s'\n", info);
}

static void DDF_GameGetPic (const char *info, void *storage)
{
	Z_Resize (buffer_wi.titlepics, char *, buffer_wi.numtitlepics + 1);

	buffer_wi.titlepics[buffer_wi.numtitlepics] = Z_StrDup (info);
	buffer_wi.numtitlepics++;
}

const wi_map_t *DDF_GameLookup (const char *name)
{
	int i;

	for (i = 0; i < num_wi_maps; i++)
	{
		if (DDF_CompareName (name, wi_maps[i]->ddf.name) == 0)
			return wi_maps[i];
	}

	DDF_Error ("Unknown game name: %s\n", name);
	return NULL;
}
