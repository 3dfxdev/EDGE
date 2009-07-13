//------------------------------------------------------------------------
//  DIALOGS
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor (C) 2001-2009 Andrew Apted
//                     (C) 1997-2003 André Majorel et al
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


bool Confirm (int, int, const char *, const char *);
int Confirm2 (int x0, int y0, confirm_t *confirm_flag,
   const char *prompt1, const char *prompt2);
void Notify (int, int, const char *, const char *);
void NotImplemented (void);


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
