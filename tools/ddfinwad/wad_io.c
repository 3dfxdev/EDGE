//----------------------------------------------------------------------------
//  EDGE Tool WAD I/O
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2000  The EDGE Team.
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
#include "i_defs.h"
#include "wad_io.h"

#define PWAD_HEADER "PWAD"

//
// TYPES
//

//
// Wad Info
//
typedef struct
{
  char id[4];        // IWAD (whole) or PWAD (part)
  int numlumps;      // Number of Lumps
  int infotableofs;  // Info table offset
}
wadinfo_t;

//
// Lump information struct
//
typedef struct
{
  int filepos;       // Position in file
  int size;          // Size
  char name[8];      // Name
}
lumpinfo_t;

//
// Lump stuff
//
typedef struct lump_s
{
  byte* data;          // Data
  int filepos;         // Position in file
  int size;            // Size
  char name[8];        // Name
}
lump_t;

// Lumpinfo list
static lumpinfo_t **lumpinfolist = NULL;
static int numoflumpinfs = 0;

// Lump list
static lump_t **lumplist = NULL;
static int numoflumps = 0;

//
// PadFile
//
// Pads a file to the nearest 4 bytes.
//
static void PadFile(FILE* fp, int filepos)
{
  int num;

  num = filepos % 4;

  if (!num)
    return;

  num = 4 - num;
  while(num)
  {
    fwrite(&num, sizeof(byte), 1, fp);
    num--;
  }
}

//
// AddLumpinfo
//
static boolean_t AddLumpinfo(lumpinfo_t *lpinf)
{
  lumpinfo_t **oldlpinflist;

  if (numoflumpinfs && lumpinfolist)
  {
    oldlpinflist = lumpinfolist;

    lumpinfolist = malloc(sizeof(lumpinfo_t*)*(numoflumpinfs+1));
    if (!lumpinfolist)
      return false;

    memset(lumpinfolist, 0, sizeof(lumpinfo_t*)*(numoflumpinfs+1));
    memcpy(lumpinfolist, oldlpinflist, sizeof(lumpinfo_t*)*(numoflumpinfs));

    free(oldlpinflist);
  }
  else
  {
    lumpinfolist = malloc(sizeof(lumpinfo_t*));

    if (!lumpinfolist)
      return false;

    numoflumpinfs = 0;
  }

  lumpinfolist[numoflumpinfs] = malloc(sizeof(lumpinfo_t));
  if (!lumpinfolist[numoflumpinfs])
    return false;

  lumpinfolist[numoflumpinfs]->filepos = lpinf->filepos;
  lumpinfolist[numoflumpinfs]->size    = lpinf->size;
  strncpy(lumpinfolist[numoflumpinfs]->name, lpinf->name, 8);

  numoflumpinfs++;
  return true;
}

//
// WAD_AddLump
//
boolean_t WAD_AddLump(byte *data, int size, char *name)
{
  lump_t **oldlumplist;
  int i;

  // Check for existing lump, overwrite if need be.
  i=WAD_LumpExists(name);
  if (i>=0)
  {
    free(lumplist[i]->data);
    lumplist[i]->size = size;
    lumplist[i]->data = data;
    return true;
  }

  if (numoflumps && lumplist)
  {
    oldlumplist = lumplist;

    lumplist = malloc(sizeof(lump_t*)*(numoflumps+1));
    if (!lumplist)
      return false;

    memset(lumplist, 0, sizeof(lump_t*)*(numoflumps+1));
    memcpy(lumplist, oldlumplist, sizeof(lump_t*)*(numoflumps));

    free(oldlumplist);
  }
  else
  {
    lumplist = malloc(sizeof(lump_t*));

    if (!lumplist)
      return false;

    numoflumps = 0;
  }

  lumplist[numoflumps] = malloc(sizeof(lump_t));
  if (!lumplist[numoflumps])
    return false;

  lumplist[numoflumps]->data = data;
  lumplist[numoflumps]->size = size;
  strncpy(lumplist[numoflumps]->name, name, 8);

  numoflumps++;

  return true;
}

//
// WAD_LumpExists
//
int WAD_LumpExists(char *name)
{
  int i;

  i=0;
  while (i<numoflumps)
  {
    if (!strncmp(lumplist[i]->name, name, 8))
      return i;

    i++;
  }

  return -1;
}

