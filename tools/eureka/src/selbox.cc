/*
 *  selbox.cc
 *  Selection box stuff.
 *  AYM 1998-07-04
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


#include "main.h"
#include "gfx.h"
#include "selbox.h"


static const int flags_1st_corner_set = 1;
static const int flags_2nd_corner_set = 1 << 1;
static const int flags_displayed      = 1 << 2;


// FIXME: borgify this into UI_Canvas class


selbox_c::selbox_c (void)
{
  flags = 0;
}


void selbox_c::set_1st_corner (int x, int y)
{
  x1 = x;
  y1 = y;
  flags |= flags_1st_corner_set;
}


void selbox_c::set_2nd_corner (int x, int y)
{
  x2 = x;
  y2 = y;
  flags |= flags_2nd_corner_set;
}


void selbox_c::get_corners (int *x1, int *y1, int *x2, int *y2)
{
  if (x1 != NULL)
    *x1 = this->x1;
  if (y1 != NULL)
    *y1 = this->y1;
  if (x2 != NULL)
    *x2 = this->x2;
  if (y2 != NULL)
    *y2 = this->y2;
}


void selbox_c::unset_corners (void)
{
  flags &= ~ (flags_1st_corner_set | flags_2nd_corner_set);
}


void selbox_c::draw (void)
{
  if ((flags & flags_1st_corner_set) && (flags & flags_2nd_corner_set))
  {
    set_colour (CYAN);
    SetDrawingMode (1);
///!!!    DrawMapLine (x1, y1, x1, y2);
///!!!    DrawMapLine (x1, y2, x2, y2);
///!!!    DrawMapLine (x2, y2, x2, y1);
///!!!    DrawMapLine (x2, y1, x1, y1);
    SetDrawingMode (0);
    /* Those are needed by undraw() */
    x1_disp = x1;
    y1_disp = y1;
    x2_disp = x2;
    y2_disp = y2;
    flags |= flags_displayed;
  }
}


void selbox_c::undraw (void)
{
  if (flags & flags_displayed)
  {
    flags &= ~ flags_displayed;
  }
}


void selbox_c::clear (void)
{
  flags &= ~ flags_displayed;
}

