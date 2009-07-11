//------------------------------------------------------------------------
//  OLD WIDGET SHITE
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


#ifndef YH_EDWIDGET  /* DO NOT INSERT ANYTHING BEFORE THIS LINE */
#define YH_EDWIDGET


class edwidget_c
   {
   public :
      void const unset         ();
      void const draw          ();  // Draw yourself
      void const undraw        ();  // If you're drawn, undraw yourself
      int  const can_undraw    ();  // Can you undraw yourself ?
      int  const need_to_clear ();  // You need to undraw yself but can't ?
      void const clear         ();  // Forget you're drawn
      int  const req_width     ();  // Tell me the width you'd like to have
      int  const req_height    ();  // Tell me the height you'd like 2 have
      void const set_x0        (int x0);// This is your top left corner
      void const set_y0        (int y0);
      void const set_x1        (int x1);// This is your bottom right corner
      void const set_y1        (int y1);
      int  const get_x0        ();  // Tell me where's your top left corner
      int  const get_y0        ();
      int  const get_x1        ();  // Tell me where's your bottom right c.
      int  const get_y1        ();
   };


#endif  /* DO NOT ADD ANYTHING AFTER THIS LINE */
//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
