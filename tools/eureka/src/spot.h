/*
 *  spot.h
 *  AYM 1998-12-14
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


#include "edwidget.h"
#include "gfx.h"


//!!!! FIXME bloody hell, move implementation into .cc file
//

class spot_c : public edwidget_c
{
  public :

    /*
     *  Specific to this class
     */
    spot_c () { visible = 0; visible_disp = 0; }
    void set (int x, int y) { this->x = x; this->y = y; visible = 1; }

    /*
     *  Inherited from edwidget_c
     */
    void unset () { visible = 0; }

    void draw ()
    {
      if (visible && (! visible_disp || x_disp != x || y_disp != y))
      {
        SetDrawingMode (1);
        push_colour (LIGHTGREEN);
//!!!!        draw_map_point (x, y);
        pop_colour ();
        SetDrawingMode (0);
        visible_disp = 1;
        x_disp = x;
        y_disp = y;
      }
    }

    void undraw ()
    {
      if (visible_disp)
      {
        SetDrawingMode (1);
        push_colour (LIGHTGREEN);
//!!!!        draw_map_point (x_disp, y_disp);
        pop_colour ();
        SetDrawingMode (0);
        visible_disp = 0;
      }
    }

    int can_undraw () { return 1; }
    int need_to_clear () { return 0; }
    void clear () { visible_disp = 0; }

  private :
    int visible;
    int visible_disp;
    int x;
    int y;
    int x_disp;
    int y_disp;
};

