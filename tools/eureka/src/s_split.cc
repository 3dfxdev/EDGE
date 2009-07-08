/*
 *  s_split.cc
 *  Split sectors
 *  AYM 1998-02-03
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
#include "dialog.h"
#include "levels.h"
#include "objects.h"
#include "objid.h"
#include "s_linedefs.h"
#include "selectn.h"
#include "x_hover.h"
/// #include "entry.h"


/*
   split a sector in two, adding a new linedef between the two vertices
*/

void SplitSector (int vertex1, int vertex2)
{
SelPtr llist;
int    curv, s, l, sd;
char   msg1[80], msg2[80];

/* AYM 1998-08-09 : FIXME : I'm afraid this test is not relevant
   if the sector contains subsectors. I should ask Jim (Flynn),
   he did something about that in DETH. */
/* Check if there is a sector between the two vertices (in the middle) */
Objid o;
GetCurObject (o, OBJ_SECTORS,
        (Vertices[vertex1].x + Vertices[vertex2].x) / 2,
        (Vertices[vertex1].y + Vertices[vertex2].y) / 2);
s = o.num;
if (s < 0)
   {
   Beep ();
   sprintf (msg1, "There is no sector between vertex #%d and vertex #%d",
     vertex1, vertex2);
   Notify (-1, -1, msg1, NULL);
   return;
   }

/* Check if there is a closed path from <vertex1> to <vertex2>,
   along the edge of sector <s>. To make it faster, I scan only
   the set of linedefs that face sector <s>. */

obj_no_t *ld_numbers;
int nlinedefs = linedefs_of_sector (s, ld_numbers);
if (nlinedefs < 1)  // Can't happen
   {
   nf_bug ("SplitSector: no linedef for sector %d\n", s);
   return;
   }
llist = NULL;
curv = vertex1;
while (curv != vertex2)
   {
   printf ("%d\n", curv);
   int n;
   for (n = 0; n < nlinedefs; n++)
      {
      if (IsSelected (llist, ld_numbers[n]))
   continue;  // Already been there
      const LDPtr ld = LineDefs + ld_numbers[n];
      if (ld->start == curv
    && is_sidedef (ld->sidedef1) && SideDefs[ld->sidedef1].sector == s)
         {
   curv = ld->end;
   SelectObject (&llist, ld_numbers[n]);
   break;
         }
      if (ld->end == curv
    && is_sidedef (ld->sidedef2) && SideDefs[ld->sidedef2].sector == s)
   {
   curv = ld->start;
   SelectObject (&llist, ld_numbers[n]);
   break;
         }
      }
   if (n >= nlinedefs)
      {
      Beep ();
      sprintf (msg1, "Cannot find a closed path from vertex #%d to vertex #%d",
        vertex1, vertex2);
      if (curv == vertex1)
   sprintf (msg2, "There is no sidedef starting from vertex #%d"
     " on sector #%d", vertex1, s);
      else
   sprintf (msg2, "Check if sector #%d is closed"
     " (cannot go past vertex #%d)", s, curv);
      Notify (-1, -1, msg1, msg2);
      ForgetSelection (&llist);
      delete[] ld_numbers;
      return;
      }
   if (curv == vertex1)
      {
      Beep ();
      sprintf (msg1, "Vertex #%d is not on the same sector (#%d)"
        " as vertex #%d", vertex2, s, vertex1);
      Notify (-1, -1, msg1, NULL);
      ForgetSelection (&llist);
      delete[] ld_numbers;
      return;
      }
   }
delete[] ld_numbers;
/* now, the list of linedefs for the new sector is in llist */

/* add the new sector, linedef and sidedefs */
InsertObject (OBJ_SECTORS, s, 0, 0);
InsertObject (OBJ_LINEDEFS, -1, 0, 0);
LineDefs[NumLineDefs - 1].start = vertex1;
LineDefs[NumLineDefs - 1].end = vertex2;
LineDefs[NumLineDefs - 1].flags = 4;
InsertObject (OBJ_SIDEDEFS, -1, 0, 0);
SideDefs[NumSideDefs - 1].sector = s;
strncpy (SideDefs[NumSideDefs - 1].middle, "-", WAD_TEX_NAME);
InsertObject (OBJ_SIDEDEFS, -1, 0, 0);
strncpy (SideDefs[NumSideDefs - 1].middle, "-", WAD_TEX_NAME);

LineDefs[NumLineDefs - 1].sidedef1 = NumSideDefs - 2;
LineDefs[NumLineDefs - 1].sidedef2 = NumSideDefs - 1;

/* bind all linedefs in llist to the new sector */
while (llist)
{
   sd = LineDefs[llist->objnum].sidedef1;
   if (sd < 0 || SideDefs[sd].sector != s)
      sd = LineDefs[llist->objnum].sidedef2;
   SideDefs[sd].sector = NumSectors - 1;
   UnSelectObject (&llist, llist->objnum);
}

/* second check... useful for sectors within sectors */

for (l = 0; l < NumLineDefs; l++)
{
   sd = LineDefs[l].sidedef1;
   if (sd >= 0 && SideDefs[sd].sector == s)
   {
      curv = GetOppositeSector (l, 1);
      
      if (curv == NumSectors - 1)
   SideDefs[sd].sector = NumSectors - 1;
   }
   sd = LineDefs[l].sidedef2;
   if (sd >= 0 && SideDefs[sd].sector == s)
   {
      curv = GetOppositeSector (l, 0);
      
      if (curv == NumSectors - 1)
   SideDefs[sd].sector = NumSectors - 1;
   }
}

MadeChanges = 1;
MadeMapChanges = 1;
}



/*
   split two linedefs, then split the sector and add a new linedef between the new vertices
*/

void SplitLineDefsAndSector (int linedef1, int linedef2)
{
SelPtr llist;
int    s1, s2, s3, s4;
char   msg[80];

/* check if the two linedefs are adjacent to the same sector */

s1 = LineDefs[linedef1].sidedef1;
s2 = LineDefs[linedef1].sidedef2;
s3 = LineDefs[linedef2].sidedef1;
s4 = LineDefs[linedef2].sidedef2;

if (s1 >= 0)
   s1 = SideDefs[s1].sector;
if (s2 >= 0)
   s2 = SideDefs[s2].sector;
if (s3 >= 0)
   s3 = SideDefs[s3].sector;
if (s4 >= 0)
   s4 = SideDefs[s4].sector;
if ((s1 < 0 || (s1 != s3 && s1 != s4)) && (s2 < 0 || (s2 != s3 && s2 != s4)))
{
   Beep ();
   sprintf (msg, "Linedefs #%d and #%d are not adjacent to the same sector",
     linedef1, linedef2);
   Notify (-1, -1, msg, NULL);
   return;
}
/* split the two linedefs and create two new vertices */
llist = NULL;
SelectObject (&llist, linedef1);
SelectObject (&llist, linedef2);
SplitLineDefs (llist);
ForgetSelection (&llist);
/* split the sector and create a linedef between the two vertices */
SplitSector (NumVertices - 1, NumVertices - 2);
}



