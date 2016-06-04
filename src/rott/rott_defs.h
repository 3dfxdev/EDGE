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


#endif // __ROTT_RAWDEF_H__
