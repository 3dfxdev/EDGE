/*
 *  l_misc.c
 *  Linedef/sidedef misc operations.
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
#include "levels.h"
#include "objects.h"
#include "objid.h"
#include "selectn.h"
#include <math.h>


static void SliceLinedef (int linedefno, int times);


/*
   flip one or several LineDefs
*/

void FlipLineDefs ( SelPtr obj, bool swapvertices)
{
SelPtr cur;
int    tmp;


for (cur = obj; cur; cur = cur->next)
{
   if (swapvertices)
   {
      /* swap starting and ending Vertices */
      tmp = LineDefs[ cur->objnum].end;
      LineDefs[ cur->objnum].end = LineDefs[ cur->objnum].start;
      LineDefs[ cur->objnum].start = tmp;
   }
   /* swap first and second SideDefs */
   tmp = LineDefs[ cur->objnum].sidedef1;
   LineDefs[ cur->objnum].sidedef1 = LineDefs[ cur->objnum].sidedef2;
   LineDefs[ cur->objnum].sidedef2 = tmp;
}
MadeChanges = 1;
MadeMapChanges = 1;
}


/*
   split one or more LineDefs in two, adding new Vertices in the middle
*/

void SplitLineDefs ( SelPtr obj)
{
SelPtr cur;
int    vstart, vend;


for (cur = obj; cur; cur = cur->next)
  {
  vstart = LineDefs[cur->objnum].start;
  vend   = LineDefs[cur->objnum].end;
  SliceLinedef (cur->objnum, 1);
  Vertices[NumVertices-1].x = (Vertices[vstart].x + Vertices[vend].x) / 2;
  Vertices[NumVertices-1].y = (Vertices[vstart].y + Vertices[vend].y) / 2;
  }
MadeChanges = 1;
MadeMapChanges = 1;
}


/*
 *  MakeRectangularNook - Make a nook or boss in a wall
 *  
 *  Before :    After :
 *          ^-->
 *          |  |
 *  +----------------->     +------->  v------->
 *      1st sidedef             1st sidedef
 *
 *  The length of the sides of the nook is sidelen.
 *  This is true when convex is false. If convex is true, the nook
 *  is actually a bump when viewed from the 1st sidedef.
 */
void MakeRectangularNook (SelPtr obj, int width, int depth, int convex)
{
SelPtr cur;


for (cur = obj; cur; cur = cur->next)
  {
  int vstart, vend;
  int x0;
  int y0;
  int dx0, dx1, dx2;
  int dy0, dy1, dy2;
  int line_len;
  double real_width;
  double angle;

  vstart = LineDefs[cur->objnum].start;
  vend   = LineDefs[cur->objnum].end;
  x0     = Vertices[vstart].x;
  y0     = Vertices[vstart].y;
  dx0    = Vertices[vend].x - x0;
  dy0    = Vertices[vend].y - y0;

  /* First split the line 4 times */
  SliceLinedef (cur->objnum, 4);

  /* Then position the vertices */
  angle  = atan2 (dy0, dx0);
 
  /* If line to split is not longer than sidelen,
     force sidelen to 1/3 of length */
  line_len   = ComputeDist (dx0, dy0);
  real_width = line_len > width ? width : line_len / 3;
    
  dx2 = (int) (real_width * cos (angle));
  dy2 = (int) (real_width * sin (angle));

  dx1 = (dx0 - dx2) / 2;
  dy1 = (dy0 - dy2) / 2;

  {
  double normal = convex ? angle-HALFPI : angle+HALFPI;
  Vertices[NumVertices-1-3].x = x0 + dx1;
  Vertices[NumVertices-1-3].y = y0 + dy1;
  Vertices[NumVertices-1-2].x = x0 + dx1 + (int) (depth * cos (normal));
  Vertices[NumVertices-1-2].y = y0 + dy1 + (int) (depth * sin (normal));
  Vertices[NumVertices-1-1].x = x0 + dx1 + dx2 + (int) (depth * cos (normal));
  Vertices[NumVertices-1-1].y = y0 + dy1 + dy2 + (int) (depth * sin (normal));
  Vertices[NumVertices-1  ].x = x0 + dx1 + dx2;
  Vertices[NumVertices-1  ].y = y0 + dy1 + dy2;
  }

  MadeChanges = 1;
  MadeMapChanges = 1;
  }
}


/*
 *  SliceLinedef - Split a linedef several times
 *
 *  Splits linedef no. <linedefno> <times> times.
 *  Side-effects : creates <times> new vertices, <times> new
 *  linedefs and 0, <times> or 2*<times> new sidedefs.
 *  The new vertices are put at (0,0).
 *  See SplitLineDefs() and MakeRectangularNook() for example of use.
 */
static void SliceLinedef (int linedefno, int times)
{
int prev_ld_no;
for (prev_ld_no = linedefno; times > 0; times--, prev_ld_no = NumLineDefs-1)
  {
  int sd;

  InsertObject (OBJ_VERTICES, -1, 0, 0);
  InsertObject (OBJ_LINEDEFS, linedefno, 0, 0);
  LineDefs[NumLineDefs-1].start = NumVertices - 1;
  LineDefs[NumLineDefs-1].end   = LineDefs[prev_ld_no].end;
  LineDefs[prev_ld_no   ].end   = NumVertices - 1;
  
  sd = LineDefs[linedefno].sidedef1;
  if (sd >= 0)
    {
    InsertObject (OBJ_SIDEDEFS, sd, 0, 0);
    
    LineDefs[NumLineDefs-1].sidedef1 = NumSideDefs - 1;
    }
  sd = LineDefs[linedefno].sidedef2;
  if (sd >= 0)
    {
    InsertObject (OBJ_SIDEDEFS, sd, 0, 0);
    
    LineDefs[NumLineDefs-1].sidedef2 = NumSideDefs - 1;
    }
  }
}


/*
 *  SetLinedefLength
 *  Move either vertex to set length of linedef to desired value
 */
void SetLinedefLength (SelPtr obj, int length, int move_2nd_vertex)
{
SelPtr cur;


for (cur = obj; cur; cur = cur->next)
  {
  VPtr vertex1 = Vertices + LineDefs[cur->objnum].start;
  VPtr vertex2 = Vertices + LineDefs[cur->objnum].end;
  double angle = atan2 (vertex2->y - vertex1->y, vertex2->x - vertex1->x);
  int dx       = (int) (length * cos (angle));
  int dy       = (int) (length * sin (angle));

  if (move_2nd_vertex)
    {
    vertex2->x = vertex1->x + dx;
    vertex2->y = vertex1->y + dy;
    }
  else
    {
    vertex1->x = vertex2->x - dx;
    vertex1->y = vertex2->y - dy;
    }

  MadeChanges = 1;
  MadeMapChanges = 1;
  }
}


