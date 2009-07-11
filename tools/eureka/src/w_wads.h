//------------------------------------------------------------------------
//  WAD STUFF
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor (C) 2001-2009 Andrew Apted
//                     (C) 1997-2003 André Majorel et al
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
//
//  Based on Yadex which incorporated code from DEU 5.21 that was put
//  in the public domain in 1994 by Raphaël Quinet and Brendon Wyber.
//
//------------------------------------------------------------------------


#ifndef YH_WADS2  /* DO NOT INSERT ANYTHING BEFORE THIS LINE */
#define YH_WADS2


class Wad_file;


int OpenMainWad (const char *);
int OpenPatchWad (const char *);
void CloseWadFiles (void);
void CloseUnusedWadFiles (void);
Wad_file *BasicWadOpen (const char *, ygpf_t pic_format);
void ListMasterDirectory (FILE *);
void ListFileDirectory (FILE *, const Wad_file *);
void DumpDirectoryEntry (FILE *, const char *);
void SaveDirectoryEntry (FILE *, const char *);
void SaveEntryToRawFile (FILE *, const char *);
void SaveEntryFromRawFile (FILE *, FILE *, const char *);


#endif  /* DO NOT ADD ANYTHING AFTER THIS LINE */
//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
