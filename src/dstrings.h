//----------------------------------------------------------------------------
//  EDGE2 Strings
//----------------------------------------------------------------------------
//
//  Copyright (c) 1999-2017  The EDGE2 Team.
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
#define REQUIREDWAD    "edge"
#define REQUIREDPAK    "edge"
#define EDGECONFIGFILE "EDGE.ini"
#define EDGELOGFILE    "EDGE.log"
#define REQHERETICPWAD "herfix" //Not needed anymore..?
#define BASEDDFDIR     "base"

#define EDGEGWAEXT     "gwa"
#define EDGEHWAEXT     "hwa"
#define EDGEWADEXT     "wad"
#define EDGEPAKEXT     "pak"
#define EDGEEPKEXT     "epk" // New extension for "Edge PaK"

// Wolfenstein 3D
#define WOLFDATEXT     "wl6" ///regular Wolfenstein3D extension (in WL6 format!)
#define WOLFAUDIOHED "audiohed"
#define WOLFAUDIOT   "audiot"
#define WOLFGAMEMAPS "gamemaps"
#define WOLFMAPHEAD  "maphead"
#define WOLFVGADICT  "vgadict"
#define WOLFVGAGRAPH "vragraph"
#define WOLFVGAHEAD  "vgahead"
#define WOLFVSWAP    "vswap"
#define WOLFREDUXPAK    "wolf"


#define SAVEGAMEBASE   "save"
#define SAVEGAMEEXT    "esg"
#define SAVEGAMEMODE   0755

//#define BASEDDFDIR     "base"
#define CACHEDIR       "cache"
#define SAVEGAMEDIR    "savegame"
#define SCRNSHOTDIR    "screenshots"

#define HUBDIR         "hubs"
#define HUBBASE        "hub"

#define FRAGEXT "fp"
#define VERTEXT "vp"

#ifdef WIN32 //TODO: Application Data (WinXP?), do we need this explicit?
#define EDGEHOMESUBDIR  "Application Data\\Edge"
#elif MACOSX
#define EDGEHOMESUBDIR  "Library/Application Support/EDGE"
#else // Linux
#define EDGEHOMESUBDIR  ".edge"
#endif


#endif // __DOOM_STRINGS__

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
