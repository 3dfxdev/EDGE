//------------------------------------------------------------------------
//  Abstact OPERATIONS (Undoable)
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

#ifndef __G_OPS_H__
#define __G_OPS_H__


class abstract_op_c
{
public:
  abstract_op_c() { }
  virtual ~abstract_op_c() { }

public:
  virtual void Perform() = 0;
  // perform the operation.

  virtual void Undo() = 0;
  // undo the previously performed operation.

  virtual const char * Describe() const = 0;
  // get a short description of this operation (for the UNDO menu).
  // The result must be freed using StringFree() in lib_util.h.
};


#endif // __G_OPS_H__

//--- editor settings ---
// vi:ts=2:sw=2:expandtab
