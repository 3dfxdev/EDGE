//------------------------------------------------------------------------
//  Editing Operations
//------------------------------------------------------------------------
//
//  RTS Layout Tool (C) 2007 Andrew Apted
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

#include "headers.h"

#include "lib_util.h"
#include "main.h"

#include "g_edit.h"


static script_c *active_script;



void edit_op_c::Perform()
{
  Apply(new_val);
}

void edit_op_c::Undo()
{
  Apply(old_val);
}

void edit_op_c::Apply(const edit_value_u& what)
{
  section_c *map = active_script->bits.at(ref.M);
  SYS_ASSERT(map);
  SYS_ASSERT(map->kind == section_c::START_MAP);

  section_c *r_piece = map->pieces.at(ref.R);
  SYS_ASSERT(r_piece);
  SYS_ASSERT(r_piece->kind == section_c::RAD_TRIG);

  rad_trigger_c *rad = r_piece->trig;

  if (ref.T >= 0 || ref.F >= 0)
  { SYS_ASSERT(rad); }

  thing_spawn_c *thing = NULL;

  if (ref.T >= 0)
  {
    thing = rad->things.at(ref.T);

    if (ref.F >= 0)
    { SYS_ASSERT(thing); }
  }

  switch (type)
  {
    case FIELD_RadTrigPtr:
      if (need_old)
      {
        old_val.e_rad = rad;
        need_old = false;
      }

      r_piece->trig = new_val.e_rad;
      return;

    case FIELD_ThingPtr:
      if (need_old)
      {
        old_val.e_thing = thing;
        need_old = false;
      }

      SYS_ASSERT(ref.T >= 0);
      rad->things.at(ref.T) = new_val.e_thing;
      return;

    default:
      // FIXME !!!
      break;
  }
}

//--- editor settings ---
// vi:ts=2:sw=2:expandtab
