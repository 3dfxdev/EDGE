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


class highlight_c
   {
   public :
      highlight_c (void);
      void set (Objid& obj);

      /* Methods declared in edwidget */
      void unset () { obj.nil (); }
      void draw          ();
      void undraw        ();
      int can_undraw () { return 1; }  // I have the ability to undraw myself.
      int need_to_clear () { return 0; }  // I know how to undraw myself.
      void clear () { obj_disp.nil (); }

///   private :
      Objid obj;  // The object we should highlight
      Objid obj_disp; // The object that is really highlighted
   };

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
