//------------------------------------------------------------------------
//  SECTOR MISC OPS
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2009 Andrew Apted
//  Copyright (C) 1997-2003 André Majorel et al
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
//
//  Based on Yadex which incorporated code from DEU 5.21 that was put
//  in the public domain in 1994 by Raphaël Quinet and Brendon Wyber.
//
//------------------------------------------------------------------------


#ifndef YH_S_SLICE  /* DO NOT INSERT ANYTHING BEFORE THIS LINE */
#define YH_S_SLICE

void sector_slice (obj_no_t linedef1, obj_no_t linedef2);

void DistributeSectorFloors (SelPtr);
void DistributeSectorCeilings (SelPtr);
void RaiseOrLowerSectors (SelPtr obj);
void BrightenOrDarkenSectors (SelPtr obj);
void SuperSectorSelector (int map_x, int map_y, int new_sec);

void SplitSector (int, int);
void SplitLineDefsAndSector (int, int);

#endif  /* DO NOT ADD ANYTHING AFTER THIS LINE */
//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
