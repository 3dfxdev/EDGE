//----------------------------------------------------------------------------
//  EDGE Inline Function Definitions
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2003  The EDGE Team.
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
//----------------------------------------------------------------------------
// This file will put all the inline functions into an object file, so that
// they can be referred to via function pointers, and also so they can be
// called if the system doesn't support function inlining.

#include "i_defs.h"

#ifdef EDGE_INLINE
#undef EDGE_INLINE
#endif

#define EDGE_INLINE(decl, body) decl ; decl body

#include "m_inline.h"

