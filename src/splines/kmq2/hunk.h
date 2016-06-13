//----------------------------------------------------------------------------
//  EDGE2 Hunk Allocator
//----------------------------------------------------------------------------
// 
//  Copyright (c) 2015 Isotope SoftWorks and Contributors.
//  Based on the Quake 3: Arena source, (C) id Software.
//
//  Uses code from KMQuake II, (C) Mark "KnightMare" Shan.
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void *Hunk_Alloc_Aligned (int size, int alignment);

void *Hunk_Begin (int maxsize);
void *Hunk_Alloc (int size);
int Hunk_End (void);
void Hunk_Free (void *base);

