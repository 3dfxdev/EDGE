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

static std::list<edit_op_c *> undo_list;
static std::list<edit_op_c *> redo_list;

extern void UI_ListenRadTrig(rad_trigger_c *, int);
extern void UI_ListenThing(thing_spawn_c *, int);



edit_op_c::edit_op_c()
{ }

edit_op_c::~edit_op_c()
{ }

edit_op_c::edit_op_c(thing_spawn_c *TH, int field) :
    desc("THING"), sisters()
{
  ref.M = active_startmap->sec_Index;
  ref.R = TH->parent->section->sec_Index;
  ref.T = TH->th_Index;
  ref.F = field;

  switch (field)
  {
    case thing_spawn_c::F_AMBUSH:  type = FIELD_Integer; break;
    case thing_spawn_c::F_TYPE:    type = FIELD_String; break;
    case thing_spawn_c::F_X:       type = FIELD_Float; break;
    case thing_spawn_c::F_Y:       type = FIELD_Float; break;
    case thing_spawn_c::F_Z:       type = FIELD_Float; break;
    case thing_spawn_c::F_ANGLE:   type = FIELD_Float; break;
    case thing_spawn_c::F_TAG:     type = FIELD_Integer; break;
    case thing_spawn_c::F_WHEN_APPEAR: type = FIELD_Integer; break;

    default: Main_FatalError("INTERNAL ERROR\n"); return;
  }

  new_val.e_str = NULL;
  old_val.e_str = NULL;

  need_old = true;
}

edit_op_c::edit_op_c(rad_trigger_c *RAD, int field) :
    desc("RAD-TRIG"), sisters()
{
  ref.M = active_startmap->sec_Index;
  ref.R = RAD->section->sec_Index;
  ref.T = -1;
  ref.F = field;

  switch (field)
  {
    case rad_trigger_c::F_IS_RECT: type = FIELD_Integer; break;
    case rad_trigger_c::F_MX:   type = FIELD_Float; break;
    case rad_trigger_c::F_MY:   type = FIELD_Float; break;
    case rad_trigger_c::F_RX:   type = FIELD_Float; break;
    case rad_trigger_c::F_RY:   type = FIELD_Float; break;
    case rad_trigger_c::F_Z1:   type = FIELD_Float; break;
    case rad_trigger_c::F_Z2:   type = FIELD_Float; break;
    case rad_trigger_c::F_NAME: type = FIELD_String; break;
    case rad_trigger_c::F_TAG:  type = FIELD_Integer; break;
    case rad_trigger_c::F_WHEN_APPEAR: type = FIELD_Integer; break;

    default: Main_FatalError("INTERNAL ERROR\n"); return;
  }

  new_val.e_str = NULL;
  old_val.e_str = NULL;

  need_old = true;
}


//------------------------------------------------------------------------

void edit_op_c::Perform()
{
  Apply(new_val);

  for (std::vector<edit_op_c *>::iterator EI = sisters.begin();
       EI != sisters.end(); EI++)
    (*EI)->Perform();
}

void edit_op_c::Undo()
{
  Apply(old_val);

  for (std::vector<edit_op_c *>::iterator EI = sisters.begin();
       EI != sisters.end(); EI++)
    (*EI)->Undo();
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

  // notify the UI (allow it to update the info box)
  if (rad && ref.F >= 0)
    UI_ListenRadTrig(rad, ref.F);

  if (thing && ref.F >= 0)
    UI_ListenThing(thing, ref.F);
}


//------------------------------------------------------------------------

static void Edit_ClearRedoStack(void)
{
  while (! redo_list.empty())
  {
    edit_op_c *OP = redo_list.front();
    redo_list.pop_front();

    delete OP;
  }
}

static void Edit_Push(edit_op_c *OP)
{
  OP->Perform();

  undo_list.push_back(OP);

  Edit_ClearRedoStack();
}

void Edit_Undo(void)
{
  if (undo_list.empty())
    return;

  edit_op_c *OP = undo_list.back();
  undo_list.pop_back();

  OP->Undo();

  redo_list.push_front(OP);
}

void Edit_Redo(void)
{
  if (redo_list.empty())
    return;

  edit_op_c *OP = redo_list.front();
  redo_list.pop_front();

  OP->Perform();

  undo_list.push_back(OP);
}


void Edit_MoveThing(thing_spawn_c *TH,  float new_x, float new_y)
{
  edit_op_c *OP_X = new edit_op_c(TH, thing_spawn_c::F_X);
  edit_op_c *OP_Y = new edit_op_c(TH, thing_spawn_c::F_Y);

  OP_X->new_val.e_float = new_x;
  OP_Y->new_val.e_float = new_y;

  OP_X->sisters.push_back(OP_Y);

  Edit_Push(OP_X);
}

void Edit_MoveRad(rad_trigger_c *RAD, float new_mx, float new_my)
{
  edit_op_c *OP_MX = new edit_op_c(RAD, rad_trigger_c::F_MX);
  edit_op_c *OP_MY = new edit_op_c(RAD, rad_trigger_c::F_MY);

  OP_MX->new_val.e_float = new_mx;
  OP_MY->new_val.e_float = new_my;

  OP_MX->sisters.push_back(OP_MY);

  Edit_Push(OP_MX);
}

