//----------------------------------------------------------------------------
//  EDGE Strings
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

#ifndef __DOOM_STRINGS__
#define __DOOM_STRINGS__

// Misc. other strings.
// 98-7-10 KM Support for more savegame slots.
#define SAVEGAMENAME	"save"
#define SAVEGAMEDIR	"savegame"
#define NETSAVEDIR	"savegame.net"

#define SAVEGAMEMODE    0755
#define NETSAVEMODE     0755

#define DEVMAPS "devmaps"
#define DEVDATA "devdata"

extern const char *chat_macros[10];

extern const char *destination_keys;
extern const char *gammamsg[5];

#endif
