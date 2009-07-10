/*
 *  highlt.cc
 *  AYM 1998-09-20
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
#include "highlt.h"
#include "objects.h"

#include "im_color.h"
#include "ui_window.h"


highlight_c::highlight_c (void)
{
obj.nil ();
obj_disp.nil ();
}


void highlight_c::set (Objid& obj)
{
  // same ??
  if (this->obj.type == obj.type && this->obj.num == obj.num)
    return;

  this->obj = obj;
  main_win->canvas->redraw();
}

void highlight_c::draw (void)
{
// fprintf(stderr, "highlight_c::draw %d,%d\n", obj.type, obj.num);

  if (obj ()) //// ! obj_disp () && obj ())
  {
    main_win->canvas->HighlightObject (obj.type, obj.num, YELLOW);
    ////   obj_disp = obj;
  }
}


void highlight_c::undraw (void)
{
if (obj_disp () && ! (obj_disp == obj))
   {
   main_win->canvas->HighlightObject (obj_disp.type, obj_disp.num, YELLOW);
   obj_disp.nil ();
   }
}


