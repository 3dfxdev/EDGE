//----------------------------------------------------------------------------
//  EDGE Data Definition File Code (Music Playlist Handling)
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2009  The EDGE Team.
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

#include "playlist.h"


static void DDF_MusicParseInfo(const char *info, void *storage);

#undef  DDF_CMD_BASE
#define DDF_CMD_BASE  dummy_plentry
static pl_entry_c dummy_plentry;

static const commandlist_t musplaylistcmds[] =
{
	DDF_CMD("MUSICINFO", name, DDF_MusicParseInfo),

	DDF_CMD_END
};


pl_entry_container_c playlist;

static pl_entry_c *dynamic_plentry;


//
//  DDF PARSE ROUTINES
//

static void PlaylistStartEntry(const char *name, bool extend)
{	
	int number = MAX(0, atoi(name));

	if (number == 0)
		DDF_Error("Bad music number in playlist.ddf: %s\n", name);

	dynamic_plentry = playlist.Find(number);

	if (extend)
	{
		if (! dynamic_plentry)
			DDF_Error("Unknown playlist to extend: %s\n", name);
		return;
	}

	// replaces the existing entry
	if (dynamic_plentry)
	{
		dynamic_plentry->Default();
		return;
	}

	// create a new one and insert it
	dynamic_plentry = new pl_entry_c;
	dynamic_plentry->name = name;

	playlist.Insert(dynamic_plentry);
}


static void PlaylistParseField(const char *field, const char *contents,
		int index, bool is_last)
{
#if (DEBUG_DDF)  
	I_Debugf("PLAYLIST_PARSE: %s = %s;\n", field, contents);
#endif

	if (! DDF_MainParseField((char *)dynamic_plentry, musplaylistcmds, field, contents))
	{
		DDF_WarnError("Unknown playlist.ddf command: %s\n", field);
	}
}

static void PlaylistFinishEntry(void)
{
	// nothing needed
}

static void PlaylistClearAll(void)
{
	// safe to just remove all current entries
	playlist.Clear();
}


readinfo_t playlist_readinfo =
{
	"DDF_InitMusicPlaylist",  // message
	"PLAYLISTS",  // tag

	PlaylistStartEntry,
	PlaylistParseField,
	PlaylistFinishEntry,
	PlaylistClearAll 
};


void DDF_MusicPlaylistInit(void)
{
	// -ACB- 2004/05/04 Use container 
	playlist.Clear();
}

void DDF_MusicPlaylistCleanUp(void)
{
	// -ACB- 2004/05/04 Cut our playlist down to size
	playlist.Trim();
}

//
// Parses the music information given.
//
static void DDF_MusicParseInfo(const char *info, void *storage)
{
	pl_entry_c *pl = (pl_entry_c *) storage;

	static const char *const musstrtype[] = { "UNKNOWN", "CD", "MIDI", "MUS", "OGG", "MP3", NULL };
	static const char *const musinftype[] = { "UNKNOWN", "TRACK", "LUMP", "FILE", NULL };
	char charbuff[256];
	int pos,i;

	// Get the music type
	i=0;
	pos=0;
	while (info[pos] != ':' && i<255)
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
	while (i!=ENDOFMUSTYPES && stricmp(charbuff, musstrtype[i]) != 0)
		i++;

	if (i==ENDOFMUSTYPES)
		DDF_Error("DDF_MusicParseInfo: Unknown music type: '%s'\n", charbuff);
	else
		pl->type = (musictype_t)i;

	// Data Type
	i=0;
	pos++;
	while (info[pos] != ':' && i<255)
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
	while (musinftype[i] != NULL && stricmp(charbuff, musinftype[i]) != 0)
		i++;

	if (i==ENDOFMUSINFTYPES)
		DDF_Error("DDF_MusicParseInfo: Unknown music info: '%s'\n", charbuff);
	else
		pl->infotype = (musicinftype_e)i; // technically speaking this is proper

	// Remained is the string reference: filename/lumpname/track-number
	pos++;

	pl->info.Set(&info[pos]);
}


// --> pl_entry_c class

pl_entry_c::pl_entry_c() : name()
{
	Default();
}

pl_entry_c::~pl_entry_c()
{
}


void pl_entry_c::Default()
{
	type = MUS_UNKNOWN;     
	infotype = MUSINF_UNKNOWN;
	info.clear();             
}

//
// Copy everything with exception of ddf identifier
//
void pl_entry_c::CopyDetail(pl_entry_c &src)
{
	type = src.type;
	infotype = src.infotype;
	info = src.info;
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
		if (atoi(p->name.c_str()) == number)
			return p;
	}

	return NULL;
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
