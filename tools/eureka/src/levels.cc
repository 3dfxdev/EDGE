/*
 *  levels.cc
 *  Level loading and saving routines,
 *  global variables used to hold the level data.
 *  BW & RQ sometime in 1993 or 1994.
 */


/*
This file is part of Yadex.

Yadex incorporates code from DEU 5.21 that was put in the public domain in
1994 by Raphaël Quinet and Brendon Wyber.

The rest of Yadex is Copyright © 1997-2003 André Majorel and others.

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation; either version 2 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program; if not, write to the Free Software Foundation, Inc., 59 Temple
Place, Suite 330, Boston, MA 02111-1307, USA.
*/


#include "yadex.h"
#include "m_bitvec.h"
#include "dialog.h"
#include "game.h"
#include "levels.h"
#include "objid.h"
#include "w_structs.h"
#include "things.h"
#include "w_file.h"
#include "w_io.h"
#include "w_wads.h"


/*
  FIXME
  All these variables should be turned
  into members of a "Level" class.
*/
MDirPtr Level;      /* master dictionary entry for the level */
int NumThings;      /* number of things */
TPtr Things;      /* things data */
int NumLineDefs;    /* number of line defs */
LDPtr LineDefs;     /* line defs data */
int NumSideDefs;    /* number of side defs */
SDPtr SideDefs;     /* side defs data */
int NumVertices;    /* number of vertexes */
VPtr Vertices;      /* vertex data */
int NumSectors;     /* number of sectors */
SPtr Sectors;     /* sectors data */

// FIXME should be somewhere else
int NumWTexture;    /* number of wall textures */
char **WTexture;    /* array of wall texture names */

// FIXME all the flat list stuff should be put in a separate class
size_t NumFTexture;   /* number of floor/ceiling textures */
flat_list_entry_t *flat_list; // List of all flats in the directory

int MapMaxX = -32767;   /* maximum X value of map */
int MapMaxY = -32767;   /* maximum Y value of map */
int MapMinX = 32767;    /* minimum X value of map */
int MapMinY = 32767;    /* minimum Y value of map */
bool MadeChanges;   /* made changes? */
bool MadeMapChanges;    /* made changes that need rebuilding? */
unsigned long things_angles;  // See levels.h for description.
unsigned long things_types; // See levels.h for description.
char Level_name[WAD_NAME + 1];  /* The name of the level (E.G.
           "MAP01" or "E1M1"), followed by a
           NUL. If the Level has been created as
           the result of a "c" command with no
           argument, an empty string. The name
           is not necesarily in upper case but
           it always a valid lump name, not a
           command line shortcut like "17". */

y_file_name_t Level_file_name;  /* The name of the file in which
           the level would be saved. If the
           level has been created as the result
           of a "c" command, with or without
           argument, an empty string. */

y_file_name_t Level_file_name_saved;  /* The name of the file in
           which the level was last saved. If
           the Level has never been saved yet,
           an empty string. */

void EmptyLevelData (const char *levelname)
{
Things = 0;
NumThings = 0;
things_angles++;
things_types++;
LineDefs = 0;
NumLineDefs = 0;
SideDefs = 0;
NumSideDefs = 0;
Sectors = 0;
NumSectors = 0;
Vertices = 0;
NumVertices = 0;
}


/*
 *  texno_texname
 *  A convenience function when loading Doom alpha levels
 */
static char *tex_list = 0;
static size_t ntex = 0;
static char tex_name[WAD_TEX_NAME + 1];
inline const char *texno_texname (s16_t texno)
{
if (texno < 0)
   return "-";
else
   if (yg_texture_format == YGTF_NAMELESS)
      {
      sprintf (tex_name, "TEX%04u", (unsigned) texno);
      return tex_name;
      }
   else
      {
      if (texno < (s16_t) ntex)
   return tex_list + WAD_TEX_NAME * texno;
      else
   return "unknown";
      }
}


/*
   read in the level data
*/

