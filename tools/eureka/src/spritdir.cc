/*
 *  spritdir.cc - Sprite_dir class
 *  AYM 2000-06-01
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
#include <functional>
#include "spritdir.h"
#include "w_name.h"


/*
 *  Sprite_dir::loc_by_root - find sprite by prefix
 *  
 *      Return the (wad, offset, length) location of the first
 *      lump by alphabetical order whose name begins with
 *      <name>. If not found, set loc.wad to 0.
 */
void Sprite_loc_by_root (const char *name, Lump_loc& loc)
{
  char buffer[16];

  strcpy(buffer, name);

  if (strlen(buffer) == 4)
    strcat(buffer, "A");

  if (strlen(buffer) == 5)
    strcat(buffer, "0");

  MDirPtr m = FindMasterDir(MasterDir, buffer);

  if (! m)
  {
    buffer[5] = '1';
    m = FindMasterDir(MasterDir, buffer);
  }

  if (! m)
  {
    strcat(buffer, "C1");
    m = FindMasterDir(MasterDir, buffer);
  }

  if (! m)
  {
    buffer[6] = 'D';
    m = FindMasterDir(MasterDir, buffer);
  }

  if (! m)
  {
    loc.wad = NULL;
    loc.ofs = loc.len = 0;
    return;
  }

  loc.wad = m->wadfile;
  loc.ofs = m->dir.start;
  loc.len = m->dir.size;
}

