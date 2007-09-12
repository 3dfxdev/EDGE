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


script_c  *active_script;
section_c *active_startmap;

static rad_trigger_c *radtrig_listener;
static thing_spawn_c *thing_listener;

extern void UI_RadTrigListener(rad_trigger_c *, int);
extern void UI_ThingListener(thing_spawn_c *, int);


void edit_op_c::Perform()
{
  Apply(new_val);
}

void edit_op_c::Undo()
{
  Apply(old_val);
}

static int& GetIntRef(rad_trigger_c *rad, thing_spawn_c *th, int F)
{
  if (th)
    return th->GetIntRef(F);
  else
    return rad->GetIntRef(F);
}

static float& GetFloatRef(rad_trigger_c *rad, thing_spawn_c *th, int F)
{
  if (th)
    return th->GetFloatRef(F);
  else
    return rad->GetFloatRef(F);
}

static std::string& GetStringRef(rad_trigger_c *rad, thing_spawn_c *th, int F)
{
  if (th)
    return th->GetStringRef(F);
  else
    return rad->GetStringRef(F);
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
    {
      if (need_old)
      {
        old_val.e_rad = rad;
        need_old = false;
      }

      r_piece->trig = new_val.e_rad;
      break;
    }

    case FIELD_ThingPtr:
    {
      SYS_ASSERT(ref.T >= 0);

      if (need_old)
      {
        old_val.e_thing = thing;
        need_old = false;
      }

      rad->things.at(ref.T) = new_val.e_thing;
      break;
    }

    case FIELD_Integer:
    {
      SYS_ASSERT(ref.F >= 0);

      int& var = GetIntRef(rad, thing, ref.F);

      if (need_old)
      {
        old_val.e_int = var;
        need_old = false;
      }

      var = new_val.e_int;
      break;
    }

    case FIELD_Float:
    {
      SYS_ASSERT(ref.F >= 0);

      float& var = GetFloatRef(rad, thing, ref.F);

      if (need_old)
      {
        old_val.e_float = var;
        need_old = false;
      }

      var = new_val.e_float;
      break;
    }

    case FIELD_String:
    {
      SYS_ASSERT(ref.F >= 0);

      std::string & var = GetStringRef(rad, thing, ref.F);

      if (need_old)
      {
        old_val.e_str = StringDup(var.c_str());
        need_old = false;
      }

      var = std::string(new_val.e_str);

      if (thing && ref.F == thing_spawn_c::F_TYPE)
        thing->ddf_info = NULL; //!!!! LOOKUP NEW ONE
      break;
    }

    default:
      Main_FatalError("INTERNAL ERROR: edit_op_c::Apply() unknown type!\n");
      return; /* NOT REACHED */
  }

  // notify the UI when a visible field changes
  if (ref.F >= 0)
  {
    if (rad && rad == radtrig_listener)
      UI_RadTrigListener(rad, ref.F);

    if (thing && thing == thing_listener)
      UI_ThingListener(thing, ref.F);
  }
}

//--- editor settings ---
// vi:ts=2:sw=2:expandtab
