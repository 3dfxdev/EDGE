/*
 *  spritdir.h
 *  Sprite_dir class
 *  AYM 2000-05-31
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


#ifndef YH_SPRITDIR  /* DO NOT INSERT ANYTHING BEFORE THIS LINE */
#define YH_SPRITDIR



/*
 *  Sprite_dir
 *
 *  This is a specialization of Lump_dir that has an
 *  additional method : loc_by_root() which retrieves the
 *  first entry whose name begins with the argument.
 */


void Sprite_loc_by_root (const char *name, Lump_loc& loc);


#endif  /* DO NOT ADD ANYTHING AFTER THIS LINE */
