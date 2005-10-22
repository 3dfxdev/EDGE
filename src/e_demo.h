//----------------------------------------------------------------------------
//  EDGE Demo Code
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2005  The EDGE Team.
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
//
//  Based on the DOOM source code, released by Id Software under the
//  following copyright:
//
//    Copyright (C) 1993-1996 by id Software, Inc.
//
//----------------------------------------------------------------------------

#ifndef __E_DEMO_H__
#define __E_DEMO_H__

#include "dm_defs.h"
#include "e_event.h"
#include "ddf_main.h"

#include <epi/strings.h>

extern bool demo_notbegun;
extern bool netdemo;

void G_DeferredPlayDemo(const char *filename);
void G_DeferredTimeDemo(const char *filename);

void G_DoPlayDemo(void);

// Only called by startup code.
void G_RecordDemo(const char *filename);

void G_BeginRecording(void);
bool G_FinishDemo(void);

void E_DemoReadTick(void);
void E_DemoWriteTick(void);

#endif  /* __E_DEMO_H__ */
