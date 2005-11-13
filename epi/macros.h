//------------------------------------------------------------------------
//  EDGE Macros
//------------------------------------------------------------------------
//
//  Copyright (c) 2003-2005  The EDGE Team.
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public License
//  (LGPL) as published by the Free Software Foundation.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//------------------------------------------------------------------------

#ifndef __EPI_MACRO_H__
#define __EPI_MACRO_H__

// basic macros

#define CHECK_SELF_ASSIGN(param)  \
    if (this == &param) return *this;


#ifndef NULL
#define NULL    ((void*) 0)
#endif

#ifndef MAX
#define MAX(a,b)  ((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a,b)  ((a) < (b) ? (a) : (b))
#endif

#ifndef ABS
#define ABS(a)  ((a) < 0 ? -(a) : (a))
#endif

#ifndef SGN
#define SGN(a)  ((a) < 0 ? -1 : (a) > 0 ? +1 : 0)
#endif

#ifndef CLAMP
#define CLAMP(x,low,high)  \
    ((x) < (low) ? (low) : (x) > (high) ? (high) : (x))
#endif

#endif  /* __EPI_MACRO_H__ */
