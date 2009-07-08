/*
 *  selrect.cc
 *  AYM 1999-03-24
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
#include "levels.h"
#include "objid.h"
#include "selectn.h"
#include "w_structs.h"


/*
   select all objects inside a given box
*/

void SelectObjectsInBox (SelPtr *list, int objtype, int x0, int y0, int x1, int y1)
{
int n, m;

if (x1 < x0)
   {
   n = x0;
   x0 = x1;
   x1 = n;
   }
if (y1 < y0)
   {
   n = y0;
   y0 = y1;
   y1 = n;
   }

switch (objtype)
   {
   case OBJ_THINGS:
      
      for (n = 0; n < NumThings; n++)
   if (Things[n].x >= x0 && Things[n].x <= x1
    && Things[n].y >= y0 && Things[n].y <= y1)
            select_unselect_obj (list, n);
      break;
   case OBJ_VERTICES:
      
      for (n = 0; n < NumVertices; n++)
   if (Vertices[n].x >= x0 && Vertices[n].x <= x1
    && Vertices[n].y >= y0 && Vertices[n].y <= y1)
            select_unselect_obj (list, n);
      break;
   case OBJ_LINEDEFS:
      
      for (n = 0; n < NumLineDefs; n++)
   {
   /* the two ends of the line must be in the box */
   m = LineDefs[n].start;
   if (Vertices[m].x < x0 || Vertices[m].x > x1
    || Vertices[m].y < y0 || Vertices[m].y > y1)
      continue;
   m = LineDefs[n].end;
   if (Vertices[m].x < x0 || Vertices[m].x > x1
    || Vertices[m].y < y0 || Vertices[m].y > y1)
      continue;
         select_unselect_obj (list, n);
   }
      break;
   case OBJ_SECTORS:
      {
      signed char *sector_status;
      LDPtr ld;

      sector_status = (signed char *) GetMemory (NumSectors);
      memset (sector_status, 0, NumSectors);
      for (n = 0, ld = LineDefs; n < NumLineDefs; n++, ld++)
         {
         VPtr v1, v2;
         int s1, s2;

         v1 = Vertices + ld->start;
         v2 = Vertices + ld->end;

         // Get the numbers of the sectors on both sides of the linedef
   if (is_sidedef (ld->sidedef1)
    && is_sector (SideDefs[ld->sidedef1].sector))
      s1 = SideDefs[ld->sidedef1].sector;
   else
      s1 = OBJ_NO_NONE;
   if (is_sidedef (ld->sidedef2)
    && is_sector (SideDefs[ld->sidedef2].sector))
      s2 = SideDefs[ld->sidedef2].sector;
   else
      s2 = OBJ_NO_NONE;

         // The linedef is entirely within bounds.
         // The sectors it touches _might_ be within bounds too.
         if (v1->x >= x0 && v1->x <= x1
          && v1->y >= y0 && v1->y <= y1
          && v2->x >= x0 && v2->x <= x1
          && v2->y >= y0 && v2->y <= y1)
            {
            if (is_sector (s1) && sector_status[s1] >= 0)
               sector_status[s1] = 1;
            if (is_sector (s2) && sector_status[s2] >= 0)
               sector_status[s2] = 1;
            }

         // It's not. The sectors it touches _can't_ be within bounds.
         else
            {
            if (is_sector (s1))
               sector_status[s1] = -1;
            if (is_sector (s2))
               sector_status[s2] = -1;
            }
         }

      // Now select all the sectors we found to be within bounds
      ForgetSelection (list);
      for (n = 0; n < NumSectors; n++)
         if (sector_status[n] > 0)
            SelectObject (list, n);
      FreeMemory (sector_status);
      }
      break;
   }
}

