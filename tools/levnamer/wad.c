//------------------------------------------------------------------------
// WAD : WAD read/write functions.
//------------------------------------------------------------------------
//
//  LevNamer (C) 2001 Andrew Apted
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

#include "system.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <math.h>
#include <limits.h>
#include <errno.h>

#include "structs.h"
#include "wad.h"


#define DEBUG_DIR   0
#define DEBUG_LUMP  0


int load_all=0;
int hexen_mode=0;

FILE *in_file  = NULL;
FILE *out_file = NULL;

wad_t wad;


/* ---------------------------------------------------------------- */


#define NUM_LEVEL_LUMPS  11
#define NUM_GL_LUMPS     4

static const char *level_lumps[NUM_LEVEL_LUMPS]=
{
  "THINGS", "LINEDEFS", "SIDEDEFS", "VERTEXES", "SEGS", 
  "SSECTORS", "NODES", "SECTORS", "REJECT", "BLOCKMAP",
  "BEHAVIOR"  // <-- hexen support
};

static const char *gl_lumps[NUM_GL_LUMPS]=
{
  "GL_VERT", "GL_SEGS", "GL_SSECT", "GL_NODES"
};


//
// CheckMagic
//
static int CheckMagic(const char type[4])
{
  if ((type[0] == 'I' || type[0] == 'P') && 
       type[1] == 'W' && type[2] == 'A' && type[3] == 'D')
  {
    return TRUE;
  }
  
  return FALSE;
}


//
// CheckLevelName
//
// Tests if the entry name matches ExMy or MAPxx.
//
static int CheckLevelName(const char *name)
{
  // Doom 1:

  if (name[0]=='E' && name[2]=='M' &&
      name[1]>='0' && name[1]<='9' &&
      name[3]>='0' && name[3]<='9' && name[4]==0)
  {
    return TRUE;
  }

  if (name[0]=='E' && name[2]=='M' &&
      name[1]>='0' && name[1]<='9' &&
      name[3]>='0' && name[3]<='9' &&
      name[4]>='0' && name[4]<='9' && name[5]==0)
  {
    return TRUE;
  }

  // Doom 2:

  if (name[0]=='M' && name[1]=='A' && name[2]=='P' &&
      name[3]>='0' && name[3]<='9' &&
      name[4]>='0' && name[4]<='9' && name[5]==0)
  {
    return TRUE;
  }
  
  return FALSE;
}


//
// CheckLevelLumpName
//
// Tests if the entry name is one of the level lumps.
//
static int CheckLevelLumpName(const char *name)
{
  int i;

  for (i=0; i < NUM_LEVEL_LUMPS; i++)
  {
    if (strcmp(name, level_lumps[i]) == 0)
      return TRUE;
  }
  
  return FALSE;
}


//
// CheckGLLumpName
//
// Tests if the entry name matches GL_ExMy or GL_MAPxx, or one of the
// GL lump names.
//
static int CheckGLLumpName(const char *name)
{
  int i;

  if (name[0] != 'G' || name[1] != 'L' || name[2] != '_')
    return 0;

  for (i=0; i < NUM_GL_LUMPS; i++)
  {
    if (strcmp(name, gl_lumps[i]) == 0)
      return TRUE;
  }
  
  return CheckLevelName(name+3);
}


//
// NewLump
//
// Create new lump.  `name' must be allocated storage.
//
static lump_t *NewLump(char *name)
{
  lump_t *cur;

  cur = SysCalloc(sizeof(lump_t));

  cur->name = name;
  cur->start = cur->new_start = -1;
  cur->flags = 0;
  cur->length = 0;
  cur->space = 0;
  cur->data = NULL;

  return cur;
}


//
// FreeLump
//
static void FreeLump(lump_t *lump)
{
  // check `data' here, since it gets freed in WriteLumpData()
  if (lump->data)
    SysFree(lump->data);
  
  SysFree(lump->name);
  SysFree(lump);
}


//
// ReadHeader
//
static void ReadHeader(const char *filename)
{
  int len;
  raw_wad_header_t header;

  len = fread(&header, sizeof(header), 1, in_file);

  if (len != 1)
    FatalError("Trouble reading wad header for %s : %s", 
      filename, strerror(errno));

  if (! CheckMagic(header.type))
    FatalError("%s does not appear to be a wad file -- bad magic", filename);

  wad.kind = (header.type[0] == 'I') ? IWAD : PWAD;
  
  wad.num_entries = UINT32(header.num_entries);
  wad.dir_start   = UINT32(header.dir_start);

  // initialise stuff
  wad.dir_head = NULL;
  wad.dir_tail = NULL;
}


