//------------------------------------------------------------------------
//  SELECTION BOXES
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


#include "edwidget.h"


class selbox_c : public edwidget_c
{
  public :

    /* Methods used by edit_t */
    selbox_c            ();
    void set_1st_corner (int x, int y);
    void set_2nd_corner (int x, int y);
    void get_corners    (int *x1, int *y1, int *x2, int *y2);
    void unset_corners  ();
    /* Methods declared in edwidget_c */
    void unset         ();
    void draw          ();
    void undraw        ();

    int can_undraw ()
      { return 1; }  // I have the ability to undraw myself

    int need_to_clear ()
      { return 0; }  // I know how to undraw myself.

    void clear         ();

  private :

    int x1;   /* Coordinates of the first corner */
    int y1;
    int x2;   /* Coordinates of the second corner */
    int y2;
    int x1_disp;  /* x1 and y1 as they were last time draw() was called */
    int y1_disp;
    int x2_disp;  /* x2 and y2 as they were last time draw() was called */
    int y2_disp;
    int flags;
};


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
