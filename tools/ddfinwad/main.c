//----------------------------------------------------------------------------
// EDGE DDF->WAD Integrator Tool 
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

#include "wad_io.h"

#define DDFINWAD_VER_MAJOR 1
#define DDFINWAD_VER_MINOR 3

typedef struct
{
  char* lumpname;
  char* filename;
}
filelumpconv_t;

filelumpconv_t cvtable[] =
{
  { "DDFLANG",  "language.ldf" },
  { "DDFSFX",   "sounds.ddf" },
  { "DDFCOLM",  "colmap.ddf" },  
  { "DDFIMAGE", "images.ddf" },
  { "DDFFONT",  "fonts.ddf"  },
  { "DDFSTYLE", "styles.ddf" },
  { "DDFATK",   "attacks.ddf" },
  { "DDFWEAP",  "weapons.ddf" },
  { "DDFTHING", "things.ddf" },
  { "DDFPLAY",  "playlist.ddf" },
  { "DDFLINE",  "lines.ddf" },
  { "DDFSECT",  "sectors.ddf" },
  { "DDFSWTH",  "switch.ddf" },
  { "DDFANIM",  "anims.ddf" },
  { "DDFGAME",  "games.ddf" },
  { "DDFLEVL",  "levels.ddf" },
  { "RSCRIPT",  "edge.scr" },       
  { NULL,       NULL }
};

//
// E_PrintHeader
//
void E_PrintHeader(void)
{
  printf("\n=================================================\n");
        
  printf("  DDF->WAD Integrator V%d.%d\n",
                DDFINWAD_VER_MAJOR, DDFINWAD_VER_MINOR);
         
  printf("\n"
         "  The EDGE Team\n"
         "  http://edge.sourceforge.net\n"
         "\n"
         "  Released under the GNU General Public License\n"
         "=================================================\n"
         "\n");
}

//
// E_PrintUsage
//
void E_PrintUsage(void)
{
  printf("USAGE: ddfinwad <input-wad> <output-wad>\n\n");
}

//
// GetFileData
//
byte* GetFileData(char *filename, int *length)
{
  FILE *lumpfile;
  byte *data;

  if (!filename)
    return NULL;

  if (!length)
    return NULL;

  lumpfile = fopen(filename, "rb");  
  if (!lumpfile)
    return NULL;

  fseek(lumpfile, 0, SEEK_END);                   // get the end of the file
  (*length) = ftell(lumpfile);                    // get the size
  fseek(lumpfile, 0, SEEK_SET);                   // reset to beginning

  data = malloc((*length)*sizeof(byte));          // malloc the size
  if (!data)
  {
    fclose(lumpfile);                               // close the file
    return NULL;
  }

  fread(data, sizeof(char), (*length), lumpfile); // read file
  fclose(lumpfile);                               // close the file

  return data;
}

//
// main
//
int main(int argc, char** argv)
{
  boolean_t didsomething;
  boolean_t overrideresp;
  int i;
  byte* data;
  int length;
  char writeok;
  char *inname;
  char *outname;

  didsomething = false;

  E_PrintHeader();

  // We want two arguments
  if(argc<2 || argc>3)
  {
    E_PrintUsage();
    return 1;
  }
  else
  {
    inname = argv[1];

    if (argc==2)
      outname = argv[1];

    if (argc==3)
      outname = argv[2];

    printf("Attempting to read file...\n\n");
    if(!WAD_ReadFile(inname))
    {
      printf("Unable to read '%s'\n",argv[1]);
      printf("\nCreate new file? ");

      scanf("%c", &writeok);
      writeok = toupper(writeok); // make uppercase 
      if(writeok != 'Y')
      {
        WAD_Shutdown();
        printf("\nUser Cancelled\n");
        return 1;
      }

      fflush(stdin);
    }

    i=0;
    overrideresp = false;
    while(cvtable[i].lumpname && !overrideresp)
    {
      writeok = 'O';
      if(WAD_LumpExists(cvtable[i].lumpname)>=0)
      {
        printf("[O]verwrite existing DDF lumps or [Q]uit ? ");
        scanf("%c", &writeok);
        writeok = toupper(writeok); // make uppercase 

        if(writeok != 'O')
        {
          WAD_Shutdown();
          printf("User Cancelled\n");
          return 1;
        }

        overrideresp = true; // got response
        fflush(stdin);
      }

      i++;
    }

    i=0;
    while(cvtable[i].filename)
    {
      printf("Looking for '%s'...", cvtable[i].filename);
      if (!access(cvtable[i].filename, R_OK))
      {
        printf("FOUND\n");
        data = GetFileData(cvtable[i].filename, &length);
        if (!data)
        {
          WAD_Shutdown();
          printf("Malloc failure\n");
          return 1;
        }

        if (!WAD_AddLump(data, length, cvtable[i].lumpname))
        {
          WAD_Shutdown();
          printf("WAD_AddLump failure\n");
          return 1;
        }

        didsomething = true;
      }
      else
      {
        printf("not found\n");
      }
      i++;
    }
  }

  if (didsomething)
  {
    printf("\nCreating '%s'...\n",outname);
    if(!WAD_WriteFile(outname))
    {
      printf("\nUnable to write file!!!\n");
      WAD_Shutdown();
      return 1;
    }
  }
  else
  {
    printf("\nNothing to do!");
  }

  WAD_Shutdown();
  printf("\nFinished OK\n");
  return 0;
}