int ReadLevelData (const char *levelname)
{
int rc = 0;
MDirPtr dir;
int OldNumVertices;


/* Find the various level information from the master directory */
//DisplayMessage (-1, -1, "Reading data for level %s...", levelname);
Level = FindMasterDir (MasterDir, levelname);
if (!Level)
   fatal_error ("level data not found");

/* Get the number of vertices */
s32_t v_offset = 42;
s32_t v_length = 42;
{
const char *lump_name = "BUG";
if (yg_level_format == YGLF_ALPHA)  // Doom alpha
   lump_name = "POINTS";
else
   lump_name = "VERTEXES";
dir = FindMasterDir (Level, lump_name);
if (dir == 0)
   OldNumVertices = 0;
else
   {
   v_offset = dir->dir.start;
   v_length = dir->dir.size;
   if (yg_level_format == YGLF_ALPHA)  // Doom alpha: skip leading count
      {
      v_offset += 4;
      v_length -= 4;
      }
   OldNumVertices = (int) (v_length / WAD_VERTEX_BYTES);
   if ((s32_t) (OldNumVertices * WAD_VERTEX_BYTES) != v_length)
      warn ("the %s lump has a weird size."
        " The wad might be corrupt.\n", lump_name);
   }
}

// Read THINGS
{
const char *lump_name = "THINGS";
verbmsg ("Reading %s things", levelname);
s32_t offset = 42;
s32_t length;
dir = FindMasterDir (Level, lump_name);
if (dir == 0)
   NumThings = 0;
else
   {
   offset = dir->dir.start;
   length = dir->dir.size;
   if (MainWad == Iwad4)  // Hexen mode
      {
      NumThings = (int) (length / WAD_HEXEN_THING_BYTES);
      if ((s32_t) (NumThings * WAD_HEXEN_THING_BYTES) != length)
         warn ("the %s lump has a weird size."
            " The wad might be corrupt.\n", lump_name);
      }
   else                    // Doom/Heretic/Strife mode
      {
      if (yg_level_format == YGLF_ALPHA)  // Doom alpha: skip leading count
   {
   offset += 4;
   length -= 4;
   }
      size_t thing_size = yg_level_format == YGLF_ALPHA ? 12 : WAD_THING_BYTES;
      NumThings = (int) (length / thing_size);
      if ((s32_t) (NumThings * thing_size) != length)
         warn ("the %s lump has a weird size."
            " The wad might be corrupt.\n", lump_name);
      }
   }
things_angles++;
things_types++;
if (NumThings > 0)
   {
   Things = (TPtr) GetMemory ((unsigned long) NumThings
      * sizeof (struct Thing));
   const Wad_file *wf = dir->wadfile;
   wf->seek (offset);
   if (wf->error ())
      {
      err ("%s: seek error", lump_name);
      rc = 1;
      goto byebye;
      }
   if (MainWad == Iwad4)    // Hexen mode
      for (long n = 0; n < NumThings; n++)
   {
         byte dummy2[6];
   wf->read_i16   ();         // Tid
   wf->read_i16   (&Things[n].x );
   wf->read_i16   (&Things[n].y );
   wf->read_i16   ();         // Height
   wf->read_i16   (&Things[n].angle);
   wf->read_i16   (&Things[n].type );
   wf->read_i16   (&Things[n].when );
         wf->read_bytes (dummy2, sizeof dummy2);
   if (wf->error ())
      {
      err ("%s: error reading thing #%ld", lump_name, n);
      rc = 1;
      goto byebye;
      }
   }
   else         // Doom/Heretic/Strife mode
      for (long n = 0; n < NumThings; n++)
   {
   wf->read_i16 (&Things[n].x );
   wf->read_i16 (&Things[n].y );
   wf->read_i16 (&Things[n].angle);
   wf->read_i16 (&Things[n].type );
   if (yg_level_format == YGLF_ALPHA)
      wf->read_i16 ();    // Alpha. Don't know what it's for.
   wf->read_i16 (&Things[n].when );
   if (wf->error ())
      {
      err ("%s: error reading thing #%ld", lump_name, n);
      rc = 1;
      goto byebye;
      }
   }
   }
}

// Read LINEDEFS
if (yg_level_format != YGLF_ALPHA)
   {
   const char *lump_name = "LINEDEFS";
   verbmsg (" linedefs");
   dir = FindMasterDir (Level, lump_name);
   if (dir == 0)
      NumLineDefs = 0;
   else
      {
      if (MainWad == Iwad4)  // Hexen mode
   {
   NumLineDefs = (int) (dir->dir.size / WAD_HEXEN_LINEDEF_BYTES);
   if ((s32_t) (NumLineDefs * WAD_HEXEN_LINEDEF_BYTES) != dir->dir.size)
      warn ("the %s lump has a weird size."
         " The wad might be corrupt.\n", lump_name);
   }
      else                   // Doom/Heretic/Strife mode
   {
   NumLineDefs = (int) (dir->dir.size / WAD_LINEDEF_BYTES);
   if ((s32_t) (NumLineDefs * WAD_LINEDEF_BYTES) != dir->dir.size)
      warn ("the %s lump has a weird size."
         " The wad might be corrupt.\n", lump_name);
   }
      }
   if (NumLineDefs > 0)
      {
      LineDefs = (LDPtr) GetMemory ((unsigned long) NumLineDefs
   * sizeof (struct LineDef));
      const Wad_file *wf = dir->wadfile;
      wf->seek (dir->dir.start);
      if (wf->error ())
   {
   err ("%s: seek error", lump_name);
   rc = 1;
   goto byebye;
   }
      if (MainWad == Iwad4)  // Hexen mode
   for (long n = 0; n < NumLineDefs; n++)
      {
      byte dummy[6];
      wf->read_i16   (&LineDefs[n].start);
      wf->read_i16   (&LineDefs[n].end);
      wf->read_i16   (&LineDefs[n].flags);
      wf->read_bytes (dummy, sizeof dummy);
      wf->read_i16   (&LineDefs[n].sidedef1);
      wf->read_i16   (&LineDefs[n].sidedef2);
      LineDefs[n].type = dummy[0];
      LineDefs[n].tag  = dummy[1];  // arg1 often contains a tag
      if (wf->error ())
         {
         err ("%s: error reading linedef #%ld", lump_name, n);
         rc = 1;
         goto byebye;
         }
      }
      else                   // Doom/Heretic/Strife mode
   for (long n = 0; n < NumLineDefs; n++)
      {
      wf->read_i16 (&LineDefs[n].start);
      wf->read_i16 (&LineDefs[n].end);
      wf->read_i16 (&LineDefs[n].flags);
      wf->read_i16 (&LineDefs[n].type);
      wf->read_i16 (&LineDefs[n].tag);
      wf->read_i16 (&LineDefs[n].sidedef1);
      wf->read_i16 (&LineDefs[n].sidedef2);
      if (wf->error ())
         {
         err ("%s: error reading linedef #%ld", lump_name, n);
         rc = 1;
         goto byebye;
         }
      }
      }
   }

// Read SIDEDEFS
{
const char *lump_name = "SIDEDEFS";
verbmsg (" sidedefs");
dir = FindMasterDir (Level, lump_name);
if (dir)
   {
   NumSideDefs = (int) (dir->dir.size / WAD_SIDEDEF_BYTES);
   if ((s32_t) (NumSideDefs * WAD_SIDEDEF_BYTES) != dir->dir.size)
      warn ("the SIDEDEFS lump has a weird size."
         " The wad might be corrupt.\n");
   }
else
   NumSideDefs = 0;
if (NumSideDefs > 0)
   {
   SideDefs = (SDPtr) GetMemory ((unsigned long) NumSideDefs
      * sizeof (struct SideDef));
   const Wad_file *wf = dir->wadfile;
   wf->seek (dir->dir.start);
   if (wf->error ())
      {
      err ("%s: seek error", lump_name);
      rc = 1;
      goto byebye;
      }
   for (long n = 0; n < NumSideDefs; n++)
      {
      wf->read_i16   (&SideDefs[n].xoff);
      wf->read_i16   (&SideDefs[n].yoff);
      wf->read_bytes (&SideDefs[n].upper, WAD_TEX_NAME);
      wf->read_bytes (&SideDefs[n].lower, WAD_TEX_NAME);
      wf->read_bytes (&SideDefs[n].middle, WAD_TEX_NAME);
      wf->read_i16   (&SideDefs[n].sector);
      if (wf->error ())
   {
   err ("%s: error reading sidedef #%ld", lump_name, n);
   rc = 1;
   goto byebye;
   }
      }
   }
}

/* Sanity checkings on linedefs: the 1st and 2nd vertices
   must exist. The 1st and 2nd sidedefs must exist or be
   set to -1. */
for (long n = 0; n < NumLineDefs; n++)
   {
   if (LineDefs[n].sidedef1 != -1
      && outside (LineDefs[n].sidedef1, 0, NumSideDefs - 1))
      {
      err ("linedef %ld has bad 1st sidedef number %d, giving up",
   n, LineDefs[n].sidedef1);
      rc = 1;
      goto byebye;
      }
   if (LineDefs[n].sidedef2 != -1
      && outside (LineDefs[n].sidedef2, 0, NumSideDefs - 1))
      {
      err ("linedef %ld has bad 2nd sidedef number %d, giving up",
   n, LineDefs[n].sidedef2);
      rc = 1;
      goto byebye;
      }
   if (outside (LineDefs[n].start, 0, OldNumVertices -1))
      {
      err ("linedef %ld has bad 1st vertex number %d, giving up",
        n, LineDefs[n].start);
      rc = 1;
      goto byebye;
      }
   if (outside (LineDefs[n].end, 0, OldNumVertices - 1))
      {
      err ("linedef %ld has bad 2nd vertex number %d, giving up",
        n, LineDefs[n].end);
      rc = 1;
      goto byebye;
      }
   }


/* Read the vertices. If the wad has been run through a nodes
   builder, there is a bunch of vertices at the end that are not
   used by any linedefs. Those vertices have been created by the
   nodes builder for the segs. We ignore them, because they're
   useless to the level designer. However, we do NOT ignore
   unused vertices in the middle because that's where the
   "string art" bug came from.

   Note that there is absolutely no guarantee that the nodes
   builders add their own vertices at the end, instead of at the
   beginning or right in the middle, AFAIK. It's just that they
   all seem to do that (1). What if some don't ? Well, we would
   end up with many unwanted vertices in the level data. Nothing
   that a good CheckCrossReferences() couldn't take care of. */
{
verbmsg (" vertices");
int last_used_vertex = -1;
for (long n = 0; n < NumLineDefs; n++)
   {
   last_used_vertex = MAX(last_used_vertex, LineDefs[n].start);
   last_used_vertex = MAX(last_used_vertex, LineDefs[n].end);
   }
NumVertices = last_used_vertex + 1;
// This block is only here to warn me if (1) is false.
{
  bitvec_c vertex_used (OldNumVertices);
  for (long n = 0; n < NumLineDefs; n++)
     {
     vertex_used.set (LineDefs[n].start);
     vertex_used.set (LineDefs[n].end);
     }
  int unused = 0;
  for (long n = 0; n <= last_used_vertex; n++)
     {
     if (! vertex_used.get (n))
  unused++;
     }
  if (unused > 0)
     {
     warn ("this level has unused vertices in the middle.\n");
     warn ("total %d, tail %d (%d%%), unused %d (",
  OldNumVertices,
  OldNumVertices - NumVertices,
  NumVertices - unused
     ? 100 * (OldNumVertices - NumVertices) / (NumVertices - unused)
     : 0,
  unused);
     int first = 1;
     for (int n = 0; n <= last_used_vertex; n++)
        {
  if (! vertex_used.get (n))
     {
     if (n == 0 || vertex_used.get (n - 1))
        {
        if (first)
           first = 0;
        else
           warn (", ");
        warn ("%d", n);
        }
     else if (n == last_used_vertex || vertex_used.get (n + 1))
        warn ("-%d", n);
     }
  }
     warn (")\n");
     }
}
// Now load all the vertices except the unused ones at the end.
if (NumVertices > 0)
   {
   const char *lump_name = "BUG";
   Vertices = (VPtr) GetMemory ((unsigned long) NumVertices
      * sizeof (struct Vertex));
   if (yg_level_format == YGLF_ALPHA)  // Doom alpha
      lump_name = "POINTS";
   else
      lump_name = "VERTEXES";
   dir = FindMasterDir (Level, lump_name);
   if (dir == 0)
      goto vertexes_done;   // FIXME isn't that fatal ?
   {
   const Wad_file *wf = dir->wadfile;
   wf->seek (v_offset);
   if (wf->error ())
      {
      err ("%s: seek error", lump_name);
      rc = 1;
      goto byebye;
      }
   MapMaxX = -32767;
   MapMaxY = -32767;
   MapMinX = 32767;
   MapMinY = 32767;
   for (long n = 0; n < NumVertices; n++)
      {
      s16_t val;
      wf->read_i16 (&val);
      if (val < MapMinX)
   MapMinX = val;
      if (val > MapMaxX)
   MapMaxX = val;
      Vertices[n].x = val;
      wf->read_i16 (&val);
      if (val < MapMinY)
   MapMinY = val;
      if (val > MapMaxY)
   MapMaxY = val;
      Vertices[n].y = val;
      if (wf->error ())
   {
   err ("%s: error reading vertex #%ld", lump_name, n);
   rc = 1;
   goto byebye;
   }
      }
   }
   vertexes_done:
   ;
   }
}

// Ignore SEGS, SSECTORS and NODES

// Read SECTORS
{
const char *lump_name = "SECTORS";
verbmsg (" sectors\n");
dir = FindMasterDir (Level, lump_name);
if (yg_level_format != YGLF_ALPHA)
   {
   if (dir)
      {
      NumSectors = (int) (dir->dir.size / WAD_SECTOR_BYTES);
      if ((s32_t) (NumSectors * WAD_SECTOR_BYTES) != dir->dir.size)
   warn ("the %s lump has a weird size."
     " The wad might be corrupt.\n", lump_name);
      }
   else
      NumSectors = 0;
   if (NumSectors > 0)
      {
      Sectors = (SPtr) GetMemory ((unsigned long) NumSectors
   * sizeof (struct Sector));
      const Wad_file *wf = dir->wadfile;
      wf->seek (dir->dir.start);
      if (wf->error ())
   {
   err ("%s: seek error", lump_name);
   rc = 1;
   goto byebye;
   }
      for (long n = 0; n < NumSectors; n++)
   {
   wf->read_i16   (&Sectors[n].floorh);
   wf->read_i16   (&Sectors[n].ceilh);
   wf->read_bytes (&Sectors[n].floort, WAD_FLAT_NAME);
   wf->read_bytes (&Sectors[n].ceilt,  WAD_FLAT_NAME);
   wf->read_i16   (&Sectors[n].light);
   wf->read_i16   (&Sectors[n].special);
   wf->read_i16   (&Sectors[n].tag);
   if (wf->error ())
      {
      err ("%s: error reading sector #%ld", lump_name, n);
      rc = 1;
      goto byebye;
      }
   }
      }
   }
else  // Doom alpha--a wholly different SECTORS format
   {
   s32_t  *offset_table = 0;
   s32_t   nsectors     = 0;
   s32_t   nflatnames   = 0;
   char *flatnames    = 0;
   if (dir == 0)
      {
      warn ("%s: lump not found in directory\n", lump_name);  // FIXME fatal ?
      goto sectors_alpha_done;
      }
   {
   const Wad_file *wf = dir->wadfile;
   wf->seek (dir->dir.start);
   if (wf->error ())
      {
      err ("%s: seek error", lump_name);
      rc = 1;
      goto byebye;
      }
   wf->read_i32 (&nsectors);
   if (wf->error ())
      {
      err ("%s: error reading sector count", lump_name);
      rc = 1;
      goto byebye;
      }
   if (nsectors < 0)
      {
      warn ("Negative sector count. Clamping to 0.\n");
      nsectors = 0;
      }
   NumSectors = nsectors;
   Sectors = (SPtr) GetMemory ((unsigned long) NumSectors
      * sizeof (struct Sector));
   offset_table = new s32_t[nsectors];
   for (size_t n = 0; n < (size_t) nsectors; n++)
      wf->read_i32 (offset_table + n);
   if (wf->error ())
      {
      err ("%s: error reading offsets table", lump_name);
      rc = 1;
      goto sectors_alpha_done;
      }
   // Load FLATNAME
   {
      const char *lump_name = "FLATNAME";
      bool success = false;
      MDirPtr dir2 = FindMasterDir (Level, lump_name);
      if (dir2 == 0)
   {
   warn ("%s: lump not found in directory\n", lump_name);
   goto flatname_done;    // FIXME warn ?
   }
      {
      const Wad_file *wf = dir2->wadfile;
      wf->seek (dir2->dir.start);
      if (wf->error ())
   {
   warn ("%s: seek error\n", lump_name);
   goto flatname_done;
   }
      wf->read_i32 (&nflatnames);
      if (wf->error ())
   {
   warn ("%s: error reading flat name count\n", lump_name);
   nflatnames = 0;
   goto flatname_done;
   }
      if (nflatnames < 0 || nflatnames > 32767)
   {
   warn ("%s: bad flat name count, giving up\n", lump_name);
   nflatnames = 0;
   goto flatname_done;
   }
      else
   {
   flatnames = new char[WAD_FLAT_NAME * nflatnames];
   wf->read_bytes (flatnames, WAD_FLAT_NAME * nflatnames);
   if (wf->error ())
      {
      warn ("%s: error reading flat names\n", lump_name);
      nflatnames = 0;
      goto flatname_done;
      }
   success = true;
   }
      }
      flatname_done:
      if (! success)
   warn ("%s: errors found, you'll have to do without flat names\n",
         lump_name);
   }
   for (size_t n = 0; n < (size_t) nsectors; n++)
      {
      wf->seek (dir->dir.start + offset_table[n]);
      if (wf->error ())
   {
   err ("%s: seek error", lump_name);
   rc = 1;
   goto sectors_alpha_done;
   }
      s16_t index;
      wf->read_i16 (&Sectors[n].floorh);
      wf->read_i16 (&Sectors[n].ceilh );
      wf->read_i16 (&index);
      if (nflatnames && flatnames && index >= 0 && index < nflatnames)
   memcpy (Sectors[n].floort, flatnames + WAD_FLAT_NAME * index,
       WAD_FLAT_NAME);
      else
   strcpy (Sectors[n].floort, "unknown");
      wf->read_i16 (&index);
      if (nflatnames && flatnames && index >= 0 && index < nflatnames)
   memcpy (Sectors[n].ceilt, flatnames + WAD_FLAT_NAME * index,
       WAD_FLAT_NAME);
      else
   strcpy (Sectors[n].ceilt, "unknown");
      wf->read_i16 (&Sectors[n].light);
      wf->read_i16 (&Sectors[n].special);
      wf->read_i16 (&Sectors[n].tag);
      // Don't know what the tail is for. Ignore it.
      if (wf->error ())
   {
   err ("%s: error reading sector #%ld", lump_name, long (n));
   rc = 1;
   goto sectors_alpha_done;
   }
      }
   }
   
   sectors_alpha_done:
   if (offset_table != 0)
      delete[] offset_table;
   if (flatnames != 0)
      delete[] flatnames;
   if (rc != 0)
      goto byebye;
   }
}

/* Sanity checking on sidedefs: the sector must exist. I don't
   make this a fatal error, though, because it's not exceptional
   to find wads with unused sidedefs with a sector# of -1. Well
   known ones include dyst3 (MAP06, MAP07, MAP08), mm (MAP16),
   mm2 (MAP13, MAP28) and requiem (MAP03, MAP08, ...). */
for (long n = 0; n < NumSideDefs; n++)
   {
   if (outside (SideDefs[n].sector, 0, NumSectors - 1))
      warn ("sidedef %ld has bad sector number %d\n",
   n, SideDefs[n].sector);
   }

// Ignore REJECT and BLOCKMAP

// Silly statistics
verbmsg ("  %d things, %d vertices, %d linedefs, %d sidedefs, %d sectors\n",
   (int) NumThings, (int) NumVertices, (int) NumLineDefs,
   (int) NumSideDefs, (int) NumSectors);
verbmsg ("  Map: (%d,%d)-(%d,%d)\n", MapMinX, MapMinY, MapMaxX, MapMaxY);

byebye:
if (rc != 0)
   err ("%s: errors found, giving up", levelname);
return rc;
}



