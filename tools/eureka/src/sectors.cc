/*
 *  s_centre.cc
 *  AYM 1998-11-22
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
#include "levels.h"
#include "s_centre.h"
#include "s_vertices.h"


/*
 *  centre_of_sector
 *  Return the coordinates of the centre of a sector.
 *
 *  FIXME The algorithm is desesperatingly simple-minded and
 *  does not take into account concave sectors and sectors
 *  that are enclosed by several distinct paths of linedefs.
 */
void centre_of_sector (obj_no_t s, int *x, int *y)
{
bitvec_c *vertices = bv_vertices_of_sector (s);
long x_sum  = 0;
long y_sum  = 0;
int  nitems = 0;

for (size_t n = 0; n < vertices->nelements (); n++)
   if (vertices->get (n))
      {
      x_sum += Vertices[n].x;
      y_sum += Vertices[n].y;
      nitems++;
      }
if (nitems == 0)
   {
   *x = 0;
   *y = 0;
   }
else
   {
   *x = (int) (x_sum / nitems);
   *y = (int) (y_sum / nitems);
   }
delete vertices;
}


/*
 *  centre_of_sectors
 *  Return the coordinates of the centre of a group of sectors.
 */
void centre_of_sectors (SelPtr list, int *x, int *y)
{
bitvec_c *vertices;
int nitems;
long x_sum;
long y_sum;
int n;

vertices = bv_vertices_of_sectors (list);
x_sum = 0;
y_sum = 0;
nitems = 0;
for (n = 0; n < NumVertices; n++)
   if (vertices->get (n))
      {
      x_sum += Vertices[n].x;
      y_sum += Vertices[n].y;
      nitems++;
      }
if (nitems == 0)
   {
   *x = 0;
   *y = 0;
   }
else
   {
   *x = (int) (x_sum / nitems);
   *y = (int) (y_sum / nitems);
   }
delete vertices;
}


