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

#ifndef __G_EDIT_H__
#define __G_EDIT_H__

#include "g_script.h"


typedef struct
{
  /* this structure is used to reference a certain object or
   * field within a whole script, which is the target of a
   * certain editing operation.
   */

  // Map = index into SCRIPT->bits[] vector.
  int M;

  // Rad-trig = index into STARTMAP->pieces[] vector.
  int R; 

  // Thing = index into RADTRIG->things[] vector.
  // will be -1 for radius trigger objects or fields.
  int T;
 
  // Field = specific field of a radius trigger or spawn thing.
  // will be -1 to reference the rad-trig or thing itself.
  int F;
}
reference_t;


typedef enum
{
  FIELD_Integer = 0,
  FIELD_Float,
  FIELD_String,

  FIELD_RadTrigPtr,
  FIELD_ThingPtr,
}
field_type_e;


typedef union
{
  int e_int;
  float e_float;
  const char *e_str;

  rad_trigger_c *e_rad;
  thing_spawn_c *e_thing;
}
edit_value_u;


class edit_op_c
{
  /* this structure embodies every kind of editing operation that
   * can be performed on the script.  Additions/Deletions of
   * radius triggers and thing-spawns are represented simply as
   * a modification where the old ptr (for additions) or the new
   * ptr (for deletions) are NULL.
   */

public:
  reference_t ref;

  field_type_e type;

  edit_value_u old_val;
  edit_value_u new_val;

  // we grab the old_val when the new value is first applied.
  bool need_old;

  // short description for Undo menu
  std::string desc;

  // a single operation may atomicly change multiple fields in
  // the target.  These extra ops are stored here.
  std::vector<edit_op_c *> sisters;

public:
   edit_op_c();
  ~edit_op_c();

  edit_op_c(thing_spawn_c *TH,  int field);
  edit_op_c(rad_trigger_c *RAD, int field);

public:
  void Perform();
  // perform (or Redo) the operation.

  void Undo();
  // undo the previously performed operation.

///---  const char * Describe() const;
///---  // get a short description of this operation (for the UNDO menu).
///---  // The result must be freed using StringFree() in lib_util.h.

private:
  void Apply(const edit_value_u& what);

};


/* VARIABLES */

extern script_c  *active_script;
extern section_c *active_startmap;


/* FUNCTIONS */

void Edit_MoveThing(thing_spawn_c *TH,  float new_x, float new_y);
void Edit_MoveRad  (rad_trigger_c *RAD, float new_mx, float new_my);
void Edit_ResizeRad(rad_trigger_c *RAD, float x1, float y1, float x2, float y2);

void Edit_ChangeFloat(thing_spawn_c *TH,  int field, float new_val);
void Edit_ChangeFloat(rad_trigger_c *RAD, int field, float new_val);

void Edit_ChangeInt(thing_spawn_c *TH,  int field, int new_val);
void Edit_ChangeInt(rad_trigger_c *RAD, int field, int new_val);

void Edit_ChangeString(thing_spawn_c *TH,  int field, const char *buffer);
void Edit_ChangeString(rad_trigger_c *RAD, int field, const char *buffer);

#endif // __G_EDIT_H__

//--- editor settings ---
// vi:ts=2:sw=2:expandtab