/*
   forget the level data
*/

void ForgetLevelData ()
{
/* forget the things */

NumThings = 0;
if (Things != 0)
   FreeMemory (Things);
Things = 0;
things_angles++;
things_types++;

/* forget the vertices */

NumVertices = 0;
if (Vertices != 0)
   FreeMemory (Vertices);
Vertices = 0;

/* forget the linedefs */

NumLineDefs = 0;
if (LineDefs != 0)
   FreeMemory (LineDefs);
LineDefs = 0;

/* forget the sidedefs */

NumSideDefs = 0;
if (SideDefs != 0)
   FreeMemory (SideDefs);
SideDefs = 0;

/* forget the sectors */

NumSectors = 0;
if (Sectors != 0)
   FreeMemory (Sectors);
Sectors = 0;

}


/*
 *  Save the level data to a pwad file
 *  The name of the level is always obtained from
 *  <level_name>, whether or not the level was created from
 *  scratch.
 *
 *  The previous contents of the pwad file are lost. Yes, it
 *  sucks but it's not easy to fix.
 *
 *  The lumps are always written in the same order, the same
 *  as the one in the Doom iwad. The length field of the
 *  marker lump is always set to 0. Its offset field is
 *  always set to the offset of the first lump of the level
 *  (THINGS).
 *
 *  If the level has been created by editing an existing
 *  level and has not been changed in a way that calls for a
 *  rebuild of the nodes, the VERTEXES, SEGS, SSECTORS,
 *  NODES, REJECT and BLOCKMAP lumps are copied from the
 *  original level. Otherwise, they are created with a
 *  length of 0 bytes and an offset equal to the offset of
 *  the previous lump plus its length.
 *
 *  Returns 0 on success and non-zero on failure (see errno).
 */
