/*
 *  editobj.cc
 *  Object editing routines.
 *  BW & RQ sometime in 1993 or 1994.
 */


/*
This file is part of Yadex.

Yadex incorporates code from DEU 5.21 that was put in the public domain in
1994 by Rapha�l Quinet and Brendon Wyber.

The rest of Yadex is Copyright � 1997-2003 Andr� Majorel and others.

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
#include "edit2.h"
#include "dialog.h"
#include "editobj.h"
#include "game.h"
#include "gfx.h"
#include "levels.h"
#include "objects.h"
#include "objid.h"
#include "sectors.h"
#include "s_misc.h"
#include "selectn.h"
#include "x_mirror.h"



/*
   insert a standard object at given position
*/

void InsertStandardObject (int xpos, int ypos, int choice)
{
#if 0  // TODO !!! InsertStandardObject

int sector;
int n;
int a, b;

/* show where the object will be inserted */
DrawPointer (1);
/* are we inside a Sector? */
Objid o;
GetCurObject (o, OBJ_SECTORS, xpos, ypos);
sector = o.num;

/* !!!! Should also check for overlapping objects (sectors) !!!! */
switch (choice)
   {
   case 1:
      a = 256;
      b = 128;
      if (Input2Numbers (-1, -1, "Width", "Height", 2000, 2000, &a, &b))
   {
   if (a < 8)
      a = 8;
   if (b < 8)
      b = 8;
   xpos = xpos - a / 2;
   ypos = ypos - b / 2;
   InsertObject (OBJ_VERTICES, -1, xpos, ypos);
   InsertObject (OBJ_VERTICES, -1, xpos + a, ypos);
   InsertObject (OBJ_VERTICES, -1, xpos + a, ypos + b);
   InsertObject (OBJ_VERTICES, -1, xpos, ypos + b);
   if (sector < 0)
      InsertObject (OBJ_SECTORS, -1, 0, 0);
   for (n = 0; n < 4; n++)
      {
      InsertObject (OBJ_LINEDEFS, -1, 0, 0);
      LineDefs[NumLineDefs - 1].sidedef1 = NumSideDefs;
      InsertObject (OBJ_SIDEDEFS, -1, 0, 0);
      if (sector >= 0)
         SideDefs[NumSideDefs - 1].sector = sector;
      }
   
   if (sector >= 0)
      {
      LineDefs[NumLineDefs - 4].start = NumVertices - 4;
      LineDefs[NumLineDefs - 4].end = NumVertices - 3;
      LineDefs[NumLineDefs - 3].start = NumVertices - 3;
      LineDefs[NumLineDefs - 3].end = NumVertices - 2;
      LineDefs[NumLineDefs - 2].start = NumVertices - 2;
      LineDefs[NumLineDefs - 2].end = NumVertices - 1;
      LineDefs[NumLineDefs - 1].start = NumVertices - 1;
      LineDefs[NumLineDefs - 1].end = NumVertices - 4;
      }
   else
      {
      LineDefs[NumLineDefs - 4].start = NumVertices - 1;
      LineDefs[NumLineDefs - 4].end = NumVertices - 2;
      LineDefs[NumLineDefs - 3].start = NumVertices - 2;
      LineDefs[NumLineDefs - 3].end = NumVertices - 3;
      LineDefs[NumLineDefs - 2].start = NumVertices - 3;
      LineDefs[NumLineDefs - 2].end = NumVertices - 4;
      LineDefs[NumLineDefs - 1].start = NumVertices - 4;
      LineDefs[NumLineDefs - 1].end = NumVertices - 1;
      }
   }
      break;
   case 2:
      a = 8;
      b = 128;
      if (Input2Numbers (-1, -1, "Number of sides", "Radius", 32, 2000, &a, &b))
   {
   if (a < 3)
      a = 3;
   if (b < 8)
      b = 8;
   InsertPolygonVertices (xpos, ypos, a, b);
   if (sector < 0)
      InsertObject (OBJ_SECTORS, -1, 0, 0);
   for (n = 0; n < a; n++)
      {
      InsertObject (OBJ_LINEDEFS, -1, 0, 0);
      LineDefs[NumLineDefs - 1].sidedef1 = NumSideDefs;
      InsertObject (OBJ_SIDEDEFS, -1, 0, 0);
      if (sector >= 0)
         SideDefs[NumSideDefs - 1].sector = sector;
      }
   
   if (sector >= 0)
      {
      LineDefs[NumLineDefs - 1].start = NumVertices - 1;
      LineDefs[NumLineDefs - 1].end = NumVertices - a;
      for (n = 2; n <= a; n++)
         {
         LineDefs[NumLineDefs - n].start = NumVertices - n;
         LineDefs[NumLineDefs - n].end = NumVertices - n + 1;
         }
      }
   else
      {
      LineDefs[NumLineDefs - 1].start = NumVertices - a;
      LineDefs[NumLineDefs - 1].end = NumVertices - 1;
      for (n = 2; n <= a; n++)
         {
         LineDefs[NumLineDefs - n].start = NumVertices - n + 1;
         LineDefs[NumLineDefs - n].end = NumVertices - n;
         }
      }
   }
      break;
   case 3:
   /*
      a = 6;
      b = 16;
      if (Input2Numbers (-1, -1, "Number of steps", "Step height", 32, 48, &a, &b))
   {
   if (a < 2)
      a = 2;
   
   n = Sectors[sector].ceilh;
   h = Sectors[sector].floorh;
   if (a * b < n - h)
      {
      Beep ();
      Notify (-1, -1, "The stairs are too high for this Sector", 0);
      return;
      }
   xpos = xpos - 32;
   ypos = ypos - 32 * a;
   for (n = 0; n < a; n++)
      {
      InsertObject (OBJ_VERTICES, -1, xpos, ypos);
      InsertObject (OBJ_VERTICES, -1, xpos + 64, ypos);
      InsertObject (OBJ_VERTICES, -1, xpos + 64, ypos + 64);
      InsertObject (OBJ_VERTICES, -1, xpos, ypos + 64);
      ypos += 64;
      InsertObject (OBJ_SECTORS, sector, 0, 0);
      h += b;
      Sectors[NumSectors - 1].floorh = h;

      InsertObject (OBJ_LINEDEFS, -1, 0, 0);
      LineDefs[NumLineDefs - 1].sidedef1 = NumSideDefs;
      LineDefs[NumLineDefs - 1].sidedef2 = NumSideDefs + 1;
      InsertObject (OBJ_SIDEDEFS, -1, 0, 0);
      SideDefs[NumSideDefs - 1].sector = sector;
      InsertObject (OBJ_SIDEDEFS, -1, 0, 0);

      
      LineDefs[NumLineDefs - 4].start = NumVertices - 4;
      LineDefs[NumLineDefs - 4].end = NumVertices - 3;
      LineDefs[NumLineDefs - 3].start = NumVertices - 3;
      LineDefs[NumLineDefs - 3].end = NumVertices - 2;
      LineDefs[NumLineDefs - 2].start = NumVertices - 2;
      LineDefs[NumLineDefs - 2].end = NumVertices - 1;
      LineDefs[NumLineDefs - 1].start = NumVertices - 1;
      LineDefs[NumLineDefs - 1].end = NumVertices - 4;
     }
   }
      break;
   */
   case 4:
      NotImplemented ();
      break;
   }
#endif
}



