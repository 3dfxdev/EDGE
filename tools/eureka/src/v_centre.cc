/*
 *  v_centre.cc
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
#include "selectn.h"
#include "v_centre.h"


/*
 *  centre_of_vertices
 *  Return the coordinates of the centre of a group of vertices.
 */
void centre_of_vertices (SelPtr list, int *x, int *y)
{
SelPtr cur;
int nitems;
long x_sum;
long y_sum;

x_sum = 0;
y_sum = 0;
for (nitems = 0, cur = list; cur; cur = cur->next, nitems++)
   {
   x_sum += Vertices[cur->objnum].x;
   y_sum += Vertices[cur->objnum].y;
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
}


/*
 *  centre_of_vertices
 *  Return the coordinates of the centre of a group of vertices.
 */
void centre_of_vertices (const bitvec_c &bv, int &x, int &y)
{
long x_sum = 0;
long y_sum = 0;
int nmax = bv.nelements ();
int nvertices = 0;
for (int n = 0; n < nmax; n++)
   {
   if (bv.get (n))
      {
      x_sum += Vertices[n].x;
      y_sum += Vertices[n].y;
      nvertices++;
      }
   }
if (nvertices == 0)
   {
   x = 0;
   y = 0;
   }
else
   {
   x = (int) (x_sum / nvertices);
   y = (int) (y_sum / nvertices);
   }
}


