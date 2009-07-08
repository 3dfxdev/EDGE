/*
 *  x_mirror.cc
 *  Flip or mirror objects
 *  AYM 1999-05-01
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
#include "l_vertices.h"
#include "levels.h"
#include "objid.h"
#include "s_vertices.h"
#include "selectn.h"
#include "t_centre.h"
#include "v_centre.h"
#include "x_mirror.h"


/*
 *  flip_mirror
 *  All objects of type <obj_type> in list <list> are
 *  flipped (if <flags> is equal to 'f') or mirrored (if
 *  <flags> is equal to 'm') around their geometric centre.
 *
 *  In addition of being moved, things have their angle
 *  changed so as to remain consistent with their
 *  environment (E.G. when mirroring or flipping the level
 *  too).
 */
void flip_mirror (SelPtr list, int obj_type, char op)
{
  enum { flip, mirror } operation;
  
  if (op == 'f')
    operation = flip;
  else if (op == 'm')
    operation = mirror;
  else
  {
    nf_bug ("flip_mirror: Bad operation %02Xh", op);
    return;
  }

  if (! list)
    return;

  /* Vertices, linedefs and sectors:
     flip/mirror the vertices. */
  if (obj_type == OBJ_VERTICES 
      || obj_type == OBJ_LINEDEFS
      || obj_type == OBJ_SECTORS)
  {
    // Get the list of vertices to flip/mirror.
    bitvec_c *vert;
    if (obj_type == OBJ_LINEDEFS)
      vert = bv_vertices_of_linedefs (list);
    else if (obj_type == OBJ_SECTORS)
      vert = bv_vertices_of_sectors (list);
    else
      vert = list_to_bitvec (list, NumVertices);

    /* Determine the geometric centre
       of that set of vertices. */
    int xref, yref;
    centre_of_vertices (*vert, xref, yref);
    xref *= 2;
    yref *= 2;
    
    /* Change the coordinates of the
       vertices. */
    if (operation == flip)
    {
      int n;
      VPtr v;
      for (n = 0, v = Vertices; n < NumVertices; n++, v++)
  if (vert->get (n))
    v->y = yref - v->y;
    }
    else if (operation == mirror)
    {
      int n;
      VPtr v;
      for (n = 0, v = Vertices; n < NumVertices; n++, v++)
  if (vert->get (n))
    v->x = xref - v->x;
    }
      
    /* Flip all the linedefs between the
       flipped/mirrored vertices by swapping
       their start and end vertices. */
    {
      int n;
      LDPtr l;
      for (n = 0, l = LineDefs; n < NumLineDefs; n++, l++)
  if (vert->get (l->start) && vert->get (l->end))
  {
    int w = l->start;
    l->start = l->end;
    l->end = w;
    MadeMapChanges = 1;
  }
    }
    
    delete vert;
  }

  /* Things: flip/mirror the things. */
  else if (obj_type == OBJ_THINGS)
  {
    /* Determine the geometric centre
       of that set of things. */
    int xref, yref;
    centre_of_things (list, &xref, &yref);
    xref *= 2;
    yref *= 2;

    /* Change the coordinates of the
       things and adjust the angles. */
    if (operation == flip)
    {
      if (list)
      {
  things_angles++;
  MadeChanges = 1;
      }
      for (SelPtr cur = list; cur; cur = cur->next)
      {
  TPtr t = Things + cur->objnum;
  t->y = yref - t->y;
  if (t->angle != 0)
    t->angle = 360 - t->angle;
      }
    }
    else if (operation == mirror)
    {
      if (list)
      {
  things_angles++;
  MadeChanges = 1;
      }
      for (SelPtr cur = list; cur; cur = cur->next)
      {
  TPtr t = Things + cur->objnum;
  t->x = xref - t->x;
  if (t->angle > 180)
    t->angle = 540 - t->angle;
  else
    t->angle = 180 - t->angle;
      }
    }
  }
}