/*
   menu of miscellaneous operations
*/

void MiscOperations (int objtype, SelPtr *list, int val)
{
char   msg[80];
int    angle, scale;

if (val > 1 && ! *list)
   {
   Beep ();
   sprintf (msg, "You must select at least one %s", GetObjectTypeName (objtype));
   Notify (-1, -1, msg, 0);
   return;
   }

/* I think this switch statement deserves a prize for "worst
   gratuitous obfuscation" or something. -- AYM 2000-11-07 */
switch (val)
   {
   case 1:
      // * -> First free tag number
      sprintf (msg, "First free tag number: %d", FindFreeTag ());
      Notify (-1, -1, msg, 0);
      break;

   case 2:
#if 0  // TODO !!!!      // * -> Rotate and scale
      if ((objtype == OBJ_THINGS
         || objtype == OBJ_VERTICES) && ! (*list)->next)
   {
   Beep ();
   sprintf (msg, "You must select more than one %s",
            GetObjectTypeName (objtype));
   Notify (-1, -1, msg, 0);
   return;
   }
      angle = 0;
      scale = 100;
      if (Input2Numbers (-1, -1, "rotation angle (�)", "scale (%)",
         360, 1000, &angle, &scale))
   RotateAndScaleObjects (objtype, *list, (double) angle * 0.0174533,
            (double) scale * 0.01);
#endif
      break;

   case 3:
      // Linedef -> Split
      if (objtype == OBJ_LINEDEFS)
   {
   SplitLineDefs (*list);
   }
      // Sector -> Make door from sector
      else if (objtype == OBJ_SECTORS)
   {
   if ((*list)->next)
      {
      Beep ();
      Notify (-1, -1, "You must select exactly one sector", 0);
      }
   else
      {
      MakeDoorFromSector ((*list)->objnum);
      }
   }
      // Thing -> Spin 45� clockwise
      else if (objtype == OBJ_THINGS)
   {
         spin_things (*list, -45);
   }
      // Vertex -> Delete and join linedefs
      else if (objtype == OBJ_VERTICES)
   {
   DeleteVerticesJoinLineDefs (*list);
   ForgetSelection (list);
   }
      break;

   case 4:
      // Linedef -> Split linedefs and sector
      if (objtype == OBJ_LINEDEFS)
   {
   if (! (*list)->next || (*list)->next->next)
      {
      Beep ();
      Notify (-1, -1, "You must select exactly two linedefs", 0);
      }
   else
      {
      SplitLineDefsAndSector ((*list)->next->objnum, (*list)->objnum);
      ForgetSelection (list);
      }
   }
      // Sector -> Make lift from sector
      else if (objtype == OBJ_SECTORS)
   {
   if ((*list)->next)
      {
      Beep ();
      Notify (-1, -1, "You must select exactly one Sector", 0);
      }
   else
      {
      MakeLiftFromSector ((*list)->objnum);
      }
   }
      // Thing -> Spin 45� counter-clockwise
      else if (objtype == OBJ_THINGS)
         spin_things (*list, 45);
      // Vertex -> Merge
      else if (objtype == OBJ_VERTICES)
   {
   MergeVertices (list);
   }
      break;

   case 5:
      // Linedef -> Delete linedefs and join sectors
      if (objtype == OBJ_LINEDEFS)
   {
   DeleteLineDefsJoinSectors (list);
   }
      // Sector -> Distribute sector floor heights
      else if (objtype == OBJ_SECTORS)
   {
   if (! (*list)->next || ! (*list)->next->next)
      {
      Beep ();
      Notify (-1, -1, "You must select three or more sectors", 0);
      }
   else
      {
      DistributeSectorFloors (*list);
      }
   }
      // Thing -> Mirror horizontally
      else if (objtype == OBJ_THINGS)
         {
   flip_mirror (*list, OBJ_THINGS, 'm');
   }
      // Vertex -> Add linedef and split sector
      else if (objtype == OBJ_VERTICES)
   {
   if (! (*list)->next || (*list)->next->next)
      {
      Beep ();
      Notify (-1, -1, "You must select exactly two vertices", 0);
      }
   else
      {
      SplitSector ((*list)->next->objnum, (*list)->objnum);
      ForgetSelection (list);
      }
   }
      break;

   case 6:
      // Linedef -> Flip
      if (objtype == OBJ_LINEDEFS)
   {
   FlipLineDefs (*list, 1);
   }
      // Sector -> Distribute ceiling heights
      else if (objtype == OBJ_SECTORS)
   {
   if (! (*list)->next || ! (*list)->next->next)
      {
      Beep ();
      Notify (-1, -1, "You must select three or more sectors", 0);
      }
   else
      {
      DistributeSectorCeilings (*list);
      }
   }
      // Things -> Mirror vertically
      else if (objtype == OBJ_THINGS)
   {
   flip_mirror (*list, OBJ_THINGS, 'f');
         }
      // Vertex -> Mirror horizontally
      else if (objtype == OBJ_VERTICES)
   {
   flip_mirror (*list, OBJ_VERTICES, 'm');
   }
      break;

   case 7:
      // Linedefs -> Swap sidedefs
      if (objtype == OBJ_LINEDEFS)
   {
   if (Expert
            || blindly_swap_sidedefs
            || Confirm (-1, -1,
               "Warning: the sector references are also swapped",
               "You may get strange results if you don't know what you are doing..."))
      FlipLineDefs (*list, 0);
   }
      // Sectors -> Raise or lower
      else if (objtype == OBJ_SECTORS)
   {
   RaiseOrLowerSectors (*list);
   }
      // Vertices -> Mirror vertically
      else if (objtype == OBJ_VERTICES)
   {
   flip_mirror (*list, OBJ_VERTICES, 'f');
   }
      break;

   case 8:
      // Linedef ->  Align textures vertically
      if (objtype == OBJ_LINEDEFS)
   {
   SelPtr sdlist, cur;

   /* select all sidedefs */
   
   sdlist = 0;
   for (cur = *list; cur; cur = cur->next)
      {
      if (LineDefs[cur->objnum].sidedef1 >= 0)
         SelectObject (&sdlist, LineDefs[cur->objnum].sidedef1);
      if (LineDefs[cur->objnum].sidedef2 >= 0)
         SelectObject (&sdlist, LineDefs[cur->objnum].sidedef2);
      }
   /* align the textures along the Y axis (height) */
   AlignTexturesY (&sdlist);
   }
      // Sector -> Brighten or darken
      else if (objtype == OBJ_SECTORS)
   {
   BrightenOrDarkenSectors (*list);
   }
      break;

   case 9:
      // Linedef -> Align texture horizontally
      if (objtype == OBJ_LINEDEFS)
   {
   SelPtr sdlist, cur;

   /* select all sidedefs */
   
   sdlist = 0;
   for (cur = *list; cur; cur = cur->next)
      {
      if (LineDefs[cur->objnum].sidedef1 >= 0)
         SelectObject (&sdlist, LineDefs[cur->objnum].sidedef1);
      if (LineDefs[cur->objnum].sidedef2 >= 0)
         SelectObject (&sdlist, LineDefs[cur->objnum].sidedef2);
      }
   /* align the textures along the X axis (width) */
   AlignTexturesX (&sdlist);
   }
      // Sector -> Unlink room
      else if (objtype == OBJ_SECTORS)
   {
   NotImplemented ();  // FIXME
   break;
   }
      break;

   case 10:
      // Linedef -> Make linedef single-sided
      if (objtype == OBJ_LINEDEFS)
   {
   SelPtr cur;
   
   for (cur = *list; cur; cur = cur->next)
      {
      struct LineDef *l = LineDefs + cur->objnum;
      l->sidedef2 = -1;  /* remove ref. to 2nd SD */
      l->flags &= ~0x04; /* clear "2S" bit */
      l->flags |= 0x01;  /* set "Im" bit */

      if (is_sidedef (l->sidedef1))
         {
         struct SideDef *s = SideDefs + l->sidedef1;
         strcpy (s->upper, "-");
         strcpy (s->lower, "-");
         strcpy (s->middle, default_middle_texture);
         }
      /* Don't delete the 2nd sidedef, it could be used
               by another linedef. And if it isn't, the next
               cross-references check will delete it anyway. */
      }
   }
      // Sector -> Mirror horizontally
      else if (objtype == OBJ_SECTORS)
   {
   flip_mirror (*list, OBJ_SECTORS, 'm');
   }
      break;

   case 11:
      // Linedef -> Make rectangular nook
      if (objtype == OBJ_LINEDEFS)
   MakeRectangularNook (*list, 32, 16, 0);
      // Sector -> Mirror vertically
      else if (objtype == OBJ_SECTORS)
   {
   flip_mirror (*list, OBJ_SECTORS, 'f');
   }
      break;

   case 12:
      // Linedef -> Make rectangular boss
      if (objtype == OBJ_LINEDEFS)
   MakeRectangularNook (*list, 32, 16, 1);
      // Sector -> Swap flats
      else if (objtype == OBJ_SECTORS)
   swap_flats (*list);
      break;

   case 13:
#if 0  // TODO   // Linedef -> Set length (1st vertex)
      if (objtype == OBJ_LINEDEFS)
   {
   static int length = 24;
   length = InputIntegerValue (-1, -1, -10000, 10000, length);
   if (length != IIV_CANCEL)
     SetLinedefLength (*list, length, 0);
   }
#endif
      break;

   case 14:
#if 0 // TODO      // Linedef -> Set length (2nd vertex)
      if (objtype == OBJ_LINEDEFS)
   {
   static int length = 24;
   length = InputIntegerValue (-1, -1, -10000, 10000, length);
   if (length != IIV_CANCEL)
     SetLinedefLength (*list, length, 1);
   }
#endif
      break;

   case 15:
      // Linedef -> Unlink 1st sidedef
      if (objtype == OBJ_LINEDEFS)
         unlink_sidedef (*list, 1, 0);
      break;

   case 16:
      // Linedef -> Unlink 2nd sidedef
      if (objtype == OBJ_LINEDEFS)
         unlink_sidedef (*list, 0, 1);
      break;

   case 17:
      // Linedef -> Mirror horizontally
      flip_mirror (*list, OBJ_LINEDEFS, 'm');
      break;
      
   case 18 :
      // Linedef -> Mirror vertically
      flip_mirror (*list, OBJ_LINEDEFS, 'f');
      break;

   case 19 :
      // Linedef -> Cut a slice out of a sector
      if (objtype == OBJ_LINEDEFS)
   {
   if (! (*list)->next || (*list)->next->next)
      {
      Beep ();
      Notify (-1, -1, "You must select exactly two linedefs", 0);
      }
   else
      {
      sector_slice ((*list)->next->objnum, (*list)->objnum);
      ForgetSelection (list);
      }
   }
      break;
   }
}



