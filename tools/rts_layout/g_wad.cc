//------------------------------------------------------------------------
//  WAD : Wad file read/write functions.
//------------------------------------------------------------------------
//
//  RTS Layout Tool (C) 2007 Andrew Apted
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
//------------------------------------------------------------------------

#include "headers.h"

#include "raw_wad.h"
#include "lib_util.h"

#include "g_wad.h"


#define DEBUG_DIR   1
#define DEBUG_LUMP  1

#define ALIGN_LEN(len)  ((((len) + 3) / 4) * 4)


lump_c::lump_c(const char *_name, int _pos, int _len) :
    start(_pos), length(_len), data(NULL)
{
  name = StringDup(_name);
}

lump_c::~lump_c()
{
  StringFree(name);

  if (data)
    delete[] data;
}

level_c::level_c(const char *_name) : children()
{
  name = StringDup(_name);
}

level_c::~level_c()
{
  StringFree(name);

  // pointers in children vector are not allocated, merely
  // refer to lumps in the wad directory.
  //
  // HENCE: no need to free anything in 'children' here.
}


//------------------------------------------------------------------------

wad_c::wad_c() :
    fp(NULL), kind(-1), num_entries(0), dir_start(-1),
    dir(), levels(), cur_level(NULL)
{ }

wad_c::~wad_c()
{
  if (fp)
    fclose(fp);

  /* free directory entries */
  while (dir.size() > 0)
  {
    lump_c *L = dir.back();
    dir.pop_back();

    delete L;
  }

  /* free the levels */
  while (levels.size() > 0)
  {
    level_c *L = levels.back();
    levels.pop_back();

    delete L;
  }
}

/* ---------------------------------------------------------------- */


#define NUM_LEVEL_LUMPS  12
#define NUM_GL_LUMPS     5

static const char *level_lumps[NUM_LEVEL_LUMPS]=
{
  "THINGS", "LINEDEFS", "SIDEDEFS", "VERTEXES", "SEGS", 
  "SSECTORS", "NODES", "SECTORS", "REJECT", "BLOCKMAP",
  "BEHAVIOR",  // <-- hexen support
  "SCRIPTS"  // -JL- Lump with script sources
};

static const char *gl_lumps[NUM_GL_LUMPS]=
{
  "GL_VERT", "GL_SEGS", "GL_SSECT", "GL_NODES",
  "GL_PVS"  // -JL- PVS (Potentially Visible Set) lump
};


static bool CheckMagic(const char type[4])
{
  if ((type[0] == 'I' || type[0] == 'P') && 
     type[1] == 'W' && type[2] == 'A' && type[3] == 'D')
  {
    return true;
  }

  return false;
}


bool wad_c::CheckLevelName(const char *name)
{
  for (int i = 0; i < num_level_names; i++)
  {
    if (strcmp(level_names[i], name) == 0)
      return true;
  }

  return false;
}

static bool HasGLPrefix(const char *name)
{
  return (name[0] == 'G' && name[1] == 'L' && name[2] == '_');
}


//
// CheckLevelLumpName
//
// Tests if the entry name is one of the level lumps.
//
static bool CheckLevelLumpName(const char *name)
{
  for (int i = 0; i < NUM_LEVEL_LUMPS; i++)
  {
    if (strcmp(name, level_lumps[i]) == 0)
      return true;
  }

  return false;
}


//
// CheckGLLumpName
//
// Tests if the entry name matches one of the GL lump names.
//
static bool CheckGLLumpName(const char *name)
{
  for (int i = 0; i < NUM_GL_LUMPS; i++)
  {
    if (strcmp(name, gl_lumps[i]) == 0)
      return true;
  }

  return false;
}


//
// ReadHeader
//
// Returns true if successful, or FALSE if there was a problem (in
// which case the error message as been setup).
//
bool wad_c::ReadHeader()
{
  raw_wad_header_t header;

  size_t len = fread(&header, sizeof(header), 1, fp);

  if (len != 1)
  {
    LogPrintf("Trouble reading wad header: %s\n", strerror(errno));
    return false;
  }

  if (! CheckMagic(header.type))
  {
    LogPrintf("This file is not a WAD file : bad magic\n");
    return false;
  }

  kind = (header.type[0] == 'I') ? IWAD : PWAD;

  num_entries = LE_U32(header.num_entries);
  dir_start   = LE_U32(header.dir_start);

  return true;
}


void wad_c::ReadDirEntry()
{
  raw_wad_entry_t entry;

  size_t len = fread(&entry, sizeof(entry), 1, fp);

  if (len != 1)
    FatalError("Trouble reading wad directory");

  char name_buf[16];
  strncpy(name_buf, entry.name, 8);
  name_buf[8] = 0;

  lump_c *lump = new lump(name_buf, LE_U32(entry.start), LE_U32(entry.length));

#if DEBUG_DIR
  DebugPrintf("Read dir... %s\n", lump->name);
#endif

  dir.push_back(lump);
}


