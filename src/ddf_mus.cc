//----------------------------------------------------------------------------
//  EDGE Data Definition File Code (Music Playlist Handling)
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

#include "i_defs.h"
#include "dm_defs.h"
#include "ddf_locl.h"
#include "z_zone.h"

static pl_entry_t buffer_plentry;
static pl_entry_t *dynamic_plentry;

static const pl_entry_t template_plentry =
{
	DDF_BASE_NIL,    // ddf
	MUS_UNKNOWN,     // type
	MUSINF_UNKNOWN,  // infotype
	NULL             // info
};

pl_entry_container_c playlist;

static void DDF_MusicParseInfo(const char *info, void *storage);

#undef  DDF_CMD_BASE
#define DDF_CMD_BASE  buffer_plentry

static const commandlist_t musplaylistcmds[] =
{
	DDF_CMD("MUSICINFO", ddf, DDF_MusicParseInfo),
	DDF_CMD_END
};

//
// DDF_MusicParseInfo
//
// Parses the music information given.
//
static void DDF_MusicParseInfo(const char *info, void *storage)
{
	char *musstrtype[] = { "UNKNOWN", "CD", "MIDI", "MUS", "OGG", "MP3", NULL };
	char *musinftype[] = { "UNKNOWN", "TRACK", "LUMP", "FILE", NULL };
	char charbuff[256];
	int pos,i;

	// Get the music type
	i=0;
	pos=0;
	while (info[pos] != DIVIDE && i<255)
	{
		if (info[i] == '\0')
			DDF_Error("DDF_MusicParseInfo: Premature end of music info\n");

		charbuff[i] = info[pos];

		i++;
		pos++;
	}

	if (i==255)
		DDF_Error("DDF_MusicParseInfo: Music info too big\n");

	// -AJA- terminate charbuff with trailing \0.
	charbuff[i] = 0;

	i=MUS_UNKNOWN;
	while (i!=ENDOFMUSTYPES && strcasecmp(charbuff, musstrtype[i]))
		i++;

	if (i==ENDOFMUSTYPES)
		DDF_Error("DDF_MusicParseInfo: Unknown music type: '%s'\n", charbuff);
	else
		buffer_plentry.type = (musictype_t)i;

	// Data Type
	i=0;
	pos++;
	while (info[pos] != DIVIDE && i<255)
	{
		if (info[pos] == '\0')
			DDF_Error("DDF_MusicParseInfo: Premature end of music info\n");

		charbuff[i] = info[pos];

		pos++;
		i++;
	}

	if (i==255)
		DDF_Error("DDF_MusicParseInfo: Music info too big\n");

	// -AJA- terminate charbuff with trailing \0.
	charbuff[i] = 0;

	i=MUSINF_UNKNOWN;
	while (musinftype[i] != NULL && strcasecmp(charbuff, musinftype[i]))
		i++;

	if (i==ENDOFMUSINFTYPES)
		DDF_Error("DDF_MusicParseInfo: Unknown music info: '%s'\n", charbuff);
	else
		buffer_plentry.infotype = (musicinftype_e)i; // technically speaking this is proper

	// Remained is the string reference: filename/lumpname/track-number
	pos++;
	buffer_plentry.info = Z_StrDup(&info[pos]);

	return;
}


//
//  DDF PARSE ROUTINES
//

static bool PlaylistStartEntry(const char *name)
{	
	pl_entry_t* existing = NULL;
	int number = MAX(0, atoi(name));

	if (number == 0)
		DDF_Error("Bad music number in playlist.ddf: %s\n", name);

	existing = playlist.Find(number);
	if (existing)
	{
		dynamic_plentry = existing;

		// This will be overwritten, so lets not leak...
		if (dynamic_plentry->info)
		{
			Z_Free(dynamic_plentry->info);
			dynamic_plentry->info = NULL;
		}
	}
	else
	{
		dynamic_plentry = new pl_entry_t;
		memset(dynamic_plentry, 0, sizeof(pl_entry_t));
		playlist.Insert(dynamic_plentry);
	}

	dynamic_plentry->ddf.name   = NULL;
	dynamic_plentry->ddf.number = number;

	// instantiate the static entry
	buffer_plentry = template_plentry;
	return (existing != NULL);
}

static void PlaylistParseField(const char *field, const char *contents,
		int index, bool is_last)
{
#if (DEBUG_DDF)  
	L_WriteDebug("PLAYLIST_PARSE: %s = %s;\n", field, contents);
#endif

	if (! DDF_MainParseField(musplaylistcmds, field, contents))
		DDF_WarnError2(0x128, "Unknown playlist.ddf command: %s\n", field);
}

static void PlaylistFinishEntry(void)
{
	ddf_base_t base;

	// transfer static entry to dynamic entry

	base = dynamic_plentry->ddf;
	dynamic_plentry[0] = buffer_plentry;
	dynamic_plentry->ddf = base;

	// Compute CRC.  In this case, there is no need, since the music
	// playlist has zero impact on the game simulation itself.
	dynamic_plentry->ddf.crc = 0;
}

static void PlaylistClearAll(void)
{
	// safe to just remove all current entries
	playlist.Clear();
}


//
// DDF_ReadMusicPlaylist
//
void DDF_ReadMusicPlaylist(void *data, int size)
{
	readinfo_t playlistinfo;

	playlistinfo.memfile = (char*)data;
	playlistinfo.memsize = size;
	playlistinfo.tag = "PLAYLISTS";
	playlistinfo.entries_per_dot = 3;

	if (playlistinfo.memfile)
	{
		playlistinfo.message  = NULL;
		playlistinfo.filename = NULL;
		playlistinfo.lumpname = "DDFPLAY";
	}
	else
	{
		playlistinfo.message  = "DDF_InitMusicPlaylist";
		playlistinfo.filename = "playlist.ddf";
		playlistinfo.lumpname = NULL;
	}

	playlistinfo.start_entry  = PlaylistStartEntry;
	playlistinfo.parse_field  = PlaylistParseField;
	playlistinfo.finish_entry = PlaylistFinishEntry;
	playlistinfo.clear_all    = PlaylistClearAll;

	DDF_MainReadFile(&playlistinfo);
}

//
// DDF_MusicPlaylistInit
//
void DDF_MusicPlaylistInit(void)
{
	// -ACB- 2004/05/04 Use container 
	playlist.Clear();
}

//
// DDF_MusicPlaylistCleanUp
//
void DDF_MusicPlaylistCleanUp(void)
{
	// -ACB- 2004/05/04 Cut our playlist down to size
	playlist.Trim();
}


// List Management

//
// pl_entry_container_c::CleanupObject()
//
void pl_entry_container_c::CleanupObject(void *obj)
{
	pl_entry_t *p = *(pl_entry_t**)obj;

	if (p)
	{
		if (p->info) { Z_Free(p->info); }
		delete p;
	}

	return;
}

//
// pl_entry_t* pl_entry_container_c::Find()
//
pl_entry_t* pl_entry_container_c::Find(int number)
{
	epi::array_iterator_c it;
	pl_entry_t *p;

	for (it = GetBaseIterator(); it.IsValid(); it++)
	{
		p = ITERATOR_TO_TYPE(it, pl_entry_t*);
		if (p->ddf.number == number)
			return p;
	}

	return NULL;
}