/*
 *   TransferThingProperties
 *
 *   -AJA- 2001-05-27
 */
void TransferThingProperties (int src_thing, SelPtr things)
{
   SelPtr cur;

   for (cur=things; cur; cur=cur->next)
   {
      if (! is_obj(cur->objnum))
         continue;

      Things[cur->objnum].angle = Things[src_thing].angle;
      Things[cur->objnum].type  = Things[src_thing].type;
      Things[cur->objnum].when  = Things[src_thing].when;

      MadeChanges = 1;

      things_types++;
      things_angles++;
   }
}


/*
 *   TransferSectorProperties
 *
 *   -AJA- 2001-05-27
 */
void TransferSectorProperties (int src_sector, SelPtr sectors)
{
   SelPtr cur;

   for (cur=sectors; cur; cur=cur->next)
   {
      if (! is_obj(cur->objnum))
         continue;

      strncpy (Sectors[cur->objnum].floort, Sectors[src_sector].floort,
            WAD_FLAT_NAME);
      strncpy (Sectors[cur->objnum].ceilt, Sectors[src_sector].ceilt,
            WAD_FLAT_NAME);

      Sectors[cur->objnum].floorh  = Sectors[src_sector].floorh;
      Sectors[cur->objnum].ceilh   = Sectors[src_sector].ceilh;
      Sectors[cur->objnum].light   = Sectors[src_sector].light;
      Sectors[cur->objnum].special = Sectors[src_sector].special;
      Sectors[cur->objnum].tag     = Sectors[src_sector].tag;

      MadeChanges = 1;
   }
}


/*
 *   TransferLinedefProperties
 *
 *   Note: right now nothing is done about sidedefs.  Being able to
 *   (intelligently) transfer sidedef properties from source line to
 *   destination linedefs could be a useful feature -- though it is
 *   unclear the best way to do it.  OTOH not touching sidedefs might
 *   be useful too.
 *
 *   -AJA- 2001-05-27
 */
#define LINEDEF_FLAG_KEEP  (1 + 4)

void TransferLinedefProperties (int src_linedef, SelPtr linedefs)
{
   SelPtr cur;
   wad_ldflags_t src_flags = LineDefs[src_linedef].flags & ~LINEDEF_FLAG_KEEP;

   for (cur=linedefs; cur; cur=cur->next)
   {
      if (! is_obj(cur->objnum))
         continue;

      // don't transfer certain flags
      LineDefs[cur->objnum].flags &= LINEDEF_FLAG_KEEP;
      LineDefs[cur->objnum].flags |= src_flags;

      LineDefs[cur->objnum].type = LineDefs[src_linedef].type;
      LineDefs[cur->objnum].tag  = LineDefs[src_linedef].tag;

      MadeChanges = 1;
   }
}