void wad_c::ReadDirectory()
{
  fseek(fp, dir_start, SEEK_SET);

  for (int i = 0; i < num_entries; i++)
  {
    ReadDirEntry();
  }

  DetermineLevels();
}


void wad_c::DetermineLevels()
{
  std::vector<lump_c *>::iterator LI;
  std::vector<lump_c *>::iterator NI;

  for (LI = dir.begin(); LI != dir.end(); LI++)
  {
    lump_c *L = *LI;

    // check if the next four lumps after the current lump match the
    // level-lump or GL-lump names.

    int count = 0;

    for (int i = 0; i < 4; i++)
    {
      NI = LI + (i+1);

      if (NI == dir.end())
        break;

      lump_c *N = *NI;

      if (strcmp(N->name, level_lumps[i]) == 0)
        count++;
    }

    if (count != 4)
      continue;

#if DEBUG_DIR
    DebugPrintf("Found level name: %s\n", L->name);
#endif

    if (strlen(L->name) > 5)
    {
      LogPrintf("Bad level '%s' in wad (name too long)\n", L->name);
      continue;
    }

    if (CheckLevelName(L->name))
    {
      LogPrintf("Level name '%s' found twice in wad!\n", L->name);
      continue;
    }

    level_c *LEV = new level_c(L->name);

    levels.push_back(LEV);

    // get the children lumps....

    LEV->children.push_back(L);

    for (int j = 0; j < NUM_LEVEL_LUMPS; j++)
    {
      NI = LI + (i+1);

      if (NI == dir.end())
        break;

      lump_c *N = *NI;

      if (CheckLevelLumpName(N->name))
        LEV->children->push_back(N);
    }
  }
}


/* ---------------------------------------------------------------- */


const byte * wad_c::CacheLump(lump_c *lump)
{
  if (! lump->data)
  {
#if DEBUG_LUMP
  DebugPrintf("Reading... %s (%d)\n", lump->name, lump->length);
#endif

    lump->data = new byte [lump->length + 1];
    lump->data[lump->length] = 0;

    fseek(fp, lump->start, SEEK_SET);

    size_t len = fread(lump->data, lump->length, 1, fp);

    if (len != 1)
    {
      if (cur_level)
        LogPrintf("Trouble reading lump '%s' in %s\n", lump->name, cur_level->name);
      else
        LogPrintf("Trouble reading lump '%s'\n", lump->name);
    }
  }

  return lump->data;
}


bool wad_c::FindLevel(const char *map_name)
{
  cur_level = NULL;

  std::vector<level_c *>::iterator MI;

  for (MI = dir.begin(); MI != dir.end(); MI++)
  {
    level_c *L = *MI;
    SYS_ASSERT(L);

    if (StrCaseCmp(L->name, map_name) == 0)
    {
      cur_level = L;
      return true;
    }
  }
 
  return false;  // not found
}


lump_c * wad_c::FindLump(const char *name)
{
  std::vector<lump_c *>::iterator LI;

  for (LI = dir.begin(); LI != dir.end(); LI++)
  {
    lump_c *L = *LI;
    SYS_ASSERT(L);

    if (StrCaseCmp(L->name, name) == 0)
      return L;
  }

  return NULL;  // not found
}


lump_c * wad_c::FindLumpInLevel(const char *name)
{
  SYS_ASSERT(cur_level);

  std::vector<lump_c *>::iterator LI;

  for (LI  = cur_level->children.begin();
       LI != cur_level->children.end(); LI++)
  {
    lump_c *L = *LI;
    SYS_ASSERT(L);

    if (StrCaseCmp(L->name, name) == 0)
      return L;
  }

  return NULL;  // not found
}


wad_c *wad_c::Load(const char *filename)
{
  wad_c *wad = new wad_c();

  // open input wad file & read header
  wad->fp = fopen(filename, "rb");

  if (! wad->fp)
  {
    LogPrintf("Cannot open WAD file %s : %s", filename, strerror(errno));

    delete wad;
    return NULL;
  }

  if (! wad->ReadHeader())
  {
    delete wad;
    return NULL;
  }

  DebugPrintf("Opened %cWAD file : %s\n", (wad->kind == IWAD) ? 'I' : 'P', filename); 
  DebugPrintf("Reading %d dir entries at 0x%X\n", wad->num_entries, wad->dir_start);

  wad->ReadDirectory();

  return wad;
}


//--- editor settings ---
// vi:ts=2:sw=2:expandtab
