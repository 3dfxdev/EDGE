/*
 *  t_spin.cc
 *  AYM 1998-12-06
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
#include "t_spin.h"


/*
 *  spin_thing - change the angle of things
 */
void spin_things (SelPtr obj, int degrees)
{
  SelPtr cur;

  if (! obj)
    return;
  for (cur = obj; cur; cur = cur->next)
  {
    Things[cur->objnum].angle += degrees;
    while (Things[cur->objnum].angle >= 360)  // No we can't use %
      Things[cur->objnum].angle -= 360;
    while (Things[cur->objnum].angle < 0) // (negatives...)
      Things[cur->objnum].angle += 360;
  }
  things_angles++;
  MadeChanges = 1;
}


