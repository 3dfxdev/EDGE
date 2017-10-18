//----------------------------------------------------------------------------
//  Rise of the Triad Structs
//----------------------------------------------------------------------------
// 
//  Copyright (c) 2016 Isotope SoftWorks
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
//
//
//
//----------------------------------------------------------------------------

#ifndef __ROTT_RAWDEF_H__
#define __ROTT_RAWDEF_H__

#include "../src/w_wad.cc" ///<--- For WAD handling

//!!!!!! FIXME:
#define GAT_PACK  __attribute__ ((packed))

typedef struct {
    int number;
    char mapname[23];
} mapinfo_t;

typedef struct {
    int nummaps;
    mapinfo_t maps[100];
} mapfileinfo_t;


/*---------------------------------------------------------------------
   Map constants
---------------------------------------------------------------------*/

#define MAXLEVELNAMELENGTH           23
#define ALLOCATEDLEVELNAMELENGTH     24
#define NUMPLANES                    3
#define NUMHEADEROFFSETS             100
#define MAPWIDTH                     128
#define MAPHEIGHT                    128
#define MAP_SPECIAL_TOGGLE_PUSHWALLS 0x0001

#define WALL_PLANE    0
#define SPRITE_PLANE  1
#define INFO_PLANE    2

/*---------------------------------------------------------------------
   Type definitions
---------------------------------------------------------------------*/

typedef struct
{
   unsigned long used;
   unsigned long CRC;
   unsigned long RLEWtag;
   unsigned long MapSpecials;
   unsigned long planestart[ NUMPLANES ];
   unsigned long planelength[ NUMPLANES ];
   char          Name[ ALLOCATEDLEVELNAMELENGTH ];
} RTLMAP;


/*---------------------------------------------------------------------
   Global variables
---------------------------------------------------------------------*/

unsigned short *mapplanes[ NUMPLANES ];


/*---------------------------------------------------------------------
   Macros
---------------------------------------------------------------------*/

#define MAPSPOT( x, y, plane ) \
   ( mapplanes[ plane ][ MAPWIDTH * ( y ) + ( x ) ] )

#define WALL_AT( x, y )   ( MAPSPOT( ( x ), ( y ), WALL_PLANE ) )
#define SPRITE_AT( x, y ) ( MAPSPOT( ( x ), ( y ), SPRITE_PLANE ) )
#define INFO_AT( x, y )   ( MAPSPOT( ( x ), ( y ), INFO_PLANE ) )

int GetWallIndex (int texture);


#endif // __ROTT_RAWDEF_H__
