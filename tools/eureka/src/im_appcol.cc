/*
 *  acolours.cc
 *  Allocate and free the application colours.
 *
 *  By "application colours", I mean the colours used to draw
 *  the windows, the menus and the map, as opposed to the
 *  "game colours" which depend on the game (they're in the
 *  PLAYPAL lump) and are used to draw the game graphics
 *  (flats, textures, sprites...).
 *
 *  The game colours are handled in gcolour1.cc and gcolour2.cc.
 *
 *  AYM 1998-11-29
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
#include "im_appcol.h"
#include "gfx.h"
#include "im_rgb.h"


