/*
 *  x_rotate.h
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


#include <math.h>


inline void RotateAndScaleCoords (int *x, int *y, double angle, double scale)
{
  double r, theta;

  r = hypot ((double) *x, (double) *y);
  theta = atan2 ((double) *y, (double) *x);
  *x = (int) (r * scale * cos (theta + angle) + 0.5);
  *y = (int) (r * scale * sin (theta + angle) + 0.5);
}

void RotateAndScaleObjects (int, SelPtr, double, double);

