//------------------------------------------------------------------------
//  HIGHLIGHTING
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor (C) 2001-2009 Andrew Apted
//                     (C) 1997-2003 André Majorel et al
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//------------------------------------------------------------------------
//
//  Based on Yadex which incorporated code from DEU 5.21 that was put
//  in the public domain in 1994 by Raphaël Quinet and Brendon Wyber.
//
//------------------------------------------------------------------------

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


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