int SaveLevelData (const char *outfile, const char *level_name)
{
FILE   *file;
MDirPtr dir;
int     n;
long  lump_offset[WAD_LL__];
size_t  lump_size[WAD_LL__];
wad_level_lump_no_t l;

if (yg_level_format == YGLF_HEXEN || ! strcmp (Game, "hexen"))
   {
   Notify (-1, -1, "I refuse to save. Hexen mode is still",
                   "too badly broken. You would lose data.");
   return 1;
   }
if (! level_name || ! levelname2levelno (level_name))
   {
   nf_bug ("SaveLevelData: bad level_name \"%s\", using \"E1M1\" instead.",
       level_name);
   level_name = "E1M1";
   }
//DisplayMessage (-1, -1, "Saving data to \"%s\"...", outfile);
LogMessage (": Saving data to \"%s\"...\n", outfile);
if ((file = fopen (outfile, "wb")) == NULL)
   {
   char buf1[81];
   char buf2[81];
   y_snprintf (buf1, sizeof buf1, "Can't open \"%.64s\"", outfile);
   y_snprintf (buf2, sizeof buf1, "for writing (%.64s)",  strerror (errno));
   Notify (-1, -1, buf1, buf2);
   return 1;
   }

/* Can we reuse the old nodes ? Not if this is a new level from
   scratch or if the structure of the level has changed. If the
   level comes from an alpha version of Doom, we can't either
   because that version of Doom didn't have SEGS, NODES, etc. */
bool reuse_nodes = Level
  && ! MadeMapChanges
  && yg_level_format != YGLF_ALPHA;

// Write the pwad header
WriteBytes (file, "PWAD", 4);   // Pwad file
file_write_i32 (file, WAD_LL__);  // Number of entries = 11
file_write_i32 (file, 0);   // Fix this up later
if (Level)
   dir = Level->next;
else
   dir = 0;  // Useless except to trap accidental dereferences

// The label (EnMm or MAPnm)
l = WAD_LL_LABEL;
lump_offset[l] = ftell (file);  // By definition
lump_size[l]   =  0;    // By definition
 
// Write the THINGS lump
l = WAD_LL_THINGS;
lump_offset[l] = ftell (file);

for (n = 0; n < NumThings; n++)
   {
   file_write_i16 (file, Things[n].x );
   file_write_i16 (file, Things[n].y );
   file_write_i16 (file, Things[n].angle);
   file_write_i16 (file, Things[n].type );
   file_write_i16 (file, Things[n].when );
   }
lump_size[l] = ftell (file) - lump_offset[l];
if (Level)
   dir = dir->next;

// Write the LINEDEFS lump
l = WAD_LL_LINEDEFS;
lump_offset[WAD_LL_LINEDEFS] = ftell (file);

for (n = 0; n < NumLineDefs; n++)
   {
   file_write_i16 (file, LineDefs[n].start   );
   file_write_i16 (file, LineDefs[n].end     );
   file_write_i16 (file, LineDefs[n].flags   );
   file_write_i16 (file, LineDefs[n].type    );
   file_write_i16 (file, LineDefs[n].tag     );
   file_write_i16 (file, LineDefs[n].sidedef1);
   file_write_i16 (file, LineDefs[n].sidedef2);
   }
lump_size[l] = ftell (file) - lump_offset[l];
if (Level)
   dir = dir->next;

// Write the SIDEDEFS lump
l = WAD_LL_SIDEDEFS;
lump_offset[l] = ftell (file);

for (n = 0; n < NumSideDefs; n++)
   {
   file_write_i16 (file, SideDefs[n].xoff);
   file_write_i16 (file, SideDefs[n].yoff);
   WriteBytes     (file, &(SideDefs[n].upper), WAD_TEX_NAME);
   WriteBytes     (file, &(SideDefs[n].lower), WAD_TEX_NAME);
   WriteBytes     (file, &(SideDefs[n].middle), WAD_TEX_NAME);
   file_write_i16 (file, SideDefs[n].sector);
   }
lump_size[l] = ftell (file) - lump_offset[l];
if (Level)
   dir = dir->next;

// Write the VERTEXES lump
l = WAD_LL_VERTEXES;
lump_offset[WAD_LL_VERTEXES] = ftell (file);
if (reuse_nodes)
   {
   /* Copy the vertices */
   
   const Wad_file *wf = dir->wadfile;
   wf->seek (dir->dir.start);
   if (wf->error ())
      {
      warn ("%s: seek error\n", wad_level_lump[l]);
      }
   copy_bytes (file, wf->fp, dir->dir.size);
   }
else
   {
   /* Write the vertices */
   
   for (n = 0; n < NumVertices; n++)
      {
      file_write_i16 (file, Vertices[n].x);
      file_write_i16 (file, Vertices[n].y);
      }
   }
lump_size[l] = ftell (file) - lump_offset[l];
if (Level)
   dir = dir->next;

// Write the SEGS, SSECTORS and NODES lumps
for (n = 0; n < 3; n++)
   {
   if (n == 0)
      l = WAD_LL_SEGS;
   else if (n == 1)
      l = WAD_LL_SSECTORS;
   else if (n == 2)
      l = WAD_LL_NODES;
   lump_offset[l] = ftell (file);
   if (reuse_nodes)
      {
      const Wad_file *wf = dir->wadfile;
      wf->seek (dir->dir.start);
      if (wf->error ())
   {
   warn ("%s: seek error\n", wad_level_lump[l]);
   }
      copy_bytes (file, wf->fp, dir->dir.size);
      }
   lump_size[l] = ftell (file) - lump_offset[l];
   if (Level)
      dir = dir->next;
   }

// Write the SECTORS lump
l = WAD_LL_SECTORS;
lump_offset[l] = ftell (file);

for (n = 0; n < NumSectors; n++)
   {
   file_write_i16 (file, Sectors[n].floorh);
   file_write_i16 (file, Sectors[n].ceilh );
   WriteBytes     (file, Sectors[n].floort, WAD_FLAT_NAME);
   WriteBytes     (file, Sectors[n].ceilt,  WAD_FLAT_NAME);
   file_write_i16 (file, Sectors[n].light  );
   file_write_i16 (file, Sectors[n].special);
   file_write_i16 (file, Sectors[n].tag    );
   }
lump_size[l] = ftell (file) - lump_offset[l];
if (Level)
   dir = dir->next;

// Write the REJECT lump
l = WAD_LL_REJECT;
lump_offset[l] = ftell (file);
if (reuse_nodes)
   {
   /* Copy the REJECT data */
   
   const Wad_file *wf = dir->wadfile;
   wf->seek (dir->dir.start);
   if (wf->error ())
      {
      warn ("%s: seek error\n", wad_level_lump[l]);
      }
   copy_bytes (file, wf->fp, dir->dir.size);
   }
lump_size[l] = ftell (file) - lump_offset[l];
if (Level)
   dir = dir->next;

// Write the BLOCKMAP lump
l = WAD_LL_BLOCKMAP;
lump_offset[l] = ftell (file);
if (reuse_nodes)
   {
   
   const Wad_file *wf = dir->wadfile;
   wf->seek (dir->dir.start);
   if (wf->error ())
      {
      warn ("%s: seek error\n", wad_level_lump[l]);
      }
   copy_bytes (file, wf->fp, dir->dir.size);
   }
lump_size[l] = ftell (file) - lump_offset[l];
if (Level)
   dir = dir->next;

// Write the actual directory
long dir_offset = ftell (file);
for (int L = 0; L < (int) WAD_LL__; L++)
   {
   file_write_i32 (file, lump_offset[L]);
   file_write_i32 (file, lump_size[L]);
   if (L == (int) WAD_LL_LABEL)
      file_write_name (file, level_name);
   else
      file_write_name (file, wad_level_lump[L].name);
   }

/* Fix up the directory start information */
if (fseek (file, 8, SEEK_SET))
   {
   char buf1[81];
   char buf2[81];
   y_snprintf (buf1, sizeof buf1, "%.64s: seek error", outfile);
   y_snprintf (buf2, sizeof buf2, "(%.64s)",           strerror (errno));
   Notify (-1, -1, buf1, buf2);
   fclose (file);
   return 1;
   }
file_write_i32 (file, dir_offset);

/* Close the file */
if (fclose (file))
   {
   char buf1[81];
   char buf2[81];
   y_snprintf (buf1, sizeof buf1, "%.64s: write error", outfile);
   y_snprintf (buf2, sizeof buf2, "(%.64s)",            strerror (errno));
   Notify (-1, -1, buf1, buf2);
   return 1;
   }

/* The file is now up to date */
if (! Level || MadeMapChanges)
   remind_to_build_nodes = 1;
MadeChanges = 0;
MadeMapChanges = 0;


/* Update pointers in Master Directory */
OpenPatchWad (outfile);

/* This should free the old "*.bak" file */
CloseUnusedWadFiles ();

/* Update MapMinX, MapMinY, MapMaxX, MapMaxY */
// Probably not necessary anymore -- AYM 1999-04-05

update_level_bounds ();
return 0;
}