//
// WAD_WriteFile
//
boolean_t WAD_WriteFile(char *name)
{
  int i;
  FILE *fp;
  char *pwadstr = PWAD_HEADER;
  int infooffset;
  int fpos;

  // Open File
  fp = fopen(name, "wb");
  if (!fp)
    return false;

  // Write Header
  fwrite((void*)pwadstr, sizeof(char), 4, fp);
  fwrite((void*)&numoflumps, sizeof(int), 1, fp);
  fwrite((void*)&numoflumps, sizeof(int), 1, fp); // dummy - write later

  // Write Lumps
  i=0;
  while(i<numoflumps)
  {
    fpos = ftell(fp);
    PadFile(fp, fpos);
    lumplist[i]->filepos = ftell(fp);
    fwrite((void*)lumplist[i]->data, sizeof(byte), lumplist[i]->size, fp);
    i++;
  }

  fpos = ftell(fp);
  PadFile(fp, fpos);
  infooffset = ftell(fp);

  // Write Lumpinfo(s)
  i=0;
  while(i<numoflumps)
  {
    fwrite((void*)&lumplist[i]->filepos, sizeof(int), 1, fp);
    fwrite((void*)&lumplist[i]->size, sizeof(int), 1, fp);
    fwrite((void*)lumplist[i]->name, sizeof(char), 8, fp);
    i++;
  }

  // Write table offset
  fseek(fp, 8, SEEK_SET);
  fwrite((void*)&infooffset, sizeof(int), 1, fp); // Now write table offset

  // Close File
  fclose(fp);

  return true;
}

//
// WAD_ReadFile
//
boolean_t WAD_ReadFile(char *name)
{
  FILE *fp;
  wadinfo_t wadinfo;
  lumpinfo_t lumpinfo;
  lump_t lump;
  int i;

  byte* data;
  char lumpname[9];

  // Open File
  fp = fopen(name, "rb");
  if (!fp)
    return false;

  // Read Header
  fread((void*)&wadinfo, sizeof(wadinfo_t), 1, fp);

  // Check Header is for a PWAD
  if (strncmp(wadinfo.id, PWAD_HEADER, 4))
  {
    printf("Got: %s\n",wadinfo.id);
    return false;
  }

#ifdef DEVELOPERS
  printf("Number of lumps: %d\n", wadinfo.numlumps);
  printf("Offset: %d\n", wadinfo.infotableofs);
#endif

  // Read and alloc for lumps
  fseek(fp, wadinfo.infotableofs, SEEK_SET);
  i = wadinfo.numlumps;
  while(i)
  {
    fread((void*)&lumpinfo.filepos, sizeof(int),  1, fp);
    fread((void*)&lumpinfo.size,    sizeof(int),  1, fp);
    fread((void*)lumpinfo.name,    sizeof(char), 8, fp);

#ifdef DEVELOPERS
    memset(lumpname, ' ', 8);
    strncpy(lumpname, lumpinfo.name, 8);
    lumpname[8] = '\0';
    printf("%s,%d,%d,\n", lumpname, lumpinfo.size, lumpinfo.filepos);
#endif

    if (!AddLumpinfo(&lumpinfo))
      return false;

    i--;
  }

  // go through lumpinfo list and load lumps
  i=0;
  while(i<numoflumpinfs)
  {
    fseek(fp, lumpinfolist[i]->filepos, SEEK_SET);

    data = malloc(sizeof(byte)*lumpinfolist[i]->size);
    fread((void*)data, sizeof(byte), lumpinfolist[i]->size, fp);

    if (!WAD_AddLump(data, lumpinfolist[i]->size, lumpinfolist[i]->name))
      return false;

    i++;
  }

  // Close file
  fclose(fp);

  // go through lumpinfo list and free lumpinfo - we don't need them now
  i=0;
  while(lumpinfolist && i<numoflumpinfs)
  {
    free(lumpinfolist[i]);
    i++;
  }

  if (lumpinfolist)
    free(lumpinfolist);

  return true;
}

//
// WAD_Shutdown
//
void WAD_Shutdown(void)
{
  int i;

  // go through lump list and free lump
  i=0;
  while(lumplist && i<numoflumps)
  {
    free(lumplist[i]);
    i++;
  }

  if (lumplist)
    free(lumplist);

  return;
}
