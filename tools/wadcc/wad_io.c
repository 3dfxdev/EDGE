//----------------------------------------------------------------------------
//  EDGE Tool WAD I/O
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
//  but WI:THOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//----------------------------------------------------------------------------
//
#include "i_defs.h"
#include "wad_io.h"

#define DEFAULT_FILE_HANDLE_COUNT 1

#define WADTYPE_LEN 4
const char *PWAD_STR = "PWAD";

// WAD Lump Info
typedef struct wad_lumpinfo_s 
{
    // Name of LUMP
    char name[WAD_LUMPNAME_LEN];

    // File offset
    int fileofs;

    // Size
    int size;

    // Linked list for maintaining order
    struct wad_lumpinfo_s *next;

    // Tree nodes for searching
    struct wad_lumpinfo_s *left;
    struct wad_lumpinfo_s *right;
}
wad_lumpinfo_t;

//
typedef enum
{
    ALLOCATED         = 0x0,
    INITED            = 0x1,
    WRITE_IN_PROGRESS = 0x2
}
wad_file_status_e;

// Parent structure - The WAD File
typedef struct wad_file_s 
{
    char *filename;
    FILE* fp;

    char id[WADTYPE_LEN];       // IWAD (whole) or PWAD (part)
    
    wad_lumpinfo_t *head;       // Linked list/Tree of lumps in this WAD 
    wad_lumpinfo_t *tail;       // Tail

    wad_file_status_e status;
}
wad_file_t;

// WAD Files Table
wad_file_t* wad_files;
int wad_file_count;

//
// WAD_Init
//
boolean_t WAD_Init()
{
    wad_files = malloc(sizeof(wad_file_t)*DEFAULT_FILE_HANDLE_COUNT);
    wad_file_count = DEFAULT_FILE_HANDLE_COUNT;
    memset(wad_files, 0, sizeof(wad_file_t)*DEFAULT_FILE_HANDLE_COUNT);

    return true;
}

//
// WAD_Shutdown
//
void WAD_Shutdown()
{
    int i;
    for (i=0; i<wad_file_count; i++)
        WAD_Close(i);

    free(wad_files);
    wad_file_count = 0;
}

//
// WAD_Open
//
int WAD_Open(const char *filename) 
{
    int i=0;
    while (i<wad_file_count && wad_files[i].status != ALLOCATED)
        i++;

    if (i == wad_file_count)
        return -1;

    // Set ID to PWAD
    wad_files[i].filename = strdup(filename);
    strncpy(wad_files[i].id, PWAD_STR, WADTYPE_LEN); 
    wad_files[i].status = INITED;

    return i;
}

//
// OpenWADForWriting
//
FILE* OpenWADForWriting(const char* filename, const char *id) 
{
    FILE *fp = fopen(filename, "wb");
    if (!fp)
        return NULL;

    const int ZERO = 0;

    // Write header
    fwrite((void*)id, sizeof(char), WADTYPE_LEN, fp);
    fwrite((void*)&ZERO, sizeof(int), 1, fp); // dummy - write later
    fwrite((void*)&ZERO, sizeof(int), 1, fp); // dummy - write later

    return fp;
}

//
// PadFile
//
// Pads a file to the nearest 4 bytes.
//
static void PadFile(FILE* fp, int filepos)
{
    const byte ZERO = 0;
    int num;

    num = filepos % 4;

    if (!num)
        return;

    num = 4 - num;
    while(num)
    {
        fwrite(&ZERO, sizeof(byte), 1, fp);
        num--;
    }
}

//
// LumpnameCompare
//
// Lump name comparison function.
//
int LumpnameCompare(const char* l1, const char* l2) 
{ 
    return strncmp(l1, l2, WAD_LUMPNAME_LEN);
}

