//----------------------------------------------------------------------------
//  EDGE Data Definition File Code (Music Playlist Handling)
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2001  The EDGE Team.
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

static playlist_t buffer_playlist;
static playlist_t *dynamic_playlist;

static const playlist_t template_playlist =
{
  DDF_BASE_NIL,    // ddf
  MUS_UNKNOWN,     // type
  MUSINF_UNKNOWN,  // infotype
  NULL             // info
};

playlist_t ** playlist = NULL;
int num_playlist = 0;

static stack_array_t playlist_a;

static void DDF_MusicParseInfo(const char *info, void *storage);

#undef  DDF_CMD_BASE
#define DDF_CMD_BASE  buffer_playlist

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
  char *musstrtype[] = { "UNKNOWN", "CD", "MIDI", "MUS", "MP3", NULL };
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
    buffer_playlist.type = (musictype_t)i;

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
    buffer_playlist.infotype = (musicinftype_e)i; // technically speaking this is proper

  // Remained is the string reference: filename/lumpname/track-number
  pos++;
  buffer_playlist.info = Z_StrDup(&info[pos]);

  return;
}


//
//  DDF PARSE ROUTINES
//

static bool PlaylistStartEntry(const char *name)
{
  int i;
  bool replaces = false;
  int number = MAX(0, atoi(name));

  if (number == 0)
    DDF_Error("Bad music number in playlist.ddf: %s\n", name);

  for (i=0; i < num_playlist; i++)
  {
    if (playlist[i]->ddf.number == number)
    {
      dynamic_playlist = playlist[i];
      replaces = true;
      break;
    }
  }
    
  // if found, adjust pointer array to keep newest entries at end
  if (replaces && i < (num_playlist-1))
  {
    Z_MoveData(playlist + i, playlist + i + 1, playlist_t *,
        num_playlist - i);
    playlist[num_playlist - 1] = dynamic_playlist;
  }

  // not found, create a new one
  if (! replaces)
  {
    Z_SetArraySize(&playlist_a, ++num_playlist);
    
    dynamic_playlist = playlist[num_playlist - 1];
  }

  dynamic_playlist->ddf.name   = NULL;
  dynamic_playlist->ddf.number = number;

  // instantiate the static entry
  buffer_playlist = template_playlist;

  return replaces;
}

static void PlaylistParseField(const char *field, const char *contents,
    int index, bool is_last)
{
#if (DEBUG_DDF)  
  L_WriteDebug("PLAYLIST_PARSE: %s = %s;\n", field, contents);
#endif

  if (! DDF_MainParseField(musplaylistcmds, field, contents))
    DDF_WarnError("Unknown playlist.ddf command: %s\n", field);
}

static void PlaylistFinishEntry(void)
{
  ddf_base_t base;
  
  // transfer static entry to dynamic entry
  
  base = dynamic_playlist->ddf;
  dynamic_playlist[0] = buffer_playlist;
  dynamic_playlist->ddf = base;

  // Compute CRC.  In this case, there is no need, since the music
  // playlist has zero impact on the game simulation itself.
  dynamic_playlist->ddf.crc = 0;
}

static void PlaylistClearAll(void)
{
  // safe to just remove all current entries

  num_playlist = 0;
  Z_SetArraySize(&playlist_a, num_playlist);
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
  Z_InitStackArray(&playlist_a, (void ***)&playlist, sizeof(playlist_t), 0);
}

//
// DDF_MusicPlaylistCleanUp
//
void DDF_MusicPlaylistCleanUp(void)
{
  /* nothing to do */
}

//
// DDF_MusicLookupNum
//
const playlist_t *DDF_MusicLookupNum(int number)
{
  int i;

  for (i=0; i < num_playlist; i++)
    if (playlist[i]->ddf.number == number)
      return playlist[i];
  
  // not found
  return NULL;
}