//
// ReadDirEntry
//
static void ReadDirEntry(void)
{
  int len;
  raw_wad_entry_t entry;
  lump_t *lump;
  
  ShowProgress(0);

  len = fread(&entry, sizeof(entry), 1, in_file);

  if (len != 1)
    FatalError("Trouble reading wad directory");

  lump = NewLump(SysStrndup(entry.name, 8));

  lump->start = UINT32(entry.start);
  lump->length = UINT32(entry.length);

  // --- ORDINARY LUMPS ---

  #if DEBUG_DIR
  PrintDebug("Read dir... %s\n", lump->name);
  #endif

  // maybe load data
  if (load_all)
    lump->flags |= LUMP_READ_ME;
  else
    lump->flags |= LUMP_COPY_ME;

  // link it in
  lump->next = NULL;
  lump->prev = wad.dir_tail;

  if (wad.dir_tail)
    wad.dir_tail->next = lump;
  else
    wad.dir_head = lump;

  wad.dir_tail = lump;
}


//
// ReadDirectory
//
static void ReadDirectory(void)
{
  int i;
  int total_entries = wad.num_entries;

  fseek(in_file, wad.dir_start, SEEK_SET);

  for (i=0; i < total_entries; i++)
  {
    ReadDirEntry();
  }
}


//
// ReadLumpData
//
static void ReadLumpData(lump_t *lump)
{
  int len;

  ShowProgress(0);

  #if DEBUG_LUMP
  PrintDebug("Reading... %s (%d)\n", lump->name, lump->length);
  #endif
  
  if (lump->length == 0)
    return;

  lump->data = SysCalloc(lump->length);

  fseek(in_file, lump->start, SEEK_SET);

  len = fread(lump->data, lump->length, 1, in_file);

  if (len != 1)
  {
    PrintWarn("Trouble reading lump `%s'\n", lump->name);
  }

  lump->flags &= ~LUMP_READ_ME;
}


//
// ReadAllLumps
//
// Returns number of entries read.
//
static int ReadAllLumps(void)
{
  lump_t *cur;
  int count = 0;

  for (cur=wad.dir_head; cur; cur=cur->next)
  {
    count++;

    if (cur->flags & LUMP_READ_ME)
      ReadLumpData(cur);
  }

  return count;
}


/* ---------------------------------------------------------------- */


//
// WriteHeader
//
static void WriteHeader(void)
{
  int len;
  raw_wad_header_t header;

  switch (wad.kind)
  {
    case IWAD:
      strncpy(header.type, "IWAD", 4);
      break;

    case PWAD:
      strncpy(header.type, "PWAD", 4);
      break;
  }

  header.num_entries = UINT32(wad.num_entries);
  header.dir_start   = UINT32(wad.dir_start);

  len = fwrite(&header, sizeof(header), 1, out_file);

  if (len != 1)
    PrintWarn("Trouble writing wad header\n");
}


//
// SortLumps
//
// Algorithm is pretty simple: for each of the names, if such a lump
// exists in the list, move it to the head of the list.  By going
// backwards through the names, we ensure the correct order.
//
static void SortLumps(lump_t ** list, const char **names, int count)
{
  int i;
  lump_t *cur;

  for (i=count-1; i >= 0; i--)
  {
    for (cur=(*list); cur; cur=cur->next)
    {
      if (strcmp(cur->name, names[i]) != 0)
        continue;

      // unlink it
      if (cur->next)
        cur->next->prev = cur->prev;

      if (cur->prev)
        cur->prev->next = cur->next;
      else
        (*list) = cur->next;
      
      // add to head
      cur->next = (*list);
      cur->prev = NULL;

      if (cur->next)
        cur->next->prev = cur;

      (*list) = cur;

      // continue with next name (important !)
      break;
    }
  }
}


//
// RecomputeDirectory
//
// Calculates all the lump offsets for the directory.
//
static void RecomputeDirectory(void)
{
  lump_t *cur;

  wad.num_entries = 0;
  wad.dir_start = sizeof(raw_wad_header_t);
  
  // run through all the lumps, computing the `new_start' fields, the
  // number of lumps in the directory, the directory starting pos, and
  // also sorting the lumps in the levels.

  for (cur=wad.dir_head; cur; cur=cur->next)
  {
    if (cur->flags & LUMP_IGNORE_ME)
      continue;

    cur->new_start = wad.dir_start;

    wad.dir_start += cur->length;
    wad.num_entries++;
  }
}


//
// WriteLumpData
//
static void WriteLumpData(lump_t *lump)
{
  int len;

  ShowProgress(0);

  #if DEBUG_LUMP
  if (lump->flags & LUMP_COPY_ME)
    PrintDebug("Copying... %s (%d)\n", lump->name, lump->length);
  else
    PrintDebug("Writing... %s (%d)\n", lump->name, lump->length);
  #endif
  
  if (ftell(out_file) != lump->new_start)
    PrintWarn("Consistency failure writing %s (%08X, %08X\n", 
      lump->name, ftell(out_file), lump->new_start);
       
  if (lump->length == 0)
    return;

  if (lump->flags & LUMP_COPY_ME)
  {
    lump->data = SysCalloc(lump->length);

    fseek(in_file, lump->start, SEEK_SET);

    len = fread(lump->data, lump->length, 1, in_file);

    if (len != 1)
      PrintWarn("Trouble reading lump %s to copy\n", lump->name);
  }

  len = fwrite(lump->data, lump->length, 1, out_file);
   
  if (len != 1)
    PrintWarn("Trouble writing lump %s\n", lump->name);
  
  SysFree(lump->data);

  lump->data = NULL;
}