void Edit_ResizeRad(rad_trigger_c *RAD, float x1, float y1, float x2, float y2)
{
  if (x1 > x2) { float tmp_x = x1; x1 = x2; x2 = tmp_x; }
  if (y1 > y2) { float tmp_y = y1; y1 = y2; y2 = tmp_y; }

  edit_op_c *OP_MX = new edit_op_c(RAD, rad_trigger_c::F_MX);
  edit_op_c *OP_MY = new edit_op_c(RAD, rad_trigger_c::F_MY);
  edit_op_c *OP_RX = new edit_op_c(RAD, rad_trigger_c::F_RX);
  edit_op_c *OP_RY = new edit_op_c(RAD, rad_trigger_c::F_RY);

  OP_MX->new_val.e_float = (x1 + x2) / 2.0;
  OP_MY->new_val.e_float = (y1 + y2) / 2.0;
  OP_RX->new_val.e_float = (x2 - x1) / 2.0;
  OP_RY->new_val.e_float = (y2 - y1) / 2.0;

  OP_MX->sisters.push_back(OP_MY);
  OP_MX->sisters.push_back(OP_RX);
  OP_MX->sisters.push_back(OP_RY);

  Edit_Push(OP_MX);
}

void Edit_ResizeRadiusOnly(rad_trigger_c *RAD, float new_r)
{
  edit_op_c *OP_RX = new edit_op_c(RAD, rad_trigger_c::F_RX);
  edit_op_c *OP_RY = new edit_op_c(RAD, rad_trigger_c::F_RY);

  OP_RX->new_val.e_float = new_r;
  OP_RY->new_val.e_float = new_r;

  OP_RX->sisters.push_back(OP_RY);

  Edit_Push(OP_RX);
}



void Edit_ChangeFloat(thing_spawn_c *TH, int field, float new_val)
{
  edit_op_c *OP = new edit_op_c(TH, field);

  OP->new_val.e_float = new_val;

  Edit_Push(OP);
}

void Edit_ChangeInt(thing_spawn_c *TH, int field, int new_val)
{
  edit_op_c *OP = new edit_op_c(TH, field);

  OP->new_val.e_int = new_val;

  Edit_Push(OP);
}

void Edit_ChangeString(thing_spawn_c *TH, int field, const char *buffer)
{
  edit_op_c *OP = new edit_op_c(TH, field);

  OP->new_val.e_str = StringDup(buffer);

  Edit_Push(OP);
}


void Edit_ChangeFloat(rad_trigger_c *RAD, int field, float new_val)
{
  edit_op_c *OP = new edit_op_c(RAD, field);

  OP->new_val.e_float = new_val;

  Edit_Push(OP);
}

void Edit_ChangeInt(rad_trigger_c *RAD, int field, int new_val)
{
  edit_op_c *OP = new edit_op_c(RAD, field);

  OP->new_val.e_int = new_val;

  Edit_Push(OP);
}

void Edit_ChangeString(rad_trigger_c *RAD, int field, const char *buffer)
{
  edit_op_c *OP = new edit_op_c(RAD, field);

  OP->new_val.e_str = StringDup(buffer);

  Edit_Push(OP);
}

void Edit_ChangeShape(rad_trigger_c *RAD, int new_is_rect)
{
  if (RAD->is_rect == new_is_rect)
    return;

  edit_op_c *OP_IR = new edit_op_c(RAD, rad_trigger_c::F_IS_RECT);
  edit_op_c *OP_RX = new edit_op_c(RAD, rad_trigger_c::F_RX);
  edit_op_c *OP_RY = new edit_op_c(RAD, rad_trigger_c::F_RY);

  float new_r = RAD->rx;

  if (! new_is_rect)
  {
    new_r = (RAD->rx + RAD->ry) / 2.0;
  }
 
  OP_IR->new_val.e_int   = new_is_rect;
  OP_RX->new_val.e_float = new_r;
  OP_RY->new_val.e_float = new_r;

  OP_IR->sisters.push_back(OP_RX);
  OP_IR->sisters.push_back(OP_RY);

  Edit_Push(OP_IR);
}


//------------------------------------------------------------------------
//  UTILITY
//------------------------------------------------------------------------

int Int_or_Unspec(const char *buf)
{
  while (isspace(*buf))
    buf++;

  if (! *buf)
    return INT_UNSPEC;

  return atoi(buf);
}

float Float_or_Unspec(const char *buf)
{
  while (isspace(*buf))
    buf++;

  if (! *buf)
    return FLOAT_UNSPEC;

  return atof(buf);
}

const char *Int_TmpStr(int val, bool unspec_ok)
{
  if (val == INT_UNSPEC && unspec_ok)
    return "";
  
  static char buffer[200];

  sprintf(buffer, "%d", val);

  return buffer;
}

const char *Float_TmpStr(float val, int prec, bool unspec_ok)
{
  // produces a 'Human Readable' float output, which removes
  // trailing zeros for a nicer result.  For example: "123.00"
  // becomes "123".

  if (val == FLOAT_UNSPEC && unspec_ok)
    return "";
  
  static char buffer[200];

  sprintf(buffer, "%1.*f", prec, val);

  if (! strchr(buffer, '.'))
    return buffer;

  char *pos = buffer + strlen(buffer) - 1;

  while (*pos == '0')
    *pos-- = 0;

  if (*pos == '.')
    *pos-- = 0;
      
  SYS_ASSERT(isdigit(*pos));

  return buffer;
}


//--- editor settings ---
// vi:ts=2:sw=2:expandtab
