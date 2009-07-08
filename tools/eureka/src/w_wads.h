/*  
 *  wads2.h
 *  Wads functions that are not needed during editing.
 *  AYM 2000-05-06
 */


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
