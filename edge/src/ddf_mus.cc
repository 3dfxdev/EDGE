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

static pl_entry_c buffer_plentry;
static pl_entry_c *dynamic_plentry;

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
	buffer_plentry.info.Set(&info[pos]);

	return;
}


//
//  DDF PARSE ROUTINES
//

static bool PlaylistStartEntry(const char *name)
{	
	pl_entry_c* existing = NULL;
	int number = MAX(0, atoi(name));

	if (number == 0)
		DDF_Error("Bad music number in playlist.ddf: %s\n", name);

	existing = playlist.Find(number);
	if (existing)
	{
		dynamic_plentry = existing;
	}
	else
	{
		dynamic_plentry = new pl_entry_c;
		dynamic_plentry->ddf.number = number;
		playlist.Insert(dynamic_plentry);
	}

	// instantiate the static entry
	buffer_plentry.Default();
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
	// transfer static entry to dynamic entry
	dynamic_plentry->CopyDetail(buffer_plentry);

	// Compute CRC.  In this case, there is no need, since the music
	// playlist has zero impact on the game simulation itself.
	dynamic_plentry->ddf.crc.Reset();
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

// --> pl_entry_c class

//
// pl_entry_c constructor
//
pl_entry_c::pl_entry_c()
{
	Default();
}

//
// pl_entry_c Copy constructor
//
pl_entry_c::pl_entry_c(pl_entry_c &rhs)
{
	Copy(rhs);
}

//
// pl_entry_c destructor
//
pl_entry_c::~pl_entry_c()
{
}

//
// pl_entry_c::Copy()
//
void pl_entry_c::Copy(pl_entry_c &src)
{
	ddf = src.ddf;
	CopyDetail(src);
}

//
// pl_entry_c::CopyDetail()
//
// Copy everything with exception ddf identifier
//
void pl_entry_c::CopyDetail(pl_entry_c &src)
{
	type = src.type;
	infotype = src.infotype;
	info = src.info;
}

//
// pl_entry_c::Default()
// 
void pl_entry_c::Default()
{
	ddf.Default();

	type = MUS_UNKNOWN;     
	infotype = MUSINF_UNKNOWN;
	info.Clear();             
}

//
// pl_entry_c copy constructor
//
pl_entry_c& pl_entry_c::operator=(pl_entry_c &rhs)
{
	if (&rhs != this)
		Copy(rhs);
	
	return *this;
}

// --> pl_entry_containter_c class

//
// pl_entry_container_c::CleanupObject()
//
void pl_entry_container_c::CleanupObject(void *obj)
{
	pl_entry_c *p = *(pl_entry_c**)obj;

	if (p)
		delete p;

	return;
}

//
// pl_entry_c* pl_entry_container_c::Find()
//
pl_entry_c* pl_entry_container_c::Find(int number)
{
	epi::array_iterator_c it;
	pl_entry_c *p;

	for (it = GetBaseIterator(); it.IsValid(); it++)
	{
		p = ITERATOR_TO_TYPE(it, pl_entry_c*);
		if (p->ddf.number == number)
			return p;
	}

	return NULL;
}
