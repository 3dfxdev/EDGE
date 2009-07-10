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
#include "linedefs.h"
#include "levels.h"
#include "objid.h"
#include "sectors.h"
#include "selectn.h"
#include "vertices.h"
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


/*
   move (x, y) to a new position: rotate and scale around (0, 0)
*/

#if 0
inline void RotateAndScaleCoords (int *x, int *y, double angle, double scale)
{
  double r, theta;

  r = hypot ((double) *x, (double) *y);
  theta = atan2 ((double) *y, (double) *x);
  *x = (int) (r * scale * cos (theta + angle) + 0.5);
  *y = (int) (r * scale * sin (theta + angle) + 0.5);
}
#endif


/*
   rotate and scale a group of objects around the centre of gravity
*/

void RotateAndScaleObjects (int objtype, SelPtr obj, double angle, double scale)
{
  int    dx, dy;
  int    centerx, centery;
  SelPtr cur, vertices;

  if (obj == NULL)
    return;
  

  switch (objtype)
  {
    case OBJ_THINGS:
      centre_of_things (obj, &centerx, &centery);
      for (cur = obj; cur; cur = cur->next)
   {
   dx = Things[cur->objnum].x - centerx;
   dy = Things[cur->objnum].y - centery;
   RotateAndScaleCoords (&dx, &dy, angle, scale);
   Things[cur->objnum].x = centerx + dx;
   Things[cur->objnum].y = centery + dy;
   }
      MadeChanges = 1;
      break;

    case OBJ_VERTICES:
      centre_of_vertices (obj, &centerx, &centery);
      for (cur = obj; cur; cur = cur->next)
   {
   dx = Vertices[cur->objnum].x - centerx;
   dy = Vertices[cur->objnum].y - centery;
   RotateAndScaleCoords (&dx, &dy, angle, scale);
   Vertices[cur->objnum].x = (centerx + dx + /*4*/ 2) & ~/*7*/3;
   Vertices[cur->objnum].y = (centery + dy + /*4*/ 2) & ~/*7*/3;
   }
      MadeChanges = 1;
      MadeMapChanges = 1;
      break;

    case OBJ_LINEDEFS:
      vertices = list_vertices_of_linedefs (obj);
      RotateAndScaleObjects (OBJ_VERTICES, vertices, angle, scale);
      ForgetSelection (&vertices);
      break;

    case OBJ_SECTORS:
      
      vertices = list_vertices_of_sectors (obj);
      RotateAndScaleObjects (OBJ_VERTICES, vertices, angle, scale);
      ForgetSelection (&vertices);
      break;
  }
}



/*
 *  centre_of_objects
 *  Return the coordinates of the centre of a group of objects
 *  of type <obj_type>.
 */
void centre_of_objects (int obj_type, SelPtr list, int *x, int *y)
{
  switch (obj_type)
  {
    case OBJ_LINEDEFS:
      centre_of_linedefs (list, x, y);
      break;
    case OBJ_SECTORS:
      centre_of_sectors (list, x, y);
      break;
    case OBJ_THINGS:
      centre_of_things (list, x, y);
      break;
    case OBJ_VERTICES:
      centre_of_vertices (list, x, y);
      break;
    default:
      fatal_error ("coo: bad obj_type %d", obj_type);
  }
}



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