//
// WriteAllLumps
//
// Returns number of entries written.
//
static int WriteAllLumps(void)
{
  lump_t *cur;
  int count = 0;

  for (cur=wad.dir_head; cur; cur=cur->next)
  {
    if (cur->flags & LUMP_IGNORE_ME)
      continue;

    WriteLumpData(cur);
    count++;
  }

  fflush(out_file);

  return count;
}


//
// WriteDirEntry
//
static void WriteDirEntry(lump_t *lump)
{
  int len;
  raw_wad_entry_t entry;

  ShowProgress(0);

  strncpy(entry.name, lump->name, 8);

  entry.start = UINT32(lump->new_start);
  entry.length = UINT32(lump->length);

  len = fwrite(&entry, sizeof(entry), 1, out_file);

  if (len != 1)
    PrintWarn("Trouble writing wad directory\n");
}


//
// WriteDirectory
//
// Returns number of entries written.
//
static int WriteDirectory(void)
{
  lump_t *cur;
  int count = 0;
  
  if (ftell(out_file) != wad.dir_start)
    PrintWarn("Consistency failure writing lump directory "
      "(%08X,%08X)\n", ftell(out_file), wad.dir_start);

  for (cur=wad.dir_head; cur; cur=cur->next)
  {
    if (cur->flags & LUMP_IGNORE_ME)
      continue;

    WriteDirEntry(cur);
    count++;

    #if DEBUG_DIR
    PrintDebug("Write dir... %s\n", cur->name);
    #endif
  }

  fflush(out_file);

  return count;
}


/* ---------------------------------------------------------------- */


//
// CheckExtension
//
int CheckExtension(const char *filename, const char *ext)
{
  int A = strlen(filename) - 1;
  int B = strlen(ext) - 1;

  for (; B >= 0; B--, A--)
  {
    if (A < 0)
      return 0;
    
    if (toupper(filename[A]) != toupper(ext[B]))
      return 0;
  }

  return (A >= 1) && (filename[A] == '.');
}

//
// ReplaceExtension
//
char *ReplaceExtension(const char *filename, const char *ext)
{
  char *dot_pos;
  char buffer[512];

  strcpy(buffer, filename);
  
  dot_pos = strrchr(buffer, '.');

  if (dot_pos)
    dot_pos[1] = 0;
  
  strcat(buffer, ext);

  return SysStrdup(buffer);
}


//
// ReadWadFile
//
void ReadWadFile(char *filename)
{
  int check;

  // open input wad file & read header
  in_file = fopen(filename, "rb");

  if (! in_file)
    FatalError("Cannot open WAD file %s : %s", filename, strerror(errno));

  ReadHeader(filename);

  PrintMsg("Opened %cWAD file : %s\n", (wad.kind == IWAD) ? 'I' : 'P', 
      filename); 
  PrintMsg("Reading %d dir entries at 0x%X\n", wad.num_entries, 
      wad.dir_start);

  StartProgress(0);

  // read directory
  ReadDirectory();

  // now read lumps
  check = ReadAllLumps();

  if (check != wad.num_entries)
    PrintWarn("Read directory count consistency failure (%d,%d)\n",
      check, wad.num_entries);
  
  ClearProgress();
}


//
// WriteWadFile
//
void WriteWadFile(char *filename)
{
  int check1, check2;

  RecomputeDirectory();

  // create output wad file & write the header
  out_file = fopen(filename, "wb");

  if (! out_file)
    FatalError("Could not open output WAD file: %s", strerror(errno));

  WriteHeader();

  StartProgress(0);

  // now write all the lumps to the output wad
  check1 = WriteAllLumps();

  // finally, write out the directory
  check2 = WriteDirectory();

  if (check1 != wad.num_entries || check2 != wad.num_entries)
    PrintWarn("Write directory count consistency failure (%d,%d,%d\n",
      check1, check2, wad.num_entries);

  ClearProgress();

  PrintMsg("\nSaved WAD as %s\n", filename);
}


//
// CloseWads
//
void CloseWads(void)
{
  if (in_file)
    fclose(in_file);
  
  if (out_file)
    fclose(out_file);
  
  // free directory entries
  while (wad.dir_head)
  {
    lump_t *head = wad.dir_head;
    wad.dir_head = head->next;

    FreeLump(head);
  }
}

