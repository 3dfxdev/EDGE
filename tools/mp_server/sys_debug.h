//------------------------------------------------------------------------
//  Debugging support
//------------------------------------------------------------------------
//
//  Edge MultiPlayer Server (C) 2005  Andrew Apted
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

#ifndef __SYS_DEBUG_H__
#define __SYS_DEBUG_H__

void DebugInit(bool enable);
void DebugTerm(void);

void DebugPrintf(const char *str, ...);

#endif /* __SYS_DEBUG_H__ */
