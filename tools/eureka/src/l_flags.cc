/*
 *  l_flags.cc
 *  AYM 1998-12-21
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
#include "w_structs.h"


/*
 *  frob_linedefs_flags
 *  For all the linedefs in <list>, apply the operator <op>
 *  with the operand <operand> on the flags field.
 */
void frob_linedefs_flags (SelPtr list, int op, int operand)
{
  SelPtr cur;
  s16_t mask;

  if (op == YO_CLEAR || op == YO_SET || op == YO_TOGGLE)
    mask = 1 << operand;
  else
    mask = operand;

  for (cur = list; cur; cur = cur->next)
  {
    if (op == YO_CLEAR)
      LineDefs[cur->objnum].flags &= ~mask;
    else if (op == YO_SET)
      LineDefs[cur->objnum].flags |= mask;
    else if (op == YO_TOGGLE)
      LineDefs[cur->objnum].flags ^= mask;
    else
    {
      nf_bug ("frob_linedef_flags: op=%02X", op);
      return;
    }
  }
  MadeChanges = 1;
}


