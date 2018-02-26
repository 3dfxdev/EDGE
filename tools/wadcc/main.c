//----------------------------------------------------------------------------
// EDGE WAD Creation Tool 
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
//

#include "wad_io.h"

//
// E_PrintUsage
//
void E_PrintUsage(void)
{
    printf("USAGE: wadcc"
           " [-o <output-wad>]"
           " [-v]"
           " <lump> <file> [<lump2> <file2> ... ]"
           "\n");
}

//
// GetFileData
//
byte* GetFileData(const char *filename, int *length)
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

#define DEFAULT_OUTPUT_WAD "out.wad";

//
// main
//
int main(int argc, char** argv)
{
    boolean_t bad_args = false;
    boolean_t verbose = false;
    const char *output_wad = DEFAULT_OUTPUT_WAD;

    const char *lumpname;
    const char *srcfilename;
    byte *data;
    int length;

    int wad_id;
    int ch;

    while ((ch = getopt(argc, argv, "o:v")) != -1 && !bad_args) 
    {
        switch(ch)
        {
            case 'o':
                output_wad = optarg;
                break;

            case 'v':
                verbose = true;
                break;

            default:
                bad_args = true;
                break;
        }
    }
    
    if (bad_args)
    {
        E_PrintUsage();
        return 1;
    }

    // Adjust the arguments so that the next arguments are the
    // none option arguments
    argc -= optind;
    argv += optind;

    // Must have a pair of arguments
    if (argc < 2 || (argc % 2) != 0)
    {
        E_PrintUsage();
        return 2;
    }

    if (!WAD_Init())
    {
        fprintf(stderr, "Init Failed\n");
        return -1;
    }

    wad_id = WAD_Open(output_wad);
    if (wad_id < 0)
    {   
        fprintf(stderr, "Open of '%s' failed\n", output_wad);
        WAD_Shutdown();
        return -1;
    }

    do
    {
        lumpname = *argv++;
        srcfilename = *argv++;
        argc -= 2;

        data = GetFileData(srcfilename, &length);
        if (!data) 
        {
            fprintf(stderr, "Could not read: '%s'\n", srcfilename);
            WAD_Shutdown();
            return -2;
        }
        
        if (!WAD_Write(wad_id, lumpname, data, length))
        {       
            fprintf(stderr, 
                    "Could not write '%s' to '%s'\n", 
                    lumpname, output_wad);
        }

        if (verbose)
            printf("Wrote '%s' (%d bytes)\n", lumpname, length);

        free(data);
    }
    while(argc);

    if (!WAD_Close(wad_id))
    {
        fprintf(stderr, "Close Failed\n");
        return -1;
    }

    if (verbose)
        printf("Write of '%s' complete.\n", output_wad);

    WAD_Shutdown();
    return 0;
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