/*
 *  flat_list_entry_cmp
 *  Function used by qsort() to sort the flat_list_entry array
 *  by ascending flat name.
 */
static int flat_list_entry_cmp (const void *a, const void *b)
{
return y_strnicmp (
    ((const flat_list_entry_t *) a)->name,
    ((const flat_list_entry_t *) b)->name,
    WAD_FLAT_NAME);
}


/*
   function used by qsort to sort the texture names
*/
static int SortTextures (const void *a, const void *b)
{
return y_strnicmp (*((const char *const *)a), *((const char *const *)b),
    WAD_TEX_NAME);
}


/*
   read the texture names
*/
void ReadWTextureNames ()
{
MDirPtr dir;
int n;
s32_t val;

verbmsg ("Reading texture names\n");

// Doom alpha 0.4 : "TEXTURES", no names
if (yg_texture_lumps == YGTL_TEXTURES
 && yg_texture_format == YGTF_NAMELESS)
   {
   const char *lump_name = "TEXTURES";
   dir = FindMasterDir (MasterDir, lump_name);
   if (dir == NULL)
      {
      warn ("%s: lump not found in directory\n", lump_name);
      goto textures04_done;
      }
   {
   const Wad_file *wf = dir->wadfile;
   wf->seek (dir->dir.start);
   if (wf->error ())
      {
      warn ("%s: seek error\n", lump_name);
      goto textures04_done;
      }
   wf->read_i32 (&val);
   if (wf->error ())
      {
      warn ("%s: error reading texture count\n", lump_name);
      goto textures04_done;
      }
   NumWTexture = (int) val + 1;
   WTexture = (char **) GetMemory ((long) NumWTexture * sizeof *WTexture);
   WTexture[0] = (char *) GetMemory (WAD_TEX_NAME + 1);
   strcpy (WTexture[0], "-");
   if (WAD_TEX_NAME < 7) nf_bug ("WAD_TEX_NAME too small");  // Sanity
   for (long n = 0; n < val; n++)
      {
      WTexture[n + 1] = (char *) GetMemory (WAD_TEX_NAME + 1);
      if (n > 9999)
   {
   warn ("more than 10,000 textures. Ignoring excess.\n");
   break;
   }
      sprintf (WTexture[n + 1], "TEX%04ld", n);
      }
   }
   textures04_done:
   ;
   }

// Doom alpha 0.5 : only "TEXTURES"
else if (yg_texture_lumps == YGTL_TEXTURES
      && (yg_texture_format == YGTF_NORMAL
    || yg_texture_format == YGTF_STRIFE11))
   {
   const char *lump_name = "TEXTURES";
   s32_t *offsets = 0;
   dir = FindMasterDir (MasterDir, lump_name);
   if (dir == NULL)  // In theory it always exists, though
      {
      warn ("%s: lump not found in directory\n", lump_name);
      goto textures05_done;
      }
   {
   const Wad_file *wf = dir->wadfile;
   wf->seek (dir->dir.start);
   if (wf->error ())
      {
      warn ("%s: seek error\n", lump_name);
      goto textures05_done;
      }
   wf->read_i32 (&val);
   if (wf->error ())
      {
      warn ("%s: error reading texture count\n", lump_name);
      goto textures05_done;
      }
   NumWTexture = (int) val + 1;
   /* read in the offsets for texture1 names */
   offsets = (s32_t *) GetMemory ((long) NumWTexture * 4);
   wf->read_i32 (offsets + 1, NumWTexture - 1);
   if (wf->error ())
      {
      warn ("%s: error reading offsets table\n", lump_name);
      goto textures05_done;
      }
   /* read in the actual names */
   WTexture = (char **) GetMemory ((long) NumWTexture * sizeof (char *));
   WTexture[0] = (char *) GetMemory (WAD_TEX_NAME + 1);
   strcpy (WTexture[0], "-");
   for (n = 1; n < NumWTexture; n++)
      {
      WTexture[n] = (char *) GetMemory (WAD_TEX_NAME + 1);
      long offset = dir->dir.start + offsets[n];
      wf->seek (offset);
      if (wf->error ())
   {
   warn ("%s: error seeking to  error\n", lump_name);
   goto textures05_done;    // FIXME cleanup
   }
      wf->read_bytes (WTexture[n], WAD_TEX_NAME);
      if (wf->error ())
   {
   warn ("%s: error reading texture names\n", lump_name);
   goto textures05_done;    // FIXME cleanup
   }
      WTexture[n][WAD_TEX_NAME] = '\0';
      }
   }
   textures05_done:
   if (offsets != 0)
      FreeMemory (offsets);
   }
// Other iwads : "TEXTURE1" and possibly "TEXTURE2"
else if (yg_texture_lumps == YGTL_NORMAL
      && (yg_texture_format == YGTF_NORMAL
    || yg_texture_format == YGTF_STRIFE11))
   {
   const char *lump_name = "TEXTURE1";
   s32_t *offsets = 0;
   dir = FindMasterDir (MasterDir, lump_name);
   if (dir != NULL)  // In theory it always exists, though
      {
      const Wad_file *wf = dir->wadfile;
      wf->seek (dir->dir.start);
      if (wf->error ())
   {
   warn ("%s: seek error\n", lump_name);
   // FIXME
   }
      wf->read_i32 (&val);
      if (wf->error ())
      {
  // FIXME
      }
      NumWTexture = (int) val + 1;
      /* read in the offsets for texture1 names */
      offsets = (s32_t *) GetMemory ((long) NumWTexture * 4);
      wf->read_i32 (offsets + 1, NumWTexture - 1);
      {
  // FIXME
      }
      /* read in the actual names */
      WTexture = (char **) GetMemory ((long) NumWTexture * sizeof (char *));
      WTexture[0] = (char *) GetMemory (WAD_TEX_NAME + 1);
      strcpy (WTexture[0], "-");
      for (n = 1; n < NumWTexture; n++)
   {
   WTexture[n] = (char *) GetMemory (WAD_TEX_NAME + 1);
   wf->seek (dir->dir.start + offsets[n]);
   if (wf->error ())
      {
      warn ("%s: seek error\n", lump_name);
      // FIXME
      }
   wf->read_bytes (WTexture[n], WAD_TEX_NAME);
   if (wf->error ())
      {
        // FIXME
      }
   WTexture[n][WAD_TEX_NAME] = '\0';
   }
      FreeMemory (offsets);
      }
   {
   dir = FindMasterDir (MasterDir, "TEXTURE2");
   if (dir)  /* Doom II has no TEXTURE2 */
      {
      const Wad_file *wf = dir->wadfile;
      wf->seek (dir->dir.start);
      if (wf->error ())
   {
   warn ("%s: seek error\n", lump_name);
   // FIXME
   }
      wf->read_i32 (&val);
      if (wf->error ())
      {
  // FIXME
      }
      /* read in the offsets for texture2 names */
      offsets = (s32_t *) GetMemory ((long) val * 4);
      wf->read_i32 (offsets, val);
      if (wf->error ())
      {
  // FIXME
      }
      /* read in the actual names */
      WTexture = (char **) ResizeMemory (WTexture,
   (NumWTexture + val) * sizeof (char *));
      for (n = 0; n < val; n++)
   {
   WTexture[NumWTexture + n] = (char *) GetMemory (WAD_TEX_NAME + 1);
   wf->seek (dir->dir.start + offsets[n]);
   if (wf->error ())
      {
      warn ("%s: seek error\n", lump_name);
      // FIXME
      }
   wf->read_bytes (WTexture[NumWTexture + n], WAD_TEX_NAME);
   if (wf->error ())
     ; // FIXME
   WTexture[NumWTexture + n][WAD_TEX_NAME] = '\0';
   }
      NumWTexture += val;
      FreeMemory (offsets);
      }
   }
   }
else
   nf_bug ("Invalid texture_format/texture_lumps combination.");

/* sort the names */
qsort (WTexture, NumWTexture, sizeof (char *), SortTextures);
}



