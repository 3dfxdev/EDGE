//----------------------------------------------------------------------------
//  EDGE Main Header
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2000  The EDGE Team.
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

#ifndef __E_MAIN__
#define __E_MAIN__

#include "e_event.h"

extern boolean_t redrawsbar;
extern boolean_t advancedemo;
extern boolean_t e_display_OK;
extern boolean_t need_save_screenshot;

void E_AddFile(const char *file);
void E_EDGEMain(void);
void E_EDGELoopRoutine(void);
boolean_t E_CheckNetGame(void);
void E_ProcessEvents(void);
void E_DoAdvanceDemo(void);
void E_PostEvent(event_t * ev);
void E_PageTicker(void);
void E_PageDrawer(void);
void E_AdvanceDemo(void);
void E_StartTitle(void);
void E_EngineShutdown(void);
void E_Display(void);

#endif