//
// FindPositionInLumpTree
//
// Find the position of this entry in the lump name. Used to
// determine duplicates.
//
wad_lumpinfo_t* FindPositionInLumpTree(wad_lumpinfo_t* root, const char* name, int *cmp_res)
{
    wad_lumpinfo_t* node = root;
    int cmp;

    while (1) 
    {
        cmp = LumpnameCompare(node->name, name);
        if (cmp < 0)
        {
            if (node->left)
                node = node->left;
            else 
                break; 
        }
        else if (cmp > 0)
        {
            if (node->right)
                node = node->right;
            else
                break;
        }
        else
        {
            break;
        }
    }

    if (cmp_res != NULL)
        *cmp_res = cmp;

    return node;
} 

//
// WAD_Write
//
boolean_t WAD_Write(int wad_id, 
                    const char* name,
                    const byte* data, 
                    const int size)
{
    if (wad_id < 0 || wad_id >= wad_file_count || !name || !data || size<=0)
        return false;

    wad_file_t *wf = wad_files + wad_id;
    if (wf->status != INITED && wf->status != WRITE_IN_PROGRESS)
        return false;

    wad_lumpinfo_t *treeparent = NULL;
    wad_lumpinfo_t *lumpinf = NULL;
    int compare_res = 0;

    if (wf->status == INITED)
    {
        wf->fp = OpenWADForWriting(wf->filename, wf->id);
        if (!wf->fp)
            return false;

        lumpinf = malloc(sizeof(wad_lumpinfo_t));
        wf->head = lumpinf;
        wf->tail = lumpinf;
        wf->status = WRITE_IN_PROGRESS;
    }
    else
    {
        treeparent = FindPositionInLumpTree(wf->head, name, &compare_res);
        if (!compare_res)
            return false;

        lumpinf = malloc(sizeof(wad_lumpinfo_t));

        // Place in tree
        if (compare_res < 0)
            treeparent->left = lumpinf;
        else // implied: if (compare_res > 0)
            treeparent->right = lumpinf;

        // Place in linked list
        wf->tail->next = lumpinf;
        wf->tail = wf->tail->next; 
    }

    PadFile(wf->fp, ftell(wf->fp));

    memset(lumpinf, 0, sizeof(wad_lumpinfo_t));
    strncpy(lumpinf->name, name, WAD_LUMPNAME_LEN); 
    lumpinf->fileofs = ftell(wf->fp);
    lumpinf->size = size;    

    return (fwrite((void*)data, sizeof(byte), size, wf->fp) == size);
}

//
// FinishWADWrite
//
boolean_t FinishWADWrite(FILE* fp, wad_lumpinfo_t *lumpinf)
{
    wad_lumpinfo_t *next = lumpinf;
    int lumpcount = 0;
    int lumptableofs = 0;

    // Pad the file 
    PadFile(fp, ftell(fp));

    // Get the position of the lump table
    lumptableofs = ftell(fp);

    // Write Lumpinfo(s)
    while(next)
    {
        lumpinf = next;

        fwrite((void*)&lumpinf->fileofs, sizeof(int), 1, fp); 
        fwrite((void*)&lumpinf->size, sizeof(int), 1, fp); 
        fwrite((void*)lumpinf->name, sizeof(char), WAD_LUMPNAME_LEN, fp);

        next = lumpinf->next;
        free(lumpinf);

        lumpcount++;
    }

    // Write table offset
    fseek(fp, WADTYPE_LEN, SEEK_SET);
    fwrite((void*)&lumpcount, sizeof(int), 1, fp);
    fwrite((void*)&lumptableofs, sizeof(int), 1, fp); // Now write table offset

    return true;
}

//
// WAD_Close
//
boolean_t WAD_Close(int wad_id)
{
    if (wad_id < 0 || wad_id >= wad_file_count)
        return false;

    wad_file_t *wf = wad_files + wad_id;

    if (wf->status == WRITE_IN_PROGRESS)
    {
        FinishWADWrite(wf->fp, wf->head);
    }

    if (wf->fp)
    {
        fclose(wf->fp);
        wf->fp = NULL;
    }
        
    if (wf->filename)
	{
        free(wf->filename);
		wf->filename = NULL;
	}

    wf->status = ALLOCATED;
    return true;
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