/*
   forget the texture names
*/

void ForgetWTextureNames ()
{
int n;

/* forget all names */
for (n = 0; n < NumWTexture; n++)
   FreeMemory (WTexture[n]);

/* forget the array */
NumWTexture = 0;
FreeMemory (WTexture);
}



/*
   read the flat names
*/

void ReadFTextureNames ()
{
MDirPtr dir;
int n;

verbmsg ("Reading flat names");
NumFTexture = 0;

for (dir = MasterDir; (dir = FindMasterDir (dir, "F_START", "FF_START"));)
   {
   bool ff_start = ! y_strnicmp (dir->dir.name, "FF_START", WAD_NAME);
   MDirPtr dir0;
   /* count the names */
   dir = dir->next;
   dir0 = dir;
   for (n = 0; dir && y_strnicmp (dir->dir.name, "F_END", WAD_NAME)
   && (! ff_start || y_strnicmp (dir->dir.name, "FF_END", WAD_NAME));
   dir = dir->next)
      {
      if (dir->dir.start == 0 || dir->dir.size == 0)
   {
   if (! (toupper (dir->dir.name[0]) == 'F'
       && (dir->dir.name[1] == '1'
        || dir->dir.name[1] == '2'
        || dir->dir.name[1] == '3'
        || toupper (dir->dir.name[1]) == 'F')
       && dir->dir.name[2] == '_'
       && (! y_strnicmp (dir->dir.name + 3, "START", WAD_NAME - 3)
     || ! y_strnicmp (dir->dir.name + 3, "END", WAD_NAME - 3))))
      warn ("unexpected label \"%.*s\" among flats.\n",
      WAD_NAME, dir->dir.name);
   continue;
   }
      if (dir->dir.size != 4096)
   warn ("flat \"%.*s\" has weird size %lu."
       " Using 4096 instead.\n",
         WAD_NAME, dir->dir.name, (unsigned long) dir->dir.size);
      n++;
      }
   /* If FF_START/FF_END followed by F_END (mm2.wad), advance
      past F_END. In fact, this does not work because the F_END
      that follows has been snatched by OpenPatchWad(), that
      thinks it replaces the F_END from the iwad. OpenPatchWad()
      needs to be kludged to take this special case into
      account. Fortunately, the only consequence is a useless
      "this wad uses FF_END" warning. -- AYM 1999-07-10 */
   if (ff_start && dir && ! y_strnicmp (dir->dir.name, "FF_END", WAD_NAME))
      if (dir->next && ! y_strnicmp (dir->next->dir.name, "F_END", WAD_NAME))
         dir = dir->next;

   verbmsg (" FF_START/%s %d", dir->dir.name, n);
   if (dir && ! y_strnicmp (dir->dir.name, "FF_END", WAD_NAME))
      warn ("this wad uses FF_END. That won't work with Doom."
   " Use F_END instead.\n");
   /* get the actual names from master dir. */
   flat_list = (flat_list_entry_t *) ResizeMemory (flat_list,
      (NumFTexture + n) * sizeof *flat_list);
   for (size_t m = NumFTexture; m < NumFTexture + n; dir0 = dir0->next)
      {
      // Skip all labels.
      if (dir0->dir.start == 0
   || dir0->dir.size == 0
   || (toupper (dir0->dir.name[0]) == 'F'
       && (dir0->dir.name[1] == '1'
        || dir0->dir.name[1] == '2'
        || dir0->dir.name[1] == '3'
        || toupper (dir0->dir.name[1]) == 'F')
       && dir0->dir.name[2] == '_'
       && (! y_strnicmp (dir0->dir.name + 3, "START", WAD_NAME - 3)
     || ! y_strnicmp (dir0->dir.name + 3, "END", WAD_NAME - 3))
       ))
   continue;
      *flat_list[m].name = '\0';
      strncat (flat_list[m].name, dir0->dir.name, sizeof flat_list[m].name - 1);
      flat_list[m].wadfile = dir0->wadfile;
      flat_list[m].offset  = dir0->dir.start;
      m++;
      }
   NumFTexture += n;
   }

verbmsg ("\n");

/* sort the flats by names */
qsort (flat_list, NumFTexture, sizeof *flat_list, flat_list_entry_cmp);

/* Eliminate all but the last duplicates of a flat. Suboptimal.
   Would be smarter to start by the end. */
for (size_t n = 0; n < NumFTexture; n++)
   {
   size_t m = n;
   while (m + 1 < NumFTexture
     && ! flat_list_entry_cmp (flat_list + n, flat_list + m + 1))
       m++;
   // m now contains the index of the last duplicate
   int nduplicates = m - n;
   if (nduplicates > 0)
      {
      memmove (flat_list + n, flat_list + m,
                                      (NumFTexture - m) * sizeof *flat_list);
      NumFTexture -= nduplicates;
      // Note that I'm too lazy to resize flat_list...
      }
   }
}


/*
 *  is_flat_name_in_list
 *  FIXME should use bsearch()
 */
int is_flat_name_in_list (const char *name)
{
  if (! flat_list)
    return 0;
  for (size_t n = 0; n < NumFTexture; n++)
    if (! y_strnicmp (name, flat_list[n].name, WAD_FLAT_NAME))
      return 1;
  return 0;
}


/*
   forget the flat names
*/

void ForgetFTextureNames ()
{
NumFTexture = 0;
FreeMemory (flat_list);
flat_list = 0;
}


/*
 *  update_level_bounds - update Map{Min,Max}{X,Y}
 */
void update_level_bounds ()
{
MapMaxX = -32767;
MapMaxY = -32767;
MapMinX = 32767;
MapMinY = 32767;
for (obj_no_t n = 0; n < NumVertices; n++)
   {
   int x = Vertices[n].x;
   if (x < MapMinX)
      MapMinX = x;
   if (x > MapMaxX)
      MapMaxX = x;
   int y = Vertices[n].y;
   if (y < MapMinY)
      MapMinY = y;
   if (y > MapMaxY)
      MapMaxY = y;
   }
}

