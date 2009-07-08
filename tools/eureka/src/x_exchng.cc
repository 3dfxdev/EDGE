/*
 *  x_exchng.cc
 *  Exchange objects numbers.
 *  AYM 1999-06-11
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
#include "selectn.h"
#include "objid.h"
#include "x_exchng.h"


/*
 *  exchange_objects_numbers
 *  Exchange the numbers of two objects
 *
 *  Return 0 on success, non-zero on failure.
 */
int exchange_objects_numbers (int obj_type, SelPtr list, bool adjust)
{
  int n1, n2;

  // Must have exactly two objects in the selection
  if (list == 0 || list->next == 0 || (list->next)->next != 0)
  {
    nf_bug ("exchange_object_numbers: wrong objects count.");
    return 1;
  }
  n1 = list->objnum;
  n2 = (list->next)->objnum;

  if (obj_type == OBJ_LINEDEFS)
  {
    struct LineDef swap_buf;
    swap_buf = LineDefs[n1];
    LineDefs[n1] = LineDefs[n2];
    LineDefs[n2] = swap_buf;
  }
  else if (obj_type == OBJ_SECTORS)
  {
    struct Sector swap_buf;
    swap_buf = Sectors[n1];
    Sectors[n1] = Sectors[n2];
    Sectors[n2] = swap_buf;
    if (adjust)
    {
      for (int n = 0; n < NumSideDefs; n++)
      {
  if (SideDefs[n].sector == n1)
    SideDefs[n].sector = n2;
  else if (SideDefs[n].sector == n2)
    SideDefs[n].sector = n1;
      }
    }
  }
  else if (obj_type == OBJ_THINGS)
  {
    struct Thing swap_buf;
    swap_buf = Things[n1];
    Things[n1] = Things[n2];
    Things[n2] = swap_buf;
  }
  else if (obj_type == OBJ_VERTICES)
  {
    struct Vertex swap_buf;
    swap_buf = Vertices[n1];
    Vertices[n1] = Vertices[n2];
    Vertices[n2] = swap_buf;
    if (adjust)
    {
      for (int n = 0; n < NumLineDefs; n++)
      {
  if (LineDefs[n].start == n1)
    LineDefs[n].start = n2;
  else if (LineDefs[n].start == n2)
    LineDefs[n].start = n1;
  if (LineDefs[n].end == n1)
    LineDefs[n].end = n2;
  else if (LineDefs[n].end == n2)
    LineDefs[n].end = n1;
      }
    }
  }
  return 0;
}


